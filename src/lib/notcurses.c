#include <ncurses.h> // needed for some definitions, see terminfo(3ncurses)
#include <time.h>
#include <term.h>
#include <fcntl.h>
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
#include "version.h"
#include "egcpool.h"

// only one notcurses object can be the target of signal handlers, due to their
// process-wide nature.
static notcurses* _Atomic signal_nc = ATOMIC_VAR_INIT(NULL); // ugh
static void (*signal_sa_handler)(int); // stashed signal handler we replaced

static void
sigwinch_handler(int signo){
  resize_seen = signo;
}

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

int ncplane_putstr_aligned(ncplane* n, int y, const char* s, ncalign_e atype){
  wchar_t* w = wchar_from_utf8(s);
  if(w == NULL){
    return -1;
  }
  int r = ncplane_putwstr_aligned(n, y, w, atype);
  free(w);
  return r;
}

int ncplane_putwstr_aligned(struct ncplane* n, int y, const wchar_t* gclustarr,
                            ncalign_e atype){
  int width = wcswidth(gclustarr, INT_MAX);
  int cols;
  int xpos;
  switch(atype){
    case NCALIGN_LEFT:
      xpos = 0;
      break;
    case NCALIGN_CENTER:
      ncplane_dim_yx(n, NULL, &cols);
      xpos = (cols - width) / 2;
      break;
    case NCALIGN_RIGHT:
      ncplane_dim_yx(n, NULL, &cols);
      xpos = cols - width;
      break;
    default:
      return -1;
  }
  if(ncplane_cursor_move_yx(n, y, xpos)){
    return -1;
  }
  return ncplane_putwstr(n, gclustarr);
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
    ncplane_updamage(p);
    egcpool_dump(&p->pool);
    free(p->damage);
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
  p->damage = malloc(sizeof(*p->damage) * rows);
  if(p->damage == NULL){
    free(p->fb);
    free(p);
    return NULL;
  }
  flash_damage_map(p->damage, rows, false);
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
  cell_init(&p->defcell);
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

ncplane* notcurses_newplane(notcurses* nc, int rows, int cols,
                            int yoff, int xoff, void* opaque){
  ncplane* n = ncplane_create(nc, rows, cols, yoff, xoff);
  if(n == NULL){
    return n;
  }
  n->userptr = opaque;
  return n;
}

// can be used on stdscr, unlike ncplane_resize() which prohibits it. sets all
// members of the plane's damage map to damaged.
static int
ncplane_resize_internal(ncplane* n, int keepy, int keepx, int keepleny,
                       int keeplenx, int yoff, int xoff, int ylen, int xlen){
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
  int rows, cols;
  ncplane_dim_yx(n, &rows, &cols);
  // FIXME make sure we're not trying to keep more than we have?
/*fprintf(stderr, "NCPLANE(RESIZING) to %dx%d at %d/%d (keeping %dx%d from %d/%d)\n",
        ylen, xlen, yoff, xoff, keepleny, keeplenx, keepy, keepx);*/
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
  // if we're the standard plane, the updamage can be charged at the end. it's
  // unsafe to call now anyway, because if we shrank, the notcurses damage map
  // has already been shrunk down
  if(n != n->nc->stdscr){
    ncplane_updamage(n); // damage any lines we were on
  }
  unsigned char* tmpdamage;
  if((tmpdamage = realloc(n->damage, sizeof(*n->damage) * ylen)) == NULL){
    free(fb);
    return -1;
  }
  n->damage = tmpdamage;
  flash_damage_map(n->damage, ylen, true);
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
    ncplane_updamage(n); // damage any lines we're now on
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
  ncplane_updamage(n); // damage any lines we're now on
  return 0;
}

int ncplane_resize(ncplane* n, int keepy, int keepx, int keepleny,
                   int keeplenx, int yoff, int xoff, int ylen, int xlen){
  if(n == n->nc->stdscr){
    fprintf(stderr, "Can't resize standard plane\n");
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
  unsigned char* tmpdamage;
  if((tmpdamage = malloc(sizeof(*n->damage) * *rows)) == NULL){
    return -1;
  }
  if(oldcols < *cols){ // all are busted if rows got bigger
    free(n->damage);
    flash_damage_map(tmpdamage, *rows, true);
  }else if(oldrows <= *rows){ // new rows are pre-busted, old are straight
    memcpy(tmpdamage, n->damage, oldrows * sizeof(*tmpdamage));
    flash_damage_map(tmpdamage + oldrows, *rows - oldrows, true);
    free(n->damage);
  }
  n->damage = tmpdamage;
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
  if(nc->cup == NULL){
    fprintf(stderr, "Required terminfo capability not defined\n");
    return -1;
  }
  if(!opts->retain_cursor){
    term_verify_seq(&nc->civis, "civis");
    term_verify_seq(&nc->cnorm, "cnorm");
  }else{
    nc->civis = nc->cnorm = NULL;
  }
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
  if(prep_special_keys(nc)){
    return -1;
  }
  // Some terminals cannot combine certain styles with colors. Don't advertise
  // support for the style in that case.
  int nocolor_stylemask = tigetnum("ncv");
  if(nocolor_stylemask > 0){
    // FIXME this doesn't work if we're using sgr, which we are at the moment!
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
  memset(&ret->stats, 0, sizeof(ret->stats));
  ret->stats.render_min_ns = 1ul << 62u;
  ret->stats.render_min_bytes = 1ul << 62u;
  ret->ttyfp = outfp;
  ret->renderfp = opts->renderfp;
  ret->inputescapes = NULL;
  ret->ttyinfp = stdin; // FIXME
  ret->mstream = NULL;
  ret->mstrsize = 0;
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
  if(ret->civis && term_emit("civis", ret->civis, ret->ttyfp, false)){
    free_plane(ret->top);
    goto err;
  }
  if(ret->smkx && term_emit("smkx", ret->smkx, ret->ttyfp, false)){
    free_plane(ret->top);
    goto err;
  }
  if(ret->smcup && term_emit("smcup", ret->smcup, ret->ttyfp, false)){
    free_plane(ret->top);
    goto err;
  }
  if((ret->damage = malloc(sizeof(*ret->damage) * ret->stdscr->leny)) == NULL){
    free_plane(ret->top);
    goto err;
  }
  if((ret->mstreamfp = open_memstream(&ret->mstream, &ret->mstrsize)) == NULL){
    free(ret->damage);
    free_plane(ret->top);
    goto err;
  }
  flash_damage_map(ret->damage, ret->stdscr->leny, false);
  // term_emit("clear", ret->clear, ret->ttyfp, false);
  char prefixbuf[BPREFIXSTRLEN + 1];
  fprintf(ret->ttyfp, "\n"
         " notcurses %s by nick black\n"
         " compiled with gcc-%s\n"
         " terminfo from %s\n"
         " avformat %u.%u.%u\n"
         " avutil %u.%u.%u\n"
         " swscale %u.%u.%u\n"
         " %d rows, %d columns (%sB), %d colors (%s)\n",
         notcurses_version(), __VERSION__,
         curses_version(), LIBAVFORMAT_VERSION_MAJOR,
         LIBAVFORMAT_VERSION_MINOR, LIBAVFORMAT_VERSION_MICRO,
         LIBAVUTIL_VERSION_MAJOR, LIBAVUTIL_VERSION_MINOR, LIBAVUTIL_VERSION_MICRO,
         LIBSWSCALE_VERSION_MAJOR, LIBSWSCALE_VERSION_MINOR, LIBSWSCALE_VERSION_MICRO,
         ret->stdscr->leny, ret->stdscr->lenx,
         bprefix(ret->stats.fbbytes, 1, prefixbuf, 0),
         ret->colors, ret->RGBflag ? "direct" : "palette");
  if(!ret->RGBflag){ // FIXME
    if(ret->colors >= 16){
      putp(tiparm(ret->setaf, 207));
    }else{
      putp(tiparm(ret->setaf, 3));
    }
    fprintf(ret->ttyfp, "\nWarning!\nYour colors are subject to https://github.com/dankamongmen/notcurses/issues/4\n");
    fprintf(ret->ttyfp, "Are you specifying a proper DirectColor TERM?\n");
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
    ret |= tcsetattr(nc->ttyfd, TCSANOW, &nc->tpreserved);
    if(nc->stats.renders){
      double avg = nc->stats.render_ns / (double)nc->stats.renders;
      fprintf(stderr, "%ju render%s, %.03gs total (%.03gs min, %.03gs max, %.02gs avg %.1f fps)\n",
              nc->stats.renders, nc->stats.renders == 1 ? "" : "s",
              nc->stats.render_ns / 1000000000.0,
              nc->stats.render_min_ns / 1000000000.0,
              nc->stats.render_max_ns / 1000000000.0,
              avg / NANOSECS_IN_SEC, NANOSECS_IN_SEC / avg);
      avg = nc->stats.render_bytes / (double)nc->stats.renders;
      fprintf(stderr, "%.03fKB total (%.03fKB min, %.03fKB max, %.02fKB avg)\n",
              nc->stats.render_bytes / 1024.0,
              nc->stats.render_min_bytes / 1024.0,
              nc->stats.render_max_bytes / 1024.0,
              avg / 1024);
    }
    fprintf(stderr, "%ju failed render%s\n", nc->stats.failed_renders,
            nc->stats.failed_renders == 1 ? "" : "s");
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
    fprintf(stderr, "Cells emitted; %ju elided: %ju (%.2f%%)\n",
            nc->stats.cellemissions, nc->stats.cellelisions,
            (nc->stats.cellemissions + nc->stats.cellelisions) == 0 ? 0 :
             (nc->stats.cellelisions * 100.0) / (nc->stats.cellemissions + nc->stats.cellelisions));
    while(nc->top){
      ncplane* p = nc->top;
      nc->top = p->z;
      free_plane(p);
    }
    free(nc->damage);
    if(nc->mstreamfp){
      fclose(nc->mstreamfp);
    }
    free(nc->mstream);
    input_free_esctrie(&nc->inputescapes);
    ret |= pthread_mutex_destroy(&nc->lock);
    free(nc);
  }
  return ret;
}

uint64_t ncplane_get_channels(const ncplane* n){
  return n->channels;
}

uint32_t ncplane_get_attr(const ncplane* n){
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

void ncplane_set_fg(ncplane* n, uint32_t channel){
  n->channels = ((uint64_t)channel << 32ul) | (n->channels & 0xffffffffull);
}

void ncplane_set_bg(ncplane* n, uint32_t channel){
  n->channels = (n->channels & 0xffffffff00000000ull) | channel;
}

int ncplane_set_fg_alpha(ncplane* n, int alpha){
  return channels_set_fg_alpha(&n->channels, alpha);
}

int ncplane_set_bg_alpha(ncplane *n, int alpha){
  return channels_set_bg_alpha(&n->channels, alpha);
}

int ncplane_set_default(ncplane* ncp, const cell* c){
  int ret = cell_duplicate(ncp, &ncp->defcell, c);
  if(ret < 0){
    return -1;
  }
  ncplane_updamage(ncp);
  return ret;
}

int ncplane_default(ncplane* ncp, cell* c){
  return cell_duplicate(ncp, c, &ncp->defcell);
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
  ncplane_updamage(n); // conservative (we might not actually be visible)
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
  ncplane_updamage(n); // conservative (we might not actually be visible)
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
  ncplane_updamage(n);
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
  ncplane_updamage(n);
  return 0;
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
  ncplane_lock(n);
  if(cursor_invalid_p(n)){
    ncplane_unlock(n);
    return -1;
  }
  cell* targ = &n->fb[fbcellidx(n, n->y, n->x)];
  if(cell_duplicate(n, targ, c) < 0){
    ncplane_unlock(n);
    return -1;
  }
  int cols = 1 + cell_double_wide_p(targ);
  n->damage[n->y] = true;
  advance_cursor(n, cols);
  ncplane_unlock(n);
  return cols;
}

int ncplane_putsimple(struct ncplane* n, char c){
  cell ce = {
    .gcluster = c,
    .attrword = ncplane_get_attr(n),
    .channels = ncplane_get_channels(n),
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

unsigned ncplane_styles(const ncplane* n){
  unsigned ret;
  ncplane_lock(n);
  ret = (n->attrword & CELL_STYLE_MASK);
  ncplane_unlock(n);
  return ret;
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

int ncplane_hline_interp(ncplane* n, const cell* c, int len,
                         uint64_t c1, uint64_t c2){
  unsigned ur, ug, ub;
  int r1, g1, b1, r2, g2, b2;
  int br1, bg1, bb1, br2, bg2, bb2;
  channels_get_fg_rgb(c1, &ur, &ug, &ub);
  r1 = ur; g1 = ug; b1 = ub;
  channels_get_fg_rgb(c2, &ur, &ug, &ub);
  r2 = ur; g2 = ug; b2 = ub;
  channels_get_bg_rgb(c1, &ur, &ug, &ub);
  br1 = ur; bg1 = ug; bb1 = ub;
  channels_get_bg_rgb(c2, &ur, &ug, &ub);
  br2 = ur; bg2 = ug; bb2 = ub;
  int deltr = (r2 - r1) / (len + 1);
  int deltg = (g2 - g1) / (len + 1);
  int deltb = (b2 - b1) / (len + 1);
  int deltbr = (br2 - br1) / (len + 1);
  int deltbg = (bg2 - bg1) / (len + 1);
  int deltbb = (bb2 - bb1) / (len + 1);
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

int ncplane_vline_interp(ncplane* n, const cell* c, int len,
                         uint64_t c1, uint64_t c2){
  unsigned ur, ug, ub;
  int r1, g1, b1, r2, g2, b2;
  int br1, bg1, bb1, br2, bg2, bb2;
  channels_get_fg_rgb(c1, &ur, &ug, &ub);
  r1 = ur; g1 = ug; b1 = ub;
  channels_get_fg_rgb(c2, &ur, &ug, &ub);
  r2 = ur; g2 = ug; b2 = ub;
  channels_get_bg_rgb(c1, &ur, &ug, &ub);
  br1 = ur; bg1 = ug; bb1 = ub;
  channels_get_bg_rgb(c2, &ur, &ug, &ub);
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

// mark all lines of the notcurses object touched by this plane as damaged
void ncplane_updamage(ncplane* n){
  int drangelow = n->absy;
  int drangehigh = n->absy + n->leny;
  if(drangehigh > n->nc->stdscr->leny){
    drangehigh = n->nc->stdscr->leny;
  }
  if(drangelow < n->nc->stdscr->absy){
    drangelow = n->nc->stdscr->absy;
  }
  if(drangelow > n->nc->stdscr->absy + n->nc->stdscr->leny - 1){
    drangelow = n->nc->stdscr->absy + n->nc->stdscr->leny - 1;
  }
  flash_damage_map(n->nc->damage + drangelow, drangehigh - drangelow, true);
}

int ncplane_move_yx(ncplane* n, int y, int x){
  if(n == n->nc->stdscr){
    return -1;
  }
  ncplane_updamage(n); // damage any lines we are currently on
  bool movedy = n->absy != y;
  n->absy = y;
  n->absx = x;
  if(movedy){
    ncplane_updamage(n);
  }
  return 0;
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
  ncplane_lock(n);
  // we must preserve the background, but a pure cell_duplicate() would be
  // wiped out by the egcpool_dump(). do a duplication (to get the attrword
  // and channels), and then reload.
  char* egc = cell_egc_copy(n, &n->defcell);
  memset(n->fb, 0, sizeof(*n->fb) * n->lenx * n->leny);
  egcpool_dump(&n->pool);
  egcpool_init(&n->pool);
  cell_load(n, &n->defcell, egc);
  free(egc);
  ncplane_updamage(n);
  ncplane_unlock(n);
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
    if(ncplane_cursor_move_yx(ncv->ncp, y, 0)){
      return -1;
    }
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
        cell_set_bg_default(&c);
        if(!rgbbase_up[3] && !rgbbase_down[3]){
          if(cell_load(ncv->ncp, &c, " ") <= 0){
            return -1;
          }
          cell_set_fg_default(&c);
        }else if(!rgbbase_up[3]){ // down has the color
          if(cell_load(ncv->ncp, &c, "\u2584") <= 0){ // lower half block
            return -1;
          }
          cell_set_fg_rgb(&c, rgbbase_down[0], rgbbase_down[1], rgbbase_down[2]);
        }else{ // up has the color
          if(cell_load(ncv->ncp, &c, "\u2580") <= 0){ // upper half block
            return -1;
          }
          cell_set_fg_rgb(&c, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2]);
        }
      }else{
        cell_set_fg_rgb(&c, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2]);
        cell_set_bg_rgb(&c, rgbbase_down[0], rgbbase_down[1], rgbbase_down[2]);
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
  flash_damage_map(ncv->ncp->damage, y, true);
  return 0;
}

int ncvisual_stream(notcurses* nc, ncvisual* ncv, int* averr, streamcb streamer){
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
    if(streamer){
      int r = streamer(nc, ncv);
      if(r){
        return r;
      }
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

// if "retain_cursor" was set, we don't have these definitions FIXME
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
