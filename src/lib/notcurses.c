#include <ncurses.h> // needed for some definitions, see terminfo(3ncurses)
#include <time.h>
#include <term.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <libavformat/version.h>
#include "notcurses.h"
#include "timespec.h"
#include "version.h"
#include "egcpool.h"

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
  egcpool pool;         // attached storage pool for UTF-8 EGCs
  uint64_t channels;    // works the same way as cells
  uint32_t attrword;    // same deal as in a cell
} ncplane;

typedef struct ncstats {
  uint64_t renders;       // number of notcurses_render() runs
  uint64_t renders_ns;    // number of nanoseconds spent in notcurses_render()
  int64_t render_max_ns;  // max ns spent in notcurses_render()
  int64_t render_min_ns;  // min ns spent in successful notcurses_render()
} ncstats;

static void
update_render_stats(const struct timespec* time1, const struct timespec* time0,
                    ncstats* stats){
  int64_t elapsed = timespec_subtract_ns(time1, time0);
  //fprintf(stderr, "Rendering took %ld.%03lds\n", elapsed / 1000000000,
  //        (elapsed % 1000000000) / 1000000);
  if(elapsed > 0){ // don't count clearly incorrect information, egads
    ++stats->renders;
    stats->renders_ns += elapsed;
    if(elapsed > stats->render_max_ns){
      stats->render_max_ns = elapsed;
    }
    if(elapsed < stats->render_min_ns || stats->render_min_ns == 0){
      stats->render_min_ns = elapsed;
    }
  }
}

typedef struct notcurses {
  int ttyfd;      // file descriptor for controlling tty (takes stdin)
  int colors;     // number of colors usable for this screen
  ncstats stats;  // some statistics across the lifetime of the notcurses ctx
  // We verify that some terminfo capabilities exist. These needn't be checked
  // before further use; just use tiparm() directly.
  char* cup;      // move cursor
  char* civis;    // hide cursor
  char* clear;    // erase screen and home cursor
  // These might be NULL, and we can more or less work without them. Check!
  char* cnorm;    // restore cursor to default state
  char* smcup;    // enter alternate mode
  char* rmcup;    // restore primary mode
  char* setaf;    // set foreground color (ANSI)
  char* setab;    // set background color (ANSI)
  char* op;       // set foreground and background color to default
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

// anyone calling this needs ensure the ncplane's framebuffer is updated
// to reflect changes in geometry.
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
term_emit(const char* seq, FILE* out){
  int ret = fprintf(out, "%s", seq);
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

// create an ncplane of the specified dimensions, but do not yet place it in
// the z-buffer. clear out all cells. this is for a wholly new context.
static ncplane*
create_initial_ncplane(notcurses* nc){
  int rows, cols;
  if(update_term_dimensions(nc, &rows, &cols)){
    return NULL;
  }
  ncplane* p = malloc(sizeof(*p));
  size_t fbsize = sizeof(*p->fb) * (rows * cols);
  if((p->fb = malloc(fbsize)) == NULL){
    free(p);
    return NULL;
  }
  memset(p->fb, 0, fbsize);
  p->leny = rows;
  p->lenx = cols;
  p->x = p->y = 0;
  p->absx = p->absy = 0;
  p->attrword = 0;
  p->channels = 0;
  egcpool_init(&p->pool);
  return p;
}

// Call this when the screen size changes. Takes a flat
// array of *rows * *cols cells (may be NULL if *rows == *cols == 0). Gets the
// new size, and copies what can be copied from the old stdscr. Assumes that
// the screen is always anchored in the same place.
int notcurses_resize(notcurses* n){
  int oldrows = n->stdscr->leny;
  int oldcols = n->stdscr->lenx;
  int rows, cols;
  if(update_term_dimensions(n, &rows, &cols)){
    return -1;
  }
  ncplane* p = n->stdscr;
  cell* preserved = p->fb;
  size_t fbsize = sizeof(*preserved) * (rows * cols);
  if((p->fb = malloc(fbsize)) == NULL){
    p->fb = preserved;
    return -1;
  }
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
      memcpy(&p->fb[idx], &preserved[y * oldcols], oldcopy * sizeof(*p->fb));
    }
    if(p->lenx - oldcopy){
      memset(&p->fb[idx + oldcopy], 0, sizeof(*p->fb) * (p->lenx - oldcopy));
    }
  }
  free(preserved);
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
  }else if(nc->RGBflag && (unsigned)nc->colors < (1u << 24u)){
    fprintf(stderr, "Warning: advertised RGB flag but only %d colors\n",
            nc->colors);
  }
  term_verify_seq(&nc->cup, "cup");
  term_verify_seq(&nc->civis, "civis");
  term_verify_seq(&nc->clear, "clear");
  if(nc->cup == NULL || nc->civis == NULL || nc->clear == NULL){
    fprintf(stderr, "Required terminfo capability not defined\n");
    return -1;
  }
  if(!opts->retain_cursor){
    if(term_emit(tiparm(cursor_invisible), stdout)){
      return -1;
    }
    term_verify_seq(&nc->cnorm, "cnorm");
  }else{
    nc->cnorm = NULL;
  }
  term_verify_seq(&nc->standout, "smso"); // smso / rmso
  term_verify_seq(&nc->uline, "smul");
  term_verify_seq(&nc->reverse, "reverse");
  term_verify_seq(&nc->blink, "blink");
  term_verify_seq(&nc->dim, "dim");
  term_verify_seq(&nc->bold, "bold");
  term_verify_seq(&nc->italics, "sitm");
  term_verify_seq(&nc->op, "op");
  term_verify_seq(&nc->clear, "clear");
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
  ret->ttyfd = opts->outfd;
  if(tcgetattr(ret->ttyfd, &ret->tpreserved)){
    fprintf(stderr, "Couldn't preserve terminal state for %d (%s)\n",
            ret->ttyfd, strerror(errno));
    free(ret);
    return NULL;
  }
  memcpy(&modtermios, &ret->tpreserved, sizeof(modtermios));
  // see termios(3). disabling ECHO and ICANON means input will not be echoed
  // to the screen, input is made available without enter-based buffering, and
  // line editing is disabled. since we have not gone into raw mode, ctrl+c
  // etc. still have their typical effects.
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
  if((ret->top = create_initial_ncplane(ret)) == NULL){
    goto err;
  }
  ret->top->z = NULL;
  ret->stdscr = ret->top;
  memset(&ret->stats, 0, sizeof(ret->stats));
  if(ret->smcup && term_emit(ret->smcup, stdout)){
    free_plane(ret->top);
    goto err;
  }
  printf("notcurses %s by nick black\nterminfo from %s\n"
         "avformat %u.%u.%u\n"
         "%d rows, %d columns (%zub), %d colors (%s)\n",
         notcurses_version(), curses_version(), LIBAVFORMAT_VERSION_MAJOR,
         LIBAVFORMAT_VERSION_MINOR, LIBAVFORMAT_VERSION_MICRO,
         ret->top->leny, ret->top->lenx,
         ret->top->lenx * ret->top->leny * sizeof(*ret->top->fb),
         ret->colors, ret->RGBflag ? "direct" : "palette");
  return ret;

err:
  tcsetattr(ret->ttyfd, TCSANOW, &ret->tpreserved);
  free(ret);
  return NULL;
}

int notcurses_stop(notcurses* nc){
  int ret = 0;
  if(nc){
    if(nc->rmcup && term_emit(nc->rmcup, stdout)){
      ret = -1;
    }
    if(nc->cnorm && term_emit(nc->cnorm, stdout)){
      return -1;
    }
    ret |= tcsetattr(nc->ttyfd, TCSANOW, &nc->tpreserved);
    double avg = nc->stats.renders ?
             nc->stats.renders_ns / (double)nc->stats.renders : 0;
    fprintf(stderr, "%ju renders, %.03gs total (%.03gs min, %.03gs max, %.02gs avg)\n",
            nc->stats.renders,
            nc->stats.renders_ns / 1000000000.0,
            nc->stats.render_min_ns / 1000000000.0,
            nc->stats.render_max_ns / 1000000000.0,
            avg / 1000000000);
    while(nc->top){
      ncplane* p = nc->top;
      nc->top = p->z;
      free_plane(p);
    }
    free(nc);
  }
  return ret;
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
term_esc_rgb(FILE* out, int esc, unsigned r, unsigned g, unsigned b){
  #define RGBESC1 "\x1b["
  #define RGBESC2 "8;2;"
                                    // rrr;ggg;bbbm
  char rgbesc[] = RGBESC1 " " RGBESC2 "            ";
  int len = strlen(RGBESC1);
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
  int w;
  if((w = fprintf(out, "%.*s", len, rgbesc)) < len){
    return -1;
  }
  return 0;
}

static int
term_bg_rgb8(notcurses* nc, FILE* out, unsigned r, unsigned g, unsigned b){
  // We typically want to use tputs() and tiperm() to acquire and write the
  // escapes, as these take into account terminal-specific delays, padding,
  // etc. For the case of DirectColor, there is no suitable terminfo entry, but
  // we're also in that case working with hopefully more robust terminals.
  // If it doesn't work, eh, it doesn't work. Fuck the world; save yourself.
  if(nc->RGBflag){
    return term_esc_rgb(out, 4, r, g, b);
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
term_fg_rgb8(notcurses* nc, FILE* out, unsigned r, unsigned g, unsigned b){
  // We typically want to use tputs() and tiperm() to acquire and write the
  // escapes, as these take into account terminal-specific delays, padding,
  // etc. For the case of DirectColor, there is no suitable terminfo entry, but
  // we're also in that case working with hopefully more robust terminals.
  // If it doesn't work, eh, it doesn't work. Fuck the world; save yourself.
  if(nc->RGBflag){
    return term_esc_rgb(out, 3, r, g, b);
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

// is it a single ASCII byte, wholly contained within the cell?
static inline bool
simple_gcluster_p(const char* gcluster){
  if(*gcluster == '\0'){
    return true;
  }
  if(*(unsigned char*)gcluster >= 0x80){
    return false;
  }
  // we might be a simple ASCII, if the next character is *not* a nonspacing
  // combining character
  return false; // FIXME
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

// write the cell's UTF-8 grapheme cluster to the provided FILE*
static int
term_putc(FILE* out, const ncplane* n, const cell* c){
  if(simple_cell_p(c)){
    if(c->gcluster == 0){
      if(fputc(' ', out) == EOF){
        return -1;
      }
    }else{
      if(fputc(c->gcluster, out) == EOF){
        return -1;
      }
    }
  }else{
    const char* ext = extended_gcluster(n, c);
    if(fprintf(out, "%s", ext) < 0){ // FIXME check for short write
      return -1;
    }
  }
  return 0;
}

static void
advance_cursor(ncplane* n, int cols){
  if(n->y == n->leny){
    if(n->x == n->lenx){
      return; // stuck!
    }
  }
  if((n->x += cols) >= n->lenx){
    if(n->y == n->leny){
      n->x = n->lenx;
    }else{
      n->x -= n->lenx;
      ++n->y;
    }
  }
}

// FIXME this needs to keep an invalidation bitmap, rather than blitting the
// world every time
int notcurses_render(notcurses* nc){
  struct timespec start, done;
  clock_gettime(CLOCK_MONOTONIC, &start);
  int ret = 0;
  int y, x;
  char* buf = NULL;
  size_t buflen = 0;
  FILE* out = open_memstream(&buf, &buflen); // worth keeping around?
  if(out == NULL){
    return -1;
  }
  term_emit(nc->clear, out);
  for(y = 0 ; y < nc->stdscr->leny ; ++y){
    for(x = 0 ; x < nc->stdscr->lenx ; ++x){
      unsigned r, g, b, br, bg, bb;
      // FIXME z-culling!
      const cell* c = &nc->stdscr->fb[fbcellidx(nc->stdscr, y, x)];
      // we allow these to be set distinctly, but terminfo only supports using
      // them both via the 'op' capability. unless we want to generate the 'op'
      // escapes ourselves, if either is set to default, we first send op, and
      // then a turnon for whichever aren't default.
      if(cell_fg_default_p(c) || cell_bg_default_p(c)){
        term_emit(nc->op, out);
      }
      if(!cell_fg_default_p(c)){
        cell_get_fg(c, &r, &g, &b);
        term_fg_rgb8(nc, out, r, g, b);
      }
      if(!cell_bg_default_p(c)){
        cell_get_bg(c, &br, &bg, &bb);
        term_bg_rgb8(nc, out, br, bg, bb);
      }
      term_putc(out, nc->stdscr, c);
    }
  }
  ret |= fclose(out);
  printf("%s", buf);
  fflush(stdout);
  clock_gettime(CLOCK_MONOTONIC, &done);
  free(buf);
  update_render_stats(&done, &start, &nc->stats);
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

int cell_duplicate(ncplane* n, cell* targ, const cell* c){
  cell_release(n, targ);
  targ->attrword = c->attrword;
  targ->channels = c->channels;
  if(simple_cell_p(c)){
    targ->gcluster = c->gcluster;
    return !!c->gcluster;
  }
  size_t ulen;
  int cols;
  // FIXME insert colcount into cell...if it's ever valid, anyway
  int eoffset = egcpool_stash(&n->pool, extended_gcluster(n, c), &ulen, &cols);
  if(eoffset < 0){
    return -1;
  }
  targ->gcluster = eoffset + 0x80;
  return ulen;
}

// FIXME check that it's not a '\n'? newlines cause annoying, tricky problems
int ncplane_putc(ncplane* n, const cell* c){
  if(n->y == n->leny && n->x == n->lenx){
    return -1;
  }
  cell* targ = &n->fb[fbcellidx(n, n->y, n->x)];
  int ret = cell_duplicate(n, targ, c);
  advance_cursor(n, 1 + cell_wide_p(c)); // FIXME
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
  int bytes;
  int cols;
  if((bytes = utf8_egc_len(gcluster, &cols)) >= 0 && bytes <= 1){
    c->gcluster = *gcluster;
    return !!c->gcluster;
  }
  if(cols > 1){
    c->channels |= CELL_WIDEASIAN_MASK;
  }else{
    c->channels &= ~CELL_WIDEASIAN_MASK;
  }
  size_t ulen;
  // FIXME feed in already-calculated lengths from prior utf8_egc_len()!
  int eoffset = egcpool_stash(&n->pool, gcluster, &ulen, &cols);
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
    c.channels = n->channels;
    c.attrword = n->attrword;
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
  if(ret > 0){
    ret = ncplane_putstr(n, buf); // FIXME handle short writes also!
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
    if(ncplane_cursor_move_yx(n, ypos + ret, xpos)){
      return -1;
    }
    if(ncplane_putc(n, c) <= 0){
      break;
    }
  }
  return ret;
}


int ncplane_box(ncplane* n, const cell* ul, const cell* ur,
                const cell* ll, const cell* lr, const cell* hl,
                const cell* vl, int ystop, int xstop){
  int yoff, xoff, ymax, xmax;
  ncplane_cursor_yx(n, &yoff, &xoff);
  if(ystop < yoff + 1){
    return -1;
  }
  if(xstop < xoff + 1){
    return -1;
  }
  ncplane_dimyx(n, &ymax, &xmax);
  if(xstop >= xmax || ystop >= ymax){
    return -1;
  }
  if(ncplane_putc(n, ul) < 0){
    return -1;
  }
  if(xstop - xoff >= 2){
    if(ncplane_hline(n, hl, xstop - xoff - 1) < 0){
      return -1;
    }
  }
  if(ncplane_putc(n, ur) < 0){
    return -1;
  }
  ++yoff;
  while(yoff < ystop){
    if(ncplane_cursor_move_yx(n, yoff, xoff)){
      return -1;
    }
    if(ncplane_putc(n, vl) < 0){
      return -1;
    }
    if(ncplane_cursor_move_yx(n, yoff, xstop)){
      return -1;
    }
    if(ncplane_putc(n, vl) < 0){
      return -1;
    }
    ++yoff;
  }
  if(ncplane_cursor_move_yx(n, yoff, xoff)){
    return -1;
  }
  if(ncplane_putc(n, ll) < 0){
    return -1;
  }
  if(xstop - xoff >= 2){
    if(ncplane_hline(n, hl, xstop - xoff - 1) < 0){
      return -1;
    }
  }
  if(ncplane_putc(n, lr) < 0){
    return -1;
  }
  return 0;
}

void ncplane_erase(ncplane* n){
  memset(n->fb, 0, sizeof(*n->fb) * n->lenx * n->leny);
  egcpool_dump(&n->pool);
  egcpool_init(&n->pool);
  n->channels = 0;
  n->attrword = 0;
}
