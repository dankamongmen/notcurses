#include <ncurses.h> // needed for some definitions, see terminfo(3ncurses)
#include <term.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include "notcurses.h"
#include "version.h"

// A cell represents a single character cell in the display. At any cell, we
// can have a short array of wchar_t (L'\0'-terminated; we need support an
// array due to the possibility of combining characters), a foreground color,
// a background color, and an attribute set. The rules on the wchar_t array are
// the same as those for an ncurses 6.1 cchar_t:
//
//  * At most one spacing character, which must be the first if present
//  * Up to CCHARW_MAX-1 nonspacing characters follow. Extra spacing characters
//    are ignored. A nonspacing character is one for which wcwidth() returns
//    zero, and is not the wide NUL (L'\0').
//  * A single control character can be present, with no other characters.
//  * If there are fewer than CCHARW_MAX wide characters, they must be
//    terminated with a wide NUL (L'\0').
//
// Multi-column characters can only have a single attribute/color.
// https://pubs.opengroup.org/onlinepubs/007908799/xcurses/intov.html
//
// Each cell occupies 32 bytes (256 bits). The surface is thus ~4MB for a
// (pretty large) 500x200 terminal. At 80x43, it's less than 200KB.
typedef struct cell {
  wchar_t cchar[CCHARW_MAX];   // 5 * 4b == 20b
  uint64_t attrs;              // 16 MSB of attr bits, 24 of fg, 24 of bg
  uint32_t reserved;           // 0 for now
} cell;

typedef struct notcurses {
  int ttyfd;  // file descriptor for controlling tty (takes stdin)
  int colors;
  int rows, cols; // most recently measured values
  char* smcup;  // enter alternate mode
  char* rmcup;  // restore primary mode
  struct termios tpreserved; // terminal state upon entry
  bool RGBflag; // terminfo reported "RGB" flag for 24bpc directcolor
  cell* plane;  // the contents of our bottommost plane
} notcurses;

static const char NOTCURSES_VERSION[] =
 notcurses_VERSION_MAJOR "."
 notcurses_VERSION_MINOR "."
 notcurses_VERSION_PATCH;

const char* notcurses_version(void){
  return NOTCURSES_VERSION;
}

int notcurses_term_dimensions(notcurses* n, int* rows, int* cols){
  struct winsize ws;
  int i = ioctl(n->ttyfd, TIOCGWINSZ, &ws);
  if(i < 0){
    fprintf(stderr, "TIOCGWINSZ failed on %d (%s)\n", n->ttyfd, strerror(errno));
    return -1;
  }
  *rows = ws.ws_row;
  *cols = ws.ws_col;
  return 0;
}

static int
term_get_seq(char** seq, const char* name){
  *seq = tigetstr(name);
  if(*seq == NULL || *seq == (char*)-1){
    fprintf(stderr, "Capability not defined for terminal: %s\n", name);
    return -1;
  }
  return 0;
}

static int
term_emit(const char* seq){
  int ret = printf("%s", seq);
  if(ret < 0){
    fprintf(stderr, "Error emitting %zub sequence (%s)\n", strlen(seq), strerror(errno));
    return -1;
  }
  if((size_t)ret != strlen(seq)){
    fprintf(stderr, "Short write (%db) for %zub sequence\n", ret, strlen(seq));
    return -1;
  }
  return 0;
}

// Call this on initialization, or when the screen size changes. Takes a flat
// array of *rows * *cols cells (may be NULL if *rows == *cols == 0). Gets the
// new size, and copies what can be copied. Assumes that the screen is always
// anchored in the same place. Never free()s oldplane.
static cell*
alloc_plane(notcurses* nc, cell* oldplane, int* rows, int* cols){
  int oldrows = *rows;
  int oldcols = *cols;
  if(notcurses_term_dimensions(nc, rows, cols)){
    return NULL;
  }
  cell* newcells = malloc(sizeof(*newcells) * (*rows * *cols));
  if(newcells == NULL){
    return NULL;
  }
  int y, idx;
  idx = 0;
  for(y = 0 ; y < *rows ; ++y){
    idx = y * *cols;
    if(y > oldrows){
      memset(&newcells[idx], 0, sizeof(*newcells) * *cols);
      continue;
    }
    if(oldcols){
      int oldcopy = oldcols;
      if(oldcopy > *cols){
        oldcopy = *cols;
      }
      memcpy(&newcells[idx], &oldplane[y * oldcols], oldcopy * sizeof(*newcells));
      idx += oldcols;
    }
    memset(&newcells[idx], 0, sizeof(*newcells) * *cols - oldcols);
  }
  return newcells;
}

notcurses* notcurses_init(void){
  struct termios modtermios;
  notcurses* ret = malloc(sizeof(*ret));
  if(ret == NULL){
    return ret;
  }
  ret->ttyfd = STDIN_FILENO; // FIXME use others if stderr is redirected?
  if(tcgetattr(ret->ttyfd, &ret->tpreserved)){
    fprintf(stderr, "Couldn't preserve terminal state for %d (%s)\n",
            ret->ttyfd, strerror(errno));
    free(ret);
    return NULL;
  }
  memcpy(&modtermios, &ret->tpreserved, sizeof(modtermios));
  modtermios.c_lflag &= (~ECHO & ~ICANON);
  if(tcsetattr(ret->ttyfd, TCSANOW, &modtermios)){
    fprintf(stderr, "Error disabling echo / canonical on %d (%s)\n",
            ret->ttyfd, strerror(errno));
    goto err;
  }
  int termerr;
  if(setupterm(NULL, ret->ttyfd, &termerr) != OK){
    fprintf(stderr, "Terminfo error %d (see terminfo(3ncurses))\n", termerr);
    goto err;
  }
  ret->RGBflag = tigetflag("RGB") == 1;
  if((ret->colors = tigetnum("colors")) <= 0){
    fprintf(stderr, "This terminal doesn't appear to support colors\n");
    goto err;
  }else if(ret->RGBflag && (unsigned)ret->colors < (1u << 23u)){
    fprintf(stderr, "Warning: advertised RGB flag but only %d colors\n",
            ret->colors);
  }else{
    printf("Colors: %d (%s)\n", ret->colors,
           ret->RGBflag ? "direct" : "palette");
  }
  int fails = 0;
  fails |= term_get_seq(&ret->smcup, "smcup");
  fails |= term_get_seq(&ret->rmcup, "rmcup");
  if(fails){
    goto err;
  }
  ret->rows = ret->cols = 0;
  if((ret->plane = alloc_plane(ret, NULL, &ret->rows, &ret->cols)) == NULL){
    goto err;
  }
  printf("Geometry: %d rows, %d columns (%zub)\n",
         ret->rows, ret->cols, ret->rows * ret->cols * sizeof(*ret->plane));
  if(term_emit(ret->smcup)){
    free(ret->plane);
    goto err;
  }
  return ret;

err:
  tcsetattr(ret->ttyfd, TCSANOW, &ret->tpreserved);
  free(ret);
  return NULL;
}

int notcurses_stop(notcurses* nc){
  int ret = 0;
  if(nc){
    ret |= term_emit(nc->rmcup);
    ret |= tcsetattr(nc->ttyfd, TCSANOW, &nc->tpreserved);
    free(nc->plane);
    free(nc);
  }
  return ret;
}
