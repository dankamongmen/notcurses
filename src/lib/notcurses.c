#include <ncurses.h> // needed for some definitions, see terminfo(3ncurses)
#include <time.h>
#include <term.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <langinfo.h>
#include <sys/poll.h>
#include <stdatomic.h>
#include <sys/ioctl.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
#include <libavutil/version.h>
#include <libswscale/version.h>
#include <libavformat/version.h>
#include "notcurses.h"
#include "internal.h"
#include "timespec.h"
#include "version.h"
#include "egcpool.h"

#define ESC "\x1b"
#define NANOSECS_IN_SEC 1000000000

// only one notcurses object can be the target of signal handlers, due to their
// process-wide nature.
static sig_atomic_t resize_seen;
static notcurses* _Atomic signal_nc = ATOMIC_VAR_INIT(NULL); // ugh
static void (*signal_sa_handler)(int); // stashed signal handler we replaced

static void
sigwinch_handler(int signo){
  resize_seen = signo;
}

// this wildly unsafe handler will attempt to restore the screen upon
// reception of SIGINT, SIGSEGV, or SIGQUIT. godspeed you, black emperor!
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
    sa.sa_flags = SA_RESETHAND; // don't try twice
    int ret = 0;
    ret |= sigaction(SIGINT, &sa, &oldact);
    ret |= sigaction(SIGQUIT, &sa, &oldact);
    ret |= sigaction(SIGSEGV, &sa, &oldact);
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

static void
update_render_stats(const struct timespec* time1, const struct timespec* time0,
                    ncstats* stats){
  int64_t elapsed = timespec_subtract_ns(time1, time0);
  //fprintf(stderr, "Rendering took %ld.%03lds\n", elapsed / NANOSECS_IN_SEC,
  //        (elapsed % NANOSECS_IN_SEC) / 1000000);
  if(elapsed > 0){ // don't count clearly incorrect information, egads
    ++stats->renders;
    stats->render_ns += elapsed;
    if(elapsed > stats->render_max_ns){
      stats->render_max_ns = elapsed;
    }
    if(elapsed < stats->render_min_ns){
      stats->render_min_ns = elapsed;
    }
  }
}

static const char NOTCURSES_VERSION[] =
 notcurses_VERSION_MAJOR "."
 notcurses_VERSION_MINOR "."
 notcurses_VERSION_PATCH;

const char* notcurses_version(void){
  return NOTCURSES_VERSION;
}

void notcurses_stats(const notcurses* nc, ncstats* stats){
  memcpy(stats, &nc->stats, sizeof(*stats));
}

static inline int
fbcellidx(const ncplane* n, int row, int col){
  return row * n->lenx + col;
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

void ncplane_dim_yx(const ncplane* n, int* rows, int* cols){
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
term_emit(const char* seq, FILE* out, bool flush){
  int ret = fprintf(out, "%s", seq);
  if(ret < 0){
    fprintf(stderr, "Error emitting %zub sequence (%s)\n", strlen(seq), strerror(errno));
    return -1;
  }
  if((size_t)ret != strlen(seq)){
    fprintf(stderr, "Short write (%db) for %zub sequence\n", ret, strlen(seq));
    return -1;
  }
  if(flush && fflush(out)){
    fprintf(stderr, "Error flushing after %db sequence (%s)\n", ret, strerror(errno));
    return -1;
  }
  return 0;
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
  cell_init(&p->background);
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

// Call this when the screen size changes. Acquires the new size, and copies
// what can be copied from the old stdscr. Assumes that the screen is always
// anchored in the same place.
// // FIXME rewrite this in terms of ncpanel_resize(n->stdscr)
int notcurses_resize(notcurses* n, int* rows, int* cols){
  int oldrows = n->stdscr->leny;
  int oldcols = n->stdscr->lenx;
  if(update_term_dimensions(n, rows, cols)){
    return -1;
  }
  ncplane* p = n->stdscr;
  cell* preserved = p->fb;
  size_t fbsize = sizeof(*preserved) * (*rows * *cols);
  if((p->fb = malloc(fbsize)) == NULL){
    p->fb = preserved;
    return -1;
  }
  int y, idx;
  idx = 0;
  p->lenx = *cols;
  p->leny = *rows;
  for(y = 0 ; y < p->leny ; ++y){
    idx = y * p->lenx;
    if(y >= oldrows){
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
    if(p->lenx > oldcopy){
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

ncplane* notcurses_newplane(notcurses* nc, int rows, int cols,
                            int yoff, int xoff, void* opaque){
  ncplane* n = ncplane_create(nc, rows, cols, yoff, xoff);
  if(n == NULL){
    return n;
  }
  n->absx = xoff;
  n->absy = yoff;
  n->userptr = opaque;
  return n;
}

int ncplane_resize(ncplane* n, int keepy, int keepx, int keepleny,
                   int keeplenx, int yoff, int xoff, int ylen, int xlen){
  if(n == n->nc->stdscr){
    fprintf(stderr, "Can't resize standard plane\n");
    return -1;
  }
  if(keepleny < 0 || keeplenx < 0){ // can't retain negative size
    fprintf(stderr, "Can't retain negative size %dx%d\n", keepleny, keeplenx);
    return -1;
  }
  if(ylen <= 0 || xlen <= 0){ // can't resize to trivial or negative size
    fprintf(stderr, "Can't achieve negative size %dx%d\n", ylen, xlen);
    return -1;
  }
  if((!keepleny && keeplenx) || (keepleny && !keeplenx)){ // both must be 0
    fprintf(stderr, "Can't keep zero dimensions %dx%d\n", keepleny, keeplenx);
    return -1;
  }
  if(ylen < keepleny || xlen < keeplenx){ // can't be smaller than our keep
    fprintf(stderr, "Can't violate space %dx%d vs %dx%d\n", keepleny, keeplenx, ylen, xlen);
    return -1;
  }
/*fprintf(stderr, "NCPLANE(RESIZING) to %dx%d at %d/%d (keeping %dx%d from %d/%d)\n",
        ylen, xlen, yoff, xoff, keepleny, keeplenx, keepy, keepx);*/
  // we're good to resize. we'll need alloc up a new framebuffer, and copy in
  // those elements we're retaining, zeroing out the rest. alternatively, if
  // we've shrunk, we will be filling the new structure.
  int keptarea = keepleny * keeplenx;
  int newarea = ylen * xlen;
  cell* fb = malloc(sizeof(*fb) * newarea);
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
  fprintf(stderr, "Term: %s\n", longname_term ? longname_term : "?");
  nc->RGBflag = tigetflag("RGB") == 1;
  nc->CCCflag = tigetflag("ccc") == 1;
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
    if(term_emit(tiparm(cursor_invisible), opts->outfp, true)){
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
  term_verify_seq(&nc->italoff, "ritm");
  term_verify_seq(&nc->op, "op");
  term_verify_seq(&nc->clear, "clear");
  term_verify_seq(&nc->left, "kcub1");
  term_verify_seq(&nc->right, "kcuf1");
  term_verify_seq(&nc->up, "kcuu1");
  term_verify_seq(&nc->down, "kcud1");
  term_verify_seq(&nc->npage, "knp");
  term_verify_seq(&nc->ppage, "kpp");
  // Some terminals cannot combine certain styles with colors. Don't advertise
  // support for the style in that case.
  int nocolor_stylemask = tigetnum("ncv");
  if(nocolor_stylemask > 0){
    // ncv is defined in terms of curses style bits, which differ from ours
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
  if(!opts->pass_through_esc){
    term_verify_seq(&nc->smkx, "smkx");
    term_verify_seq(&nc->rmkx, "rmkx");
  }else{
    nc->smkx = nc->rmkx = NULL;
  }
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

notcurses* notcurses_init(const notcurses_options* opts){
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
  memset(&ret->stats, 0, sizeof(ret->stats));
  ret->stats.render_min_ns = ~0UL;
  ret->stats.render_min_bytes = ~0UL;
  ret->ttyfp = opts->outfp;
  ret->renderfp = opts->renderfp;
  ret->ttyinfp = stdin; // FIXME
  if(make_nonblocking(ret->ttyinfp)){
    free(ret);
    return NULL;
  }
  if((ret->ttyfd = fileno(ret->ttyfp)) < 0){
    fprintf(stderr, "No file descriptor was available in opts->outfp\n");
    free(ret);
    return NULL;
  }
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
  if((ret->stdscr = create_initial_ncplane(ret)) == NULL){
    goto err;
  }
  if(ret->smkx && term_emit(ret->smkx, ret->ttyfp, true)){
    free_plane(ret->top);
    goto err;
  }
  if(ret->smcup && term_emit(ret->smcup, ret->ttyfp, true)){
    free_plane(ret->top);
    goto err;
  }
  // term_emit(ret->clear, ret->ttyfp, false);
  fprintf(ret->ttyfp, "\n"
         " notcurses %s by nick black\n"
         " compiled with gcc-%s\n"
         " terminfo from %s\n"
         " avformat %u.%u.%u\n"
         " avutil %u.%u.%u\n"
         " swscale %u.%u.%u\n"
         " %d rows, %d columns (%zub), %d colors (%s)\n",
         notcurses_version(), __VERSION__,
         curses_version(), LIBAVFORMAT_VERSION_MAJOR,
         LIBAVFORMAT_VERSION_MINOR, LIBAVFORMAT_VERSION_MICRO,
         LIBAVUTIL_VERSION_MAJOR, LIBAVUTIL_VERSION_MINOR, LIBAVUTIL_VERSION_MICRO,
         LIBSWSCALE_VERSION_MAJOR, LIBSWSCALE_VERSION_MINOR, LIBSWSCALE_VERSION_MICRO,
         ret->top->leny, ret->top->lenx,
         ret->top->lenx * ret->top->leny * sizeof(*ret->top->fb),
         ret->colors, ret->RGBflag ? "direct" : "palette");
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
    if(nc->rmcup && term_emit(nc->rmcup, nc->ttyfp, true)){
      ret = -1;
    }
    if(nc->cnorm && term_emit(nc->cnorm, nc->ttyfp, true)){
      ret = -1;
    }
    ret |= tcsetattr(nc->ttyfd, TCSANOW, &nc->tpreserved);
    double avg = nc->stats.renders ?
             nc->stats.render_ns / (double)nc->stats.renders : 0;
    fprintf(stderr, "%ju renders, %.03gs total (%.03gs min, %.03gs max, %.02gs avg)\n",
            nc->stats.renders,
            nc->stats.render_ns / 1000000000.0,
            nc->stats.render_min_ns / 1000000000.0,
            nc->stats.render_max_ns / 1000000000.0,
            avg / NANOSECS_IN_SEC);
    avg = nc->stats.renders ? nc->stats.render_bytes / (double)nc->stats.renders : 0;
    fprintf(stderr, "%.03fKB total (%.03fKB min, %.03fKB max, %.02fKB avg)\n",
            nc->stats.render_bytes / 1024.0,
            nc->stats.render_min_bytes / 1024.0,
            nc->stats.render_max_bytes / 1024.0,
            avg / 1024);
    fprintf(stderr, "Emits/elides: def %lu/%lu fg %lu/%lu bg %lu/%lu\n",
            nc->stats.defaultemissions,
            nc->stats.defaultelisions,
            nc->stats.fgemissions,
            nc->stats.fgelisions,
            nc->stats.bgemissions,
            nc->stats.bgelisions);
    fprintf(stderr, " Elide rates: %.2f%% %.2f%% %.2f%%\n",
            (nc->stats.defaultemissions + nc->stats.defaultelisions) == 0 ? 0 :
             (nc->stats.defaultelisions * 100.0) / (nc->stats.defaultemissions + nc->stats.defaultelisions),
            (nc->stats.fgemissions + nc->stats.fgelisions) == 0 ? 0 :
             (nc->stats.fgelisions * 100.0) / (nc->stats.fgemissions + nc->stats.fgelisions),
            (nc->stats.bgemissions + nc->stats.bgelisions) == 0 ? 0 :
             (nc->stats.bgelisions * 100.0) / (nc->stats.bgemissions + nc->stats.bgelisions));
    while(nc->top){
      ncplane* p = nc->top;
      nc->top = p->z;
      free_plane(p);
    }
    ret |= pthread_mutex_destroy(&nc->lock);
    free(nc);
  }
  return ret;
}

void ncplane_fg_default(struct ncplane* n){
  n->channels &= ~(CELL_FGDEFAULT_MASK);
}

void ncplane_bg_default(struct ncplane* n){
  n->channels &= ~(CELL_BGDEFAULT_MASK);
}

int ncplane_bg_rgb8(ncplane* n, int r, int g, int b){
  return notcurses_bg_prep(&n->channels, r, g, b);
}

int ncplane_fg_rgb8(ncplane* n, int r, int g, int b){
  return notcurses_fg_prep(&n->channels, r, g, b);
}

int ncplane_set_background(ncplane* ncp, const cell* c){
  return cell_duplicate(ncp, &ncp->background, c);
}

int ncplane_background(ncplane* ncp, cell* c){
  return cell_duplicate(ncp, c, &ncp->background);
}

// 3 for foreground, 4 for background, ugh FIXME
static int
term_esc_rgb(FILE* out, int esc, unsigned r, unsigned g, unsigned b){
  #define RGBESC1 ESC "["
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

static inline const char*
extended_gcluster(const ncplane* n, const cell* c){
  uint32_t idx = cell_egc_idx(c);
  return n->pool.pool + idx;
}

const char* cell_extended_gcluster(const struct ncplane* n, const cell* c){
  return extended_gcluster(n, c);
}

// write the cell's UTF-8 grapheme cluster to the provided FILE*. returns the
// number of columns occupied by this EGC (only an approximation; it's actually
// a property of the font being used).
static int
term_putc(FILE* out, const ncplane* n, const cell* c){
  if(cell_simple_p(c)){
    if(c->gcluster == 0 || iscntrl(c->gcluster)){
// fprintf(stderr, "[ ]\n");
      if(fputc(' ', out) == EOF){
        return -1;
      }
    }else{
// fprintf(stderr, "[%c]\n", c->gcluster);
      if(fputc(c->gcluster, out) == EOF){
        return -1;
      }
    }
  }else{
    const char* ext = extended_gcluster(n, c);
// fprintf(stderr, "[%s]\n", ext);
    if(fprintf(out, "%s", ext) < 0){ // FIXME check for short write?
      return -1;
    }
  }
  return 0;
}

static void
advance_cursor(ncplane* n, int cols){
  if(cursor_invalid_p(n)){
    return; // stuck!
  }
  if((n->x += cols) >= n->lenx){
    ++n->y;
    n->x -= n->lenx;
  }
}

// check the current and target style bitmasks against the specified 'stylebit'.
// if they are different, and we have the necessary capability, write the
// applicable terminfo entry to 'out'. returns -1 only on a true error.
static int
term_setstyle(FILE* out, unsigned cur, unsigned targ, unsigned stylebit,
              const char* ton, const char* toff){
  int ret = 0;
  unsigned curon = cur & stylebit;
  unsigned targon = targ & stylebit;
  if(curon != targon){
    if(targon){
      if(ton){
        ret = term_emit(ton, out, false);
      }
    }else{
      if(toff){ // how did this happen? we can turn it on, but not off?
        ret = term_emit(toff, out, false);
      }
    }
  }
  if(ret < 0){
    return -1;
  }
  return 0;
}

// write any escape sequences necessary to set the desired style
static int
term_setstyles(const notcurses* nc, FILE* out, uint32_t* curattr, const cell* c){
  if(cell_inherits_style(c)){
    return 0; // change nothing
  }
  uint32_t cellattr = cell_styles(c);
  if(cellattr == *curattr){
    return 0; // happy agreement, change nothing
  }
  int ret = 0;
  ret |= term_setstyle(out, *curattr, cellattr, CELL_STYLE_ITALIC, nc->italics, nc->italoff);
  /*ret |= term_setstyle(out, curattr, cellattr, CELL_STYLE_BOLD, nc->bold, nc->boldoff);
  ret |= term_setstyle(out, curattr, cellattr, CELL_STYLE_UNDERLINE, nc->uline, nc->ulineoff);
  ret |= term_setstyle(out, curattr, cellattr, CELL_STYLE_BLINK, nc->blink, nc->blinkoff);*/
  // FIXME a few others
  *curattr = cellattr;
  return ret;
}

// find the topmost cell for this coordinate
static const cell*
visible_cell(int y, int x, ncplane** retp){
  ncplane* p = *retp;
  while(p){
    // where in the plane this coordinate would be, based off absy/absx. the
    // true origin is 0,0, so abs=2,2 means coordinate 3,3 would be 1,1, while
    // abs=-2,-2 would make coordinate 3,3 relative 5, 5.
    int poffx, poffy;
    poffy = y - p->absy;
    poffx = x - p->absx;
    if(poffy < p->leny && poffy >= 0){
      if(poffx < p->lenx && poffx >= 0){
        *retp = p;
        const cell* vis = &p->fb[fbcellidx(p, poffy, poffx)];
        // if we never actually loaded the cell, use the background, if one
        // is defined.
        if(vis->gcluster == 0){
          vis = &p->background;
        }
        // if it's using the terminal default, chase further down
        // FIXME should probably be based off alpha channel
        if(cell_fg_default_p(vis) && cell_bg_default_p(vis) && vis->gcluster == 0){
          *retp = p->z;
          const cell* trans = visible_cell(y, x, retp);
          if(trans){
            vis = trans;
          }else{
            *retp = p;
          }
        }
        return vis;
      }
    }
    p = p->z;
  }
  // should never happen for valid y, x thanks to the stdscreen
  return NULL;
}

// 'n' ends up above 'above'
int ncplane_move_above(ncplane* restrict n, ncplane* restrict above){
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
int ncplane_move_below(ncplane* restrict n, ncplane* restrict below){
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

static int
blocking_write(int fd, const char* buf, size_t buflen){
  size_t written = 0;
  do{
    ssize_t w = write(fd, buf + written, buflen - written);
    if(w < 0){
      if(errno != EAGAIN && errno != EWOULDBLOCK){
        return -1;
      }
    }else{
      written += w;
    }
    if(written < buflen){
      struct pollfd pfd = {
        .fd = fd,
        .events = POLLOUT,
        .revents = 0,
      };
      poll(&pfd, 1, -1);
    }
  }while(written < buflen);
  return 0;
}

// determine the best palette for the current frame, and write the necessary
// escape sequences to 'out'.
static int
prep_optimized_palette(notcurses* nc, FILE* out __attribute__ ((unused))){
  if(nc->RGBflag){
    return 0; // DirectColor, no need to write palette
  }
  if(!nc->CCCflag){
    return 0; // can't change palette
  }
  // FIXME
  return 0;
}

// FIXME this needs to keep an invalidation bitmap, rather than blitting the
// world every time
static inline int
notcurses_render_internal(notcurses* nc){
  int ret = 0;
  int y, x;
  char* buf = NULL;
  size_t buflen = 0;
  FILE* out = open_memstream(&buf, &buflen); // worth keeping around?
  if(out == NULL){
    return -1;
  }
  prep_optimized_palette(nc, out); // FIXME do what on failure?
  uint32_t curattr = 0; // current attributes set (does not include colors)
  // no need to write a clearscreen, since we update everything that's been
  // changed. just move the physical cursor to the upper left corner.
  term_emit(tiparm(nc->cup, 0, 0), out, false);
  // FIXME as of at least gcc 9.2.1, we get a false -Wmaybe-uninitialized below
  // when using these without explicit initializations. for the life of me, i
  // can't see any such path, and valgrind is cool with it, so what ya gonna do?
  unsigned lastr = 0, lastg = 0, lastb = 0;
  unsigned lastbr = 0, lastbg = 0, lastbb = 0;
  // we can elide a color escape iff the color has not changed between the two
  // cells and the current cell uses no defaults, or if both the current and
  // the last used both defaults.
  bool fgelidable = false, bgelidable = false, defaultelidable = false;
  for(y = 0 ; y < nc->stdscr->leny ; ++y){
    // FIXME previous line could have ended halfway through multicol. what happens?
    // FIXME also must explicitly move to next line if we're to deal with
    //  surprise enlargenings, otherwise we just print forward
    for(x = 0 ; x < nc->stdscr->lenx ; ++x){
      unsigned r, g, b, br, bg, bb;
      ncplane* p = nc->top;
      const cell* c = visible_cell(y, x, &p);
      if(c == NULL){
        continue; // shrug?
      }
      // we allow these to be set distinctly, but terminfo only supports using
      // them both via the 'op' capability. unless we want to generate the 'op'
      // escapes ourselves, if either is set to default, we first send op, and
      // then a turnon for whichever aren't default.

      // we can elide the default set iff the previous used both defaults
      if(cell_fg_default_p(c) || cell_bg_default_p(c)){
        if(!defaultelidable){
          ++nc->stats.defaultemissions;
          term_emit(nc->op, out, false);
        }else{
          ++nc->stats.defaultelisions;
        }
        // if either is not default, this will get turned off
        defaultelidable = true;
        fgelidable = false;
        bgelidable = false;
      }

      // we can elide the foreground set iff the previous used fg and matched
      if(!cell_fg_default_p(c)){
        cell_get_fg(c, &r, &g, &b);
        if(fgelidable && lastr == r && lastg == g && lastb == b){
          ++nc->stats.fgelisions;
        }else{
          term_fg_rgb8(nc, out, r, g, b);
          ++nc->stats.fgemissions;
          fgelidable = true;
        }
        lastr = r; lastg = g; lastb = b;
        defaultelidable = false;
      }
      if(!cell_bg_default_p(c)){
        cell_get_bg(c, &br, &bg, &bb);
        if(bgelidable && lastbr == br && lastbg == bg && lastbb == bb){
          ++nc->stats.bgelisions;
        }else{
          term_bg_rgb8(nc, out, br, bg, bb);
          ++nc->stats.bgemissions;
          bgelidable = true;
        }
        lastbr = br; lastbg = bg; lastbb = bb;
        defaultelidable = false;
      }
      term_setstyles(nc, out, &curattr, c);
      // FIXME what to do if we're at the last cell, and it's wide?
// fprintf(stderr, "[%02d/%02d] %p ", y, x, c);
      term_putc(out, p, c);
      if(cell_double_wide_p(c)){
        ++x;
      }
    }
  }
  ret |= fclose(out);
  fflush(nc->ttyfp);
  if(blocking_write(nc->ttyfd, buf, buflen)){
    ret = -1;
  }
/*fprintf(stderr, "%lu/%lu %lu/%lu %lu/%lu\n", defaultelisions, defaultemissions,
     fgelisions, fgemissions, bgelisions, bgemissions);*/
  if(nc->renderfp){
    fprintf(nc->renderfp, "%s\n", buf);
  }
  free(buf);
  return buflen;
}

int notcurses_render(notcurses* nc){
  struct timespec start, done;
  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
  pthread_mutex_lock(&nc->lock);
  int bytes = notcurses_render_internal(nc);
  if(bytes > 0){
    nc->stats.render_bytes += bytes;
    if(bytes > nc->stats.render_max_bytes){
      nc->stats.render_max_bytes = bytes;
    }
    if(bytes < nc->stats.render_min_bytes){
      nc->stats.render_min_bytes = bytes;
    }
  }
  clock_gettime(CLOCK_MONOTONIC_RAW, &done);
  update_render_stats(&done, &start, &nc->stats);
  pthread_mutex_unlock(&nc->lock);
  return bytes < 0 ? -1 : 0;
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
  if(cell_simple_p(c)){
    targ->gcluster = c->gcluster;
    return !!c->gcluster;
  }
  size_t ulen = strlen(extended_gcluster(n, c));
// fprintf(stderr, "[%s] (%zu)\n", extended_gcluster(n, c), strlen(extended_gcluster(n, c)));
  int eoffset = egcpool_stash(&n->pool, extended_gcluster(n, c), ulen);
  if(eoffset < 0){
    return -1;
  }
  targ->gcluster = eoffset + 0x80;
  return ulen;
}

int ncplane_putc(ncplane* n, const cell* c){
  if(cursor_invalid_p(n)){
    return -1;
  }
  pthread_mutex_lock(&n->nc->lock);
  cell* targ = &n->fb[fbcellidx(n, n->y, n->x)];
  int ret = cell_duplicate(n, targ, c);
  advance_cursor(n, 1 + cell_double_wide_p(targ));
  pthread_mutex_unlock(&n->nc->lock);
  return ret;
}

int ncplane_putsimple(struct ncplane* n, char c, uint32_t attr, uint64_t channels){
  cell ce = {
    .gcluster = c,
    .attrword = attr,
    .channels = channels,
  };
  if(!cell_simple_p(&ce)){
    return -1;
  }
  return ncplane_putc(n, &ce);
}

int ncplane_putegc(struct ncplane* n, const char* gclust, uint32_t attr,
                   uint64_t channels, int* sbytes){
  cell c = CELL_TRIVIAL_INITIALIZER;
  int primed = cell_prime(n, &c, gclust, attr, channels);
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

void cell_release(ncplane* n, cell* c){
  if(!cell_simple_p(c)){
    egcpool_release(&n->pool, cell_egc_idx(c));
    c->gcluster = 0; // don't subject ourselves to double-release problems
  }
}

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
  int eoffset = egcpool_stash(&n->pool, gcluster, bytes);
  if(eoffset < 0){
    return -1;
  }
  c->gcluster = eoffset + 0x80;
  return bytes;
}

int ncplane_putstr(ncplane* n, const char* gclusters){
  int ret = 0;
  // FIXME speed up this blissfully naive solution
  while(*gclusters){
    // FIXME can we not dispense with this cell, and print directly in?
    cell c;
    memset(&c, 0, sizeof(c));
    int wcs = cell_load(n, &c, gclusters);
    if(ncplane_putegc(n, gclusters, n->attrword, n->channels, &wcs) < 0){
      if(wcs < 0){
        pthread_mutex_unlock(&n->nc->lock);
        return -ret;
      }
      break;
    }
    if(wcs == 0){
      break;
    }
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
  n->attrword |= ((stylebits & 0xffff) << 16u);
}

// turn off any specified stylebits
void ncplane_styles_off(ncplane* n, unsigned stylebits){
  n->attrword &= ~((stylebits & 0xffff) << 16u);
}

// set the current stylebits to exactly those provided
void ncplane_styles_set(ncplane* n, unsigned stylebits){
  n->attrword = (n->attrword & ~CELL_STYLE_MASK) |
                ((stylebits & 0xffff) << 16u);
}

unsigned ncplane_styles(const ncplane* n){
  return (n->attrword & CELL_STYLE_MASK) >> 16u;
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
  ncplane_dim_yx(n, &ymax, &xmax);
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

void ncplane_move_yx(ncplane* n, int y, int x){
  n->absy = y;
  n->absx = x;
}

void ncplane_yx(const ncplane* n, int* y, int* x){
  if(y){
    *y = n->absy;
  }
  if(x){
    *x = n->absx;
  }
}

// copy the UTF8-encoded EGC out of the cell, whether simple or complex. the
// result is not tied to the ncplane, and persists across erases / destruction.
static inline char*
cell_egc_copy(const ncplane* n, const cell* c){
  char* ret;
  if(cell_simple_p(c)){
    if( (ret = malloc(2)) ){
      ret[0] = c->gcluster;
      ret[1] = '\0';
    }
  }else{
    ret = strdup(cell_extended_gcluster(n, c));
  }
  return ret;
}

void ncplane_erase(ncplane* n){
  // we must preserve the background, but a pure cell_duplicate() would be
  // wiped out by the egcpool_dump(). do a duplication (to get the attrword
  // and channels), and then reload.
  char* egc = cell_egc_copy(n, &n->background);
  memset(n->fb, 0, sizeof(*n->fb) * n->lenx * n->leny);
  egcpool_dump(&n->pool);
  egcpool_init(&n->pool);
  cell_load(n, &n->background, egc);
  free(egc);
}

static int
handle_getc(const notcurses* nc __attribute__ ((unused)), cell* c, int kpress,
            ncspecial_key* special){
// fprintf(stderr, "KEYPRESS: %d\n", kpress);
  if(kpress < 0){
    return -1;
  }
  *special = 0;
  if(kpress == 0x04){ // ctrl-d
    return -1;
  }
  // FIXME look for keypad
  if(kpress < 0x80){
    c->gcluster = kpress;
  }else{
    // FIXME
  }
  return 1;
}

int notcurses_getc(const notcurses* nc, cell* c, ncspecial_key* special){
  if(resize_seen){
    resize_seen = 0;
    c->gcluster = 0;
    *special = NCKEY_RESIZE;
    return 1;
  }
  int r = getc(nc->ttyinfp);
  if(r < 0){
    return r;
  }
  return handle_getc(nc, c, r, special);
}

// we set our infd to non-blocking on entry, so to do a blocking call (without
// burning cpu) we'll need to set up a poll().
int notcurses_getc_blocking(const notcurses* nc, cell* c, ncspecial_key* special){
  struct pollfd pfd = {
    .fd = fileno(nc->ttyinfp),
    .events = POLLIN | POLLRDHUP,
    .revents = 0,
  };
  int pret, r;
  sigset_t smask;
  sigfillset(&smask);
  sigdelset(&smask, SIGWINCH);
  while((pret = ppoll(&pfd, 1, NULL, &smask)) >= 0){
    if(pret == 0){
      continue;
    }
    r = getc(nc->ttyinfp);
    if(r < 0){
      break; // want EINTR handling below
    }
    return handle_getc(nc, c, r, special);
  }
  if(errno == EINTR){
    if(resize_seen){
      resize_seen = 0;
      c->gcluster = 0;
      *special = NCKEY_RESIZE;
      return 1;
    }
  }
  return -1;
}

int ncvisual_render(const ncvisual* ncv){
  const AVFrame* f = ncv->oframe;
  if(f == NULL){
    return -1;
  }
  int x, y;
  int dimy, dimx;
  ncplane_dim_yx(ncv->ncp, &dimy, &dimx);
  ncplane_cursor_move_yx(ncv->ncp, 0, 0);
  const int linesize = f->linesize[0];
  const unsigned char* data = f->data[0];
  for(y = 0 ; y < f->height / 2 && y < dimy ; ++y){
    for(x = 0 ; x < f->width && x < dimx ; ++x){
      int bpp = av_get_bits_per_pixel(av_pix_fmt_desc_get(f->format));
      const unsigned char* rgbbase_up = data + (linesize * (y * 2)) + (x * bpp / CHAR_BIT);
      const unsigned char* rgbbase_down = data + (linesize * (y * 2 + 1)) + (x * bpp / CHAR_BIT);
/*fprintf(stderr, "[%04d/%04d] %p bpp: %d lsize: %d %02x %02x %02x %02x\n",
        y, x, rgbbase, bpp, linesize, rgbbase[0], rgbbase[1], rgbbase[2], rgbbase[3]);*/
      cell c = CELL_TRIVIAL_INITIALIZER;
      // use the default for the background, as that's the only way it's
      // effective in that case anyway
      if(!rgbbase_up[3] || !rgbbase_down[3]){
        cell_bg_default(&c);
        if(!rgbbase_up[3] && !rgbbase_down[3]){
          if(cell_load(ncv->ncp, &c, " ") <= 0){
            return -1;
          }
          cell_fg_default(&c);
        }else if(!rgbbase_up[3]){ // down has the color
          if(cell_load(ncv->ncp, &c, "\u2584") <= 0){ // lower half block
            return -1;
          }
          cell_set_fg(&c, rgbbase_down[0], rgbbase_down[1], rgbbase_down[2]);
        }else{ // up has the color
          if(cell_load(ncv->ncp, &c, "\u2580") <= 0){ // upper half block
            return -1;
          }
          cell_set_fg(&c, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2]);
        }
      }else{
        cell_set_fg(&c, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2]);
        cell_set_bg(&c, rgbbase_down[0], rgbbase_down[1], rgbbase_down[2]);
        if(cell_load(ncv->ncp, &c, "\u2580") <= 0){ // upper half block
          return -1;
        }
      }
      if(ncplane_putc(ncv->ncp, &c) <= 0){
        cell_release(ncv->ncp, &c);
        return -1;
      }
      cell_release(ncv->ncp, &c);
    }
  }
  return 0;
}

typedef struct planepalette {
  int size;                     // number of channel entries
  unsigned maxr, maxg, maxb;    // maxima across foreground channels
  unsigned maxbr, maxbg, maxbb; // maxima across background channels
  uint64_t* channels;           // all channels from the framebuffer
} planepalette;

// These arrays are too large to be safely placed on the stack.
static int
alloc_ncplane_palette(ncplane* n, planepalette* pp){
  int dimy, dimx;
  ncplane_dim_yx(n, &dimy, &dimx);
  pp->size = dimy * dimx;
  if((pp->channels = malloc(sizeof(*pp->channels) * pp->size)) == NULL){
    return -1;
  }
  pp->maxr = pp->maxg = pp->maxb = 0;
  pp->maxbr = pp->maxbg = pp->maxbb = 0;
  int y, x;
  for(y = 0 ; y < dimy ; ++y){
    for(x = 0 ; x < dimx ; ++x){
      uint64_t channels = n->fb[fbcellidx(n, y, x)].channels;
      pp->channels[y * dimx + x] = channels;
      unsigned r, g, b, br, bg, bb;
      cell_rgb_get_fg(channels, &r, &g, &b);
      if(r > pp->maxr){
        pp->maxr = r;
      }
      if(g > pp->maxg){
        pp->maxg = g;
      }
      if(b > pp->maxb){
        pp->maxb = b;
      }
      cell_rgb_get_bg(channels, &br, &bg, &bb);
      if(br > pp->maxbr){
        pp->maxbr = br;
      }
      if(bg > pp->maxbg){
        pp->maxbg = bg;
      }
      if(bb > pp->maxbb){
        pp->maxbb = bb;
      }
    }
  }
  return 0;
}

int ncplane_fadein(ncplane* n, const struct timespec* ts){
  planepalette pp;
  if(!n->nc->RGBflag && !n->nc->CCCflag){ // terminal can't fade
    notcurses_render(n->nc); // render at the target levels (ought we delay?)
    return -1;
  }
  if(alloc_ncplane_palette(n, &pp)){
    return -1;
  }
  int maxfsteps = pp.maxg > pp.maxr ? (pp.maxb > pp.maxg ? pp.maxb : pp.maxg) :
                  (pp.maxb > pp.maxr ? pp.maxb : pp.maxr);
  int maxbsteps = pp.maxbg > pp.maxbr ? (pp.maxbb > pp.maxbg ? pp.maxbb : pp.maxbg) :
                  (pp.maxbb > pp.maxbr ? pp.maxbb : pp.maxbr);
  int maxsteps = maxfsteps > maxbsteps ? maxfsteps : maxbsteps;
  uint64_t nanosecs_total = ts->tv_sec * NANOSECS_IN_SEC + ts->tv_nsec;
  uint64_t nanosecs_step = nanosecs_total / maxsteps;
  struct timespec times;
  clock_gettime(CLOCK_MONOTONIC, &times);
  // Start time in absolute nanoseconds
  uint64_t startns = times.tv_sec * NANOSECS_IN_SEC + times.tv_nsec;
  // Current time, sampled each iteration
  uint64_t curns;
  do{
    clock_gettime(CLOCK_MONOTONIC, &times);
    curns = times.tv_sec * NANOSECS_IN_SEC + times.tv_nsec;
    int iter = (curns - startns) / nanosecs_step + 1;
    if(iter > maxsteps){
      break;
    }
    int p;
    for(p = 0 ; p < pp.size ; ++p){
      cell* c = &n->fb[p];
      unsigned r, g, b;
      cell_rgb_get_fg(pp.channels[p], &r, &g, &b);
      unsigned br, bg, bb;
      cell_rgb_get_bg(pp.channels[p], &br, &bg, &bb);
      if(!cell_fg_default_p(c)){
        r = r * iter / maxsteps;
        g = g * iter / maxsteps;
        b = b * iter / maxsteps;
        cell_set_fg(c, r, g, b);
      }
      if(!cell_bg_default_p(c)){
        br = br * iter / maxsteps;
        bg = bg * iter / maxsteps;
        bb = bb * iter / maxsteps;
        cell_set_bg(c, br, bg, bb);
      }
    }
    notcurses_render(n->nc);
    uint64_t nextwake = (iter + 1) * nanosecs_step + startns;
    struct timespec sleepspec;
    sleepspec.tv_sec = nextwake / NANOSECS_IN_SEC;
    sleepspec.tv_nsec = nextwake % NANOSECS_IN_SEC;
    int r;
    // clock_nanosleep() has no love for CLOCK_MONOTONIC_RAW, at least as
    // of Glibc 2.29 + Linux 5.3 :/.
    r = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &sleepspec, NULL);
    if(r){
      break;
    }
  }while(true);
  free(pp.channels);
  return 0;
}

int ncplane_fadeout(ncplane* n, const struct timespec* ts){
  planepalette pp;
  if(!n->nc->RGBflag && !n->nc->CCCflag){ // terminal can't fade
    return -1;
  }
  if(alloc_ncplane_palette(n, &pp)){
    return -1;
  }
  int maxfsteps = pp.maxg > pp.maxr ? (pp.maxb > pp.maxg ? pp.maxb : pp.maxg) :
                  (pp.maxb > pp.maxr ? pp.maxb : pp.maxr);
  int maxbsteps = pp.maxbg > pp.maxbr ? (pp.maxbb > pp.maxbg ? pp.maxbb : pp.maxbg) :
                  (pp.maxbb > pp.maxbr ? pp.maxbb : pp.maxbr);
  int maxsteps = maxfsteps > maxbsteps ? maxfsteps : maxbsteps;
  uint64_t nanosecs_total = ts->tv_sec * NANOSECS_IN_SEC + ts->tv_nsec;
  uint64_t nanosecs_step = nanosecs_total / maxsteps;
  struct timespec times;
  clock_gettime(CLOCK_MONOTONIC, &times);
  // Start time in absolute nanoseconds
  uint64_t startns = times.tv_sec * NANOSECS_IN_SEC + times.tv_nsec;
  do{
    clock_gettime(CLOCK_MONOTONIC, &times);
    uint64_t curns = times.tv_sec * NANOSECS_IN_SEC + times.tv_nsec;
    int iter = (curns - startns) / nanosecs_step + 1;
    if(iter > maxsteps){
      break;
    }
    int p;
    for(p = 0 ; p < pp.size ; ++p){
      cell* c = &n->fb[p];
      unsigned r, g, b;
      cell_rgb_get_fg(pp.channels[p], &r, &g, &b);
      unsigned br, bg, bb;
      cell_rgb_get_bg(pp.channels[p], &br, &bg, &bb);
      r = r * (maxsteps - iter) / maxsteps;
      g = g * (maxsteps - iter) / maxsteps;
      b = b * (maxsteps - iter) / maxsteps;
      br = br * (maxsteps - iter) / maxsteps;
      bg = bg * (maxsteps - iter) / maxsteps;
      bb = bb * (maxsteps - iter) / maxsteps;
      cell_set_fg(c, r, g, b);
      cell_set_bg(c, br, bg, bb);
    }
    notcurses_render(n->nc);
    uint64_t nextwake = (iter + 1) * nanosecs_step + startns;
    struct timespec sleepspec;
    sleepspec.tv_sec = nextwake / NANOSECS_IN_SEC;
    sleepspec.tv_nsec = nextwake % NANOSECS_IN_SEC;
    int r;
    // clock_nanosleep() has no love for CLOCK_MONOTONIC_RAW, at least as
    // of Glibc 2.29 + Linux 5.3 :/.
    r = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &sleepspec, NULL);
    if(r){
      break;
    }
  }while(true);
  free(pp.channels);
  return 0;
}

int ncvisual_stream(notcurses* nc, ncvisual* ncv, int* averr){
  ncplane* n = ncv->ncp;
  int frame = 1;
  AVFrame* avf;
  struct timespec start;
  // FIXME should keep a start time and cumulative time; this will push things
  // out on a loaded machine
  while(clock_gettime(CLOCK_MONOTONIC, &start),
        (avf = ncvisual_decode(ncv, averr)) ){
    ncplane_cursor_move_yx(n, 0, 0);
    if(ncvisual_render(ncv)){
      return -1;
    }
    if(notcurses_render(nc)){
      return -1;
    }
    ++frame;
    uint64_t ns = avf->pkt_duration * 1000000;
    struct timespec interval = {
      .tv_sec = start.tv_sec + (long)(ns / 1000000000),
      .tv_nsec = start.tv_nsec + (long)(ns % 1000000000),
    };
    clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &interval, NULL);
  }
  if(*averr == AVERROR_EOF){
    return 0;
  }
  return -1;
}
