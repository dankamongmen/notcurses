#include <ncurses.h> // needed for some definitions, see terminfo(3ncurses)
#include <time.h>
#include <term.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <langinfo.h>
#include <sys/poll.h>
#include <stdatomic.h>
#include <sys/ioctl.h>
#include "notcurses.h"
#include "internal.h"
#include "version.h"
#include "egcpool.h"

#define ESC "\x1b"

// only one notcurses object can be the target of signal handlers, due to their
// process-wide nature.
static notcurses* _Atomic signal_nc = ATOMIC_VAR_INIT(NULL); // ugh
static void (*signal_sa_handler)(int); // stashed signal handler we replaced

// this wildly unsafe handler will attempt to restore the screen upon
// reception of SIG{INT, SEGV, ABRT, QUIT}. godspeed you, black emperor!
static void
fatal_handler(int signo){
  notcurses* nc = atomic_load(&signal_nc);
  if(nc){
    notcurses_stop(nc);
    if(signal_sa_handler){
      signal_sa_handler(signo);
    }
    raise(signo);
  }
}

static int
setup_signals(notcurses* nc, bool no_quit_sigs, bool no_winch_sig){
  notcurses* expected = NULL;
  struct sigaction oldact;
  struct sigaction sa;

  if(!atomic_compare_exchange_strong(&signal_nc, &expected, nc)){
    fprintf(stderr, "%p is already registered for signals\n", expected);
    return -1;
  }
  if(!no_winch_sig){
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigwinch_handler;
    if(sigaction(SIGWINCH, &sa, NULL)){
      atomic_store(&signal_nc, NULL);
      fprintf(stderr, "Error installing SIGWINCH handler (%s)\n",
              strerror(errno));
      return -1;
    }
  }
  if(!no_quit_sigs){
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = fatal_handler;
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGQUIT);
    sigaddset(&sa.sa_mask, SIGSEGV);
    sigaddset(&sa.sa_mask, SIGABRT);
    sa.sa_flags = SA_RESETHAND; // don't try twice
    int ret = 0;
    ret |= sigaction(SIGINT, &sa, &oldact);
    ret |= sigaction(SIGQUIT, &sa, &oldact);
    ret |= sigaction(SIGSEGV, &sa, &oldact);
    ret |= sigaction(SIGABRT, &sa, &oldact);
    if(ret){
      atomic_store(&signal_nc, NULL);
      fprintf(stderr, "Error installing fatal signal handlers (%s)\n",
              strerror(errno));
      return -1;
    }
    signal_sa_handler = oldact.sa_handler;
  }
  return 0;
}

static int
drop_signals(notcurses* nc){
  notcurses* old = nc;
  if(!atomic_compare_exchange_strong(&signal_nc, &old, NULL)){
    fprintf(stderr, "Can't drop signals: %p != %p\n", old, nc);
    return -1;
  }
  return 0;
}

// make a heap-allocated wchar_t expansion of the multibyte string at s
wchar_t* wchar_from_utf8(const char* s){
  mbstate_t ps;
  memset(&ps, 0, sizeof(ps));
  // worst case is a wchar_t for every byte of input (all-ASCII case)
  const size_t maxw = strlen(s) + 1;
  wchar_t* dst = malloc(sizeof(*dst) * maxw);
  size_t wcount = mbsrtowcs(dst, &s, maxw, &ps);
  if(wcount == (size_t)-1){
    free(dst);
    return NULL;
  }
  if(s){
    free(dst);
    return NULL;
  }
  return dst;
}

int ncplane_putstr_aligned(ncplane* n, int y, ncalign_e align, const char* s){
  wchar_t* w = wchar_from_utf8(s);
  if(w == NULL){
    return -1;
  }
  int r = ncplane_putwstr_aligned(n, y, align, w);
  free(w);
  return r;
}

static const char NOTCURSES_VERSION[] =
 notcurses_VERSION_MAJOR "."
 notcurses_VERSION_MINOR "."
 notcurses_VERSION_PATCH;

const char* notcurses_version(void){
  return NOTCURSES_VERSION;
}

void* ncplane_set_userptr(ncplane* n, void* opaque){
  void* ret = n->userptr;
  n->userptr = opaque;
  return ret;
}

void* ncplane_userptr(ncplane* n){
  return n->userptr;
}

const void* ncplane_userptr_const(const ncplane* n){
  return n->userptr;
}

// is the cursor in an invalid position? it never should be, but it's probably
// better to make sure (it's cheap) than to read from/write to random crap.
static bool
cursor_invalid_p(const ncplane* n){
  if(n->y >= n->leny || n->x >= n->lenx){
    return true;
  }
  if(n->y < 0 || n->x < 0){
    return true;
  }
  return false;
}

int ncplane_at_cursor(ncplane* n, cell* c){
  if(cursor_invalid_p(n)){
    return -1;
  }
  return cell_duplicate(n, c, &n->fb[fbcellidx(n, n->y, n->x)]);
}

int ncplane_at_yx(ncplane* n, int y, int x, cell* c){
  int ret = -1;
  pthread_mutex_lock(&n->nc->lock);
  if(y < n->leny && x < n->lenx){
    if(y >= 0 && x >= 0){
      ret = cell_duplicate(n, c, &n->fb[fbcellidx(n, y, x)]);
    }
  }
  pthread_mutex_unlock(&n->nc->lock);
  return ret;
}

cell* ncplane_cell_ref_yx(ncplane* n, int y, int x){
  return &n->fb[fbcellidx(n, y, x)];
}

void ncplane_dim_yx(ncplane* n, int* rows, int* cols){
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
  if(rows){
    *rows = ws.ws_row;
  }
  if(cols){
    *cols = ws.ws_col;
  }
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

// create a new ncplane at the specified location (relative to the true screen,
// having origin at 0,0), having the specified size, and put it at the top of
// the planestack. its cursor starts at its origin; its style starts as null.
// a plane may exceed the boundaries of the screen, but must have positive
// size in both dimensions.
static ncplane*
ncplane_create(notcurses* nc, int rows, int cols, int yoff, int xoff){
  if(rows <= 0 || cols <= 0){
    return NULL;
  }
  ncplane* p = malloc(sizeof(*p));
  size_t fbsize = sizeof(*p->fb) * (rows * cols);
  if((p->fb = malloc(fbsize)) == NULL){
    free(p);
    return NULL;
  }
  memset(p->fb, 0, fbsize);
  p->userptr = NULL;
  p->leny = rows;
  p->lenx = cols;
  p->x = p->y = 0;
  p->absx = xoff;
  p->absy = yoff;
  p->attrword = 0;
  p->channels = 0;
  egcpool_init(&p->pool);
  p->z = nc->top;
  nc->top = p;
  p->nc = nc;
  cell_init(&p->basecell);
  nc->stats.fbbytes += fbsize;
  return p;
}

// create an ncplane of the specified dimensions, but do not yet place it in
// the z-buffer. clear out all cells. this is for a wholly new context.
static ncplane*
create_initial_ncplane(notcurses* nc){
  int rows, cols;
  if(update_term_dimensions(nc, &rows, &cols)){
    return NULL;
  }
  nc->stdscr = ncplane_create(nc, rows, cols, 0, 0);
  return nc->stdscr;
}

ncplane* notcurses_stdplane(notcurses* nc){
  return nc->stdscr;
}

const ncplane* notcurses_stdplane_const(const notcurses* nc){
  return nc->stdscr;
}

ncplane* ncplane_new(notcurses* nc, int rows, int cols,
                            int yoff, int xoff, void* opaque){
  ncplane* n = ncplane_create(nc, rows, cols, yoff, xoff);
  if(n == NULL){
    return n;
  }
  n->userptr = opaque;
  return n;
}

struct ncplane* ncplane_aligned(struct ncplane* n, int rows, int cols,
                                int yoff, ncalign_e align, void* opaque){
  return ncplane_new(n->nc, rows, cols, yoff, ncplane_align(n, align, cols), opaque);
}

// can be used on stdscr, unlike ncplane_resize() which prohibits it.
static int
ncplane_resize_internal(ncplane* n, int keepy, int keepx, int keepleny,
                       int keeplenx, int yoff, int xoff, int ylen, int xlen){
  if(keepleny < 0 || keeplenx < 0){ // can't retain negative size
//fprintf(stderr, "Can't retain negative size %dx%d\n", keepleny, keeplenx);
    return -1;
  }
  if(ylen <= 0 || xlen <= 0){ // can't resize to trivial or negative size
//fprintf(stderr, "Can't achieve negative size %dx%d\n", ylen, xlen);
    return -1;
  }
  if((!keepleny && keeplenx) || (keepleny && !keeplenx)){ // both must be 0
//fprintf(stderr, "Can't keep zero dimensions %dx%d\n", keepleny, keeplenx);
    return -1;
  }
  if(ylen < keepleny || xlen < keeplenx){ // can't be smaller than our keep
//fprintf(stderr, "Can't violate space %dx%d vs %dx%d\n", keepleny, keeplenx, ylen, xlen);
    return -1;
  }
  int rows, cols;
  ncplane_dim_yx(n, &rows, &cols);
//fprintf(stderr, "NCPLANE(RESIZING) to %dx%d at %d/%d (keeping %dx%d from %d/%d)\n",
//        ylen, xlen, yoff, xoff, keepleny, keeplenx, keepy, keepx);
  // we're good to resize. we'll need alloc up a new framebuffer, and copy in
  // those elements we're retaining, zeroing out the rest. alternatively, if
  // we've shrunk, we will be filling the new structure.
  int keptarea = keepleny * keeplenx;
  int newarea = ylen * xlen;
  size_t fbsize = sizeof(cell) * newarea;
  cell* fb = malloc(fbsize);
  if(fb == NULL){
    return -1;
  }
  // update the cursor, if it would otherwise be off-plane
  if(n->y >= ylen){
    n->y = ylen - 1;
  }
  if(n->x >= xlen){
    n->x = xlen - 1;
  }
  cell* preserved = n->fb;
  n->nc->stats.fbbytes -= sizeof(*preserved) * (rows * cols);
  n->nc->stats.fbbytes += fbsize;
  n->fb = fb;
  n->absy = n->absy + keepy - yoff;
  n->absx = n->absx + keepx - xoff;
  // if we're keeping nothing, dump the old egcspool. otherwise, we go ahead
  // and keep it. perhaps we ought compact it?
  if(keptarea == 0){ // keep nothing, resize/move only
    memset(fb, 0, sizeof(*fb) * newarea);
    egcpool_dump(&n->pool);
    n->lenx = xlen;
    n->leny = ylen;
    free(preserved);
    return 0;
  }
  // we currently have maxy rows of maxx cells each. we will be keeping rows
  // keepy..keepy + keepleny - 1 and columns keepx..keepx + keeplenx - 1. they
  // will end up at keepy + yoff..keepy + keepleny - 1 + yoff and
  // keepx + xoff..keepx + keeplenx - 1 + xoff. everything else is zerod out.
  int itery;
  // we'll prepare each cell in our new framebuffer with either zeroes or a copy
  // from the old one.
  int sourceline = keepy;
  for(itery = 0 ; itery < ylen ; ++itery){
    int copyoff = itery * xlen; // our target at any given time
    // if we have nothing copied to this line, zero it out in one go
    if(itery < keepy || itery > keepy + keepleny - 1){
      memset(fb + copyoff, 0, sizeof(*fb) * xlen);
      continue;
    }
    // we do have something to copy, and zero, one, or two regions to zero out
    int copied = 0;
    if(xoff < 0){
      memset(fb + copyoff, 0, sizeof(*fb) * -xoff);
      copyoff += -xoff;
      copied += -xoff;
    }
    const int sourceidx = fbcellidx(n, sourceline, keepx);
    memcpy(fb + copyoff, preserved + sourceidx, sizeof(*fb) * keeplenx);
    copyoff += keeplenx;
    copied += keeplenx;
    if(xlen > copied){
      memset(fb + copyoff, 0, sizeof(*fb) * (xlen - copied));
    }
    ++sourceline;
  }
  n->lenx = xlen;
  n->leny = ylen;
  free(preserved);
  return 0;
}

int ncplane_resize(ncplane* n, int keepy, int keepx, int keepleny,
                   int keeplenx, int yoff, int xoff, int ylen, int xlen){
  if(n == n->nc->stdscr){
//fprintf(stderr, "Can't resize standard plane\n");
    return -1;
  }
  return ncplane_resize_internal(n, keepy, keepx, keepleny, keeplenx,
                                 yoff, xoff, ylen, xlen);
}

// Call this when the screen size changes. Acquires the new size, and copies
// what can be copied from the old stdscr. Assumes that the screen is always
// anchored in the same place.
int notcurses_resize(notcurses* n, int* rows, int* cols){
  int r, c;
  if(rows == NULL){
    rows = &r;
  }
  if(cols == NULL){
    cols = &c;
  }
  int oldrows = n->stdscr->leny;
  int oldcols = n->stdscr->lenx;
  if(update_term_dimensions(n, rows, cols)){
    return -1;
  }
  if(*rows == oldrows && *cols == oldcols){
    return 0; // no change
  }
  int keepy = *rows;
  if(keepy > oldrows){
    keepy = oldrows;
  }
  int keepx = *cols;
  if(keepx > oldcols){
    keepx = oldcols;
  }
  if(ncplane_resize_internal(n->stdscr, 0, 0, keepy, keepx, 0, 0, *rows, *cols)){
    return -1;
  }
  return 0;
}

// find the pointer on the z-index referencing the specified plane. writing to
// this pointer will remove the plane (and everything below it) from the stack.
static ncplane**
find_above_ncplane(ncplane* n){
  notcurses* nc = n->nc;
  ncplane** above = &nc->top;
  while(*above){
    if(*above == n){
      return above;
    }
    above = &(*above)->z;
  }
  return NULL;
}

int ncplane_destroy(ncplane* ncp){
  if(ncp == NULL){
    return 0;
  }
  if(ncp->nc->stdscr == ncp){
    return -1;
  }
  ncplane** above = find_above_ncplane(ncp);
  if(*above){
    *above = ncp->z; // splice it out of the list
    free_plane(ncp);
    return 0;
  }
  // couldn't find it in our stack. don't try to free this interloper.
  return -1;
}

static int
interrogate_terminfo(notcurses* nc, const notcurses_options* opts){
  char* longname_term = longname();
  if(!opts->suppress_banner){
    fprintf(stderr, "Term: %s\n", longname_term ? longname_term : "?");
  }
  nc->RGBflag = tigetflag("RGB") == 1;
  if (nc->RGBflag == 0) {
    // RGB terminfo capability being a new thing (as of ncurses 6.1), it's not commonly found in
    // terminal entries today. COLORTERM, however, is a de-facto (if imperfect/kludgy) standard way
    // of indicating DirectColor support for a terminal. The variable takes one of two case-sensitive
    // values:
    //
    //   truecolor
    //   24bit
    //
    // https://gist.github.com/XVilka/8346728#true-color-detection gives some more information about
    // the topic
    //
    const char* cterm = getenv("COLORTERM");
    nc->RGBflag = cterm && (strcmp(cterm, "truecolor") == 0 || strcmp(cterm, "24bit") == 0);
  }
  nc->CCCflag = tigetflag("ccc") == 1;
  if((nc->colors = tigetnum("colors")) <= 0){
    if(!opts->suppress_banner){
      fprintf(stderr, "This terminal doesn't appear to support colors\n");
    }
    nc->colors = 1;
  }else if(nc->RGBflag && (unsigned)nc->colors < (1u << 24u)){
    if(!opts->suppress_banner){
      fprintf(stderr, "Warning: advertised RGB flag but only %d colors\n",
              nc->colors);
    }
  }
  term_verify_seq(&nc->cup, "cup");
  if(nc->cup == NULL){
    fprintf(stderr, "Required terminfo capability not defined\n");
    return -1;
  }
  term_verify_seq(&nc->civis, "civis");
  term_verify_seq(&nc->cnorm, "cnorm");
  term_verify_seq(&nc->standout, "smso"); // smso / rmso
  term_verify_seq(&nc->uline, "smul");
  term_verify_seq(&nc->reverse, "reverse");
  term_verify_seq(&nc->blink, "blink");
  term_verify_seq(&nc->dim, "dim");
  term_verify_seq(&nc->bold, "bold");
  term_verify_seq(&nc->italics, "sitm");
  term_verify_seq(&nc->italoff, "ritm");
  term_verify_seq(&nc->sgr, "sgr");
  term_verify_seq(&nc->sgr0, "sgr0");
  term_verify_seq(&nc->op, "op");
  term_verify_seq(&nc->clearscr, "clear");
  term_verify_seq(&nc->cleareol, "el");
  term_verify_seq(&nc->clearbol, "el1");
  term_verify_seq(&nc->cuf, "cuf"); // n non-destructive spaces
  term_verify_seq(&nc->cuf1, "cuf1"); // non-destructive space
  term_verify_seq(&nc->cub1, "cub1");
  term_verify_seq(&nc->smkx, "smkx"); // set application mode
  if(nc->smkx){
    if(putp(tiparm(nc->smkx)) != OK){
      fprintf(stderr, "Error entering application mode\n");
      return -1;
    }
  }
  if(prep_special_keys(nc)){
    return -1;
  }
  // Some terminals cannot combine certain styles with colors. Don't advertise
  // support for the style in that case.
  int nocolor_stylemask = tigetnum("ncv");
  if(nocolor_stylemask > 0){
    if(nocolor_stylemask & WA_STANDOUT){ // ncv is composed of terminfo bits, not ours
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
  term_verify_seq(&nc->getm, "getm"); // get mouse events
  // Not all terminals support setting the fore/background independently
  term_verify_seq(&nc->setaf, "setaf");
  term_verify_seq(&nc->setab, "setab");
  term_verify_seq(&nc->smkx, "smkx");
  term_verify_seq(&nc->rmkx, "rmkx");
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

static int
make_nonblocking(FILE* fp){
  int fd = fileno(fp);
  if(fd < 0){
    return -1;
  }
  int flags = fcntl(fd, F_GETFL, 0);
  if(flags < 0){
    return -1;
  }
  return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void
reset_stats(ncstats* stats){
  uint64_t fbbytes = stats->fbbytes;
  memset(stats, 0, sizeof(*stats));
  stats->render_min_ns = 1ul << 62u;
  stats->render_min_bytes = 1ul << 62u;
  stats->fbbytes = fbbytes;
}

// add the current stats to the cumulative stashed stats, and reset them
static void
stash_stats(notcurses* nc){
  nc->stashstats.renders += nc->stats.renders;
  nc->stashstats.render_ns += nc->stats.render_ns;
  nc->stashstats.failed_renders += nc->stats.failed_renders;
  nc->stashstats.render_bytes += nc->stats.render_bytes;
  if(nc->stashstats.render_max_bytes < nc->stats.render_max_bytes){
    nc->stashstats.render_max_bytes = nc->stats.render_max_bytes;
  }
  if(nc->stashstats.render_max_ns < nc->stats.render_max_ns){
    nc->stashstats.render_max_ns = nc->stats.render_max_ns;
  }
  if(nc->stashstats.render_min_bytes > nc->stats.render_min_bytes){
    nc->stashstats.render_min_bytes = nc->stats.render_min_bytes;
  }
  if(nc->stashstats.render_min_ns > nc->stats.render_min_ns){
    nc->stashstats.render_min_ns = nc->stats.render_min_ns;
  }
  nc->stashstats.cellelisions += nc->stats.cellelisions;
  nc->stashstats.cellemissions += nc->stats.cellemissions;
  nc->stashstats.fgelisions += nc->stats.fgelisions;
  nc->stashstats.fgemissions += nc->stats.fgemissions;
  nc->stashstats.bgelisions += nc->stats.bgelisions;
  nc->stashstats.bgemissions += nc->stats.bgemissions;
  nc->stashstats.defaultelisions += nc->stats.defaultelisions;
  nc->stashstats.defaultemissions += nc->stats.defaultemissions;
  // fbbytes aren't stashed
  reset_stats(&nc->stats);
}

void notcurses_stats(notcurses* nc, ncstats* stats){
  pthread_mutex_lock(&nc->lock);
  memcpy(stats, &nc->stats, sizeof(*stats));
  pthread_mutex_unlock(&nc->lock);
}

void notcurses_reset_stats(notcurses* nc, ncstats* stats){
  pthread_mutex_lock(&nc->lock);
  memcpy(stats, &nc->stats, sizeof(*stats));
  stash_stats(nc);
  pthread_mutex_unlock(&nc->lock);
}

// Convert a notcurses log level to its ffmpeg equivalent.
static int
ffmpeg_log_level(ncloglevel_e level){
  switch(level){
    case NCLOGLEVEL_SILENT: return AV_LOG_QUIET;
    case NCLOGLEVEL_PANIC: return AV_LOG_PANIC;
    case NCLOGLEVEL_FATAL: return AV_LOG_FATAL;
    case NCLOGLEVEL_ERROR: return AV_LOG_ERROR;
    case NCLOGLEVEL_WARNING: return AV_LOG_WARNING;
    case NCLOGLEVEL_INFO: return AV_LOG_INFO;
    case NCLOGLEVEL_VERBOSE: return AV_LOG_VERBOSE;
    case NCLOGLEVEL_DEBUG: return AV_LOG_DEBUG;
    case NCLOGLEVEL_TRACE: return AV_LOG_TRACE;
    default: break;
  }
  fprintf(stderr, "Invalid log level: %d\n", level);
  return AV_LOG_TRACE;
}

notcurses* notcurses_init(const notcurses_options* opts, FILE* outfp){
  const char* encoding = nl_langinfo(CODESET);
  if(encoding == NULL || strcmp(encoding, "UTF-8")){
    fprintf(stderr, "Encoding (\"%s\") wasn't UTF-8, refusing to start\n",
            encoding ? encoding : "none found");
    return NULL;
  }
  struct termios modtermios;
  notcurses* ret = malloc(sizeof(*ret));
  if(ret == NULL){
    return ret;
  }
  if(pthread_mutex_init(&ret->lock, NULL)){
    free(ret);
    return NULL;
  }
  ret->stats.fbbytes = 0;
  ret->stashstats.fbbytes = 0;
  reset_stats(&ret->stats);
  reset_stats(&ret->stashstats);
  ret->ttyfp = outfp;
  ret->renderfp = opts->renderfp;
  ret->inputescapes = NULL;
  ret->ttyinfp = stdin; // FIXME
  memset(&ret->rstate, 0, sizeof(ret->rstate));
  ret->lastframe = NULL;
  ret->lfdimy = 0;
  ret->lfdimx = 0;
  egcpool_init(&ret->pool);
  if(make_nonblocking(ret->ttyinfp)){
    free(ret);
    return NULL;
  }
  ret->inputbuf_occupied = 0;
  ret->inputbuf_valid_starts = 0;
  ret->inputbuf_write_at = 0;
  if((ret->ttyfd = fileno(ret->ttyfp)) < 0){
    fprintf(stderr, "No file descriptor was available in outfp %p\n", outfp);
    free(ret);
    return NULL;
  }
  notcurses_mouse_disable(ret);
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
  if(setup_signals(ret, opts->no_quit_sighandlers, opts->no_winch_sighandler)){
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
  if(ncvisual_init(ffmpeg_log_level(opts->loglevel))){
    goto err;
  }
  if((ret->stdscr = create_initial_ncplane(ret)) == NULL){
    goto err;
  }
  if(!opts->retain_cursor){
    if(ret->civis && term_emit("civis", ret->civis, ret->ttyfp, false)){
      free_plane(ret->top);
      goto err;
    }
  }
  if(ret->smkx && term_emit("smkx", ret->smkx, ret->ttyfp, false)){
    free_plane(ret->top);
    goto err;
  }
  if(ret->smcup && term_emit("smcup", ret->smcup, ret->ttyfp, false)){
    free_plane(ret->top);
    goto err;
  }
  if((ret->rstate.mstreamfp = open_memstream(&ret->rstate.mstream, &ret->rstate.mstrsize)) == NULL){
    free_plane(ret->top);
    goto err;
  }
  ret->suppress_banner = opts->suppress_banner;
  if(!opts->suppress_banner){
    char prefixbuf[BPREFIXSTRLEN + 1];
    fprintf(ret->ttyfp, "\n"
          " notcurses %s by nick black\n"
          " %d rows, %d columns (%sB), %d colors (%s)\n"
          " compiled with gcc-%s\n"
          " terminfo from %s\n",
          notcurses_version(),
          ret->stdscr->leny, ret->stdscr->lenx,
          bprefix(ret->stats.fbbytes, 1, prefixbuf, 0),
          ret->colors, ret->RGBflag ? "direct" : "palette",
          __VERSION__, curses_version());
#ifndef DISABLE_FFMPEG
    fprintf(ret->ttyfp, " avformat %u.%u.%u\n avutil %u.%u.%u\n swscale %u.%u.%u\n",
          LIBAVFORMAT_VERSION_MAJOR, LIBAVFORMAT_VERSION_MINOR, LIBAVFORMAT_VERSION_MICRO,
          LIBAVUTIL_VERSION_MAJOR, LIBAVUTIL_VERSION_MINOR, LIBAVUTIL_VERSION_MICRO,
          LIBSWSCALE_VERSION_MAJOR, LIBSWSCALE_VERSION_MINOR, LIBSWSCALE_VERSION_MICRO);
#else
    putp(tiparm(ret->setaf, 3));
    fprintf(ret->ttyfp, " warning: built without ffmpeg support\n");
#endif
    if(!ret->RGBflag){ // FIXME
      if(ret->colors >= 16){
        putp(tiparm(ret->setaf, 207));
      }else{
        putp(tiparm(ret->setaf, 3));
      }
      fprintf(ret->ttyfp, "\nWarning!\nYour colors are subject to https://github.com/dankamongmen/notcurses/issues/4\n");
      fprintf(ret->ttyfp, "Are you specifying a proper DirectColor TERM?\n");
    }
  }
  if(opts->clear_screen_start){
    term_emit("clear", ret->clearscr, ret->ttyfp, false);
  }
  return ret;

err:
  tcsetattr(ret->ttyfd, TCSANOW, &ret->tpreserved);
  pthread_mutex_destroy(&ret->lock);
  free(ret);
  return NULL;
}

int notcurses_stop(notcurses* nc){
  int ret = 0;
  if(nc){
    drop_signals(nc);
    // FIXME these can fail if we stop in the middle of a rendering operation.
    // turn the fd back to blocking, perhaps?
    if(nc->rmcup && term_emit("rmcup", nc->rmcup, nc->ttyfp, true)){
      ret = -1;
    }
    if(nc->cnorm && term_emit("cnorm", nc->cnorm, nc->ttyfp, true)){
      ret = -1;
    }
    if(nc->op && term_emit("op", nc->op, nc->ttyfp, true)){
      ret = -1;
    }
    if(nc->sgr0 && term_emit("sgr0", nc->sgr0, nc->ttyfp, true)){
      ret = -1;
    }
    ret |= notcurses_mouse_disable(nc);
    ret |= tcsetattr(nc->ttyfd, TCSANOW, &nc->tpreserved);
    while(nc->top){
      ncplane* p = nc->top;
      nc->top = p->z;
      free_plane(p);
    }
    if(nc->rstate.mstreamfp){
      fclose(nc->rstate.mstreamfp);
    }
    egcpool_dump(&nc->pool);
    free(nc->lastframe);
    free(nc->rstate.mstream);
    input_free_esctrie(&nc->inputescapes);
    ret |= pthread_mutex_destroy(&nc->lock);
    stash_stats(nc);
    if(!nc->suppress_banner){
      if(nc->stashstats.renders){
        char totalbuf[BPREFIXSTRLEN + 1];
        char minbuf[BPREFIXSTRLEN + 1];
        char maxbuf[BPREFIXSTRLEN + 1];
        double avg = nc->stashstats.render_ns / (double)nc->stashstats.renders;
        fprintf(stderr, "\n%ju render%s, %.03gs total (%.03gs min, %.03gs max, %.02gs avg %.1f fps)\n",
                nc->stashstats.renders, nc->stashstats.renders == 1 ? "" : "s",
                nc->stashstats.render_ns / 1000000000.0,
                nc->stashstats.render_min_ns / 1000000000.0,
                nc->stashstats.render_max_ns / 1000000000.0,
                avg / NANOSECS_IN_SEC, NANOSECS_IN_SEC / avg);
        avg = nc->stashstats.render_bytes / (double)nc->stashstats.renders;
        bprefix(nc->stashstats.render_bytes, 1, totalbuf, 0),
        bprefix(nc->stashstats.render_min_bytes, 1, minbuf, 0),
        bprefix(nc->stashstats.render_max_bytes, 1, maxbuf, 0),
        fprintf(stderr, "%sB total (%sB min, %sB max, %.02fKiB avg)\n",
                totalbuf, minbuf, maxbuf,
                avg / 1024);
      }
      fprintf(stderr, "%ju failed render%s\n", nc->stashstats.failed_renders,
              nc->stashstats.failed_renders == 1 ? "" : "s");
      fprintf(stderr, "Emits/elides: def %lu/%lu fg %lu/%lu bg %lu/%lu\n",
              nc->stashstats.defaultemissions,
              nc->stashstats.defaultelisions,
              nc->stashstats.fgemissions,
              nc->stashstats.fgelisions,
              nc->stashstats.bgemissions,
              nc->stashstats.bgelisions);
      fprintf(stderr, " Elide rates: %.2f%% %.2f%% %.2f%%\n",
              (nc->stashstats.defaultemissions + nc->stashstats.defaultelisions) == 0 ? 0 :
              (nc->stashstats.defaultelisions * 100.0) / (nc->stashstats.defaultemissions + nc->stashstats.defaultelisions),
              (nc->stashstats.fgemissions + nc->stashstats.fgelisions) == 0 ? 0 :
              (nc->stashstats.fgelisions * 100.0) / (nc->stashstats.fgemissions + nc->stashstats.fgelisions),
              (nc->stashstats.bgemissions + nc->stashstats.bgelisions) == 0 ? 0 :
              (nc->stashstats.bgelisions * 100.0) / (nc->stashstats.bgemissions + nc->stashstats.bgelisions));
      fprintf(stderr, "Cells emitted: %ju elided: %ju (%.2f%%)\n",
              nc->stashstats.cellemissions, nc->stashstats.cellelisions,
              (nc->stashstats.cellemissions + nc->stashstats.cellelisions) == 0 ? 0 :
              (nc->stashstats.cellelisions * 100.0) / (nc->stashstats.cellemissions + nc->stashstats.cellelisions));
    }
    free(nc);
  }
  return ret;
}

uint64_t ncplane_channels(const ncplane* n){
  return n->channels;
}

uint32_t ncplane_attr(const ncplane* n){
  return n->attrword;
}

void ncplane_set_fg_default(struct ncplane* n){
  channels_set_fg_default(&n->channels);
}

void ncplane_set_bg_default(struct ncplane* n){
  channels_set_bg_default(&n->channels);
}

int ncplane_set_bg_rgb(ncplane* n, int r, int g, int b){
  return channels_set_bg_rgb(&n->channels, r, g, b);
}

int ncplane_set_fg_rgb(ncplane* n, int r, int g, int b){
  return channels_set_fg_rgb(&n->channels, r, g, b);
}

int ncplane_set_fg(ncplane* n, unsigned channel){
  return channels_set_fg(&n->channels, channel);
}

int ncplane_set_bg(ncplane* n, unsigned channel){
  return channels_set_bg(&n->channels, channel);
}

int ncplane_set_fg_alpha(ncplane* n, int alpha){
  return channels_set_fg_alpha(&n->channels, alpha);
}

int ncplane_set_bg_alpha(ncplane *n, int alpha){
  return channels_set_bg_alpha(&n->channels, alpha);
}

int ncplane_set_base(ncplane* ncp, const cell* c){
  int ret = cell_duplicate(ncp, &ncp->basecell, c);
  if(ret < 0){
    return -1;
  }
  return ret;
}

int ncplane_base(ncplane* ncp, cell* c){
  return cell_duplicate(ncp, c, &ncp->basecell);
}

const char* cell_extended_gcluster(const struct ncplane* n, const cell* c){
  return extended_gcluster(n, c);
}

static void
advance_cursor(ncplane* n, int cols){
  if(cursor_invalid_p(n)){
    return; // stuck!
  }
  if((n->x += cols) >= n->lenx){
    ++n->y;
    n->x = 0;
  }
}

// 'n' ends up above 'above'
int ncplane_move_above_unsafe(ncplane* restrict n, ncplane* restrict above){
  ncplane** an = find_above_ncplane(n);
  if(an == NULL){
    return -1;
  }
  ncplane** aa = find_above_ncplane(above);
  if(aa == NULL){
    return -1;
  }
  *an = n->z; // splice n out
  n->z = above; // attach above below n
  *aa = n; // spline n in above
  return 0;
}

// 'n' ends up below 'below'
int ncplane_move_below_unsafe(ncplane* restrict n, ncplane* restrict below){
  ncplane** an = find_above_ncplane(n);
  if(an == NULL){
    return -1;
  }
  *an = n->z; // splice n out
  n->z = below->z; // reattach subbelow list to n
  below->z = n; // splice n in below
  return 0;
}

int ncplane_move_top(ncplane* n){
  ncplane** an = find_above_ncplane(n);
  if(an == NULL){
    return -1;
  }
  *an = n->z; // splice n out
  n->z = n->nc->top;
  n->nc->top = n;
  return 0;
}

int ncplane_move_bottom(ncplane* n){
  ncplane** an = find_above_ncplane(n);
  if(an == NULL){
    return -1;
  }
  *an = n->z; // splice n out
  an = &n->nc->top;
  while(*an){
    an = &(*an)->z;
  }
  *an = n;
  n->z = NULL;
  return 0;
}

int ncplane_cursor_move_yx(ncplane* n, int y, int x){
  if(x >= n->lenx){
    return -1;
  }else if(x < 0){
    if(x < -1){
      return -1;
    }
  }else{
    n->x = x;
  }
  if(y >= n->leny){
    return -1;
  }else if(y < 0){
    if(y < -1){
      return -1;
    }
  }else{
    n->y = y;
  }
  return 0;
}

void ncplane_cursor_yx(ncplane* n, int* y, int* x){
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

static inline void
cell_obliterate(ncplane* n, cell* c){
  cell_release(n, c);
  cell_init(c);
}

int ncplane_putc(ncplane* n, const cell* c){
  ncplane_lock(n);
  if(cursor_invalid_p(n)){
    ncplane_unlock(n);
    return -1;
  }
  bool wide = cell_double_wide_p(c);
  if(wide && (n->x + 1 == n->lenx)){
    ncplane_unlock(n);
    return -1;
  }
  // A wide character obliterates anything to its immediate right (and marks
  // that cell as wide). Any character placed atop one half of a wide character
  // obliterates the other half. Note that a wide char can thus obliterate two
  // wide chars, totalling four columns.
  cell* targ = &n->fb[fbcellidx(n, n->y, n->x)];
  if(n->x > 0){
    if(cell_double_wide_p(targ)){ // replaced cell is half of a wide char
      if(targ->gcluster == 0){ // we're the right half
        cell_obliterate(n, &n->fb[fbcellidx(n, n->y, n->x - 1)]);
      }else{
        cell_obliterate(n, &n->fb[fbcellidx(n, n->y, n->x + 1)]);
      }
    }
  }
  if(cell_duplicate(n, targ, c) < 0){
    ncplane_unlock(n);
    return -1;
  }
  int cols = 1;
  if(wide){
    ++cols;
    cell* rtarg = &n->fb[fbcellidx(n, n->y, n->x + 1)];
    cell_release(n, rtarg);
    cell_init(rtarg);
    cell_set_wide(rtarg);
  }
  if(wide){ // must set our right wide, and check for further damage
    if(n->x < n->lenx - 1){ // check to our right
      cell* candidate = &n->fb[fbcellidx(n, n->y, n->x + 1)];
      if(n->x < n->lenx - 2){
        if(cell_wide_left_p(candidate)){
          cell_obliterate(n, &n->fb[fbcellidx(n, n->y, n->x + 2)]);
        }
      }
      cell_set_wide(candidate);
      cell_release(n, candidate);
    }
  }
  advance_cursor(n, cols);
  ncplane_unlock(n);
  return cols;
}

int ncplane_putsimple(struct ncplane* n, char c){
  cell ce = CELL_INITIALIZER(c, ncplane_attr(n), ncplane_channels(n));
  if(!cell_simple_p(&ce)){
    return -1;
  }
  return ncplane_putc(n, &ce);
}

int ncplane_putegc(struct ncplane* n, const char* gclust, int* sbytes){
  cell c = CELL_TRIVIAL_INITIALIZER;
  int primed = cell_prime(n, &c, gclust, n->attrword, n->channels);
  if(primed < 0){
    return -1;
  }
  if(sbytes){
    *sbytes = primed;
  }
  int ret = ncplane_putc(n, &c);
  cell_release(n, &c);
  return ret;
}

int ncplane_cursor_at(const ncplane* n, cell* c, char** gclust){
  if(n->y == n->leny && n->x == n->lenx){
    return -1;
  }
  const cell* src = &n->fb[fbcellidx(n, n->y, n->x)];
  memcpy(c, src, sizeof(*src));
  *gclust = NULL;
  if(!cell_simple_p(src)){
    *gclust = strdup(extended_gcluster(n, src));
    if(*gclust == NULL){
      return -1;
    }
  }
  return 0;
}

int cell_load(ncplane* n, cell* c, const char* gcluster){
  cell_release(n, c);
  int bytes;
  int cols;
  if((bytes = utf8_egc_len(gcluster, &cols)) >= 0 && bytes <= 1){
    c->channels &= ~CELL_WIDEASIAN_MASK;
    c->gcluster = *gcluster;
    return !!c->gcluster;
  }
  if(cols > 1){
    c->channels |= CELL_WIDEASIAN_MASK;
  }else{
    c->channels &= ~CELL_WIDEASIAN_MASK;
  }
  int eoffset = egcpool_stash(&n->pool, gcluster, bytes);
  if(eoffset < 0){
    return -1;
  }
  c->gcluster = eoffset + 0x80;
  return bytes;
}

int ncplane_putstr_yx(ncplane* n, int y, int x, const char* gclusters){
  int ret = 0;
  // FIXME speed up this blissfully naive solution
  while(*gclusters){
    // FIXME can we not dispense with this cell, and print directly in?
    cell c;
    memset(&c, 0, sizeof(c));
    int wcs = cell_load(n, &c, gclusters);
    int cols = ncplane_putegc_yx(n, y, x, gclusters, &wcs);
    if(cols < 0){
      if(wcs < 0){
        return -ret;
      }
      break;
    }
    if(wcs == 0){
      break;
    }
    ncplane_cursor_yx(n, &y, &x);
    gclusters += wcs;
    ret += wcs;
    if(ncplane_cursor_stuck(n)){
      break;
    }
  }
  return ret;
}

unsigned notcurses_supported_styles(const notcurses* nc){
  unsigned styles = 0;
  styles |= nc->standout ? CELL_STYLE_STANDOUT : 0;
  styles |= nc->uline ? CELL_STYLE_UNDERLINE : 0;
  styles |= nc->reverse ? CELL_STYLE_REVERSE : 0;
  styles |= nc->blink ? CELL_STYLE_BLINK : 0;
  styles |= nc->dim ? CELL_STYLE_DIM : 0;
  styles |= nc->bold ? CELL_STYLE_BOLD : 0;
  styles |= nc->italics ? CELL_STYLE_ITALIC : 0;
  return styles;
}

int notcurses_palette_size(const notcurses* nc){
  return nc->colors;
}

// turn on any specified stylebits
void ncplane_styles_on(ncplane* n, unsigned stylebits){
  ncplane_lock(n);
  n->attrword |= (stylebits & CELL_STYLE_MASK);
  ncplane_unlock(n);
}

// turn off any specified stylebits
void ncplane_styles_off(ncplane* n, unsigned stylebits){
  ncplane_lock(n);
  n->attrword &= ~(stylebits & CELL_STYLE_MASK);
  ncplane_unlock(n);
}

// set the current stylebits to exactly those provided
void ncplane_styles_set(ncplane* n, unsigned stylebits){
  ncplane_lock(n);
  n->attrword = (n->attrword & ~CELL_STYLE_MASK) | ((stylebits & CELL_STYLE_MASK));
  ncplane_unlock(n);
}

unsigned ncplane_styles(ncplane* n){
  unsigned ret;
  ncplane_lock(n);
  ret = (n->attrword & CELL_STYLE_MASK);
  ncplane_unlock(n);
  return ret;
}

// i hate the big allocation and two copies here, but eh what you gonna do?
// well, for one, we don't need the huge allocation FIXME
char* ncplane_vprintf_prep(ncplane* n, const char* format, va_list ap){
  const size_t size = n->lenx + 1; // healthy estimate, can embiggen below
  char* buf = malloc(size);
  if(buf == NULL){
    return NULL;
  }
  int ret = vsnprintf(buf, size, format, ap);
  if(ret < 0){
    free(buf);
    return NULL;
  }
  if((size_t)ret >= size){
    char* tmp = realloc(buf, ret + 1);
    if(tmp == NULL){
      free(buf);
      return NULL;
    }
    buf = tmp;
  }
  return buf;
}

int ncplane_vprintf_yx(ncplane* n, int y, int x, const char* format, va_list ap){
  char* r = ncplane_vprintf_prep(n, format, ap);
  if(r == NULL){
    return -1;
  }
  int ret = ncplane_putstr_yx(n, y, x, r);
  free(r);
  return ret;
}

int ncplane_vprintf_aligned(ncplane* n, int y, ncalign_e align,
                            const char* format, va_list ap){
  char* r = ncplane_vprintf_prep(n, format, ap);
  if(r == NULL){
    return -1;
  }
  int ret = ncplane_putstr_aligned(n, y, align, r);
  free(r);
  return ret;
}

int ncplane_hline_interp(ncplane* n, const cell* c, int len,
                         uint64_t c1, uint64_t c2){
  unsigned ur, ug, ub;
  int r1, g1, b1, r2, g2, b2;
  int br1, bg1, bb1, br2, bg2, bb2;
  channels_fg_rgb(c1, &ur, &ug, &ub);
  r1 = ur; g1 = ug; b1 = ub;
  channels_fg_rgb(c2, &ur, &ug, &ub);
  r2 = ur; g2 = ug; b2 = ub;
  channels_bg_rgb(c1, &ur, &ug, &ub);
  br1 = ur; bg1 = ug; bb1 = ub;
  channels_bg_rgb(c2, &ur, &ug, &ub);
  br2 = ur; bg2 = ug; bb2 = ub;
  int deltr = r2 - r1;
  int deltg = g2 - g1;
  int deltb = b2 - b1;
  int deltbr = br2 - br1;
  int deltbg = bg2 - bg1;
  int deltbb = bb2 - bb1;
  int ret;
  cell dupc = CELL_TRIVIAL_INITIALIZER;
  if(cell_duplicate(n, &dupc, c) < 0){
    return -1;
  }
  bool fgdef = false, bgdef = false;
  if(channels_fg_default_p(c1) && channels_fg_default_p(c2)){
    fgdef = true;
  }
  if(channels_bg_default_p(c1) && channels_bg_default_p(c2)){
    bgdef = true;
  }
  for(ret = 0 ; ret < len ; ++ret){
    int r = (deltr * ret) / len + r1;
    int g = (deltg * ret) / len + g1;
    int b = (deltb * ret) / len + b1;
    int br = (deltbr * ret) / len + br1;
    int bg = (deltbg * ret) / len + bg1;
    int bb = (deltbb * ret) / len + bb1;
    if(!fgdef){
      cell_set_fg_rgb(&dupc, r, g, b);
    }
    if(!bgdef){
      cell_set_bg_rgb(&dupc, br, bg, bb);
    }
    if(ncplane_putc(n, &dupc) <= 0){
      break;
    }
  }
  cell_release(n, &dupc);
  return ret;
}

int ncplane_vline_interp(ncplane* n, const cell* c, int len,
                         uint64_t c1, uint64_t c2){
  unsigned ur, ug, ub;
  int r1, g1, b1, r2, g2, b2;
  int br1, bg1, bb1, br2, bg2, bb2;
  channels_fg_rgb(c1, &ur, &ug, &ub);
  r1 = ur; g1 = ug; b1 = ub;
  channels_fg_rgb(c2, &ur, &ug, &ub);
  r2 = ur; g2 = ug; b2 = ub;
  channels_bg_rgb(c1, &ur, &ug, &ub);
  br1 = ur; bg1 = ug; bb1 = ub;
  channels_bg_rgb(c2, &ur, &ug, &ub);
  br2 = ur; bg2 = ug; bb2 = ub;
  int deltr = (r2 - r1) / (len + 1);
  int deltg = (g2 - g1) / (len + 1);
  int deltb = (b2 - b1) / (len + 1);
  int deltbr = (br2 - br1) / (len + 1);
  int deltbg = (bg2 - bg1) / (len + 1);
  int deltbb = (bb2 - bb1) / (len + 1);
  int ret, ypos, xpos;
  ncplane_cursor_yx(n, &ypos, &xpos);
  cell dupc = CELL_TRIVIAL_INITIALIZER;
  if(cell_duplicate(n, &dupc, c) < 0){
    return -1;
  }
  bool fgdef = false, bgdef = false;
  if(channels_fg_default_p(c1) && channels_fg_default_p(c2)){
    fgdef = true;
  }
  if(channels_bg_default_p(c1) && channels_bg_default_p(c2)){
    bgdef = true;
  }
  for(ret = 0 ; ret < len ; ++ret){
    if(ncplane_cursor_move_yx(n, ypos + ret, xpos)){
      return -1;
    }
    r1 += deltr;
    g1 += deltg;
    b1 += deltb;
    br1 += deltbr;
    bg1 += deltbg;
    bb1 += deltbb;
    if(!fgdef){
      cell_set_fg_rgb(&dupc, r1, g1, b1);
    }
    if(!bgdef){
      cell_set_bg_rgb(&dupc, br1, bg1, bb1);
    }
    if(ncplane_putc(n, &dupc) <= 0){
      break;
    }
  }
  cell_release(n, &dupc);
  return ret;
}

// how many edges need touch a corner for it to be printed?
static inline unsigned
box_corner_needs(unsigned ctlword){
  return (ctlword & NCBOXCORNER_MASK) >> NCBOXCORNER_SHIFT;
}

int ncplane_box(ncplane* n, const cell* ul, const cell* ur,
                const cell* ll, const cell* lr, const cell* hl,
                const cell* vl, int ystop, int xstop,
                unsigned ctlword){
  int yoff, xoff, ymax, xmax;
  ncplane_cursor_yx(n, &yoff, &xoff);
  // must be at least 2x2, with its upper-left corner at the current cursor
  if(ystop < yoff + 1){
    return -1;
  }
  if(xstop < xoff + 1){
    return -1;
  }
  ncplane_dim_yx(n, &ymax, &xmax);
  // must be within the ncplane
  if(xstop >= xmax || ystop >= ymax){
    return -1;
  }
  unsigned edges;
  edges = !(ctlword & NCBOXMASK_TOP) + !(ctlword & NCBOXMASK_LEFT);
  if(edges >= box_corner_needs(ctlword)){
    if(ncplane_putc(n, ul) < 0){
      return -1;
    }
  }
  if(!(ctlword & NCBOXMASK_TOP)){ // draw top border, if called for
    if(xstop - xoff >= 2){
      if(ncplane_cursor_move_yx(n, yoff, xoff + 1)){
        return -1;
      }
      if(!(ctlword & NCBOXGRAD_TOP)){ // cell styling, hl
        if(ncplane_hline(n, hl, xstop - xoff - 1) < 0){
          return -1;
        }
      }else{ // gradient, ul -> ur
        if(ncplane_hline_interp(n, hl, xstop - xoff - 1, ul->channels, ur->channels) < 0){
          return -1;
        }
      }
    }
  }
  edges = !(ctlword & NCBOXMASK_TOP) + !(ctlword & NCBOXMASK_RIGHT);
  if(edges >= box_corner_needs(ctlword)){
    if(ncplane_cursor_move_yx(n, yoff, xstop)){
      return -1;
    }
    if(ncplane_putc(n, ur) < 0){
      return -1;
    }
  }
  ++yoff;
  // middle rows (vertical lines)
  if(yoff < ystop){
    if(!(ctlword & NCBOXMASK_LEFT)){
      if(ncplane_cursor_move_yx(n, yoff, xoff)){
        return -1;
      }
      if((ctlword & NCBOXGRAD_LEFT)){ // grad styling, ul->ll
        if(ncplane_vline_interp(n, vl, ystop - yoff, ul->channels, ll->channels) < 0){
          return -1;
        }
      }else{
        if(ncplane_vline(n, vl, ystop - yoff) < 0){
          return -1;
        }
      }
    }
    if(!(ctlword & NCBOXMASK_RIGHT)){
      if(ncplane_cursor_move_yx(n, yoff, xstop)){
        return -1;
      }
      if((ctlword & NCBOXGRAD_RIGHT)){ // grad styling, ur->lr
        if(ncplane_vline_interp(n, vl, ystop - yoff, ur->channels, lr->channels) < 0){
          return -1;
        }
      }else{
        if(ncplane_vline(n, vl, ystop - yoff) < 0){
          return -1;
        }
      }
    }
  }
  // bottom line
  yoff = ystop;
  edges = !(ctlword & NCBOXMASK_BOTTOM) + !(ctlword & NCBOXMASK_LEFT);
  if(edges >= box_corner_needs(ctlword)){
    if(ncplane_cursor_move_yx(n, yoff, xoff)){
      return -1;
    }
    if(ncplane_putc(n, ll) < 0){
      return -1;
    }
  }
  if(!(ctlword & NCBOXMASK_BOTTOM)){
    if(xstop - xoff >= 2){
      if(ncplane_cursor_move_yx(n, yoff, xoff + 1)){
        return -1;
      }
      if(!(ctlword & NCBOXGRAD_BOTTOM)){ // cell styling, hl
        if(ncplane_hline(n, hl, xstop - xoff - 1) < 0){
          return -1;
        }
      }else{
        if(ncplane_hline_interp(n, hl, xstop - xoff - 1, ll->channels, lr->channels) < 0){
          return -1;
        }
      }
    }
  }
  edges = !(ctlword & NCBOXMASK_BOTTOM) + !(ctlword & NCBOXMASK_RIGHT);
  if(edges >= box_corner_needs(ctlword)){
    if(ncplane_cursor_move_yx(n, yoff, xstop)){
      return -1;
    }
    if(ncplane_putc(n, lr) < 0){
      return -1;
    }
  }
  return 0;
}

int ncplane_move_yx(ncplane* n, int y, int x){
  ncplane_lock(n);
  if(n == n->nc->stdscr){
    ncplane_unlock(n);
    return -1;
  }
  n->absy = y;
  n->absx = x;
  ncplane_unlock(n);
  return 0;
}

void ncplane_yx(const ncplane* n, int* y, int* x){
  ncplane_lock(n);
  if(y){
    *y = n->absy;
  }
  if(x){
    *x = n->absx;
  }
  ncplane_unlock(n);
}

void ncplane_erase(ncplane* n){
  ncplane_lock(n);
  // we must preserve the background, but a pure cell_duplicate() would be
  // wiped out by the egcpool_dump(). do a duplication (to get the attrword
  // and channels), and then reload.
  char* egc = cell_egc_copy(n, &n->basecell);
  memset(n->fb, 0, sizeof(*n->fb) * n->lenx * n->leny);
  egcpool_dump(&n->pool);
  egcpool_init(&n->pool);
  cell_load(n, &n->basecell, egc);
  free(egc);
  ncplane_unlock(n);
}

void notcurses_cursor_enable(notcurses* nc){
  if(nc->cnorm){
    term_emit("cnorm", nc->cnorm, nc->ttyfp, false);
  }
}

void notcurses_cursor_disable(notcurses* nc){
  if(nc->civis){
    term_emit("civis", nc->civis, nc->ttyfp, false);
  }
}

ncplane* notcurses_top(notcurses* n){
  return n->top;
}

ncplane* ncplane_below(ncplane* n){
  return n->z;
}

// FIXME this clears the screen for some reason! what's up?
#define SET_BTN_EVENT_MOUSE   "1002"
#define SET_FOCUS_EVENT_MOUSE "1004"
#define SET_SGR_MODE_MOUSE    "1006"
int notcurses_mouse_enable(notcurses* n){
  return term_emit("mouse", ESC "[?" SET_BTN_EVENT_MOUSE ";"
                   /*SET_FOCUS_EVENT_MOUSE ";" */SET_SGR_MODE_MOUSE "h",
                   n->ttyfp, true);
}

// this seems to work (note difference in suffix, 'l' vs 'h'), but what about
// the sequences 1000 etc?
int notcurses_mouse_disable(notcurses* n){
  return term_emit("mouse", ESC "[?" SET_BTN_EVENT_MOUSE ";"
                   /*SET_FOCUS_EVENT_MOUSE ";" */SET_SGR_MODE_MOUSE "l",
                   n->ttyfp, true);
}

bool notcurses_canfade(const notcurses* nc){
  return nc->CCCflag || nc->RGBflag;
}
