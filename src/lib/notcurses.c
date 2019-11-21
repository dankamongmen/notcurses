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
//  * At most one spacing character, which must be the first if present.
//  * Up to CCHARW_MAX-1 nonspacing characters follow. Extra spacing characters
//    are ignored. A nonspacing character is one for which wcwidth() returns
//    zero, and is not the wide NUL (L'\0').
//  * A single control character can be present, with no other characters (save
//    an immediate wide NUL (L'\0').
//  * If there are fewer than CCHARW_MAX wide characters, they must be
//    terminated with a wide NUL (L'\0').
//
// Multi-column characters can only have a single attribute/color.
// https://pubs.opengroup.org/onlinepubs/007908799/xcurses/intov.html
//
// Each cell occupies 32 bytes (256 bits). The surface is thus ~4MB for a
// (pretty large) 500x200 terminal. At 80x43, it's less than 200KB.
typedef struct cell {
  wchar_t cchar[CCHARW_MAX + 1]; // 6 * 4b == 24b
  // The attrword covers classic NCURSES attributes (16 bits), plus foreground
  // and background color, stored as 3x8bits of RGB. At render time, these
  // 24-bit values are quantized down to terminal capabilities, if necessary.
  uint64_t attrs;
} cell;

// Some capabilities are so fundamental that we don't attempt to run without
// them. Essentially, we require a two-dimensional, random-access terminal.
static const char* required_caps[] = {
  "cup",
  "clear",
  NULL
};

// A plane is memory for some rectilinear virtual window, plus current cursor
// state for that window. A notcurses context describes a single terminal, and
// has a z-order of planes (I see no advantage to maintaining a poset, and we
// instead just use a list, top-to-bottom). Every cell on the terminal is part
// of at least one plane, and at least one plane covers the entirety of the
// terminal (this plane is created during initialization).
//
// Functions update these virtual planes over a series of API calls. Eventually,
// notcurses_render() is called. We then do a depth buffer blit of updated
// cells. A cell is updated if the topmost plane including that cell updates it,
// not simply if any plane updates it.
//
// A plane may be partially or wholly offscreen--this might occur if the
// screen is resized, for example. Offscreen portions will not be rendered.
// Accesses beyond the borders of a panel, however, are errors.
typedef struct ncplane {
  cell* fb;        // "framebuffer" of character cells
  int x, y;        // current location within this plane
  int absx, absy;  // origin of the plane relative to the screen
  int lenx, leny;  // size of the plane, [0..len{x,y}) is addressable
  struct ncplane* z; // plane below us
  struct notcurses* nc; // our parent nc, kinda lame waste of memory FIXME
} ncplane;

typedef struct notcurses {
  int ttyfd;      // file descriptor for controlling tty (takes stdin)
  int colors;     // number of colors usable for this screen
  // We verify that some capabilities exist (see required_caps). Those needn't
  // be checked before further use; just use tiparm() directly. These might be
  // NULL, and we can more or less work without them.
  char* smcup;    // enter alternate mode
  char* rmcup;    // restore primary mode
  char* setaf;    // set foreground color (ANSI)
  char* setab;    // set background color (ANSI)
  struct termios tpreserved; // terminal state upon entry
  bool RGBflag;   // terminfo-reported "RGB" flag for 24bpc directcolor
  ncplane* top;   // the contents of our topmost plane (initially entire screen)
  ncplane* stdscr;// aliases some plane from the z-buffer, covers screen
} notcurses;

static const char NOTCURSES_VERSION[] =
 notcurses_VERSION_MAJOR "."
 notcurses_VERSION_MINOR "."
 notcurses_VERSION_PATCH;

const char* notcurses_version(void){
  return NOTCURSES_VERSION;
}

static inline int
fbcellidx(const ncplane* n, int row, int col){
  return row * n->lenx + col;
}

void ncplane_dimyx(const ncplane* n, int* rows, int* cols){
  if(rows){
    *rows = n->leny;
  }
  if(cols){
    *cols = n->lenx;
  }
}

// This should really only be called from within alloc_stdscr()
static int
update_term_dimensions(notcurses* n, int* rows, int* cols){
  struct winsize ws;
  int i = ioctl(n->ttyfd, TIOCGWINSZ, &ws);
  if(i < 0){
    fprintf(stderr, "TIOCGWINSZ failed on %d (%s)\n", n->ttyfd, strerror(errno));
    return -1;
  }
  if(ws.ws_row <= 0 || ws.ws_col <= 0){
    fprintf(stderr, "Bogus return from TIOCGWINSZ on %d (%d/%d)\n",
            n->ttyfd, ws.ws_row, ws.ws_col);
    return -1;
  }
  *rows = ws.ws_row;
  *cols = ws.ws_col;
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

static void
free_plane(ncplane* p){
  if(p){
    free(p->fb);
    free(p);
  }
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

// Create an ncplane of the specified dimensions, but does not place it in
// the z-buffer.
static ncplane*
create_ncplane(notcurses* nc, int rows, int cols){
  ncplane* p = NULL;
  if((p = malloc(sizeof(*p))) == NULL){
    return NULL;
  }
  if((p->fb = malloc(sizeof(*p->fb) * (rows * cols))) == NULL){
    free(p);
    return NULL;
  }
  p->leny = rows;
  p->lenx = cols;
  p->nc = nc;
  p->x = p->y = 0;
  p->absx = p->absy = 0;
  return p;
}

// Call this on initialization, or when the screen size changes. Takes a flat
// array of *rows * *cols cells (may be NULL if *rows == *cols == 0). Gets the
// new size, and copies what can be copied from the old stdscr. Assumes that
// the screen is always anchored in the same place.
static ncplane*
alloc_stdscr(notcurses* nc){
  ncplane* p = NULL;
  int oldrows = 0;
  int oldcols = 0;
  if(nc->stdscr){
    oldrows = nc->stdscr->leny;
    oldcols = nc->stdscr->lenx;
  }
  int rows, cols;
  if(update_term_dimensions(nc, &rows, &cols)){
    goto err;
  }
  if((p = create_ncplane(nc, rows, cols)) == NULL){
    goto err;
  }
  ncplane** oldscr;
  ncplane* preserve;
  // if we ever make this a doubly-linked list, turn this into o(1)
  // if nc->stdscr is NULL, we're all good -- there are no entries
  for(oldscr = &nc->top ; *oldscr ; oldscr = &(*oldscr)->z){
    if(*oldscr == nc->stdscr){
      break;
    }
  }
  if( (preserve = *oldscr) ){
    p->z = preserve->z;
  }else{
    p->z = NULL;
  }
  *oldscr = p;
  nc->stdscr = p;
  int y, idx;
  idx = 0;
  for(y = 0 ; y < p->leny ; ++y){
    idx = y * p->lenx;
    if(y > oldrows){
      memset(&p->fb[idx], 0, sizeof(*p->fb) * p->lenx);
      continue;
    }
    int oldcopy = oldcols;
    if(oldcopy){
      if(oldcopy > p->lenx){
        oldcopy = p->lenx;
      }
      memcpy(&p->fb[idx], &preserve->fb[y * oldcols], oldcopy * sizeof(*p->fb));
    }
    if(p->lenx - oldcopy){
      memset(&p->fb[idx + oldcopy], 0, sizeof(*p->fb) * (p->lenx - oldcopy));
    }
  }
  free_plane(preserve);
  return p;

err:
  free_plane(p);
  return NULL;
}

int notcurses_resize(notcurses* n){
  if(alloc_stdscr(n) == NULL){
    return -1;
  }
  return 0;
}

ncplane* notcurses_stdplane(notcurses* nc){
  return nc->stdscr;
}

const ncplane* notcurses_stdplane_const(const notcurses* nc){
  return nc->stdscr;
}

// FIXME should probably register a SIGWINCH handler here
// FIXME install other sighandlers to clean things up
notcurses* notcurses_init(const notcurses_options* opts){
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
  ret->top = ret->stdscr = NULL;
  if(alloc_stdscr(ret) == NULL){
    goto err;
  }
  ret->stdscr = ret->top;
  printf("Geometry: %d rows, %d columns (%zub)\n",
         ret->top->leny, ret->top->lenx,
         ret->top->lenx * ret->top->leny * sizeof(*ret->top->fb));
  if(ret->smcup && term_emit(ret->smcup)){
    free_plane(ret->top);
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
    while(nc->top){
      ncplane* p = nc->top;
      nc->top = p->z;
      free_plane(p);
    }
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

#define CELL_RMASK 0x0000ff0000000000ull
#define CELL_GMASK 0x000000ff00000000ull
#define CELL_BMASK 0x00000000ff000000ull
#define CELL_RGBMASK (CELL_RMASK | CELL_GMASK | CELL_BMASK)

static void
cell_set_fg(cell* c, unsigned r, unsigned g, unsigned b){
  uint64_t rgb = (r & 0xffull) << 40u;
  rgb += (g & 0xffull) << 32u;
  rgb += (b & 0xffull) << 24u;
  c->attrs = (c->attrs & ~CELL_RGBMASK) | rgb;
}

static void
cell_get_fb(const cell* c, unsigned* r, unsigned* g, unsigned* b){
  *r = (c->attrs & CELL_RMASK) >> 40u;
  *g = (c->attrs & CELL_GMASK) >> 32;
  *b = (c->attrs & CELL_BMASK) >> 24u;
}

int ncplane_fg_rgb8(ncplane* n, int r, int g, int b){
  if(r >= 256 || g >= 256 || b >= 256){
    return -1;
  }
  if(r < 0 || g < 0 || b < 0){
    return -1;
  }
  cell_set_fg(&n->fb[fbcellidx(n, n->y, n->x)], r, g, b);
  return 0;
}

static int
term_fg_rgb8(notcurses* nc, unsigned r, unsigned g, unsigned b){
  // We typically want to use tputs() and tiperm() to acquire and write the
  // escapes, as these take into account terminal-specific delays, padding,
  // etc. For the case of DirectColor, there is no suitable terminfo entry, but
  // we're also in that case working with hopefully more robust terminals.
  // If it doesn't work, eh, it doesn't work. Fuck the world; save yourself.
  if(nc->RGBflag){
    #define RGBESC "\x1b[38;2;"
                         // rrr;ggg;bbb;m
    char rgbesc[] = RGBESC "             ";
    size_t len = strlen(RGBESC);
    if(r > 99){ rgbesc[len++] = r / 100; }
    if(r > 9){ rgbesc[len++] = (r % 100) / 10; }
    rgbesc[len++] = r % 10;
    rgbesc[len++] = ';';
    if(g > 99){ rgbesc[len++] = g / 100; }
    if(g > 9){ rgbesc[len++] = (g % 100) / 10; }
    rgbesc[len++] = g % 10;
    rgbesc[len++] = ';';
    if(b > 99){ rgbesc[len++] = b / 100; }
    if(b > 9){ rgbesc[len++] = (b % 100) / 10; }
    rgbesc[len++] = b % 10;
    rgbesc[len++] = ';';
    rgbesc[len++] = 'm';
    rgbesc[len] = '\0';
    ssize_t w;
    if((w = write(nc->ttyfd, rgbesc, len)) < 0 || (size_t)w != len){
      return -1;
    }
  }else{
    if(nc->setaf == NULL){
      return -1;
    }
    // For 256-color indexed mode, start constructing a palette based off
    // the inputs *if we can change the palette*. If more than 256 are used on
    // a single screen, start... combining close ones? For 8-color mode, simple
    // interpolation. I have no idea what to do for 88 colors. FIXME
    return -1;
  }
  return 0;
}

// Move to the given coordinates on the physical terminal
static int
term_movyx(int y, int x){
  char* tstr = tiparm(cursor_address, y, x);
  if(tstr == NULL){
    return -1;
  }
  if(tputs(tstr, 1, erpchar) != OK){
    return -1;
  }
  return 0;
}

// Write the cchar (one cell's worth of wchar_t's) to the physical terminal
static int
term_putw(const notcurses* nc, const cell* c){
  ssize_t w;
  size_t len = wcslen(c->cchar);
  if(len == 0){
    if((w = write(nc->ttyfd, " ", 1)) < 0 || (size_t)w != 1){
      return -1;
    }
    return 0;
  }
  if((w = write(nc->ttyfd, c->cchar, len * sizeof(*c->cchar))) < 0){
    return -1;
  }
  if((size_t)w != len * sizeof(*c->cchar)){
    return -1;
  }
  return 0;
}

// FIXME this needs to keep an invalidation bitmap, rather than blitting the
// world every time
int notcurses_render(notcurses* nc){
  int ret = 0;
  int y, x;
  if(term_movyx(0, 0)){
    return -1;
  }
  for(y = 0 ; y < nc->stdscr->leny ; ++y){
    for(x = 0 ; x < nc->stdscr->lenx ; ++x){
      unsigned r, g, b;
      // FIXME z-culling
      const cell* c = &nc->stdscr->fb[fbcellidx(nc->stdscr, y, x)];
      cell_get_fb(c, &r, &g, &b);
      term_fg_rgb8(nc, r, g, b);
      term_putw(nc, c);
    }
  }
  return ret;
}

int ncplane_movyx(ncplane* n, int y, int x){
  if(x >= n->lenx || x < 0){
    return -1;
  }
  if(y >= n->leny || y < 0){
    return -1;
  }
  n->x = x;
  n->y = y;
  return 0;
}

void ncplane_posyx(const ncplane* n, int* y, int* x){
  if(y){
    *y = n->y;
  }
  if(x){
    *x = n->x;
  }
}

static inline bool
validate_wchar_cell(const cell* c, const wchar_t* wcs){
  if(wcslen(wcs) >= sizeof(c->cchar) / sizeof(*c->cchar)){
    return false;
  }
  // FIXME check other crap
  return true;
}

static void
advance_cursor(struct ncplane* n){
  if(++n->x == n->lenx){
    n->x = 0;
    if(++n->y == n->leny){
      n->y = 0;
    }
  }
}

int ncplane_putwc(struct ncplane* n, const wchar_t* wcs){
  cell* c = &n->fb[fbcellidx(n, n->y, n->x)];
  if(!validate_wchar_cell(c, wcs)){
    return -1;
  }
  memcpy(c->cchar, wcs, wcslen(wcs) * sizeof(*wcs));
  advance_cursor(n);
  return 0;
}
