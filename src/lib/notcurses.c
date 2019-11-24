#include <ncurses.h> // needed for some definitions, see terminfo(3ncurses)
#include <time.h>
#include <term.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include "notcurses.h"
#include "version.h"
#include "egcpool.h"

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
  cell* fb;             // "framebuffer" of character cells
  int x, y;             // current location within this plane
  int absx, absy;       // origin of the plane relative to the screen
  int lenx, leny;       // size of the plane, [0..len{x,y}) is addressable
  struct ncplane* z;    // plane below us
  struct notcurses* nc; // our parent nc, kinda lame waste of memory FIXME
  egcpool pool;         // attached storage pool for UTF-8 EGCs
  uint64_t channels;    // works the same way as cells
  uint32_t attrword;    // same deal as in a cell
} ncplane;

typedef struct notcurses {
  int ttyfd;      // file descriptor for controlling tty (takes stdin)
  timer_t timer;  // CLOCK_MONOTONIC timer for benchmarking
  int colors;     // number of colors usable for this screen
  // We verify that some capabilities exist (see required_caps). Those needn't
  // be checked before further use; just use tiparm() directly. These might be
  // NULL, and we can more or less work without them.
  char* smcup;    // enter alternate mode
  char* rmcup;    // restore primary mode
  char* setaf;    // set foreground color (ANSI)
  char* setab;    // set background color (ANSI)
  char* standout; // WA_STANDOUT
  char* uline;    // WA_UNDERLINK
  char* reverse;  // WA_REVERSE
  char* blink;    // WA_BLINK
  char* dim;      // WA_DIM
  char* bold;     // WA_BOLD
  char* italics;  // WA_ITALIC
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
    return -1;
  }
  return 0;
}

static void
free_plane(ncplane* p){
  if(p){
    egcpool_dump(&p->pool);
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
  size_t fbsize = sizeof(*p->fb) * (rows * cols);
  if((p->fb = malloc(fbsize)) == NULL){
    free(p);
    return NULL;
  }
  p->leny = rows;
  p->lenx = cols;
  p->nc = nc;
  p->x = p->y = 0;
  p->absx = p->absy = 0;
  // framebuffer is initialized by calling function, might be copy
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
  egcpool_init(&p->pool);
  p->attrword = 0;
  p->channels = 0;
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

static int
interrogate_terminfo(notcurses* nc, const notcurses_options* opts){
  char* longname_term = longname();
  fprintf(stderr, "Term: %s\n", longname_term ? longname_term : "?");
  nc->RGBflag = tigetflag("RGB") == 1;
  if((nc->colors = tigetnum("colors")) <= 0){
    fprintf(stderr, "This terminal doesn't appear to support colors\n");
    nc->colors = 1;
  }else if(nc->RGBflag && (unsigned)nc->colors < (1u << 23u)){
    fprintf(stderr, "Warning: advertised RGB flag but only %d colors\n",
            nc->colors);
  }else{
    printf("Colors: %d (%s)\n", nc->colors, nc->RGBflag ? "direct" : "palette");
  }
  const char** cap;
  for(cap = required_caps ; *cap ; ++cap){
    if(term_verify_seq(NULL, *cap)){
      fprintf(stderr, "Capability not defined for terminal: %s\n", *cap);
      return -1;
    }
  }
  term_verify_seq(&nc->standout, "smso");
  term_verify_seq(&nc->uline, "smul");
  term_verify_seq(&nc->reverse, "reverse");
  term_verify_seq(&nc->blink, "blink");
  term_verify_seq(&nc->dim, "dim");
  term_verify_seq(&nc->bold, "bold");
  term_verify_seq(&nc->italics, "sitm");
  // Some terminals cannot combine certain styles with colors. Don't advertise
  // support for the style in that case.
  int nocolor_stylemask = tigetnum("ncv");
  if(nocolor_stylemask > 0){
    if(nocolor_stylemask & WA_STANDOUT){
      nc->standout = NULL;
    }
    if(nocolor_stylemask & WA_UNDERLINE){
      nc->uline = NULL;
    }
    if(nocolor_stylemask & WA_REVERSE){
      nc->reverse = NULL;
    }
    if(nocolor_stylemask & WA_BLINK){
      nc->blink = NULL;
    }
    if(nocolor_stylemask & WA_DIM){
      nc->dim = NULL;
    }
    if(nocolor_stylemask & WA_BOLD){
      nc->bold = NULL;
    }
    if(nocolor_stylemask & WA_ITALIC){
      nc->italics = NULL;
    }
  }
  // Not all terminals support setting the fore/background independently
  term_verify_seq(&nc->setaf, "setaf");
  term_verify_seq(&nc->setab, "setab");
  // Neither of these is supported on e.g. the "linux" virtual console.
  if(!opts->inhibit_alternate_screen){
    term_verify_seq(&nc->smcup, "smcup");
    term_verify_seq(&nc->rmcup, "rmcup");
  }else{
    nc->smcup = nc->rmcup = NULL;
  }
  nc->top = nc->stdscr = NULL;
  return 0;
}

// FIXME should probably register a SIGWINCH handler here
// FIXME install other sighandlers to clean things up
notcurses* notcurses_init(const notcurses_options* opts){
  struct termios modtermios;
  notcurses* ret = malloc(sizeof(*ret));
  if(ret == NULL){
    return ret;
  }
  if(timer_create(CLOCK_MONOTONIC, NULL, &ret->timer)){
    fprintf(stderr, "Error initializing monotonic clock (%s)\n", strerror(errno));
    free(ret);
    return NULL;
  }
  ret->ttyfd = opts->outfd;
  if(tcgetattr(ret->ttyfd, &ret->tpreserved)){
    fprintf(stderr, "Couldn't preserve terminal state for %d (%s)\n",
            ret->ttyfd, strerror(errno));
    timer_delete(ret->timer);
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
  if(interrogate_terminfo(ret, opts)){
    goto err;
  }
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
  timer_delete(ret->timer);
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

static int
erpchar(int c){
  if(write(STDOUT_FILENO, &c, 1) == 1){
    return c;
  }
  return EOF;
}

int ncplane_bg_rgb8(ncplane* n, int r, int g, int b){
  if(r >= 256 || g >= 256 || b >= 256){
    return -1;
  }
  if(r < 0 || g < 0 || b < 0){
    return -1;
  }
  cell_rgb_set_bg(&n->channels, r, g, b);
  return 0;
}

int ncplane_fg_rgb8(ncplane* n, int r, int g, int b){
  if(r >= 256 || g >= 256 || b >= 256){
    return -1;
  }
  if(r < 0 || g < 0 || b < 0){
    return -1;
  }
  cell_rgb_set_fg(&n->channels, r, g, b);
  return 0;
}

// 3 for foreground, 4 for background, ugh FIXME
static int
term_esc_rgb(notcurses* nc, int esc, unsigned r, unsigned g, unsigned b){
  #define RGBESC1 "\x1b["
  #define RGBESC2 "8;2;"
                                    // rrr;ggg;bbbm
  char rgbesc[] = RGBESC1 " " RGBESC2 "            ";
  size_t len = strlen(RGBESC1);
  rgbesc[len++] = esc + '0';
  len += strlen(RGBESC2);
  if(r > 99){ rgbesc[len++] = r / 100 + '0'; }
  if(r > 9){ rgbesc[len++] = (r % 100) / 10 + '0'; }
  rgbesc[len++] = (r % 10) + '0';
  rgbesc[len++] = ';';
  if(g > 99){ rgbesc[len++] = g / 100 + '0'; }
  if(g > 9){ rgbesc[len++] = (g % 100) / 10 + '0'; }
  rgbesc[len++] = g % 10 + '0';
  rgbesc[len++] = ';';
  if(b > 99){ rgbesc[len++] = b / 100 + '0'; }
  if(b > 9){ rgbesc[len++] = (b % 100) / 10 + '0'; }
  rgbesc[len++] = b % 10 + '0';
  rgbesc[len++] = 'm';
  rgbesc[len] = '\0';
  ssize_t w;
  if((w = write(nc->ttyfd, rgbesc, len)) < 0 || (size_t)w != len){
    return -1;
  }
  return 0;
}

static int
term_bg_rgb8(notcurses* nc, unsigned r, unsigned g, unsigned b){
  // We typically want to use tputs() and tiperm() to acquire and write the
  // escapes, as these take into account terminal-specific delays, padding,
  // etc. For the case of DirectColor, there is no suitable terminfo entry, but
  // we're also in that case working with hopefully more robust terminals.
  // If it doesn't work, eh, it doesn't work. Fuck the world; save yourself.
  if(nc->RGBflag){
    return term_esc_rgb(nc, 4, r, g, b);
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

static int
term_fg_rgb8(notcurses* nc, unsigned r, unsigned g, unsigned b){
  // We typically want to use tputs() and tiperm() to acquire and write the
  // escapes, as these take into account terminal-specific delays, padding,
  // etc. For the case of DirectColor, there is no suitable terminfo entry, but
  // we're also in that case working with hopefully more robust terminals.
  // If it doesn't work, eh, it doesn't work. Fuck the world; save yourself.
  if(nc->RGBflag){
    return term_esc_rgb(nc, 3, r, g, b);
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

// is it a single ASCII byte, wholly contained within the cell?
static inline bool
simple_gcluster_p(const char* gcluster){
  return *gcluster == '\0' ||
         // FIXME need to ensure next character is not a nonspacer!
         (*(unsigned char*)gcluster < 0x80);
}

static inline bool
simple_cell_p(const cell* c){
  return c->gcluster < 0x80;
}

static inline uint32_t
extended_gcluster_idx(const cell* c){
  return c->gcluster - 0x80;
}

static inline const char*
extended_gcluster(const ncplane* n, const cell* c){
  uint32_t idx = extended_gcluster_idx(c);
  return n->pool.pool + idx;
}

// Write the cell's UTF-8 grapheme cluster to the physical terminal.
// FIXME maybe want to use a wmemstream
static int
term_putc(const notcurses* nc, const ncplane* n, const cell* c){
  ssize_t w;
  if(simple_cell_p(c)){
    if(c->gcluster == 0){
      if((w = write(nc->ttyfd, " ", 1)) < 0 || (size_t)w != 1){ // FIXME
        return -1;
      }
      return 0;
    }
    if((w = write(nc->ttyfd, &c->gcluster, 1)) < 0 || (size_t)w != 1){ // FIXME
      return -1;
    }
    return 0;
  }
  const char* ext = extended_gcluster(n, c);
  size_t len = strlen(ext);
  if((w = write(nc->ttyfd, ext, len)) < 0 || (size_t)w != len){
    return -1;
  }
  return 0;
}

static void
advance_cursor(ncplane* n){
  if(n->y == n->leny){
    if(n->x == n->lenx){
      return; // stuck!
    }
  }
  if(++n->x == n->lenx){
    n->x = 0;
    ++n->y;
  }
}

// FIXME this needs to keep an invalidation bitmap, rather than blitting the
// world every time
int notcurses_render(notcurses* nc){
  int ret = 0;
  int y, x;
  if(term_movyx(0, 0)){
    return -1;
  }
  unsigned pr = 0, pg = 0, pb = 0;
  unsigned pbr = 0, pbg = 0, pbb = 0;
  for(y = 0 ; y < nc->stdscr->leny ; ++y){
    for(x = 0 ; x < nc->stdscr->lenx ; ++x){
      unsigned r, g, b, br, bg, bb;
      // FIXME z-culling
      const cell* c = &nc->stdscr->fb[fbcellidx(nc->stdscr, y, x)];
      cell_get_fg(c, &r, &g, &b);
      if(cell_fg_default_p(c)){
        r = 255; g = 255; b = 255; // FIXME
      }
      cell_get_bg(c, &br, &bg, &bb);
      if(r != pr || g != pg || b != pb || br != pbr || bg != pbg || bb != pbb ||
          (x == 0 && y == 0)){
        term_fg_rgb8(nc, r, g, b);
        term_bg_rgb8(nc, br, bg, bb);
        pr = r;
        pg = g;
        pb = b;
        pbr = br;
        pbg = bg;
        pbb = bb;
      }
      term_putc(nc, nc->stdscr, c);
    }
  }
  return ret;
}

int ncplane_cursor_move_yx(ncplane* n, int y, int x){
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

void ncplane_cursor_yx(const ncplane* n, int* y, int* x){
  if(y){
    *y = n->y;
  }
  if(x){
    *x = n->x;
  }
}

static inline bool
ncplane_cursor_stuck(const ncplane* n){
  return (n->x == n->lenx && n->y == n->leny);
}

static int
cell_duplicate(ncplane* n, cell* targ, const cell* c){
  cell_release(n, targ);
  if(simple_cell_p(c)){
    targ->gcluster = c->gcluster;
    return !!c->gcluster;
  }
  size_t ulen;
  int eoffset = egcpool_stash(&n->pool, extended_gcluster(n, c), &ulen);
  if(eoffset < 0){
    return -1;
  }
  targ->gcluster = eoffset + 0x80;
  targ->attrword = c->attrword;
  targ->channels = c->channels;
  return ulen;
}

int ncplane_putc(ncplane* n, const cell* c){
  if(n->y == n->leny && n->x == n->lenx){
    return -1;
  }
  cell* targ = &n->fb[fbcellidx(n, n->y, n->x)];
  int ret = cell_duplicate(n, targ, c);
  advance_cursor(n);
  return ret;
}

int ncplane_getc(const ncplane* n, cell* c, char** gclust){
  if(n->y == n->leny && n->x == n->lenx){
    return -1;
  }
  const cell* src = &n->fb[fbcellidx(n, n->y, n->x)];
  memcpy(c, src, sizeof(*src));
  *gclust = NULL;
  if(!simple_cell_p(src)){
    *gclust = strdup(extended_gcluster(n, src));
    if(*gclust == NULL){
      return -1;
    }
  }
  return 0;
}

void cell_release(ncplane* n, cell* c){
  if(!simple_cell_p(c)){
    egcpool_release(&n->pool, extended_gcluster_idx(c));
  }
}

// loads the cell with the next EGC from 'gcluster'. returns the number of
// bytes copied out of 'gcluster', or -1 on failure.
int cell_load(ncplane* n, cell* c, const char* gcluster){
  cell_release(n, c);
  if(simple_gcluster_p(gcluster)){
    c->gcluster = *gcluster;
    return !!c->gcluster;
  }
  size_t ulen;
  int eoffset = egcpool_stash(&n->pool, gcluster, &ulen);
  if(eoffset < 0){
    return -1;
  }
  c->gcluster = eoffset + 0x80;
  return ulen;
}

int ncplane_putstr(ncplane* n, const char* gcluster){
  int ret = 0;
  // FIXME speed up this blissfully naive solution
  while(*gcluster){
    cell c;
    memset(&c, 0, sizeof(c));
    uint32_t rgb = cell_fg_rgb(n->channels);
    cell_set_fg(&c, cell_rgb_red(rgb), cell_rgb_green(rgb), cell_rgb_blue(rgb));
    rgb = cell_bg_rgb(n->channels);
    cell_set_bg(&c, cell_rgb_red(rgb), cell_rgb_green(rgb), cell_rgb_blue(rgb));
    int wcs = cell_load(n, &c, gcluster);
    if(wcs < 0){
      return -ret;
    }
    if(wcs == 0){
      break;
    }
    ncplane_putc(n, &c);
    gcluster += wcs;
    ret += wcs;
    if(ncplane_cursor_stuck(n)){
      break;
    }
  }
  return ret;
}

unsigned notcurses_supported_styles(const notcurses* nc){
  unsigned styles = 0;
  styles |= nc->standout ? WA_STANDOUT : 0;
  styles |= nc->uline ? WA_UNDERLINE : 0;
  styles |= nc->reverse ? WA_REVERSE : 0;
  styles |= nc->blink ? WA_BLINK : 0;
  styles |= nc->dim ? WA_DIM : 0;
  styles |= nc->bold ? WA_BOLD : 0;
  styles |= nc->italics ? WA_ITALIC : 0;
  return styles;
}

int notcurses_palette_size(const notcurses* nc){
  return nc->colors;
}

void ncplane_enable_styles(ncplane* n, unsigned stylebits){
  n->attrword |= ((stylebits & 0xffff) << 16u);
}

void ncplane_disable_styles(ncplane* n, unsigned stylebits){
  n->attrword &= ~((stylebits & 0xffff) << 16u);
}

void ncplane_set_style(ncplane* n, unsigned stylebits){
  n->attrword = (n->attrword & ~CELL_STYLE_MASK) |
                ((stylebits & 0xffff) << 16u);
}

int ncplane_printf(ncplane* n, const char* format, ...){
  int ret;
  va_list va;
  va_start(va, format);
  ret = ncplane_vprintf(n, format, va);
  va_end(va);
  return ret;
}

// i hate the big allocation and two copies here, but eh what you gonna do?
// well, for one, we don't need the huge allocation FIXME
int ncplane_vprintf(ncplane* n, const char* format, va_list ap){
  // FIXME technically insufficient to cover area due to EGCs, lol
  const size_t size = n->leny * n->lenx + 1;
  char* buf = malloc(size);
  if(buf == NULL){
    return -1;
  }
  int ret = vsnprintf(buf, size, format, ap);
  if(ncplane_putstr(n, buf)){ // FIXME handle short writes also!
    ret = -1;
  }
  free(buf);
  return ret;
}

int ncplane_hline(ncplane* n, const cell* c, int len){
  int ret;
  for(ret = 0 ; ret < len ; ++ret){
    if(ncplane_putc(n, c) <= 0){
      break;
    }
  }
  return ret;
}

int ncplane_vline(ncplane* n, const cell* c, int len){
  int ret, ypos, xpos;
  ncplane_cursor_yx(n, &ypos, &xpos);
  for(ret = 0 ; ret < len ; ++ret){
fprintf(stderr, "MOCNG: %d %d\n", ypos + ret, xpos);
    if(ncplane_cursor_move_yx(n, ypos + ret, xpos)){
      return -1;
    }
    if(ncplane_putc(n, c) <= 0){
      break;
    }
  }
fprintf(stderr, "RETURN %d of %d\n", ret, len);
  return ret;
}


int ncplane_box(ncplane* n, const cell* ul, const cell* ur,
                const cell* ll, const cell* lr, const cell* hl,
                const cell* vl, int ylen, int xlen){
  int yoff, xoff;
  ncplane_cursor_yx(n, &yoff, &xoff);
  if(xlen < 2 || ylen < 2){
    return -1;
  }
  if(ncplane_putc(n, ul) < 0){
    return -1;
  }
  if(xlen > 2){
    if(ncplane_hline(n, hl, xlen - 2) < 0){
      return -1;
    }
  }
  if(ncplane_putc(n, ur) < 0){
    return -1;
  }
  ++yoff;
  while(yoff < ylen - 1){
    if(ncplane_cursor_move_yx(n, yoff, xoff)){
      return -1;
    }
    if(ncplane_putc(n, vl) < 0){
      return -1;
    }
    if(ncplane_cursor_move_yx(n, yoff, xoff + xlen - 1)){
      return -1;
    }
    if(ncplane_putc(n, vl) < 0){
      return -1;
    }
    ++yoff;
  }
  if(ncplane_putc(n, ll) < 0){
    return -1;
  }
  if(xlen > 2){
    if(ncplane_hline(n, hl, xlen - 2) < 0){
      return -1;
    }
  }
  if(ncplane_putc(n, lr) < 0){
    return -1;
  }
  return 0;
}

void ncplane_erase(ncplane* n){
  // FIXME if we've been shrunk since creation, and haven't explicitly resized,
  // what happens here...?
  memset(n->fb, 0, sizeof(*n->fb) * n->lenx * n->leny);
  egcpool_dump(&n->pool);
  egcpool_init(&n->pool);
}
