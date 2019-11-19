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
  uint32_t reserved;           // 0 for now, serves to pad out struct
} cell;

static const char* required_caps[] = {
  "cup",
  "clear",
  NULL
};

typedef struct notcurses {
  int ttyfd;      // file descriptor for controlling tty (takes stdin)
  int colors;
  int rows, cols; // most recently measured values
  int x, y;       // our location on the screen
  // We verify that some capabilities exist (see required_caps). Those needn't
  // be checked before further use; just use tiparm() directly. These might be
  // NULL, and we can more or less work without them.
  char* smcup;    // enter alternate mode
  char* rmcup;    // restore primary mode
  char* setaf;    // set foreground color (ANSI)
  char* setab;    // set background color (ANSI)
  struct termios tpreserved; // terminal state upon entry
  bool RGBflag;   // terminfo reported "RGB" flag for 24bpc directcolor
  cell* plane;    // the contents of our bottommost plane
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
  n->rows = *rows = ws.ws_row;
  n->cols = *cols = ws.ws_col;
  return 0;
}

static int
term_verify_seq(char** gseq, const char* name){
  char* seq;
  if(gseq == NULL){
    gseq = &seq;
  }
  *gseq = tigetstr(name);
  if(*gseq == NULL || *gseq == (char*)-1){
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

// FIXME should probably register a SIGWINCH handler here
// FIXME install other sighandlers to clean things up
notcurses* notcurses_init(const notcurses_options* opts){
if(!ttyname(opts->outfd)){
  exit(EXIT_FAILURE);
}
  struct termios modtermios;
  notcurses* ret = malloc(sizeof(*ret));
  if(ret == NULL){
    return ret;
  }
  ret->ttyfd = opts->outfd;
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
  if(setupterm(opts->termtype, ret->ttyfd, &termerr) != OK){
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
  const char** cap;
  for(cap = required_caps ; *cap ; ++cap){
    if(term_verify_seq(NULL, *cap)){
      goto err;
    }
  }
  // Not all terminals support setting the fore/background independently
  term_verify_seq(&ret->setaf, "setaf");
  term_verify_seq(&ret->setab, "setab");
  // Neither of these is supported on e.g. the "linux" virtual console.
  if(!opts->inhibit_alternate_screen){
    term_verify_seq(&ret->smcup, "smcup");
    term_verify_seq(&ret->rmcup, "rmcup");
  }else{
    ret->smcup = ret->rmcup = NULL;
  }
  if((ret->plane = alloc_plane(ret, NULL, &ret->rows, &ret->cols)) == NULL){
    goto err;
  }
  printf("Geometry: %d rows, %d columns (%zub)\n",
         ret->rows, ret->cols, ret->rows * ret->cols * sizeof(*ret->plane));
  if(ret->smcup && term_emit(ret->smcup)){
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
    if(nc->rmcup && term_emit(nc->rmcup)){
      ret = -1;
    }
    ret |= tcsetattr(nc->ttyfd, TCSANOW, &nc->tpreserved);
    free(nc->plane);
    free(nc);
  }
  return ret;
}

// FIXME don't go using STDOUT_FILENO here, we want nc->outfd!
// only works with string literals!
#define strout(x) write(STDOUT_FILENO, (x), sizeof(x))

static int
erpchar(int c){
  if(write(STDOUT_FILENO, &c, 1) == 1){
    return c;
  }
  return EOF;
}

int notcurses_fg_rgb8(notcurses* nc, unsigned r, unsigned g, unsigned b){
  if(r >= 256 || g >= 256 || b >= 256){
    return -1;
  }
  if(nc->setaf == NULL){
    return -1;
  }
  // We typically want to use tputs() and tiperm() to acquire and write the
  // escapes, as these take into account terminal-specific delays, padding,
  // etc. For the case of DirectColor, there is no suitable terminfo entry, but
  // we're also in that case working with hopefully more robust terminals.
  // If it doesn't work, eh, it doesn't work. Fuck the world; save yourself.
  static char rgbesc[] = "\x1b[38;2;%u;%u;%um";
  if(nc->RGBflag){
    if(write(nc->ttyfd, rgbesc, sizeof(rgbesc)) != sizeof(rgbesc)){
      return -1;
    }
  }else{
    // For 256-color indexed mode, start constructing a palette based off
    // the inputs *if we can change the palette*. If more than 256 are used on
    // a single screen, start... combining close ones? For 8-color mode, simple
    // interpolation. I have no idea what to do for 88 colors. FIXME
    return -1;
  }
  return 0;
}

// FIXME this needs to keep an invalidation bitmap, rather than blitting the
// world every time
int notcurses_render(notcurses* nc){
  int ret = 0;
  strout("make it happen!\n"); // FIXME
  return ret;
}

int notcurses_move(notcurses* n, int x, int y){
  char* tstr = tiparm(cursor_address, y, x);
  if(tstr == NULL){
    return -1;
  }
  if(tputs(tstr, 1, erpchar) != OK){
    return -1;
  }
  return 0;
}
