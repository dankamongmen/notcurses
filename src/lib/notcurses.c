#include "version.h"
#include "egcpool.h"
#include "internal.h"
#include <ncurses.h> // needed for some definitions, see terminfo(3ncurses)
#include <poll.h>
#include <time.h>
#include <term.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <unistr.h>
#include <locale.h>
#include <uniwbrk.h>
#include <langinfo.h>
#include <sys/ioctl.h>
#include <notcurses/direct.h>

#define ESC "\x1b"

// there does not exist any true standard terminal size. with that said, we
// need assume *something* for the case where we're not actually attached to
// a terminal (mainly unit tests, but also daemon environments).
static const int DEFAULT_ROWS = 24;
static const int DEFAULT_COLS = 80;

void notcurses_version_components(int* major, int* minor, int* patch, int* tweak){
  *major = NOTCURSES_VERNUM_MAJOR;
  *minor = NOTCURSES_VERNUM_MINOR;
  *patch = NOTCURSES_VERNUM_PATCH;
  *tweak = atoi(NOTCURSES_VERSION_TWEAK);
}

// reset the current colors, styles, and palette. called on startup (to purge
// any preexisting styling) and shutdown (to not affect further programs).
static int
reset_term_attributes(notcurses* nc){
  int ret = 0;
  if(nc->tcache.op && term_emit("op", nc->tcache.op, nc->ttyfp, false)){
    ret = -1;
  }
  if(nc->tcache.sgr0 && term_emit("sgr0", nc->tcache.sgr0, nc->ttyfp, false)){
    ret = -1;
  }
  if(nc->tcache.oc && term_emit("oc", nc->tcache.oc, nc->ttyfp, true)){
    ret = -1;
  }
  ret |= notcurses_mouse_disable(nc);
  return ret;
}

// Do the minimum necessary stuff to restore the terminal, then return. This is
// the end of the line for fatal signal handlers. notcurses_stop() will go on
// to tear down and account for internal structures.
static int
notcurses_stop_minimal(void* vnc){
  notcurses* nc = vnc;
  int ret = drop_signals(nc);
  // be sure to write the restoration sequences *prior* to running rmcup, as
  // they apply to the screen (alternate or otherwise) we're actually using.
  ret |= reset_term_attributes(nc);
  if(nc->ttyfd >= 0){
    if(nc->tcache.rmcup && tty_emit("rmcup", nc->tcache.rmcup, nc->ttyfd)){
      ret = -1;
    }
    if(nc->tcache.cnorm && tty_emit("cnorm", nc->tcache.cnorm, nc->ttyfd)){
      ret = -1;
    }
    ret |= tcsetattr(nc->ttyfd, TCSANOW, &nc->tpreserved);
  }
  return ret;
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
 NOTCURSES_VERSION_MAJOR "."
 NOTCURSES_VERSION_MINOR "."
 NOTCURSES_VERSION_PATCH;

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

char* ncplane_at_cursor(ncplane* n, uint16_t* stylemask, uint64_t* channels){
  return ncplane_at_yx(n, n->y, n->x, stylemask, channels);
}

char* ncplane_at_yx(const ncplane* n, int y, int x, uint16_t* stylemask, uint64_t* channels){
  if(y < n->leny && x < n->lenx){
    if(y >= 0 && x >= 0){
      return cell_extract(n, &n->fb[nfbcellidx(n, y, x)], stylemask, channels);
    }
  }
  return NULL;
}

int ncplane_at_cursor_cell(ncplane* n, nccell* c){
  return ncplane_at_yx_cell(n, n->y, n->x, c);
}

int ncplane_at_yx_cell(ncplane* n, int y, int x, nccell* c){
  if(y < n->leny && x < n->lenx){
    if(y >= 0 && x >= 0){
      nccell* targ = ncplane_cell_ref_yx(n, y, x);
      if(cell_duplicate(n, c, targ) == 0){
        return strlen(cell_extended_gcluster(n, targ));
      }
    }
  }
  return -1;
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
// to reflect changes in geometry. also called at startup for standard plane.
int update_term_dimensions(int fd, int* rows, int* cols){
  // if we're not a real tty, we presumably haven't changed geometry, return
  if(fd < 0){
    *rows = DEFAULT_ROWS;
    *cols = DEFAULT_COLS;
    return 0;
  }
  struct winsize ws;
  int i = ioctl(fd, TIOCGWINSZ, &ws);
  if(i < 0){
    fprintf(stderr, "TIOCGWINSZ failed on %d (%s)\n", fd, strerror(errno));
    return -1;
  }
  if(ws.ws_row <= 0 || ws.ws_col <= 0){
    fprintf(stderr, "Bogus return from TIOCGWINSZ on %d (%d/%d)\n",
            fd, ws.ws_row, ws.ws_col);
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

// destroy an empty ncpile. only call with pilelock held.
static void
ncpile_destroy(ncpile* pile){
  if(pile){
    pile->prev->next = pile->next;
    pile->next->prev = pile->prev;
    free(pile->crender);
    free(pile);
  }
}

void free_plane(ncplane* p){
  if(p){
    // ncdirect fakes an ncplane with no ->pile
    if(ncplane_pile(p)){
      --ncplane_notcurses(p)->stats.planes;
      ncplane_notcurses(p)->stats.fbbytes -= sizeof(*p->fb) * p->leny * p->lenx;
      if(p->above == NULL && p->below == NULL){
        struct notcurses* nc = ncplane_notcurses(p);
        pthread_mutex_lock(&nc->pilelock);
        ncpile_destroy(ncplane_pile(p));
        pthread_mutex_unlock(&nc->pilelock);
      }
    }
    egcpool_dump(&p->pool);
    free(p->name);
    free(p->fb);
    free(p);
  }
}

// create a new ncpile. only call with pilelock held.
static ncpile*
make_ncpile(notcurses* nc, ncplane* n){
  ncpile* ret = malloc(sizeof(*ret));
  if(ret){
    ret->nc = nc;
    ret->top = n;
    ret->bottom = n;
    ret->roots = n;
    n->bprev = &ret->roots;
    if(nc->stdplane){ // stdplane (and thus stdpile) has already been created
      ret->prev = ncplane_pile(nc->stdplane)->prev;
      ncplane_pile(nc->stdplane)->prev->next = ret;
      ret->next = ncplane_pile(nc->stdplane);
      ncplane_pile(nc->stdplane)->prev = ret;
    }else{
      ret->prev = ret;
      ret->next = ret;
    }
    n->pile = ret;
    n->above = NULL;
    n->below = NULL;
    ret->dimy = 0;
    ret->dimx = 0;
    ret->crender = NULL;
  }
  return ret;
}

// create a new ncplane at the specified location (relative to the true screen,
// having origin at 0,0), having the specified size, and put it at the top of
// the planestack. its cursor starts at its origin; its style starts as null.
// a plane may exceed the boundaries of the screen, but must have positive
// size in both dimensions. bind the plane to 'n', which may be NULL to create
// a new pile. if bound to a plane instead, this plane moves when that plane
// moves, and coordinates to move to are relative to that plane.
// there are two denormalized case we also must handle, that of the "fake"
// isolated ncplane created by ncdirect for rendering visuals. in that case
// (and only in that case), nc is NULL (as is n). there's also creation of the
// initial standard plane, in which case nc is not NULL, but nc->stdplane *is*
// (as once more is n).
ncplane* ncplane_new_internal(notcurses* nc, ncplane* n,
                              const ncplane_options* nopts){
  if(nopts->flags >= (NCPLANE_OPTION_HORALIGNED << 1u)){
    logwarn(nc, "Provided unsupported flags %016jx\n", (uintmax_t)nopts->flags);
  }
  if(nopts->rows <= 0 || nopts->cols <= 0){
    logerror(nc, "Won't create denormalized plane (r=%d, c=%d)\n",
             nopts->rows, nopts->cols);
    return NULL;
  }
  ncplane* p = malloc(sizeof(*p));
  size_t fbsize = sizeof(*p->fb) * (nopts->rows * nopts->cols);
  if((p->fb = malloc(fbsize)) == NULL){
    logerror(nc, "Error allocating cellmatrix (r=%d, c=%d)\n",
             nopts->rows, nopts->cols);
    free(p);
    return NULL;
  }
  memset(p->fb, 0, fbsize);
  p->scrolling = false;
  p->leny = nopts->rows;
  p->lenx = nopts->cols;
  p->x = p->y = 0;
  p->logrow = 0;
  p->blist = NULL;
  p->name = strdup(nopts->name ? nopts->name : "");
  p->align = NCALIGN_UNALIGNED;
  if(!n){ // new root/standard plane
    p->absy = nopts->y;
    p->absx = nopts->x;
    p->bnext = NULL;
    p->bprev = NULL;
    p->boundto = p;
    p->align = NCALIGN_UNALIGNED;
  }else{ // bound to preexisting pile
    if(nopts->flags & NCPLANE_OPTION_HORALIGNED){
      p->absx = ncplane_align(n, nopts->x, nopts->cols);
      p->align = nopts->x;
    }else{
      p->absx = nopts->x;
      p->align = NCALIGN_UNALIGNED;
    }
    p->absx += n->absx;
    p->absy = nopts->y + n->absy;
    if( (p->bnext = n->blist) ){
      n->blist->bprev = &p->bnext;
    }
    p->bprev = &n->blist;
    *p->bprev = p;
    p->boundto = n;
  }
  p->resizecb = nopts->resizecb;
  p->stylemask = 0;
  p->channels = 0;
  egcpool_init(&p->pool);
  cell_init(&p->basecell);
  p->userptr = nopts->userptr;
  if(nc == NULL){ // fake ncplane backing ncdirect object
    p->above = NULL;
    p->below = NULL;
    p->pile = NULL;
  }else{
    pthread_mutex_lock(&nc->pilelock);
    ncpile* pile = n ? ncplane_pile(n) : NULL;
    if( (p->pile = pile) ){ // existing pile
      p->above = NULL;
      if( (p->below = pile->top) ){ // always happens save initial plane
        pile->top->above = p;
      }else{
        pile->bottom = p;
      }
      pile->top = p;
    }else{ // new pile
      make_ncpile(nc, p);
    }
    nc->stats.fbbytes += fbsize;
    ++nc->stats.planes;
    pthread_mutex_unlock(&nc->pilelock);
  }
  loginfo(nc, "Created new %dx%d plane \"%s\" @ %dx%d\n",
          nopts->rows, nopts->cols, p->name, p->absy, p->absx);
  return p;
}

// create an ncplane of the specified dimensions, but do not yet place it in
// the z-buffer. clear out all cells. this is for a wholly new context.
// FIXME set up using resizecb rather than special-purpose from SIGWINCH
static ncplane*
create_initial_ncplane(notcurses* nc, int dimy, int dimx){
  ncplane_options nopts = {
    .y = nc->margin_t,
    .x = nc->margin_l,
    .rows = dimy - (nc->margin_t + nc->margin_b),
    .cols = dimx - (nc->margin_l + nc->margin_r),
    .userptr = NULL,
    .name = "std",
    .resizecb = ncplane_resize_maximize,
    .flags = 0,
  };
  return nc->stdplane = ncplane_new_internal(nc, NULL, &nopts);
}

ncplane* notcurses_stdplane(notcurses* nc){
  return nc->stdplane;
}

const ncplane* notcurses_stdplane_const(const notcurses* nc){
  return nc->stdplane;
}

ncplane* ncplane_create(ncplane* n, const ncplane_options* nopts){
  return ncplane_new_internal(ncplane_notcurses(n), n, nopts);
}

ncplane* ncpile_create(notcurses* nc, const struct ncplane_options* nopts){
  return ncplane_new_internal(nc, NULL, nopts);
}

struct ncplane* ncplane_new(struct ncplane* n, int rows, int cols, int y, int x, void* opaque, const char* name){
  ncplane_options nopts = {
    .y = y,
    .x = x,
    .rows = rows,
    .cols = cols,
    .userptr = opaque,
    .name = name,
    .resizecb = NULL,
    .flags = 0,
  };
  return ncplane_create(n, &nopts);
}

void ncplane_home(ncplane* n){
  n->x = 0;
  n->y = 0;
}

inline int ncplane_cursor_move_yx(ncplane* n, int y, int x){
  if(x >= n->lenx){
    logerror(ncplane_notcurses(n), "Target x %d >= length %d\n", x, n->lenx);
    return -1;
  }else if(x < 0){
    if(x < -1){
      logerror(ncplane_notcurses(n), "Negative target x %d\n", x);
      return -1;
    }
  }else{
    n->x = x;
  }
  if(y >= n->leny){
    logerror(ncplane_notcurses(n), "Target y %d >= height %d\n", y, n->leny);
    return -1;
  }else if(y < 0){
    if(y < -1){
      logerror(ncplane_notcurses(n), "Negative target y %d\n", y);
      return -1;
    }
  }else{
    n->y = y;
  }
  if(cursor_invalid_p(n)){
    logerror(ncplane_notcurses(n), "Invalid cursor following move (%d/%d)\n", n->y, n->x);
    return -1;
  }
  return 0;
}

ncplane* ncplane_dup(const ncplane* n, void* opaque){
  int dimy = n->leny;
  int dimx = n->lenx;
  // if we're duping the standard plane, we need adjust for marginalia
  const struct notcurses* nc = ncplane_notcurses_const(n);
  const int placey = n->absy - nc->margin_t;
  const int placex = n->absx - nc->margin_l;
  struct ncplane_options nopts = {
    .y = placey,
    .x = placex,
    .rows = dimy,
    .cols = dimx,
    .userptr = opaque,
    .name = n->name,
    .resizecb = ncplane_resizecb(n),
    .flags = 0,
  };
  ncplane* newn = ncplane_create(n->boundto, &nopts);
  if(newn){
    if(egcpool_dup(&newn->pool, &n->pool)){
      ncplane_destroy(newn);
      return NULL;
    }else{
      if(ncplane_cursor_move_yx(newn, n->y, n->x) < 0){
        ncplane_destroy(newn);
        return NULL;
      }
      newn->align = n->align;
      newn->stylemask = ncplane_styles(n);
      newn->channels = ncplane_channels(n);
      memmove(newn->fb, n->fb, sizeof(*n->fb) * dimx * dimy);
      // we dupd the egcpool, so just dup the goffset
      newn->basecell = n->basecell;
    }
  }
  return newn;
}

// call the resize callback for each bound child in turn. we only need to do
// the first generation; if they resize, they'll invoke
// ncplane_resize_internal(), leading to this function being called anew.
int resize_callbacks_children(ncplane* n){
  int ret = 0;
  for(struct ncplane* child = n->blist ; child ; child = child->bnext){
    if(child->resizecb){
      ret |= child->resizecb(child);
    }
  }
  return ret;
}

// can be used on stdplane, unlike ncplane_resize() which prohibits it.
int ncplane_resize_internal(ncplane* n, int keepy, int keepx, int keepleny,
                            int keeplenx, int yoff, int xoff, int ylen, int xlen){
  if(keepleny < 0 || keeplenx < 0){ // can't retain negative size
    logerror(ncplane_notcurses(n), "Can't retain negative size %dx%d\n", keepleny, keeplenx);
    return -1;
  }
  if(keepy < 0 || keepx < 0){ // can't start at negative origin
    logerror(ncplane_notcurses(n), "Can't retain negative offset %dx%d\n", keepy, keepx);
    return -1;
  }
  if((!keepleny && keeplenx) || (keepleny && !keeplenx)){ // both must be 0
    logerror(ncplane_notcurses(n), "Can't retain null dimension %dx%d\n", keepleny, keeplenx);
    return -1;
  }
  // can't be smaller than keep length + abs(offset from keep area)
  const int yprescribed = keepleny + (yoff < 0 ? -yoff : yoff);
  if(ylen < yprescribed){
    logerror(ncplane_notcurses(n), "Can't map in y dimension: %d < %d\n", ylen, yprescribed);
    return -1;
  }
  const int xprescribed = keeplenx + (xoff < 0 ? -xoff : xoff);
  if(xlen < xprescribed){
    logerror(ncplane_notcurses(n), "Can't map in x dimension: %d < %d\n", xlen, xprescribed);
    return -1;
  }
  if(ylen <= 0 || xlen <= 0){ // can't resize to trivial or negative size
    logerror(ncplane_notcurses(n), "Can't achieve meaningless size %dx%d\n", ylen, xlen);
    return -1;
  }
  int rows, cols;
  ncplane_dim_yx(n, &rows, &cols);
  if(keepleny + keepy > rows){
    logerror(ncplane_notcurses(n), "Can't keep %d@%d rows from %d\n", keepleny, keepy, rows);
    return -1;
  }
  if(keeplenx + keepx > cols){
    logerror(ncplane_notcurses(n), "Can't keep %d@%d cols from %d\n", keeplenx, keepx, cols);
    return -1;
  }
  loginfo(ncplane_notcurses(n), "%dx%d @ %d/%d → %d/%d @ %d/%d (keeping %dx%d from %d/%d)\n", rows, cols, n->absy, n->absx, ylen, xlen, n->absy + keepy + yoff, n->absx + keepx + xoff, keepleny, keeplenx, keepy, keepx);
  // we're good to resize. we'll need alloc up a new framebuffer, and copy in
  // those elements we're retaining, zeroing out the rest. alternatively, if
  // we've shrunk, we will be filling the new structure.
  int keptarea = keepleny * keeplenx;
  int newarea = ylen * xlen;
  size_t fbsize = sizeof(nccell) * newarea;
  nccell* fb = malloc(fbsize);
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
  nccell* preserved = n->fb;
  ncplane_notcurses(n)->stats.fbbytes -= sizeof(*preserved) * (rows * cols);
  ncplane_notcurses(n)->stats.fbbytes += fbsize;
  n->fb = fb;
  const int oldabsy = n->absy;
  // go ahead and move. we can no longer fail at this point. but don't yet
  // resize, because n->len[xy] are used in fbcellidx() in the loop below. we
  // don't use ncplane_move_yx(), because we want to planebinding-invariant.
  n->absy += keepy + yoff;
  n->absx += keepx + xoff;
//fprintf(stderr, "absx: %d keepx: %d xoff: %d\n", n->absx, keepx, xoff);
  if(keptarea == 0){ // keep nothing, resize/move only
    // if we're keeping nothing, dump the old egcspool. otherwise, we go ahead
    // and keep it. perhaps we ought compact it?
    memset(fb, 0, sizeof(*fb) * newarea);
    egcpool_dump(&n->pool);
    n->lenx = xlen;
    n->leny = ylen;
    free(preserved);
    return resize_callbacks_children(n);
  }
  // we currently have maxy rows of maxx cells each. we will be keeping rows
  // keepy..keepy + keepleny - 1 and columns keepx..keepx + keeplenx - 1.
  // anything else is zerod out. itery is the row we're writing *to*, and we
  // must write to each (and every cell in each).
  for(int itery = 0 ; itery < ylen ; ++itery){
    int truey = itery + n->absy;
    int sourceoffy = truey - oldabsy;
//fprintf(stderr, "sourceoffy: %d keepy: %d ylen: %d\n", sourceoffy, keepy, ylen);
    // if we have nothing copied to this line, zero it out in one go
    if(sourceoffy < keepy || sourceoffy >= keepy + keepleny){
//fprintf(stderr, "writing 0s to line %d of %d\n", itery, ylen);
      memset(fb + (itery * xlen), 0, sizeof(*fb) * xlen);
    }else{
      int copyoff = itery * xlen; // our target at any given time
      // we do have something to copy, and zero, one, or two regions to zero out
      int copied = 0;
      if(xoff < 0){
        memset(fb + copyoff, 0, sizeof(*fb) * -xoff);
        copyoff += -xoff;
        copied += -xoff;
      }
      const int sourceidx = nfbcellidx(n, sourceoffy, keepx);
//fprintf(stderr, "copying line %d (%d) to %d (%d)\n", sourceoffy, sourceidx, copyoff / xlen, copyoff);
      memcpy(fb + copyoff, preserved + sourceidx, sizeof(*fb) * keeplenx);
      copyoff += keeplenx;
      copied += keeplenx;
      if(xlen > copied){
        memset(fb + copyoff, 0, sizeof(*fb) * (xlen - copied));
      }
    }
  }
  n->lenx = xlen;
  n->leny = ylen;
  free(preserved);
  return resize_callbacks_children(n);
}

int ncplane_resize(ncplane* n, int keepy, int keepx, int keepleny,
                   int keeplenx, int yoff, int xoff, int ylen, int xlen){
  if(n == ncplane_notcurses(n)->stdplane){
//fprintf(stderr, "Can't resize standard plane\n");
    return -1;
  }
  return ncplane_resize_internal(n, keepy, keepx, keepleny, keeplenx,
                                 yoff, xoff, ylen, xlen);
}

int ncplane_destroy(ncplane* ncp){
  if(ncp == NULL){
    return 0;
  }
  if(ncplane_notcurses(ncp)->stdplane == ncp){
    logerror(ncplane_notcurses(ncp), "Won't destroy standard plane\n");
    return -1;
  }
  if(ncp->above){
    ncp->above->below = ncp->below;
  }else{
    ncplane_pile(ncp)->top = ncp->below;
  }
  if(ncp->below){
    ncp->below->above = ncp->above;
  }else{
    ncplane_pile(ncp)->bottom = ncp->above;
  }
  if(ncp->bprev){
    if( (*ncp->bprev = ncp->bnext) ){
      ncp->bnext->bprev = ncp->bprev;
    }
  }
  int ret = 0;
  struct ncplane* bound = ncp->blist;
  while(bound){
    struct ncplane* tmp = bound->bnext;
    if(ncplane_reparent(bound, ncp->boundto) == NULL){
      ret = -1;
    }
    bound = tmp;
  }
  free_plane(ncp);
  return ret;
}

int ncplane_genocide(ncplane *ncp){
  if(ncp == NULL){
    return 0;
  }
  if(ncplane_notcurses(ncp)->stdplane == ncp){
    logerror(ncplane_notcurses(ncp), "Won't destroy standard plane\n");
    return -1;
  }
  int ret = 0;
  while(ncp->blist){
    ret |= ncplane_genocide(ncp->blist);
  }
  ret |= ncplane_destroy(ncp);
  return ret;
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
  stats->render_min_ns = 1ull << 62u;
  stats->render_min_bytes = 1ull << 62u;
  stats->writeout_min_ns = 1ull << 62u;
  stats->fbbytes = fbbytes;
}

// add the current stats to the cumulative stashed stats, and reset them
static void
stash_stats(notcurses* nc){
  nc->stashstats.renders += nc->stats.renders;
  nc->stashstats.render_ns += nc->stats.render_ns;
  nc->stashstats.writeouts += nc->stats.writeouts;
  nc->stashstats.writeout_ns += nc->stats.writeout_ns;
  nc->stashstats.failed_renders += nc->stats.failed_renders;
  nc->stashstats.failed_writeouts += nc->stats.failed_writeouts;
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
  if(nc->stashstats.writeout_max_ns < nc->stats.writeout_max_ns){
    nc->stashstats.writeout_max_ns = nc->stats.writeout_max_ns;
  }
  if(nc->stashstats.writeout_min_ns > nc->stats.writeout_min_ns){
    nc->stashstats.writeout_min_ns = nc->stats.writeout_min_ns;
  }
  nc->stashstats.cellelisions += nc->stats.cellelisions;
  nc->stashstats.cellemissions += nc->stats.cellemissions;
  nc->stashstats.fgelisions += nc->stats.fgelisions;
  nc->stashstats.fgemissions += nc->stats.fgemissions;
  nc->stashstats.bgelisions += nc->stats.bgelisions;
  nc->stashstats.bgemissions += nc->stats.bgemissions;
  nc->stashstats.defaultelisions += nc->stats.defaultelisions;
  nc->stashstats.defaultemissions += nc->stats.defaultemissions;
  nc->stashstats.refreshes += nc->stats.refreshes;
  // fbbytes aren't stashed
  reset_stats(&nc->stats);
}

void notcurses_stats(const notcurses* nc, ncstats* stats){
  memcpy(stats, &nc->stats, sizeof(*stats));
}

ncstats* notcurses_stats_alloc(const notcurses* nc __attribute__ ((unused))){
  return malloc(sizeof(ncstats));
}

void notcurses_stats_reset(notcurses* nc, ncstats* stats){
  if(stats){
    memcpy(stats, &nc->stats, sizeof(*stats));
  }
  stash_stats(nc);
}

// Convert a notcurses log level to some multimedia library equivalent.
int ffmpeg_log_level(ncloglevel_e level){
#ifdef USE_FFMPEG
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
#else
  return level;
#endif
}

// unless the suppress_banner flag was set, print some version information and
// (if applicable) warnings to stdout. we are not yet on the alternate screen.
static void
init_banner(const notcurses* nc){
  if(!nc->suppress_banner){
    char prefixbuf[BPREFIXSTRLEN + 1];
    term_fg_palindex(nc, stdout, nc->tcache.colors <= 256 ? 50 % nc->tcache.colors : 0x20e080);
    printf("\n notcurses %s by nick black et al", notcurses_version());
    term_fg_palindex(nc, stdout, nc->tcache.colors <= 256 ? 12 % nc->tcache.colors : 0x2080e0);
    printf("\n  %d rows %d cols (%sB) %zuB cells %d colors%s\n"
           "  compiled with gcc-%s, %s-endian\n"
           "  terminfo from %s\n",
           nc->stdplane->leny, nc->stdplane->lenx,
           bprefix(nc->stats.fbbytes, 1, prefixbuf, 0), sizeof(nccell),
           nc->tcache.colors, nc->tcache.RGBflag ? "+RGB" : "",
           __VERSION__,
#ifdef __BYTE_ORDER__
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
           "little"
#else
           "big"
#endif
#else
#error "No __BYTE_ORDER__ definition"
#endif
           , curses_version());
#ifdef USE_FFMPEG
    printf("  avformat %u.%u.%u avutil %u.%u.%u swscale %u.%u.%u\n",
          LIBAVFORMAT_VERSION_MAJOR, LIBAVFORMAT_VERSION_MINOR, LIBAVFORMAT_VERSION_MICRO,
          LIBAVUTIL_VERSION_MAJOR, LIBAVUTIL_VERSION_MINOR, LIBAVUTIL_VERSION_MICRO,
          LIBSWSCALE_VERSION_MAJOR, LIBSWSCALE_VERSION_MINOR, LIBSWSCALE_VERSION_MICRO);
#else
#ifdef USE_OIIO
    printf("  openimageio %s\n", oiio_version());
#else
    term_fg_palindex(nc, stderr, nc->tcache.colors <= 88 ? 1 % nc->tcache.colors : 0xcb);
    fprintf(stderr, "\n Warning! Notcurses was built without multimedia support.\n");
#endif
#endif
    fflush(stdout);
    term_fg_palindex(nc, stderr, nc->tcache.colors <= 88 ? 1 % nc->tcache.colors : 0xcb);
    if(!nc->tcache.RGBflag){ // FIXME
      fprintf(stderr, "\n Warning! Colors subject to https://github.com/dankamongmen/notcurses/issues/4");
      fprintf(stderr, "\n  Specify a (correct) TrueColor TERM, or COLORTERM=24bit.\n");
    }else{
      if(!nc->tcache.CCCflag){
        fprintf(stderr, "\n Warning! Advertised TrueColor but no 'ccc' flag\n");
      }
    }
    if(!notcurses_canutf8(nc)){
      fprintf(stderr, "\n Warning! Encoding is not UTF-8; output may be degraded.\n");
    }
    if(nc->tcache.sgr0){
      term_emit("sgr0", nc->tcache.sgr0, stderr, true);
    }
  }
}

// it's critical that we're using UTF-8 encoding if at all possible. since the
// client might not have called setlocale(2) (if they weren't reading the
// directions...), go ahead and try calling setlocale(LC_ALL, "") and then
// setlocale(LC_CTYPE, "C.UTF-8") ourselves *iff* we're not using UTF-8 *and*
// LANG is not explicitly set to "C" nor "POSIX". this still requires the user
// to have a proper locale generated and available on disk. either way, they're
// going to get a diagnostic (unless the user has explicitly configured a LANG
// of "C" or "POSIX"). recommended practice is for the client code to have
// called setlocale() themselves, and set the NCOPTION_INHIBIT_SETLOCALE flag.
// if that flag is set, we take the locale and encoding as we get them.
void init_lang(struct notcurses* nc){
  const char* encoding = nl_langinfo(CODESET);
  if(encoding && !strcmp(encoding, "UTF-8")){
    return; // already utf-8, great!
  }
  const char* lang = getenv("LANG");
  // if LANG was explicitly set to C/POSIX, life sucks, roll with it
  if(lang && (!strcmp(lang, "C") || !strcmp(lang, "POSIX"))){
    loginfo(nc, "LANG was explicitly set to %s, not changing locale\n", lang);
    return;
  }
  setlocale(LC_ALL, "");
  encoding = nl_langinfo(CODESET);
  if(encoding && !strcmp(encoding, "UTF-8")){
    loginfo(nc, "Set locale from LANG; client should call setlocale(2)!\n");
    return;
  }
  setlocale(LC_CTYPE, "C.UTF-8");
  encoding = nl_langinfo(CODESET);
  if(encoding && !strcmp(encoding, "UTF-8")){
    loginfo(nc, "Forced UTF-8 encoding; client should call setlocale(2)!\n");
    return;
  }
}

// if ttyfp is a tty, return a file descriptor extracted from it. otherwise,
// try to get the controlling terminal. otherwise, return -1.
static int
get_tty_fd(notcurses* nc, FILE* ttyfp){
  int fd = -1;
  if(ttyfp){
    if((fd = fileno(ttyfp)) < 0){
      logwarn(nc, "No file descriptor was available in outfp %p\n", ttyfp);
    }else{
      if(isatty(fd)){
        fd = dup(fd);
      }else{
        loginfo(nc, "File descriptor %d was not a TTY\n", fd);
        fd = -1;
      }
    }
  }
  if(fd < 0){
    fd = open("/dev/tty", O_RDWR | O_CLOEXEC);
    if(fd < 0){
      loginfo(nc, "Error opening /dev/tty (%s)\n", strerror(errno));
    }else{
      if(!isatty(fd)){
        loginfo(nc, "File descriptor for /dev/tty (%d) is not actually a TTY\n", fd);
        close(fd);
        fd = -1;
      }
    }
  }
  return fd;
}

int ncinputlayer_init(ncinputlayer* nilayer, FILE* infp){
  setbuffer(infp, NULL, 0);
  nilayer->inputescapes = NULL;
  nilayer->ttyinfp = infp;
  if(prep_special_keys(nilayer)){
    return -1;
  }
  nilayer->inputbuf_occupied = 0;
  nilayer->inputbuf_valid_starts = 0;
  nilayer->inputbuf_write_at = 0;
  nilayer->input_events = 0;
  return 0;
}

// initialize a recursive mutex lock in a way that works on both glibc + musl
int recursive_lock_init(pthread_mutex_t *lock){
#ifndef __GLIBC__
#define PTHREAD_MUTEX_RECURSIVE_NP PTHREAD_MUTEX_RECURSIVE
#endif
  pthread_mutexattr_t attr;
  if(pthread_mutexattr_init(&attr)){
    return -1;
  }
  if(pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP)){
    pthread_mutexattr_destroy(&attr);
    return -1;
  }
  if(pthread_mutex_init(lock, &attr)){
    pthread_mutexattr_destroy(&attr);
    return -1;
  }
  pthread_mutexattr_destroy(&attr);
  return 0;
#ifndef __GLIBC__
#undef PTHREAD_MUTEX_RECURSIVE_NP
#endif
}

notcurses* notcurses_init(const notcurses_options* opts, FILE* outfp){
  notcurses_options defaultopts;
  memset(&defaultopts, 0, sizeof(defaultopts));
  if(!opts){
    opts = &defaultopts;
  }
  if(opts->margin_t < 0 || opts->margin_b < 0 || opts->margin_l < 0 || opts->margin_r < 0){
    fprintf(stderr, "Provided an illegal negative margin, refusing to start\n");
    return NULL;
  }
  if(opts->flags >= (NCOPTION_NO_FONT_CHANGES << 1u)){
    fprintf(stderr, "Warning: unknown Notcurses options %016jx\n", (uintmax_t)opts->flags);
  }
  notcurses* ret = malloc(sizeof(*ret));
  if(ret == NULL){
    return ret;
  }
  ret->loglevel = opts->loglevel;
  if(!(opts->flags & NCOPTION_INHIBIT_SETLOCALE)){
    init_lang(ret);
  }
  const char* encoding = nl_langinfo(CODESET);
  if(encoding && !strcmp(encoding, "UTF-8")){
    ret->utf8 = true;
  }else if(encoding && (!strcmp(encoding, "ANSI_X3.4-1968") || !strcmp(encoding, "US-ASCII"))){
    ret->utf8 = false;
  }else{
    fprintf(stderr, "Encoding (\"%s\") was neither ANSI_X3.4-1968 nor UTF-8, refusing to start\n Did you call setlocale()?\n",
            encoding ? encoding : "none found");
    free(ret);
    return NULL;
  }
  if(outfp == NULL){
    outfp = stdout;
  }
  if(recursive_lock_init(&ret->pilelock)){
    fprintf(stderr, "Couldn't initialize pile mutex\n");
    free(ret);
    return NULL;
  }
  ret->margin_t = opts->margin_t;
  ret->margin_b = opts->margin_b;
  ret->margin_l = opts->margin_l;
  ret->margin_r = opts->margin_r;
  ret->cursory = ret->cursorx = -1;
  ret->stats.fbbytes = 0;
  ret->stashstats.fbbytes = 0;
  reset_stats(&ret->stats);
  reset_stats(&ret->stashstats);
  ret->ttyfp = outfp;
  ret->renderfp = opts->renderfp;
  memset(&ret->rstate, 0, sizeof(ret->rstate));
  memset(&ret->palette_damage, 0, sizeof(ret->palette_damage));
  memset(&ret->palette, 0, sizeof(ret->palette));
  ret->lastframe = NULL;
  ret->lfdimy = 0;
  ret->lfdimx = 0;
  ret->libsixel = false;
  egcpool_init(&ret->pool);
  if((ret->loglevel = opts->loglevel) > NCLOGLEVEL_TRACE || ret->loglevel < 0){
    fprintf(stderr, "Invalid loglevel %d\n", ret->loglevel);
    free(ret);
    return NULL;
  }
  ret->ttyfd = get_tty_fd(ret, ret->ttyfp);
  is_linux_console(ret, !!(opts->flags & NCOPTION_NO_FONT_CHANGES));
  if(ret->ttyfd >= 0){
    if(tcgetattr(ret->ttyfd, &ret->tpreserved)){
      fprintf(stderr, "Couldn't preserve terminal state for %d (%s)\n", ret->ttyfd, strerror(errno));
      free(ret);
      return NULL;
    }
    if(cbreak_mode(ret->ttyfd, &ret->tpreserved)){
      free(ret);
      return NULL;
    }
  }else{
    fprintf(stderr, "Defaulting to %dx%d (output is not to a terminal)\n", DEFAULT_ROWS, DEFAULT_COLS);
  }
  if(setup_signals(ret, (opts->flags & NCOPTION_NO_QUIT_SIGHANDLERS),
                   (opts->flags & NCOPTION_NO_WINCH_SIGHANDLER),
                   notcurses_stop_minimal)){
    // don't treat failure here as an error. it screws up unit tests, and one
    // day we'll need support multiple notcurses contexts. FIXME
  }
  int termerr;
  if(setupterm(opts->termtype, ret->ttyfd, &termerr) != OK){
    fprintf(stderr, "Terminfo error %d (see terminfo(3ncurses))\n", termerr);
    goto err;
  }
  int dimy, dimx;
  if(update_term_dimensions(ret->ttyfd, &dimy, &dimx)){
    goto err;
  }
  ret->suppress_banner = opts->flags & NCOPTION_SUPPRESS_BANNERS;
  char* shortname_term = termname();
  char* longname_term = longname();
  if(!ret->suppress_banner){
    fprintf(stderr, "Term: %dx%d %s (%s)\n", dimy, dimx,
            shortname_term ? shortname_term : "?",
            longname_term ? longname_term : "?");
  }
  if(interrogate_terminfo(&ret->tcache, shortname_term)){
    goto err;
  }
  warn_terminfo(ret, &ret->tcache);
  reset_term_attributes(ret);
  if(ncinputlayer_init(&ret->input, stdin)){
    goto err;
  }
  if(make_nonblocking(ret->input.ttyinfp)){
    goto err;
  }
  // Neither of these is supported on e.g. the "linux" virtual console.
  if(!(opts->flags & NCOPTION_NO_ALTERNATE_SCREEN)){
    terminfostr(&ret->tcache.smcup, "smcup");
    terminfostr(&ret->tcache.rmcup, "rmcup");
  }
  if(ncvisual_init(ffmpeg_log_level(ret->loglevel))){
    goto err;
  }
  ret->stdplane = NULL;
  if((ret->stdplane = create_initial_ncplane(ret, dimy, dimx)) == NULL){
    goto err;
  }
  if(ret->ttyfd >= 0){
    if(ret->tcache.smkx && tty_emit("smkx", ret->tcache.smkx, ret->ttyfd)){
      free_plane(ret->stdplane);
      goto err;
    }
    if(ret->tcache.civis && tty_emit("civis", ret->tcache.civis, ret->ttyfd)){
      free_plane(ret->stdplane);
      goto err;
    }
  }
  if((ret->rstate.mstreamfp = open_memstream(&ret->rstate.mstream, &ret->rstate.mstrsize)) == NULL){
    free_plane(ret->stdplane);
    goto err;
  }
  ret->rstate.x = ret->rstate.y = -1;
  init_banner(ret);
  // flush on the switch to alternate screen, lest initial output be swept away
  if(ret->ttyfd >= 0){
    if(ret->tcache.smcup){
      if(tty_emit("smcup", ret->tcache.smcup, ret->ttyfd)){
        free_plane(ret->stdplane);
        goto err;
      }
      // explicit clear even though smcup *might* clear
      if(tty_emit("clear", ret->tcache.clearscr, ret->ttyfd)){
        notcurses_refresh(ret, NULL, NULL);
      }
    }else if(!(opts->flags & NCOPTION_NO_ALTERNATE_SCREEN)){
      // if they expected the alternate screen, but we didn't have one to
      // offer, at least clear the screen. try using "clear"; if that doesn't
      // fly, use notcurses_refresh() to force a clearing via iterated writes.
      if(tty_emit("clear", ret->tcache.clearscr, ret->ttyfd)){
        notcurses_refresh(ret, NULL, NULL);
      }
    }
  }
  return ret;

err:
  fprintf(stderr, "Alas, you will not be going to space today.\n");
  // FIXME looks like we have some memory leaks on this error path?
  tcsetattr(ret->ttyfd, TCSANOW, &ret->tpreserved);
  drop_signals(ret);
  free(ret);
  return NULL;
}

// updates *pile to point at (*pile)->next, frees all but standard pile/plane
static void
ncpile_drop(notcurses* nc, ncpile** pile){
  bool sawstdplane = false;
  ncpile* next = (*pile)->next;
  ncplane* p = (*pile)->top;
  while(p){
    ncplane* tmp = p->below;
    if(nc->stdplane != p){
      free_plane(p);
    }else{
      sawstdplane = true;
    }
    p = tmp;
  }
  *pile = next;
  if(sawstdplane){
    ncplane_pile(nc->stdplane)->top = nc->stdplane;
    ncplane_pile(nc->stdplane)->bottom = nc->stdplane;
    nc->stdplane->above = nc->stdplane->below = NULL;
    nc->stdplane->blist = NULL;
  }
}

// drop all piles and all planes, save the standard plane and its pile
void notcurses_drop_planes(notcurses* nc){
  pthread_mutex_lock(&nc->pilelock);
  ncpile* p = ncplane_pile(nc->stdplane);
  ncpile* p0 = p;
  do{
    ncpile_drop(nc, &p);
  }while(p0 != p);
  pthread_mutex_unlock(&nc->pilelock);
}

int notcurses_stop(notcurses* nc){
  int ret = 0;
  if(nc){
    ret |= notcurses_stop_minimal(nc);
    if(nc->stdplane){
      notcurses_drop_planes(nc);
      free_plane(nc->stdplane);
    }
    if(nc->rstate.mstreamfp){
      fclose(nc->rstate.mstreamfp);
    }
    if(nc->ttyfd >= 0){
      ret |= close(nc->ttyfd);
    }
    egcpool_dump(&nc->pool);
    free(nc->lastframe);
    free(nc->rstate.mstream);
    input_free_esctrie(&nc->input.inputescapes);
    stash_stats(nc);
    if(!nc->suppress_banner){
      if(nc->stashstats.renders){
        char totalbuf[BPREFIXSTRLEN + 1];
        char minbuf[BPREFIXSTRLEN + 1];
        char maxbuf[BPREFIXSTRLEN + 1];
        char avgbuf[BPREFIXSTRLEN + 1];
        qprefix(nc->stashstats.render_ns, NANOSECS_IN_SEC, totalbuf, 0);
        qprefix(nc->stashstats.render_min_ns, NANOSECS_IN_SEC, minbuf, 0);
        qprefix(nc->stashstats.render_max_ns, NANOSECS_IN_SEC, maxbuf, 0);
        qprefix(nc->stashstats.render_ns / nc->stashstats.renders, NANOSECS_IN_SEC, avgbuf, 0);
        fprintf(stderr, "\n%ju render%s, %ss total (%ss min, %ss max, %ss avg)\n",
                nc->stashstats.renders, nc->stashstats.renders == 1 ? "" : "s",
                totalbuf, minbuf, maxbuf, avgbuf);
        qprefix(nc->stashstats.writeout_ns, NANOSECS_IN_SEC, totalbuf, 0);
        qprefix(nc->stashstats.writeout_min_ns, NANOSECS_IN_SEC, minbuf, 0);
        qprefix(nc->stashstats.writeout_max_ns, NANOSECS_IN_SEC, maxbuf, 0);
        qprefix(nc->stashstats.writeouts ? nc->stashstats.writeout_ns / nc->stashstats.writeouts : 0,
                NANOSECS_IN_SEC, avgbuf, 0);
        fprintf(stderr, "%ju write%s, %ss total (%ss min, %ss max, %ss avg)\n",
                nc->stashstats.writeouts, nc->stashstats.writeouts == 1 ? "" : "s",
                totalbuf, minbuf, maxbuf, avgbuf);
        bprefix(nc->stashstats.render_bytes, 1, totalbuf, 0),
        bprefix(nc->stashstats.render_min_bytes, 1, minbuf, 0),
        bprefix(nc->stashstats.render_max_bytes, 1, maxbuf, 0),
        fprintf(stderr, "%sB total (%sB min, %sB max, %.03gKiB avg)\n",
                totalbuf, minbuf, maxbuf,
                nc->stashstats.render_bytes / (double)nc->stashstats.renders / 1024);
      }
      if(nc->stashstats.renders || nc->stashstats.failed_renders){
        fprintf(stderr, "%ju failed render%s, %ju failed write%s, %ju refresh%s\n",
                nc->stashstats.failed_renders,
                nc->stashstats.failed_renders == 1 ? "" : "s",
                nc->stashstats.failed_writeouts,
                nc->stashstats.failed_writeouts == 1 ? "" : "s",
                nc->stashstats.refreshes,
                nc->stashstats.refreshes == 1 ? "" : "es");
        fprintf(stderr, "RGB emits:elides: def %ju:%ju fg %ju:%ju bg %ju:%ju\n",
                nc->stashstats.defaultemissions,
                nc->stashstats.defaultelisions,
                nc->stashstats.fgemissions,
                nc->stashstats.fgelisions,
                nc->stashstats.bgemissions,
                nc->stashstats.bgelisions);
        fprintf(stderr, "Cell emits:elides: %ju/%ju (%.2f%%) %.2f%% %.2f%% %.2f%%\n",
                nc->stashstats.cellemissions, nc->stashstats.cellelisions,
                (nc->stashstats.cellemissions + nc->stashstats.cellelisions) == 0 ? 0 :
                (nc->stashstats.cellelisions * 100.0) / (nc->stashstats.cellemissions + nc->stashstats.cellelisions),
                (nc->stashstats.defaultemissions + nc->stashstats.defaultelisions) == 0 ? 0 :
                (nc->stashstats.defaultelisions * 100.0) / (nc->stashstats.defaultemissions + nc->stashstats.defaultelisions),
                (nc->stashstats.fgemissions + nc->stashstats.fgelisions) == 0 ? 0 :
                (nc->stashstats.fgelisions * 100.0) / (nc->stashstats.fgemissions + nc->stashstats.fgelisions),
                (nc->stashstats.bgemissions + nc->stashstats.bgelisions) == 0 ? 0 :
                (nc->stashstats.bgelisions * 100.0) / (nc->stashstats.bgemissions + nc->stashstats.bgelisions));
      }
    }
    del_curterm(cur_term);
    ret |= pthread_mutex_destroy(&nc->pilelock);
    free(nc);
  }
  return ret;
}

uint64_t ncplane_channels(const ncplane* n){
  return n->channels;
}

uint16_t ncplane_styles(const ncplane* n){
  return n->stylemask;
}

void ncplane_set_channels(ncplane* n, uint64_t channels){
  n->channels = channels;
}

void ncplane_set_fg_default(ncplane* n){
  channels_set_fg_default(&n->channels);
}

uint64_t ncplane_set_fchannel(ncplane* n, uint32_t channel){
  return channels_set_fchannel(&n->channels, channel);
}

uint64_t ncplane_set_bchannel(ncplane* n, uint32_t channel){
  return channels_set_bchannel(&n->channels, channel);
}

void ncplane_set_bg_default(ncplane* n){
  channels_set_bg_default(&n->channels);
}

void ncplane_set_bg_rgb8_clipped(ncplane* n, int r, int g, int b){
  channels_set_bg_rgb8_clipped(&n->channels, r, g, b);
}

int ncplane_set_bg_rgb8(ncplane* n, int r, int g, int b){
  return channels_set_bg_rgb8(&n->channels, r, g, b);
}

void ncplane_set_fg_rgb8_clipped(ncplane* n, int r, int g, int b){
  channels_set_fg_rgb8_clipped(&n->channels, r, g, b);
}

int ncplane_set_fg_rgb8(ncplane* n, int r, int g, int b){
  return channels_set_fg_rgb8(&n->channels, r, g, b);
}

int ncplane_set_fg_rgb(ncplane* n, unsigned channel){
  return channels_set_fg_rgb(&n->channels, channel);
}

int ncplane_set_bg_rgb(ncplane* n, unsigned channel){
  return channels_set_bg_rgb(&n->channels, channel);
}

int ncplane_set_fg_alpha(ncplane* n, int alpha){
  return channels_set_fg_alpha(&n->channels, alpha);
}

int ncplane_set_bg_alpha(ncplane *n, int alpha){
  return channels_set_bg_alpha(&n->channels, alpha);
}

int ncplane_set_fg_palindex(ncplane* n, int idx){
  if(idx < 0 || idx >= NCPALETTESIZE){
    return -1;
  }
  n->channels |= CELL_FGDEFAULT_MASK;
  n->channels |= CELL_FG_PALETTE;
  channels_set_fg_alpha(&n->channels, CELL_ALPHA_OPAQUE);
  n->stylemask &= 0xffff00ff;
  n->stylemask |= (idx << 8u);
  return 0;
}

int ncplane_set_bg_palindex(ncplane* n, int idx){
  if(idx < 0 || idx >= NCPALETTESIZE){
    return -1;
  }
  n->channels |= CELL_BGDEFAULT_MASK;
  n->channels |= CELL_BG_PALETTE;
  channels_set_bg_alpha(&n->channels, CELL_ALPHA_OPAQUE);
  n->stylemask &= 0xffffff00;
  n->stylemask |= idx;
  return 0;
}

int ncplane_set_base_cell(ncplane* ncp, const nccell* c){
  if(cell_wide_right_p(c)){
    return -1;
  }
  return cell_duplicate(ncp, &ncp->basecell, c);
}

int ncplane_set_base(ncplane* ncp, const char* egc, uint32_t stylemask, uint64_t channels){
  return cell_prime(ncp, &ncp->basecell, egc, stylemask, channels);
}

int ncplane_base(ncplane* ncp, nccell* c){
  return cell_duplicate(ncp, c, &ncp->basecell);
}

const char* cell_extended_gcluster(const ncplane* n, const nccell* c){
  if(cell_simple_p(c)){
    return (const char*)&c->gcluster;
  }
  return egcpool_extended_gcluster(&n->pool, c);
}

// 'n' ends up above 'above'
int ncplane_move_above(ncplane* restrict n, ncplane* restrict above){
  if(n == above){
    return -1;
  }
  if(ncplane_pile(n) != ncplane_pile(above)){ // can't move among piles
    return -1;
  }
  if(n->below != above){
    // splice out 'n'
    if(n->below){
      n->below->above = n->above;
    }else{
      ncplane_pile(n)->bottom = n->above;
    }
    if(n->above){
      n->above->below = n->below;
    }else{
      ncplane_pile(n)->top = n->below;
    }
    if( (n->above = above->above) ){
      above->above->below = n;
    }else{
      ncplane_pile(n)->top = n;
    }
    above->above = n;
    n->below = above;
  }
  return 0;
}

// 'n' ends up below 'below'
int ncplane_move_below(ncplane* restrict n, ncplane* restrict below){
  if(n == below){
    return -1;
  }
  if(ncplane_pile(n) != ncplane_pile(below)){ // can't move among piles
    return -1;
  }
  if(n->above != below){
    if(n->below){
      n->below->above = n->above;
    }else{
      ncplane_pile(n)->bottom = n->above;
    }
    if(n->above){
      n->above->below = n->below;
    }else{
      ncplane_pile(n)->top = n->below;
    }
    if( (n->below = below->below) ){
      below->below->above = n;
    }else{
      ncplane_pile(n)->bottom = n;
    }
    below->below = n;
    n->above = below;
  }
  return 0;
}

void ncplane_move_top(ncplane* n){
  if(n->above){
    if( (n->above->below = n->below) ){
      n->below->above = n->above;
    }else{
      ncplane_pile(n)->bottom = n->above;
    }
    n->above = NULL;
    if( (n->below = ncplane_pile(n)->top) ){
      n->below->above = n;
    }
    ncplane_pile(n)->top = n;
  }
}

void ncplane_move_bottom(ncplane* n){
  if(n->below){
    if( (n->below->above = n->above) ){
      n->above->below = n->below;
    }else{
      ncplane_pile(n)->top = n->below;
    }
    n->below = NULL;
    if( (n->above = ncplane_pile(n)->bottom) ){
      n->above->below = n;
    }
    ncplane_pile(n)->bottom = n;
  }
}

void ncplane_cursor_yx(const ncplane* n, int* y, int* x){
  if(y){
    *y = n->y;
  }
  if(x){
    *x = n->x;
  }
}

static inline void
cell_obliterate(ncplane* n, nccell* c){
  cell_release(n, c);
  cell_init(c);
}

// increment y by 1 and rotate the framebuffer up one line. x moves to 0.
void scroll_down(ncplane* n){
  n->x = 0;
  if(n->y == n->leny - 1){
    n->logrow = (n->logrow + 1) % n->leny;
    nccell* row = n->fb + nfbcellidx(n, n->y, 0);
    for(int clearx = 0 ; clearx < n->lenx ; ++clearx){
      cell_release(n, &row[clearx]);
    }
    memset(row, 0, sizeof(*row) * n->lenx);
  }else{
    ++n->y;
  }
}

int cell_load(ncplane* n, nccell* c, const char* gcluster){
  return pool_load(&n->pool, c, gcluster);
}

// where the magic happens. write the single EGC completely described by |egc|,
// occupying |cols| columns, to the ncplane |n| at the coordinate |y|, |x|. if
// either or both of |y|/|x| is -1, the current cursor location for that
// dimension will be used. if the glyph cannot fit on the current line, it is
// an error unless scrolling is enabled.
static inline int
ncplane_put(ncplane* n, int y, int x, const char* egc, int cols,
            uint16_t stylemask, uint64_t channels, int bytes){
  // if scrolling is enabled, check *before ncplane_cursor_move_yx()* whether
  // we're past the end of the line, and move to the next line if so. if x or y
  // are specified, however, we must always try to print there.
  if(x != -1){
    if(x + cols > n->lenx){
      logerror(ncplane_notcurses(n), "Target x %d + %d cols >= length %d\n", x, cols, n->lenx);
      ncplane_cursor_move_yx(n, y, x); // update cursor, though
      return -1;
    }
  }else if(y == -1 && n->x + cols > n->lenx){
    if(!n->scrolling){
      logerror(ncplane_notcurses(n), "No room to output [%.*s] %d/%d\n", bytes, egc, n->y, n->x);
      return -1;
    }
    scroll_down(n);
  }
  if(ncplane_cursor_move_yx(n, y, x)){
    return -1;
  }
  // FIXME don't we need to check here for wide character on edge (though our
  // docs currently claim that a wide char on edge is allowed...)?
  if(*egc == '\n'){
    if(n->scrolling){
      scroll_down(n);
      return 0;
    }
    // FIXME isn't it an error if scrolling is disabled?
  }
  // A wide character obliterates anything to its immediate right (and marks
  // that cell as wide). Any character placed atop one half of a wide character
  // obliterates the other half. Note that a wide char can thus obliterate two
  // wide chars, totalling four columns.
  nccell* targ = ncplane_cell_ref_yx(n, n->y, n->x);
  if(n->x > 0){
    if(cell_double_wide_p(targ)){ // replaced cell is half of a wide char
      nccell* sacrifice = targ->gcluster == 0 ?
        // right half will never be on the first column of a row
        &n->fb[nfbcellidx(n, n->y, n->x - 1)] :
        // left half will never be on the last column of a row
        &n->fb[nfbcellidx(n, n->y, n->x + 1)];
      cell_obliterate(n, sacrifice);
    }
  }
  targ->stylemask = stylemask;
  targ->channels = channels;
  if(cell_load_direct(n, targ, egc, bytes, cols) < 0){
    return -1;
  }
//fprintf(stderr, "%08x %016lx %c %d %d\n", targ->gcluster, targ->channels, cell_double_wide_p(targ) ? 'D' : 'd', bytes, cols);
  // must set our right hand sides wide, and check for further damage
  ++n->x;
  for(int i = 1 ; i < cols ; ++i){
    nccell* candidate = &n->fb[nfbcellidx(n, n->y, n->x)];
    if(cell_wide_left_p(candidate)){
      cell_obliterate(n, &n->fb[nfbcellidx(n, n->y, n->x + 1)]);
    }
    cell_release(n, candidate);
    candidate->channels = targ->channels;
    candidate->stylemask = targ->stylemask;
    candidate->width = targ->width;
    ++n->x;
  }
  return cols;
}

int ncplane_putc_yx(ncplane* n, int y, int x, const nccell* c){
  const int cols = cell_double_wide_p(c) ? 2 : 1;
  const char* egc = cell_extended_gcluster(n, c);
  return ncplane_put(n, y, x, egc, cols, c->stylemask, c->channels, strlen(egc));
}

int ncplane_putegc_yx(ncplane* n, int y, int x, const char* gclust, int* sbytes){
  int cols;
  int bytes = utf8_egc_len(gclust, &cols);
  if(bytes < 0){
    return -1;
  }
  if(sbytes){
    *sbytes = bytes;
  }
//fprintf(stderr, "cols: %d wcs: %d\n", cols, bytes);
  return ncplane_put(n, y, x, gclust, cols, n->stylemask, n->channels, bytes);
}

int ncplane_putchar_stained(ncplane* n, char c){
  uint64_t channels = n->channels;
  uint32_t stylemask = n->stylemask;
  const nccell* targ = &n->fb[nfbcellidx(n, n->y, n->x)];
  n->channels = targ->channels;
  n->stylemask = targ->stylemask;
  int ret = ncplane_putchar(n, c);
  n->channels = channels;
  n->stylemask = stylemask;
  return ret;
}

int ncplane_putwegc_stained(ncplane* n, const wchar_t* gclust, int* sbytes){
  uint64_t channels = n->channels;
  uint32_t stylemask = n->stylemask;
  const nccell* targ = &n->fb[nfbcellidx(n, n->y, n->x)];
  n->channels = targ->channels;
  n->stylemask = targ->stylemask;
  int ret = ncplane_putwegc(n, gclust, sbytes);
  n->channels = channels;
  n->stylemask = stylemask;
  return ret;
}

int ncplane_putegc_stained(ncplane* n, const char* gclust, int* sbytes){
  uint64_t channels = n->channels;
  uint32_t stylemask = n->stylemask;
  const nccell* targ = &n->fb[nfbcellidx(n, n->y, n->x)];
  n->channels = targ->channels;
  n->stylemask = targ->stylemask;
  int ret = ncplane_putegc(n, gclust, sbytes);
  n->channels = channels;
  n->stylemask = stylemask;
  return ret;
}

int ncplane_cursor_at(const ncplane* n, nccell* c, char** gclust){
  if(n->y == n->leny && n->x == n->lenx){
    return -1;
  }
  const nccell* src = &n->fb[nfbcellidx(n, n->y, n->x)];
  memcpy(c, src, sizeof(*src));
  if(cell_simple_p(c)){
    *gclust = NULL;
  }else if((*gclust = strdup(cell_extended_gcluster(n, src))) == NULL){
    return -1;
  }
  return 0;
}

unsigned notcurses_supported_styles(const notcurses* nc){
  unsigned styles = 0;
  styles |= nc->tcache.standout ? NCSTYLE_STANDOUT : 0;
  styles |= nc->tcache.uline ? NCSTYLE_UNDERLINE : 0;
  styles |= nc->tcache.reverse ? NCSTYLE_REVERSE : 0;
  styles |= nc->tcache.blink ? NCSTYLE_BLINK : 0;
  styles |= nc->tcache.dim ? NCSTYLE_DIM : 0;
  styles |= nc->tcache.bold ? NCSTYLE_BOLD : 0;
  styles |= nc->tcache.italics ? NCSTYLE_ITALIC : 0;
  styles |= nc->tcache.struck ? NCSTYLE_STRUCK : 0;
  return styles;
}

unsigned notcurses_palette_size(const notcurses* nc){
  return nc->tcache.colors;
}

bool notcurses_cantruecolor(const notcurses* nc){
  return nc->tcache.RGBflag;
}

// conform to the specified stylebits
void ncplane_set_styles(ncplane* n, unsigned stylebits){
  n->stylemask = (stylebits & NCSTYLE_MASK);
}

void ncplane_styles_set(ncplane* n, unsigned stylebits){ // deprecated
  ncplane_set_styles(n, stylebits);
}

// turn on any specified stylebits
void ncplane_on_styles(ncplane* n, unsigned stylebits){
  n->stylemask |= (stylebits & NCSTYLE_MASK);
}

void ncplane_styles_on(ncplane* n, unsigned stylebits){ // deprecated
  ncplane_on_styles(n, stylebits);
}

// turn off any specified stylebits
void ncplane_off_styles(ncplane* n, unsigned stylebits){
  n->stylemask &= ~(stylebits & NCSTYLE_MASK);
}

void ncplane_styles_off(ncplane* n, unsigned stylebits){ // deprecated
  ncplane_off_styles(n, stylebits);
}

// i hate the big allocation and two copies here, but eh what you gonna do?
// well, for one, we don't need the huge allocation FIXME
char* ncplane_vprintf_prep(const char* format, va_list ap){
  const size_t size = BUFSIZ; // healthy estimate, can embiggen below
  char* buf = malloc(size);
  if(buf == NULL){
    return NULL;
  }
  va_list vacopy;
  va_copy(vacopy, ap);
  int ret = vsnprintf(buf, size, format, ap);
  if(ret < 0){
    free(buf);
    va_end(vacopy);
    return NULL;
  }
  if((size_t)ret >= size){
    char* tmp = realloc(buf, ret + 1);
    if(tmp == NULL){
      free(buf);
      va_end(vacopy);
      return NULL;
    }
    buf = tmp;
    vsprintf(buf, format, vacopy);
  }
  va_end(vacopy);
  return buf;
}

int ncplane_vprintf_yx(ncplane* n, int y, int x, const char* format, va_list ap){
  char* r = ncplane_vprintf_prep(format, ap);
  if(r == NULL){
    return -1;
  }
  int ret = ncplane_putstr_yx(n, y, x, r);
  free(r);
  return ret;
}

int ncplane_vprintf_aligned(ncplane* n, int y, ncalign_e align,
                            const char* format, va_list ap){
  char* r = ncplane_vprintf_prep(format, ap);
  if(r == NULL){
    return -1;
  }
  int ret = ncplane_putstr_aligned(n, y, align, r);
  free(r);
  return ret;
}

int ncplane_vprintf_stained(struct ncplane* n, const char* format, va_list ap){
  char* r = ncplane_vprintf_prep(format, ap);
  if(r == NULL){
    return -1;
  }
  int ret = ncplane_putstr_stained(n, r);
  free(r);
  return ret;
}

int ncplane_hline_interp(ncplane* n, const nccell* c, int len,
                         uint64_t c1, uint64_t c2){
  unsigned ur, ug, ub;
  int r1, g1, b1, r2, g2, b2;
  int br1, bg1, bb1, br2, bg2, bb2;
  channels_fg_rgb8(c1, &ur, &ug, &ub);
  r1 = ur; g1 = ug; b1 = ub;
  channels_fg_rgb8(c2, &ur, &ug, &ub);
  r2 = ur; g2 = ug; b2 = ub;
  channels_bg_rgb8(c1, &ur, &ug, &ub);
  br1 = ur; bg1 = ug; bb1 = ub;
  channels_bg_rgb8(c2, &ur, &ug, &ub);
  br2 = ur; bg2 = ug; bb2 = ub;
  int deltr = r2 - r1;
  int deltg = g2 - g1;
  int deltb = b2 - b1;
  int deltbr = br2 - br1;
  int deltbg = bg2 - bg1;
  int deltbb = bb2 - bb1;
  int ret;
  nccell dupc = CELL_TRIVIAL_INITIALIZER;
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
      cell_set_fg_rgb8(&dupc, r, g, b);
    }
    if(!bgdef){
      cell_set_bg_rgb8(&dupc, br, bg, bb);
    }
    if(ncplane_putc(n, &dupc) <= 0){
      break;
    }
  }
  cell_release(n, &dupc);
  return ret;
}

int ncplane_vline_interp(ncplane* n, const nccell* c, int len,
                         uint64_t c1, uint64_t c2){
  unsigned ur, ug, ub;
  int r1, g1, b1, r2, g2, b2;
  int br1, bg1, bb1, br2, bg2, bb2;
  channels_fg_rgb8(c1, &ur, &ug, &ub);
  r1 = ur; g1 = ug; b1 = ub;
  channels_fg_rgb8(c2, &ur, &ug, &ub);
  r2 = ur; g2 = ug; b2 = ub;
  channels_bg_rgb8(c1, &ur, &ug, &ub);
  br1 = ur; bg1 = ug; bb1 = ub;
  channels_bg_rgb8(c2, &ur, &ug, &ub);
  br2 = ur; bg2 = ug; bb2 = ub;
  int deltr = (r2 - r1) / (len + 1);
  int deltg = (g2 - g1) / (len + 1);
  int deltb = (b2 - b1) / (len + 1);
  int deltbr = (br2 - br1) / (len + 1);
  int deltbg = (bg2 - bg1) / (len + 1);
  int deltbb = (bb2 - bb1) / (len + 1);
  int ret, ypos, xpos;
  ncplane_cursor_yx(n, &ypos, &xpos);
  nccell dupc = CELL_TRIVIAL_INITIALIZER;
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
      cell_set_fg_rgb8(&dupc, r1, g1, b1);
    }
    if(!bgdef){
      cell_set_bg_rgb8(&dupc, br1, bg1, bb1);
    }
    if(ncplane_putc(n, &dupc) <= 0){
      break;
    }
  }
  cell_release(n, &dupc);
  return ret;
}

int ncplane_box(ncplane* n, const nccell* ul, const nccell* ur,
                const nccell* ll, const nccell* lr, const nccell* hl,
                const nccell* vl, int ystop, int xstop,
                unsigned ctlword){
  int yoff, xoff, ymax, xmax;
  ncplane_cursor_yx(n, &yoff, &xoff);
  // must be at least 2x2, with its upper-left corner at the current cursor
  if(ystop < yoff + 1){
    logerror(ncplane_notcurses(n), "ystop (%d) insufficient for yoff (%d)\n", ystop, yoff);
    return -1;
  }
  if(xstop < xoff + 1){
    logerror(ncplane_notcurses(n), "xstop (%d) insufficient for xoff (%d)\n", xstop, xoff);
    return -1;
  }
  ncplane_dim_yx(n, &ymax, &xmax);
  // must be within the ncplane
  if(xstop >= xmax || ystop >= ymax){
    logerror(ncplane_notcurses(n), "Boundary (%dx%d) beyond plane (%dx%d)\n", ystop, xstop, ymax, xmax);
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

// takes the head of a list of bound planes. performs a DFS on all planes bound
// to 'n', and all planes down-list from 'n', moving all *by* 'dy' and 'dx'.
static void
move_bound_planes(ncplane* n, int dy, int dx){
  while(n){
    n->absy += dy;
    n->absx += dx;
    move_bound_planes(n->blist, dy, dx);
    n = n->bnext;
  }
}

int ncplane_move_yx(ncplane* n, int y, int x){
  if(n == ncplane_notcurses(n)->stdplane){
    return -1;
  }
  int dy, dx; // amount moved
  dy = (n->boundto->absy + y) - n->absy;
  dx = (n->boundto->absx + x) - n->absx;
  n->absx += dx;
  n->absy += dy;
  move_bound_planes(n->blist, dy, dx);
  return 0;
}

int ncplane_y(const ncplane* n){
  if(n->boundto == n){
    return n->absy - ncplane_notcurses_const(n)->margin_t;
  }
  return n->absy - n->boundto->absy;
}

int ncplane_x(const ncplane* n){
  if(n->boundto == n){
    return n->absx - ncplane_notcurses_const(n)->margin_t;
  }
  return n->absx - n->boundto->absx;
}

void ncplane_yx(const ncplane* n, int* y, int* x){
  if(y){
    *y = ncplane_y(n);
  }
  if(x){
    *x = ncplane_x(n);
  }
}

void ncplane_erase(ncplane* n){
  // we must preserve the background, but a pure cell_duplicate() would be
  // wiped out by the egcpool_dump(). do a duplication (to get the stylemask
  // and channels), and then reload.
  char* egc = cell_strdup(n, &n->basecell);
  memset(n->fb, 0, sizeof(*n->fb) * n->lenx * n->leny);
  egcpool_dump(&n->pool);
  egcpool_init(&n->pool);
  // we need to zero out the EGC before handing this off to cell_load, but
  // we don't want to lose the channels/attributes, so explicit gcluster load.
  n->basecell.gcluster = 0;
  cell_load(n, &n->basecell, egc);
  free(egc);
  n->y = n->x = 0;
}

ncplane* notcurses_top(notcurses* n){
  return ncplane_pile(n->stdplane)->top;
}

ncplane* notcurses_bottom(notcurses* n){
  return ncplane_pile(n->stdplane)->bottom;
}

ncplane* ncpile_top(ncplane* n){
  return ncplane_pile(n)->top;
}

ncplane* ncpile_bottom(ncplane* n){
  return ncplane_pile(n)->bottom;
}

ncplane* ncplane_below(ncplane* n){
  return n->below;
}

ncplane* ncplane_above(ncplane* n){
  return n->above;
}

#define SET_BTN_EVENT_MOUSE   "1002"
#define SET_FOCUS_EVENT_MOUSE "1004"
#define SET_SGR_MODE_MOUSE    "1006"
int notcurses_mouse_enable(notcurses* n){
  if(n->ttyfd >= 0){
    return tty_emit("mouse", ESC "[?" SET_BTN_EVENT_MOUSE ";"
                    /*SET_FOCUS_EVENT_MOUSE ";" */SET_SGR_MODE_MOUSE "h",
                    n->ttyfd);
  }
  return 0;
}

// this seems to work (note difference in suffix, 'l' vs 'h'), but what about
// the sequences 1000 etc?
int notcurses_mouse_disable(notcurses* n){
  if(n->ttyfd >= 0){
    return tty_emit("mouse", ESC "[?" SET_BTN_EVENT_MOUSE ";"
                    /*SET_FOCUS_EVENT_MOUSE ";" */SET_SGR_MODE_MOUSE "l",
                    n->ttyfd);
  }
  return 0;
}

bool notcurses_canutf8(const notcurses* nc){
  return nc->utf8;
}

bool notcurses_cansextant(const notcurses* nc){
  return nc->tcache.sextants && nc->utf8;
}

bool notcurses_canfade(const notcurses* nc){
  return nc->tcache.CCCflag || nc->tcache.RGBflag;
}

bool notcurses_canchangecolor(const notcurses* nc){
  if(!nc->tcache.CCCflag){
    return false;
  }
  palette256* p;
  if((unsigned)nc->tcache.colors < sizeof(p->chans) / sizeof(*p->chans)){
    return false;
  }
  return true;
}

bool notcurses_cansixel(const notcurses* nc){
  return nc->libsixel;
}

palette256* palette256_new(notcurses* nc){
  palette256* p = malloc(sizeof(*p));
  if(p){
    memcpy(p, &nc->palette, sizeof(*p));
  }
  return p;
}

int palette256_use(notcurses* nc, const palette256* p){
  int ret = -1;
  if(!notcurses_canchangecolor(nc)){
    return -1;
  }
  for(size_t z = 0 ; z < sizeof(p->chans) / sizeof(*p->chans) ; ++z){
    if(nc->palette.chans[z] != p->chans[z]){
      nc->palette.chans[z] = p->chans[z];
      nc->palette_damage[z] = true;
    }
  }
  ret = 0;
  return ret;
}

void palette256_free(palette256* p){
  free(p);
}

bool ncplane_translate_abs(const ncplane* n, int* restrict y, int* restrict x){
  ncplane_translate(ncplane_stdplane_const(n), n, y, x);
  if(y){
    if(*y < 0){
      return false;
    }
    if(*y >= n->leny){
      return false;
    }
  }
  if(x){
    if(*x < 0){
      return false;
    }
    if(*x >= n->lenx){
      return false;
    }
  }
  return true;
}

void ncplane_translate(const ncplane* src, const ncplane* dst,
                       int* restrict y, int* restrict x){
  if(dst == NULL){
    dst = ncplane_stdplane_const(src);
  }
  if(y){
    *y = src->absy - dst->absy + *y;
  }
  if(x){
    *x = src->absx - dst->absx + *x;
  }
}

notcurses* ncplane_notcurses(ncplane* n){
  return ncplane_pile(n)->nc;
}

const notcurses* ncplane_notcurses_const(const ncplane* n){
  return ncplane_pile_const(n)->nc;
}

ncplane* ncplane_parent(ncplane* n){
  return n->boundto;
}

const ncplane* ncplane_parent_const(const ncplane* n){
  return n->boundto;
}

void ncplane_set_resizecb(ncplane* n, int(*resizecb)(ncplane*)){
  if(n == notcurses_stdplane(ncplane_notcurses(n))){
    return;
  }
  n->resizecb = resizecb;
}

int (*ncplane_resizecb(const ncplane* n))(ncplane*){
  return n->resizecb;
}

int ncplane_resize_maximize(ncplane* n){
  const ncpile* pile = ncplane_pile(n);
  const int rows = pile->dimy;
  const int cols = pile->dimx;
  int oldy, oldx;
  ncplane_dim_yx(n, &oldy, &oldx); // current dimensions of 'n'
//fprintf(stderr, "CURRENT: %d/%d TERM: %d/%d\n", oldy, oldx, rows, cols);
  int keepleny = oldy > rows ? rows : oldy;
  int keeplenx = oldx > cols ? cols : oldx;
  return ncplane_resize_internal(n, 0, 0, keepleny, keeplenx, 0, 0, rows, cols);
}

int ncplane_resize_realign(ncplane* n){
  const ncplane* parent = ncplane_parent_const(n);
  if(parent == n){ // somehow got stdplane, should never get here
    logerror(ncplane_notcurses(n), "Passed the standard plane");
    return -1;
  }
  if(n->align == NCALIGN_UNALIGNED){
    logerror(ncplane_notcurses(n), "Passed a non-aligned plane");
    return -1;
  }
  int xpos = ncplane_align(parent, n->align, ncplane_dim_x(n));
  return ncplane_move_yx(n, ncplane_y(n), xpos);
}

// The standard plane cannot be reparented; we return NULL in that case.
// If provided |newparent|==|n|, we are moving |n| to its own stack. If |n|
// is already bound to |newparent|, this is a no-op, and we return |n|.
// This is essentially a wrapper around ncplane_reparent_family() that first
// reparents any children to the parent of 'n', or makes them root planes if
// 'n' is a root plane.
ncplane* ncplane_reparent(ncplane* n, ncplane* newparent){
  if(n == ncplane_notcurses(n)->stdplane){
    return NULL; // can't reparent standard plane
  }
  if(n->boundto == newparent){
    return n;
  }
  if(n->blist){
    if(n->boundto == n){ // children become new root planes
      ncplane* lastlink;
      for(ncplane* child = n->blist ; child ; child = child->bnext){
        child->boundto = child;
        lastlink = child;
      } // n->blist != NULL -> lastlink != NULL
      if( (lastlink->bnext = ncplane_pile(n)->roots) ){
        lastlink->bnext->bprev = &lastlink->bnext;
      }
      n->blist->bprev = &ncplane_pile(n)->roots;
      ncplane_pile(n)->roots = n->blist;
    }else{ // children are rebound to current parent
      ncplane* lastlink;
      for(ncplane* child = n->blist ; child ; child = child->bnext){
        child->boundto = n->boundto;
        lastlink = child;
      } // n->blist != NULL -> lastlink != NULL
      if( (lastlink->bnext = n->boundto->blist) ){
        lastlink->bnext->bprev = &lastlink->bnext;
      }
      n->blist->bprev = &n->boundto->blist;
      n->boundto->blist = n->blist;
    }
    n->blist = NULL;
  }
//notcurses_debug(ncplane_notcurses(n), stderr);
  return ncplane_reparent_family(n, newparent);
}

// unsplice self from the z-axis, and then unsplice all children, recursively.
// to be called before unbinding 'n' from old pile.
static void
unsplice_zaxis_recursive(ncplane* n){
  if(ncplane_pile(n)->top == n){
    ncplane_pile(n)->top = n->below;
  }else{
    n->above->below = n->below;
  }
  if(ncplane_pile(n)->bottom == n){
    ncplane_pile(n)->bottom = n->above;
  }else{
    n->below->above = n->above;
  }
  for(ncplane* child = n->blist ; child ; child = child->bnext){
    unsplice_zaxis_recursive(child);
  }
}

// recursively splice 'n' and children into the z-axis, above 'n->boundto'.
// handles 'n' == 'n->boundto'. to be called after binding 'n' into new pile.
static void
splice_zaxis_recursive(ncplane* n){
  if(n != n->boundto){
    if((n->above = n->boundto->above) == NULL){
      n->pile->top = n;
    }else{
      n->boundto->above->below = n;
    }
    n->below = n->boundto;
    n->boundto->above = n;
  }
  for(ncplane* child = n->blist ; child ; child = child->bnext){
    splice_zaxis_recursive(child);
  }
}

ncplane* ncplane_reparent_family(ncplane* n, ncplane* newparent){
  if(n == ncplane_notcurses(n)->stdplane){
    return NULL; // can't reparent standard plane
  }
  if(ncplane_descendant_p(newparent, n)){
    return NULL;
  }
  if(n->boundto == newparent){ // no-op
    return n;
  }
  // if we are not a root plane, adjust our origin
  if(n->boundto != n){
    n->absx -= n->boundto->absx;
    n->absy -= n->boundto->absy;
  }
  if(n->bprev){ // extract from sibling list
    if( (*n->bprev = n->bnext) ){
      n->bnext->bprev = n->bprev;
    }
  }
  // if leaving a pile, extract n from the old zaxis
  if(n == newparent || ncplane_pile(n) != ncplane_pile(newparent)){
    unsplice_zaxis_recursive(n);
  }
  n->boundto = newparent;
  if(n == n->boundto){ // we're a new root plane
    n->bnext = NULL;
    n->bprev = NULL;
    pthread_mutex_lock(&ncplane_notcurses(n)->pilelock);
    if(ncplane_pile(n)->top == NULL){ // did we just empty our pile?
      ncpile_destroy(ncplane_pile(n));
    }
    make_ncpile(ncplane_notcurses(n), n);
    pthread_mutex_unlock(&ncplane_notcurses(n)->pilelock);
    splice_zaxis_recursive(n);
  }else{ // establish ourselves as a sibling of new parent's children
    n->absx += n->boundto->absx;
    n->absy += n->boundto->absy;
    if( (n->bnext = newparent->blist) ){
      n->bnext->bprev = &n->bnext;
    }
    n->bprev = &newparent->blist;
    newparent->blist = n;
    // place it immediately above the new binding plane if crossing piles
    if(n->pile != ncplane_pile(n->boundto)){
      n->pile = ncplane_pile(n->boundto);
      splice_zaxis_recursive(n);
    }
  }
  return n;
}

bool ncplane_set_scrolling(ncplane* n, bool scrollp){
  bool old = n->scrolling;
  n->scrolling = scrollp;
  return old;
}

// extract an integer, which must be non-negative, and followed by either a
// comma or a NUL terminator.
static int
lex_long(const char* op, int* i, char** endptr){
  errno = 0;
  long l = strtol(op, endptr, 10);
  if(l < 0 || (l == LONG_MAX && errno == ERANGE) || (l > INT_MAX)){
    fprintf(stderr, "Invalid margin: %s\n", op);
    return -1;
  }
  if((**endptr != ',' && **endptr) || *endptr == op){
    fprintf(stderr, "Invalid margin: %s\n", op);
    return -1;
  }
  *i = l;
  return 0;
}

int notcurses_lex_scalemode(const char* op, ncscale_e* scalemode){
  if(strcasecmp(op, "stretch") == 0){
    *scalemode = NCSCALE_STRETCH;
  }else if(strcasecmp(op, "scalehi") == 0){
    *scalemode = NCSCALE_SCALE_HIRES;
  }else if(strcasecmp(op, "hires") == 0){
    *scalemode = NCSCALE_NONE_HIRES;
  }else if(strcasecmp(op, "scale") == 0){
    *scalemode = NCSCALE_SCALE;
  }else if(strcasecmp(op, "none") == 0){
    *scalemode = NCSCALE_NONE;
  }else{
    return -1;
  }
  return 0;
}

const char* notcurses_str_scalemode(ncscale_e scalemode){
  if(scalemode == NCSCALE_STRETCH){
    return "stretch";
  }else if(scalemode == NCSCALE_SCALE){
    return "scale";
  }else if(scalemode == NCSCALE_NONE){
    return "none";
  }else if(scalemode == NCSCALE_NONE_HIRES){
    return "hires";
  }else if(scalemode == NCSCALE_SCALE_HIRES){
    return "scalehi";
  }
  return NULL;
}

int notcurses_lex_margins(const char* op, notcurses_options* opts){
  char* eptr;
  if(lex_long(op, &opts->margin_t, &eptr)){
    return -1;
  }
  if(!*eptr){ // allow a single value to be specified for all four margins
    opts->margin_r = opts->margin_l = opts->margin_b = opts->margin_t;
    return 0;
  }
  op = ++eptr; // once here, we require four values
  if(lex_long(op, &opts->margin_r, &eptr) || !*eptr){
    return -1;
  }
  op = ++eptr;
  if(lex_long(op, &opts->margin_b, &eptr) || !*eptr){
    return -1;
  }
  op = ++eptr;
  if(lex_long(op, &opts->margin_l, &eptr) || *eptr){ // must end in NUL
    return -1;
  }
  return 0;
}

int notcurses_inputready_fd(notcurses* n){
  return fileno(n->input.ttyinfp);
}

int ncdirect_inputready_fd(ncdirect* n){
  return fileno(n->input.ttyinfp);
}

uint32_t* ncplane_rgba(const ncplane* nc, ncblitter_e blit,
                       int begy, int begx, int leny, int lenx){
  if(begy < 0 || begx < 0){
    return NULL;
  }
  if(begx >= nc->lenx || begy >= nc->leny){
    return NULL;
  }
  if(lenx == -1){ // -1 means "to the end"; use all space available
    lenx = nc->lenx - begx;
  }
  if(leny == -1){
    leny = nc->leny - begy;
  }
  if(lenx < 0 || leny < 0){ // no need to draw zero-size object, exit
    return NULL;
  }
//fprintf(stderr, "sum: %d/%d avail: %d/%d\n", begy + leny, begx + lenx, nc->leny, nc->lenx);
  if(begx + lenx > nc->lenx || begy + leny > nc->leny){
    return NULL;
  }
//fprintf(stderr, "ALLOCATING %zu\n", 4u * lenx * leny * 2);
  uint32_t* ret = malloc(sizeof(*ret) * lenx * leny * 2);
  if(ret){
    for(int y = begy, targy = 0 ; y < begy + leny ; ++y, targy += 2){
      for(int x = begx, targx = 0 ; x < begx + lenx ; ++x, ++targx){
        // FIXME what if there's a wide glyph to the left of the selection?
        uint16_t stylemask;
        uint64_t channels;
        char* c = ncplane_at_yx(nc, y, x, &stylemask, &channels);
        if(c == NULL){
          free(ret);
          return NULL;
        }
        uint32_t* top = &ret[targy * lenx + targx];
        uint32_t* bot = &ret[(targy + 1) * lenx + targx];
        unsigned fr, fg, fb, br, bg, bb;
        channels_fg_rgb8(channels, &fr, &fb, &fg);
        channels_bg_rgb8(channels, &br, &bb, &bg);
        // FIXME how do we deal with transparency?
        uint32_t frgba = (fr) + (fg << 16u) + (fb << 8u) + 0xff000000;
        uint32_t brgba = (br) + (bg << 16u) + (bb << 8u) + 0xff000000;
        // FIXME integrate 'blit'
        (void)blit;
        // FIXME need to be able to pick up quadrants!
        if((strcmp(c, " ") == 0) || (strcmp(c, "") == 0)){
          *top = *bot = brgba;
        }else if(strcmp(c, "▄") == 0){
          *top = frgba;
          *bot = brgba;
        }else if(strcmp(c, "▀") == 0){
          *top = brgba;
          *bot = frgba;
        }else if(strcmp(c, "█") == 0){
          *top = *bot = frgba;
        }else{
          free(c);
          free(ret);
//fprintf(stderr, "bad rgba character: %s\n", c);
          return NULL;
        }
        free(c);
      }
    }
  }
  return ret;
}

// return a heap-allocated copy of the contents
char* ncplane_contents(const ncplane* nc, int begy, int begx, int leny, int lenx){
  if(begy < 0 || begx < 0){
    logerror(ncplane_notcurses_const(nc), "Beginning coordinates (%d/%d) below 0\n", begy, begx);
    return NULL;
  }
  if(begx >= nc->lenx || begy >= nc->leny){
    logerror(ncplane_notcurses_const(nc), "Beginning coordinates (%d/%d) exceeded lengths (%d/%d)\n",
             begy, begx, nc->leny, nc->lenx);
    return NULL;
  }
  if(lenx == -1){ // -1 means "to the end"; use all space available
    lenx = nc->lenx - begx;
  }
  if(leny == -1){
    leny = nc->leny - begy;
  }
  if(lenx < 0 || leny < 0){ // no need to draw zero-size object, exit
    logerror(ncplane_notcurses_const(nc), "Lengths (%d/%d) below 0\n", leny, lenx);
    return NULL;
  }
  if(begx + lenx > nc->lenx || begy + leny > nc->leny){
    logerror(ncplane_notcurses_const(nc), "Ending coordinates (%d/%d) exceeded lengths (%d/%d)\n",
             begy + leny, begx + lenx, nc->leny, nc->lenx);
    return NULL;
  }
  size_t retlen = 1;
  char* ret = malloc(retlen);
  if(ret){
    for(int y = begy, targy = 0 ; y < begy + leny ; ++y, targy += 2){
      for(int x = begx, targx = 0 ; x < begx + lenx ; ++x, ++targx){
        uint16_t stylemask;
        uint64_t channels;
        char* c = ncplane_at_yx(nc, y, x, &stylemask, &channels);
        if(!c){
          free(ret);
          return NULL;
        }
        size_t clen = strlen(c);
        if(clen){
          char* tmp = realloc(ret, retlen + clen);
          if(!tmp){
            free(c);
            free(ret);
            return NULL;
          }
          ret = tmp;
          memcpy(ret + retlen - 1, c, clen);
          retlen += clen;
        }
        free(c);
      }
    }
    ret[retlen - 1] = '\0';
  }
  return ret;
}

int cells_ascii_box(struct ncplane* n, uint32_t attr, uint64_t channels,
                    nccell* ul, nccell* ur, nccell* ll, nccell* lr, nccell* hl, nccell* vl){
  return cells_load_box(n, attr, channels, ul, ur, ll, lr, hl, vl, "/\\\\/-|");
}

int cells_double_box(struct ncplane* n, uint32_t attr, uint64_t channels,
                     nccell* ul, nccell* ur, nccell* ll, nccell* lr, nccell* hl, nccell* vl){
  if(notcurses_canutf8(ncplane_notcurses(n))){
    return cells_load_box(n, attr, channels, ul, ur, ll, lr, hl, vl, "╔╗╚╝═║");
  }
  return cells_ascii_box(n, attr, channels, ul, ur, ll, lr, hl, vl);
}

int cells_rounded_box(struct ncplane* n, uint32_t attr, uint64_t channels,
                      nccell* ul, nccell* ur, nccell* ll, nccell* lr, nccell* hl, nccell* vl){
  if(notcurses_canutf8(ncplane_notcurses(n))){
    return cells_load_box(n, attr, channels, ul, ur, ll, lr, hl, vl, "╭╮╰╯─│");
  }
  return cells_ascii_box(n, attr, channels, ul, ur, ll, lr, hl, vl);
}

// find the center coordinate of a plane, preferring the top/left in the
// case of an even number of rows/columns (in such a case, there will be one
// more cell to the bottom/right of the center than the top/left). the
// center is then modified relative to the plane's origin.
void ncplane_center_abs(const ncplane* n, int* RESTRICT y, int* RESTRICT x){
  ncplane_center(n, y, x);
  if(y){
    *y += n->absy;
  }
  if(x){
    *x += n->absx;
  }
}

void nclog(const char* fmt, ...){
  va_list va;
  va_start(va, fmt);
  vfprintf(stderr, fmt, va);
  va_end(va);
}

int ncplane_putstr_yx(struct ncplane* n, int y, int x, const char* gclusters){
  int ret = 0;
  // FIXME speed up this blissfully naive solution
  while(*gclusters){
    int wcs;
    int cols = ncplane_putegc_yx(n, y, x, gclusters, &wcs);
//fprintf(stderr, "wrote %.*s %d cols %d bytes now at %d/%d\n", wcs, gclusters, cols, wcs, n->y, n->x);
    if(cols < 0){
      return -ret;
    }
    if(wcs == 0){
      break;
    }
    // after the first iteration, just let the cursor code control where we
    // print, so that scrolling is taken into account
    y = -1;
    x = -1;
    gclusters += wcs;
    ret += cols;
  }
  return ret;
}

int ncplane_putstr_stained(struct ncplane* n, const char* gclusters){
  int ret = 0;
  // FIXME speed up this blissfully naive solution
  while(*gclusters){
    int wcs;
    int cols = ncplane_putegc_stained(n, gclusters, &wcs);
    if(cols < 0){
      return -ret;
    }
    if(wcs == 0){
      break;
    }
    gclusters += wcs;
    ret += cols;
  }
  return ret;
}

int ncplane_putwstr_stained(ncplane* n, const wchar_t* gclustarr){
  // maximum of six UTF8-encoded bytes per wchar_t
  const size_t mbytes = (wcslen(gclustarr) * WCHAR_MAX_UTF8BYTES) + 1;
  char* mbstr = malloc(mbytes); // need cast for c++ callers
  if(mbstr == NULL){
    return -1;
  }
  size_t s = wcstombs(mbstr, gclustarr, mbytes);
  if(s == (size_t)-1){
    free(mbstr);
    return -1;
  }
  int r = ncplane_putstr_stained(n, mbstr);
  free(mbstr);
  return r;
}

int ncplane_putnstr_aligned(struct ncplane* n, int y, ncalign_e align, size_t s, const char* str){
  char* chopped = strndup(str, s);
  int ret = ncplane_putstr_aligned(n, y, align, chopped);
  free(chopped);
  return ret;
}

int ncplane_putnstr_yx(struct ncplane* n, int y, int x, size_t s, const char* gclusters){
  int ret = 0;
  int offset = 0;
//fprintf(stderr, "PUT %zu at %d/%d [%.*s]\n", s, y, x, (int)s, gclusters);
  // FIXME speed up this blissfully naive solution
  while((size_t)offset < s && gclusters[offset]){
    int wcs;
    int cols = ncplane_putegc_yx(n, y, x, gclusters + offset, &wcs);
    if(cols < 0){
      return -ret;
    }
    if(wcs == 0){
      break;
    }
    // after the first iteration, just let the cursor code control where we
    // print, so that scrolling is taken into account
    y = -1;
    x = -1;
    offset += wcs;
    ret += cols;
  }
  return ret;
}

int notcurses_ucs32_to_utf8(const char32_t* ucs32, unsigned ucs32count,
                            unsigned char* resultbuf, size_t buflen){
  if(u32_to_u8(ucs32, ucs32count, resultbuf, &buflen) == NULL){
    return -1;
  }
  return buflen;
}

int ncstrwidth(const char* mbs){
  int cols = 0;   // number of columns consumed thus far
  do{
    int thesecols, thesebytes;
    thesebytes = utf8_egc_len(mbs, &thesecols);
    if(thesebytes < 0){
      return -1;
    }
    mbs += thesebytes;
    cols += thesecols;
  }while(*mbs);
  return cols;
}
