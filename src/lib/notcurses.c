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
int reset_term_attributes(const tinfo* ti, FILE* fp){
  int ret = 0;
  const char* esc;
  if((esc = get_escape(ti, ESCAPE_OP)) && term_emit(esc, fp, true)){
    ret = -1;
  }
  if((esc = get_escape(ti, ESCAPE_SGR0)) && term_emit(esc, fp, true)){
    ret = -1;
  }
  if((esc = get_escape(ti, ESCAPE_OC)) && term_emit(esc, fp, true)){
    ret = -1;
  }
  return ret;
}

// Do the minimum necessary stuff to restore the terminal, then return. This is
// the end of the line for fatal signal handlers. notcurses_stop() will go on
// to tear down and account for internal structures.
static int
notcurses_stop_minimal(void* vnc){
  notcurses* nc = vnc;
  int ret = 0;
  // see notcurses_core_init()--don't treat failure here as an error. it
  // screws up unit tests, and one day we'll need support multiple notcurses
  // contexts. FIXME
  drop_signals(nc);
  // be sure to write the restoration sequences *prior* to running rmcup, as
  // they apply to the screen (alternate or otherwise) we're actually using.
  if(nc->ttyfd >= 0){
    // ECMA-48 suggests that we can interrupt an escape code with a NUL
    // byte. if we leave an active escape open, it can lock up the terminal.
    // we only want to do it when in the middle of a rasterization, though. FIXME
    if(nc->tcache.pixel_shutdown){
      ret |= nc->tcache.pixel_shutdown(nc->ttyfd);
    }
    ret |= reset_term_attributes(&nc->tcache, nc->ttyfp);
    ret |= notcurses_mouse_disable(nc);
    const char* esc;
    if((esc = get_escape(&nc->tcache, ESCAPE_RMCUP)) && tty_emit(esc, nc->ttyfd)){
      ret = -1;
    }
    if((esc = get_escape(&nc->tcache, ESCAPE_RMKX)) && tty_emit(esc, nc->ttyfd)){
      ret = -1;
    }
    const char* cnorm = get_escape(&nc->tcache, ESCAPE_CNORM);
    if(cnorm && tty_emit(cnorm, nc->ttyfd)){
      ret = -1;
    }
    ret |= tcsetattr(nc->ttyfd, TCSANOW, &nc->tcache.tpreserved);
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
      const cell* yx = &n->fb[nfbcellidx(n, y, x)];
      // if we're the right side of a wide glyph, we return the main glyph
      if(nccell_wide_right_p(yx)){
        return ncplane_at_yx(n, y, x - 1, stylemask, channels);
      }
      char* ret = nccell_extract(n, yx, stylemask, channels);
      if(ret == NULL){
        return NULL;
      }
//fprintf(stderr, "GOT [%s]\n", ret);
      if(strcmp(ret, "") == 0){
        free(ret);
        ret = nccell_strdup(n, &n->basecell);
        if(ret == NULL){
          return NULL;
        }
        if(stylemask){
          *stylemask = n->basecell.stylemask;
        }
      }
      // FIXME load basecell channels if appropriate
      return ret;
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
      if(nccell_duplicate(n, c, targ) == 0){
        // FIXME take base cell into account where necessary!
        return strlen(nccell_extended_gcluster(n, targ));
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
int update_term_dimensions(int fd, int* rows, int* cols, tinfo* tcache,
                           int margin_b){
  // if we're not a real tty, we presumably haven't changed geometry, return
  if(fd < 0){
    if(rows){
      *rows = DEFAULT_ROWS;
    }
    if(cols){
      *cols = DEFAULT_COLS;
    }
    if(tcache){
      tcache->cellpixy = 0;
      tcache->cellpixx = 0;
    }
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
  int rowsafe;
  if(rows == NULL){
    rows = &rowsafe;
  }
  *rows = ws.ws_row;
  if(cols){
    *cols = ws.ws_col;
  }
  if(tcache){
    tcache->cellpixy = ws.ws_row ? ws.ws_ypixel / ws.ws_row : 0;
    tcache->cellpixx = ws.ws_col ? ws.ws_xpixel / ws.ws_col : 0;
    if(tcache->cellpixy == 0 || tcache->cellpixx == 0){
      tcache->pixel_draw = NULL; // disable support
    }
  }
  if(tcache->sixel_maxy_pristine){
    tcache->sixel_maxy = tcache->sixel_maxy_pristine;
    int sixelrows = *rows - 1;
    // if the bottom margin is at least one row, we can draw into the last
    // row of our visible area. we must leave the true bottom row alone.
    if(margin_b){
      ++sixelrows;
    }
    tcache->sixel_maxy = sixelrows * tcache->cellpixy;
  }
  return 0;
}

// destroy the sprixels of an ncpile (this will not hide the sprixels)
static void
free_sprixels(ncpile* n){
  while(n->sprixelcache){
    sprixel* tmp = n->sprixelcache->next;
    sprixel_free(n->sprixelcache);
    n->sprixelcache = tmp;
  }
}

// destroy an empty ncpile. only call with pilelock held.
static void
ncpile_destroy(ncpile* pile){
  if(pile){
    pile->prev->next = pile->next;
    pile->next->prev = pile->prev;
    free_sprixels(pile);
    free(pile->crender);
    free(pile);
  }
}

void free_plane(ncplane* p){
  if(p){
    // ncdirect fakes an ncplane with no ->pile
    if(ncplane_pile(p)){
      notcurses* nc = ncplane_notcurses(p);
      pthread_mutex_lock(&nc->statlock);
      --ncplane_notcurses(p)->stats.planes;
      ncplane_notcurses(p)->stats.fbbytes -= sizeof(*p->fb) * p->leny * p->lenx;
      pthread_mutex_unlock(&nc->statlock);
      if(p->above == NULL && p->below == NULL){
        pthread_mutex_lock(&nc->pilelock);
        ncpile_destroy(ncplane_pile(p));
        pthread_mutex_unlock(&nc->pilelock);
      }
    }
    if(p->sprite){
      sprixel_hide(p->sprite);
    }
    if(p->tam){
      for(int y = 0 ; y < p->leny ; ++y){
        for(int x = 0 ; x < p->lenx ; ++x){
          free(p->tam[y * p->lenx + x].auxvector);
          p->tam[y * p->lenx + x].auxvector = NULL;
        }
      }
    }
    free(p->tam);
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
    ret->crenderlen = 0;
    ret->sprixelcache = NULL;
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
  if(nopts->flags >= (NCPLANE_OPTION_MARGINALIZED << 1u)){
    logwarn("Provided unsupported flags %016jx\n", (uintmax_t)nopts->flags);
  }
  if(nopts->flags & NCPLANE_OPTION_HORALIGNED || nopts->flags & NCPLANE_OPTION_VERALIGNED){
    if(n == NULL){
      logerror("Alignment requires a parent plane\n");
      return NULL;
    }
  }
  if(nopts->flags & NCPLANE_OPTION_MARGINALIZED){
    if(nopts->rows != 0 || nopts->cols != 0){
      logerror("Geometry specified with margins (r=%d, c=%d)\n",
               nopts->rows, nopts->cols);
      return NULL;
    }
  }else if(nopts->rows <= 0 || nopts->cols <= 0){
    logerror("Won't create denormalized plane (r=%d, c=%d)\n",
             nopts->rows, nopts->cols);
    return NULL;
  }
  ncplane* p = malloc(sizeof(*p));
  if(p == NULL){
    return NULL;
  }
  p->scrolling = false;
  if(nopts->flags & NCPLANE_OPTION_MARGINALIZED){
    p->margin_b = nopts->margin_b;
    p->margin_r = nopts->margin_r;
    if(n){ // use parent size
      p->leny = ncplane_dim_y(n);
      p->lenx = ncplane_dim_x(n);
    }else{ // use pile size
      notcurses_term_dim_yx(nc, &p->leny, &p->lenx);
    }
    if((p->leny -= p->margin_b) <= 0){
      p->leny = 1;
    }
    if((p->lenx -= p->margin_r) <= 0){
      p->lenx = 1;
    }
  }else{
    p->leny = nopts->rows;
    p->lenx = nopts->cols;
  }
  size_t fbsize = sizeof(*p->fb) * (p->leny * p->lenx);
  if((p->fb = malloc(fbsize)) == NULL){
    logerror("Error allocating cellmatrix (r=%d, c=%d)\n",
             p->leny, p->lenx);
    free(p);
    return NULL;
  }
  memset(p->fb, 0, fbsize);
  p->x = p->y = 0;
  p->logrow = 0;
  p->sprite = NULL;
  p->blist = NULL;
  p->name = strdup(nopts->name ? nopts->name : "");
  p->halign = NCALIGN_UNALIGNED;
  p->valign = NCALIGN_UNALIGNED;
  p->tam = NULL;
  if(!n){ // new root/standard plane
    p->absy = nopts->y;
    p->absx = nopts->x;
    p->bnext = NULL;
    p->bprev = NULL;
    p->boundto = p;
  }else{ // bound to preexisting pile
    if(nopts->flags & NCPLANE_OPTION_HORALIGNED){
      p->absx = ncplane_halign(n, nopts->x, nopts->cols);
      p->halign = nopts->x;
    }else{
      p->absx = nopts->x;
    }
    p->absx += n->absx;
    if(nopts->flags & NCPLANE_OPTION_VERALIGNED){
      p->absy = ncplane_valign(n, nopts->y, nopts->rows);
      p->valign = nopts->y;
    }else{
      p->absy = nopts->y;
    }
    p->absy += n->absy;
    if( (p->bnext = n->blist) ){
      n->blist->bprev = &p->bnext;
    }
    p->bprev = &n->blist;
    *p->bprev = p;
    p->boundto = n;
  }
  // FIXME handle top/left margins
  p->resizecb = nopts->resizecb;
  p->stylemask = 0;
  p->channels = 0;
  egcpool_init(&p->pool);
  nccell_init(&p->basecell);
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
    pthread_mutex_lock(&nc->statlock);
    nc->stats.fbbytes += fbsize;
    ++nc->stats.planes;
    pthread_mutex_unlock(&nc->statlock);
    pthread_mutex_unlock(&nc->pilelock);
  }
  loginfo("Created new %dx%d plane \"%s\" @ %dx%d\n",
          p->leny, p->lenx, p->name ? p->name : "", p->absy, p->absx);
  return p;
}

// create an ncplane of the specified dimensions, but do not yet place it in
// the z-buffer. clear out all cells. this is for a wholly new context.
// FIXME set up using resizecb rather than special-purpose from SIGWINCH
static ncplane*
create_initial_ncplane(notcurses* nc, int dimy, int dimx){
  ncplane_options nopts = {
    .y = 0, .x = 0,
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
    logerror("Target x %d >= length %d\n", x, n->lenx);
    return -1;
  }else if(x < 0){
    if(x < -1){
      logerror("Negative target x %d\n", x);
      return -1;
    }
  }else{
    n->x = x;
  }
  if(y >= n->leny){
    logerror("Target y %d >= height %d\n", y, n->leny);
    return -1;
  }else if(y < 0){
    if(y < -1){
      logerror("Negative target y %d\n", y);
      return -1;
    }
  }else{
    n->y = y;
  }
  if(cursor_invalid_p(n)){
    logerror("Invalid cursor following move (%d/%d)\n", n->y, n->x);
    return -1;
  }
  return 0;
}

ncplane* ncplane_dup(const ncplane* n, void* opaque){
  int dimy = n->leny;
  int dimx = n->lenx;
  const int placey = n->absy;
  const int placex = n->absx;
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
  if(newn == NULL){
    return NULL;
  }
  // we don't duplicate sprites...though i'm unsure why not
  size_t fbsize = sizeof(*n->fb) * dimx * dimy;
  if(egcpool_dup(&newn->pool, &n->pool)){
    ncplane_destroy(newn);
    return NULL;
  }
  memmove(newn->fb, n->fb, fbsize);
  if(ncplane_cursor_move_yx(newn, n->y, n->x) < 0){
    ncplane_destroy(newn);
    return NULL;
  }
  newn->halign = n->halign;
  newn->stylemask = ncplane_styles(n);
  newn->channels = ncplane_channels(n);
  // we dupd the egcpool, so just dup the goffset
  newn->basecell = n->basecell;
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
    logerror("Can't retain negative size %dx%d\n", keepleny, keeplenx);
    return -1;
  }
  if(keepy < 0 || keepx < 0){ // can't start at negative origin
    logerror("Can't retain negative offset %dx%d\n", keepy, keepx);
    return -1;
  }
  if((!keepleny && keeplenx) || (keepleny && !keeplenx)){ // both must be 0
    logerror("Can't retain null dimension %dx%d\n", keepleny, keeplenx);
    return -1;
  }
  // can't be smaller than keep length
  if(ylen < keepleny){
    logerror("Can't map in y dimension: %d < %d\n", ylen, keepleny);
    return -1;
  }
  if(xlen < keeplenx){
    logerror("Can't map in x dimension: %d < %d\n", xlen, keeplenx);
    return -1;
  }
  if(ylen <= 0 || xlen <= 0){ // can't resize to trivial or negative size
    logerror("Can't achieve meaningless size %dx%d\n", ylen, xlen);
    return -1;
  }
  int rows, cols;
  ncplane_dim_yx(n, &rows, &cols);
  if(keepleny + keepy > rows){
    logerror("Can't keep %d@%d rows from %d\n", keepleny, keepy, rows);
    return -1;
  }
  if(keeplenx + keepx > cols){
    logerror("Can't keep %d@%d cols from %d\n", keeplenx, keepx, cols);
    return -1;
  }
  loginfo("%dx%d @ %d/%d → %d/%d @ %d/%d (keeping %dx%d from %d/%d)\n", rows, cols, n->absy, n->absx, ylen, xlen, n->absy + keepy + yoff, n->absx + keepx + xoff, keepleny, keeplenx, keepy, keepx);
  if(n->absy == n->absy + keepy && n->absx == n->absx + keepx &&
      rows == ylen && cols == xlen){
    return 0;
  }
  notcurses* nc = ncplane_notcurses(n);
  if(n->sprite){
    sprixel_hide(n->sprite);
  }
  // we're good to resize. we'll need alloc up a new framebuffer, and copy in
  // those elements we're retaining, zeroing out the rest. alternatively, if
  // we've shrunk, we will be filling the new structure.
  int oldarea = rows * cols;
  int keptarea = keepleny * keeplenx;
  int newarea = ylen * xlen;
  size_t fbsize = sizeof(nccell) * newarea;
  nccell* fb = malloc(fbsize);
  if(fb == NULL){
    return -1;
  }
  if(n->tam){
    loginfo("TAM realloc to %d entries\n", newarea);
    tament* tmptam = realloc(n->tam, sizeof(*tmptam) * newarea);
    if(tmptam == NULL){
      free(fb);
      return -1;
    }
    n->tam = tmptam;
    // FIXME need to set up the entries based on new distribution for
    // cell-pixel geometry change, and split across new rows
    if(newarea > oldarea){
      memset(n->tam + oldarea, 0, sizeof(*n->tam) * (newarea - oldarea));
    }
  }
  // update the cursor, if it would otherwise be off-plane
  if(n->y >= ylen){
    n->y = ylen - 1;
  }
  if(n->x >= xlen){
    n->x = xlen - 1;
  }
  nccell* preserved = n->fb;
  pthread_mutex_lock(&nc->statlock);
  ncplane_notcurses(n)->stats.fbbytes -= sizeof(*preserved) * (rows * cols);
  ncplane_notcurses(n)->stats.fbbytes += fbsize;
  pthread_mutex_unlock(&nc->statlock);
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
    logerror("Won't destroy standard plane\n");
    return -1;
  }
//notcurses_debug(ncplane_notcurses(ncp), stderr);
  loginfo("Destroying %dx%d plane \"%s\" @ %dx%d\n",
          ncp->leny, ncp->lenx, ncp->name ? ncp->name : NULL, ncp->absy, ncp->absx);
  int ret = 0;
  // dissolve our binding from behind (->bprev is either NULL, or its
  // predecessor on the bound list's ->bnext, or &ncp->boundto->blist)
  if(ncp->bprev){
    if( (*ncp->bprev = ncp->bnext) ){
      ncp->bnext->bprev = ncp->bprev;
    }
  }
  // recursively reparent our children to the plane to which we are bound.
  // this will extract each one from the sibling list.
  struct ncplane* bound = ncp->blist;
  while(bound){
    struct ncplane* tmp = bound->bnext;
    if(ncplane_reparent_family(bound, ncp->boundto) == NULL){
      ret = -1;
    }
    bound = tmp;
  }
  // extract ourselves from the z-axis. do this *after* reparenting, in case
  // reparenting shifts up the z-axis somehow (though i don't think it can,
  // at least not within a pile?).
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
  free_plane(ncp);
  return ret;
}

int ncplane_genocide(ncplane *ncp){
  if(ncp == NULL){
    return 0;
  }
  if(ncplane_notcurses(ncp)->stdplane == ncp){
    logerror("Won't destroy standard plane\n");
    return -1;
  }
  int ret = 0;
  while(ncp->blist){
    ret |= ncplane_genocide(ncp->blist);
  }
  ret |= ncplane_destroy(ncp);
  return ret;
}

// only invoked without suppress banners flag. prints various warnings based on
// the environment / terminal definition.
static void
init_banner_warnings(const notcurses* nc, FILE* out){
  // might be using stderr, so don't just reuse stdout decision
  const bool tty = isatty(fileno(out));
  if(tty){
    term_fg_palindex(nc, out, nc->tcache.caps.colors <= 88 ? 1 : 0xcb);
  }
  if(!nc->tcache.caps.rgb){
    fprintf(out, "\n Warning! Colors subject to https://github.com/dankamongmen/notcurses/issues/4");
    fprintf(out, "\n  Specify a (correct) TrueColor TERM, or COLORTERM=24bit.\n");
  }else{
    if(!nc->tcache.caps.can_change_colors){
      fprintf(out, "\n Warning! Advertised TrueColor but no 'ccc' flag\n");
    }
  }
  if(!notcurses_canutf8(nc)){
    fprintf(out, "\n Warning! Encoding is not UTF-8; output may be degraded.\n");
  }
  if(!get_escape(&nc->tcache, ESCAPE_HPA)){
    fprintf(out, "\n Warning! No absolute horizontal placement.\n");
  }
  const char* sgr0;
  if( (sgr0 = get_escape(&nc->tcache, ESCAPE_SGR0)) ){
    if(tty){
      term_emit(sgr0, out, true);
    }
  }
}

// unless the suppress_banner flag was set, print some version information and
// (if applicable) warnings to stdout. we are not yet on the alternate screen.
static void
init_banner(const notcurses* nc){
  if(!nc->suppress_banner){
    char prefixbuf[BPREFIXSTRLEN + 1];
    term_fg_palindex(nc, stdout, 50 % nc->tcache.caps.colors);
    printf("\n notcurses %s by nick black et al", notcurses_version());
    printf(" on %s %s", nc->tcache.termname ? nc->tcache.termname : "?",
                        nc->tcache.termversion ? nc->tcache.termversion : "");
    term_fg_palindex(nc, stdout, 12 % nc->tcache.caps.colors);
    if(nc->tcache.cellpixy && nc->tcache.cellpixx){
      printf("\n  %d rows (%dpx) %d cols (%dpx) (%sB) %zuB crend %d colors",
             nc->stdplane->leny, nc->tcache.cellpixy,
             nc->stdplane->lenx, nc->tcache.cellpixx,
             bprefix(nc->stats.fbbytes, 1, prefixbuf, 0),
             sizeof(struct crender), nc->tcache.caps.colors);
    }else{
      printf("\n  %d rows %d cols (%sB) %zuB crend %d colors",
             nc->stdplane->leny, nc->stdplane->lenx,
             bprefix(nc->stats.fbbytes, 1, prefixbuf, 0),
             sizeof(struct crender), nc->tcache.caps.colors);
    }
    const char* setaf;
    if(nc->tcache.caps.rgb && (setaf = get_escape(&nc->tcache, ESCAPE_SETAF))){
      putc('+', stdout);
      term_fg_rgb8(&nc->tcache, stdout, 0xe0, 0x60, 0x60);
      putc('R', stdout);
      term_fg_rgb8(&nc->tcache, stdout, 0x60, 0xe0, 0x60);
      putc('G', stdout);
      term_fg_rgb8(&nc->tcache, stdout, 0x20, 0x80, 0xff);
      putc('B', stdout);
      term_fg_palindex(nc, stdout, nc->tcache.caps.colors <= 256 ?
                       12 % nc->tcache.caps.colors : 0x2080e0);
    }
    printf("\n  compiled with gcc-%s, %zuB %s-endian cells\n"
           "  terminfo from %s\n",
           __VERSION__,
           sizeof(nccell),
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
    ncvisual_printbanner(nc);
    fflush(stdout);
    init_banner_warnings(nc, stderr);
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
void init_lang(void){
  const char* encoding = nl_langinfo(CODESET);
  if(encoding && !strcmp(encoding, "UTF-8")){
    return; // already utf-8, great!
  }
  const char* lang = getenv("LANG");
  // if LANG was explicitly set to C/POSIX, life sucks, roll with it
  if(lang && (!strcmp(lang, "C") || !strcmp(lang, "POSIX"))){
    loginfo("LANG was explicitly set to %s, not changing locale\n", lang);
    return;
  }
  setlocale(LC_ALL, "");
  encoding = nl_langinfo(CODESET);
  if(encoding && !strcmp(encoding, "UTF-8")){
    loginfo("Set locale from LANG; client should call setlocale(2)!\n");
    return;
  }
  setlocale(LC_CTYPE, "C.UTF-8");
  encoding = nl_langinfo(CODESET);
  if(encoding && !strcmp(encoding, "UTF-8")){
    loginfo("Forced UTF-8 encoding; client should call setlocale(2)!\n");
    return;
  }
}

// initialize a recursive mutex lock in a way that works on both glibc + musl
static int
recursive_lock_init(pthread_mutex_t *lock){
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

int notcurses_check_pixel_support(const notcurses* nc){
  if(nc->tcache.pixel_draw){
    return 1;
  }
  return 0;
}

// FIXME cut this up into a few distinct pieces, yearrrgh
notcurses* notcurses_core_init(const notcurses_options* opts, FILE* outfp){
  notcurses_options defaultopts = { };
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
  ret->rstate.mstream = NULL;
  ret->rstate.mstreamfp = NULL;
  ret->loglevel = opts->loglevel;
  if(!(opts->flags & NCOPTION_INHIBIT_SETLOCALE)){
    init_lang();
  }
  const char* encoding = nl_langinfo(CODESET);
  bool utf8;
  if(encoding && !strcmp(encoding, "UTF-8")){
    utf8 = true;
  }else if(encoding && (!strcmp(encoding, "ANSI_X3.4-1968") || !strcmp(encoding, "US-ASCII"))){
    utf8 = false;
  }else{
    fprintf(stderr, "Encoding (\"%s\") was neither ANSI_X3.4-1968 nor UTF-8, refusing to start\n Did you call setlocale()?\n",
            encoding ? encoding : "none found");
    return NULL;
  }
  if(outfp == NULL){
    outfp = stdout;
  }
  ret->margin_t = opts->margin_t;
  ret->margin_b = opts->margin_b;
  ret->margin_l = opts->margin_l;
  ret->margin_r = opts->margin_r;
  ret->cursory = ret->cursorx = -1;
  memset(&ret->stats, 0, sizeof(ret->stats));
  memset(&ret->stashed_stats, 0, sizeof(ret->stashed_stats));
  reset_stats(&ret->stats);
  reset_stats(&ret->stashed_stats);
  ret->ttyfp = outfp;
  ret->renderfp = opts->renderfp;
  memset(&ret->rstate, 0, sizeof(ret->rstate));
  memset(&ret->palette_damage, 0, sizeof(ret->palette_damage));
  memset(&ret->palette, 0, sizeof(ret->palette));
  ret->lastframe = NULL;
  ret->lfdimy = 0;
  ret->lfdimx = 0;
  egcpool_init(&ret->pool);
  if((ret->loglevel = opts->loglevel) > NCLOGLEVEL_TRACE || ret->loglevel < 0){
    fprintf(stderr, "Invalid loglevel %d\n", ret->loglevel);
    free(ret);
    return NULL;
  }
  ret->ttyfd = get_tty_fd(ret->ttyfp);
  is_linux_console(ret, !!(opts->flags & NCOPTION_NO_FONT_CHANGES));
  if(ret->ttyfd < 0){
    fprintf(stderr, "Defaulting to %dx%d (output is not to a terminal)\n", DEFAULT_ROWS, DEFAULT_COLS);
  }
  if(recursive_lock_init(&ret->pilelock)){
    fprintf(stderr, "Couldn't initialize pile mutex\n");
    free(ret);
    return NULL;
  }
  if(pthread_mutex_init(&ret->statlock, NULL)){
    pthread_mutex_destroy(&ret->pilelock);
    free(ret);
    return NULL;
  }
  if(setup_signals(ret, (opts->flags & NCOPTION_NO_QUIT_SIGHANDLERS),
                   (opts->flags & NCOPTION_NO_WINCH_SIGHANDLER),
                   notcurses_stop_minimal)){
    pthread_mutex_destroy(&ret->pilelock);
    pthread_mutex_destroy(&ret->statlock);
    free(ret);
    return NULL;
  }
  // don't set loglevel until we've acquired the signal handler, lest we
  // change the loglevel out from under a running instance
  loglevel = opts->loglevel;
  int termerr;
  if(setupterm(opts->termtype, ret->ttyfd, &termerr) != OK){
    fprintf(stderr, "Terminfo error %d (see terminfo(3ncurses))\n", termerr);
    drop_signals(ret);
    pthread_mutex_destroy(&ret->statlock);
    pthread_mutex_destroy(&ret->pilelock);
    free(ret);
    return NULL;
  }
  const char* shortname_term = termname();
// const char* longname_term = longname();
  if(interrogate_terminfo(&ret->tcache, ret->ttyfd, shortname_term, utf8,
                          opts->flags & NCOPTION_NO_ALTERNATE_SCREEN, 0)){
    goto err;
  }
  int dimy, dimx;
  if(update_term_dimensions(ret->ttyfd, &dimy, &dimx, &ret->tcache,
                            ret->margin_b)){
    goto err;
  }
  ret->suppress_banner = opts->flags & NCOPTION_SUPPRESS_BANNERS;
  if(set_fd_nonblocking(ret->tcache.input.infd, 1, &ret->stdio_blocking_save)){
    goto err;
  }
  if(ncvisual_init(ret->loglevel)){
    goto err;
  }
  ret->stdplane = NULL;
  if((ret->stdplane = create_initial_ncplane(ret, dimy, dimx)) == NULL){
    fprintf(stderr, "Couldn't create the initial plane (bad margins?)\n");
    goto err;
  }
  if(ret->ttyfd >= 0){
    reset_term_attributes(&ret->tcache, ret->ttyfp);
    if(!(opts->flags & NCOPTION_NO_CLEAR_BITMAPS)){
      if(sprite_clear_all(&ret->tcache, ret->ttyfd)){
        free_plane(ret->stdplane);
        goto err;
      }
    }
    const char* smkx = get_escape(&ret->tcache, ESCAPE_SMKX);
    if(smkx && tty_emit(smkx, ret->ttyfd)){
      free_plane(ret->stdplane);
      goto err;
    }
    const char* cinvis = get_escape(&ret->tcache, ESCAPE_CIVIS);
    if(cinvis && tty_emit(cinvis, ret->ttyfd)){
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
  const char* clearscr = get_escape(&ret->tcache, ESCAPE_CLEAR);
  if(ret->ttyfd >= 0){
    const char* smcup = get_escape(&ret->tcache, ESCAPE_SMCUP);
    if(smcup){
      if(tty_emit(smcup, ret->ttyfd)){
        free_plane(ret->stdplane);
        goto err;
      }
      // explicit clear even though smcup *might* clear
      if(!clearscr || tty_emit(clearscr, ret->ttyfd)){
        notcurses_refresh(ret, NULL, NULL);
      }
    }else if(!(opts->flags & NCOPTION_NO_ALTERNATE_SCREEN)){
      // if they expected the alternate screen, but we didn't have one to
      // offer, at least clear the screen. try using "clear"; if that doesn't
      // fly, use notcurses_refresh() to force a clearing via iterated writes.
      if(!clearscr || tty_emit(clearscr, ret->ttyfd)){
        notcurses_refresh(ret, NULL, NULL);
      }
    }
  }
  return ret;

err:
  fprintf(stderr, "Alas, you will not be going to space today.\n");
  // FIXME looks like we have some memory leaks on this error path?
  if(ret->rstate.mstreamfp){
    fclose(ret->rstate.mstreamfp);
  }
  free(ret->rstate.mstream);
  tcsetattr(ret->ttyfd, TCSANOW, &ret->tcache.tpreserved);
  drop_signals(ret);
  pthread_mutex_destroy(&ret->statlock);
  pthread_mutex_destroy(&ret->pilelock);
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
    ret |= set_fd_nonblocking(nc->tcache.input.infd, nc->stdio_blocking_save, NULL);
    if(nc->stdplane){
      notcurses_drop_planes(nc);
      free_plane(nc->stdplane);
    }
    if(nc->rstate.mstreamfp){
      fclose(nc->rstate.mstreamfp);
    }
    // if we were not using the alternate screen, our cursor's wherever we last
    // wrote. move it to the bottom left of the screen.
    if(!get_escape(&nc->tcache, ESCAPE_SMCUP)){
      // if ldimy is 0, we've not yet written anything; leave it untouched
      if(nc->lfdimy){
        int targy = nc->lfdimy + nc->margin_t - 1;
        // cup is required, no need to test for existence
        tty_emit(tiparm(get_escape(&nc->tcache, ESCAPE_CUP), targy, 0), nc->ttyfd);
      }
    }
    if(nc->ttyfd >= 0){
      ret |= close(nc->ttyfd);
    }
    egcpool_dump(&nc->pool);
    free(nc->lastframe);
    free(nc->rstate.mstream);
    // get any current stats loaded into stash_stats
    notcurses_stats_reset(nc, NULL);
    if(!nc->suppress_banner){
      summarize_stats(nc);
    }
    del_curterm(cur_term);
    ret |= pthread_mutex_destroy(&nc->statlock);
    ret |= pthread_mutex_destroy(&nc->pilelock);
    free_terminfo_cache(&nc->tcache);
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
  ncchannels_set_fg_default(&n->channels);
}

uint64_t ncplane_set_fchannel(ncplane* n, uint32_t channel){
  return ncchannels_set_fchannel(&n->channels, channel);
}

uint64_t ncplane_set_bchannel(ncplane* n, uint32_t channel){
  return ncchannels_set_bchannel(&n->channels, channel);
}

void ncplane_set_bg_default(ncplane* n){
  ncchannels_set_bg_default(&n->channels);
}

void ncplane_set_bg_rgb8_clipped(ncplane* n, int r, int g, int b){
  ncchannels_set_bg_rgb8_clipped(&n->channels, r, g, b);
}

int ncplane_set_bg_rgb8(ncplane* n, int r, int g, int b){
  return ncchannels_set_bg_rgb8(&n->channels, r, g, b);
}

void ncplane_set_fg_rgb8_clipped(ncplane* n, int r, int g, int b){
  ncchannels_set_fg_rgb8_clipped(&n->channels, r, g, b);
}

int ncplane_set_fg_rgb8(ncplane* n, int r, int g, int b){
  return ncchannels_set_fg_rgb8(&n->channels, r, g, b);
}

int ncplane_set_fg_rgb(ncplane* n, unsigned channel){
  return ncchannels_set_fg_rgb(&n->channels, channel);
}

int ncplane_set_bg_rgb(ncplane* n, unsigned channel){
  return ncchannels_set_bg_rgb(&n->channels, channel);
}

int ncplane_set_fg_alpha(ncplane* n, int alpha){
  return ncchannels_set_fg_alpha(&n->channels, alpha);
}

int ncplane_set_bg_alpha(ncplane *n, int alpha){
  return ncchannels_set_bg_alpha(&n->channels, alpha);
}

int ncplane_set_fg_palindex(ncplane* n, int idx){
  if(idx < 0 || idx >= NCPALETTESIZE){
    return -1;
  }
  n->channels |= CELL_FGDEFAULT_MASK;
  n->channels |= CELL_FG_PALETTE;
  ncchannels_set_fg_alpha(&n->channels, NCALPHA_OPAQUE);
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
  ncchannels_set_bg_alpha(&n->channels, NCALPHA_OPAQUE);
  n->stylemask &= 0xffffff00;
  n->stylemask |= idx;
  return 0;
}

int ncplane_set_base_cell(ncplane* ncp, const nccell* c){
  if(nccell_wide_right_p(c)){
    return -1;
  }
  return nccell_duplicate(ncp, &ncp->basecell, c);
}

int ncplane_set_base(ncplane* ncp, const char* egc, uint32_t stylemask, uint64_t channels){
  return nccell_prime(ncp, &ncp->basecell, egc, stylemask, channels);
}

int ncplane_base(ncplane* ncp, nccell* c){
  return nccell_duplicate(ncp, c, &ncp->basecell);
}

const char* nccell_extended_gcluster(const ncplane* n, const nccell* c){
  if(cell_simple_p(c)){
    return (const char*)&c->gcluster;
  }
  return egcpool_extended_gcluster(&n->pool, c);
}

const char* cell_extended_gcluster(const struct ncplane* n, const nccell* c){
  return nccell_extended_gcluster(n, c);
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
nccell_obliterate(ncplane* n, nccell* c){
  nccell_release(n, c);
  nccell_init(c);
}

// increment y by 1 and rotate the framebuffer up one line. x moves to 0.
void scroll_down(ncplane* n){
  n->x = 0;
  if(n->y == n->leny - 1){
    n->logrow = (n->logrow + 1) % n->leny;
    nccell* row = n->fb + nfbcellidx(n, n->y, 0);
    for(int clearx = 0 ; clearx < n->lenx ; ++clearx){
      nccell_release(n, &row[clearx]);
    }
    memset(row, 0, sizeof(*row) * n->lenx);
  }else{
    ++n->y;
  }
}

int nccell_width(const ncplane* n, const nccell* c){
  const char* egc = nccell_extended_gcluster(n, c);
  if(egc == NULL){
    return -1;
  }
  int cols;
  if(utf8_egc_len(egc, &cols) < 0){
    return -1;
  }
  return cols;
}

int nccell_load(ncplane* n, nccell* c, const char* gcluster){
  int cols;
  int bytes = utf8_egc_len(gcluster, &cols);
  return pool_load_direct(&n->pool, c, gcluster, bytes, cols);
}

int cell_load(ncplane* n, nccell* c, const char* gcluster){
  return nccell_load(n, c, gcluster);
}

// where the magic happens. write the single EGC completely described by |egc|,
// occupying |cols| columns, to the ncplane |n| at the coordinate |y|, |x|. if
// either or both of |y|/|x| is -1, the current cursor location for that
// dimension will be used. if the glyph cannot fit on the current line, it is
// an error unless scrolling is enabled.
static inline int
ncplane_put(ncplane* n, int y, int x, const char* egc, int cols,
            uint16_t stylemask, uint64_t channels, int bytes){
  if(n->sprite){
    logerror("Can't write glyphs (%s) to sprixelated plane\n", egc);
    return -1;
  }
  // FIXME reject any control or space characters here--should be iswgraph()
  // check *before ncplane_cursor_move_yx()* whether we're past the end of the
  // line. if scrolling is enabled, move to the next line if so. if x or y are
  // specified, we must always try to print at exactly that location.
  if(x != -1){
    if(x + cols > n->lenx){
      logerror("Target x %d + %d cols >= length %d\n", x, cols, n->lenx);
      ncplane_cursor_move_yx(n, y, x); // update cursor, though
      return -1;
    }
  }else if(y == -1 && n->x + cols > n->lenx){
    if(!n->scrolling){
      logerror("No room to output [%.*s] %d/%d\n", bytes, egc, n->y, n->x);
      return -1;
    }
    scroll_down(n);
  }
  if(ncplane_cursor_move_yx(n, y, x)){
    return -1;
  }
  // FIXME don't we need to check here for wide character on edge (though our
  // docs currently claim that a wide char on edge is allowed...)?
  // FIXME ought not be allowed when scrolling is disabled!
  if(*egc == '\n'){
    scroll_down(n);
    return 0;
  }
  // A wide character obliterates anything to its immediate right (and marks
  // that cell as wide). Any character placed atop one half of a wide character
  // obliterates the other half. Note that a wide char can thus obliterate two
  // wide chars, totalling four columns.
  nccell* targ = ncplane_cell_ref_yx(n, n->y, n->x);
  if(n->x > 0){
    if(nccell_double_wide_p(targ)){ // replaced cell is half of a wide char
      nccell* sacrifice = targ->gcluster == 0 ?
        // right half will never be on the first column of a row
        &n->fb[nfbcellidx(n, n->y, n->x - 1)] :
        // left half will never be on the last column of a row
        &n->fb[nfbcellidx(n, n->y, n->x + 1)];
      nccell_obliterate(n, sacrifice);
    }
  }
  targ->stylemask = stylemask;
  targ->channels = channels;
  if(cell_load_direct(n, targ, egc, bytes, cols) < 0){
    return -1;
  }
//fprintf(stderr, "%08x %016lx %c %d %d\n", targ->gcluster, targ->channels, nccell_double_wide_p(targ) ? 'D' : 'd', bytes, cols);
  // must set our right hand sides wide, and check for further damage
  ++n->x;
  for(int i = 1 ; i < cols ; ++i){
    nccell* candidate = &n->fb[nfbcellidx(n, n->y, n->x)];
    if(nccell_wide_left_p(candidate)){
      nccell_obliterate(n, &n->fb[nfbcellidx(n, n->y, n->x + 1)]);
    }
    nccell_release(n, candidate);
    candidate->channels = targ->channels;
    candidate->stylemask = targ->stylemask;
    candidate->width = targ->width;
    ++n->x;
  }
  return cols;
}

int ncplane_putc_yx(ncplane* n, int y, int x, const nccell* c){
  const int cols = nccell_double_wide_p(c) ? 2 : 1;
  const char* egc = nccell_extended_gcluster(n, c);
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
  }else if((*gclust = strdup(nccell_extended_gcluster(n, src))) == NULL){
    return -1;
  }
  return 0;
}

unsigned notcurses_supported_styles(const notcurses* nc){
  return term_supported_styles(&nc->tcache);
}

unsigned notcurses_palette_size(const notcurses* nc){
  return nc->tcache.caps.colors;
}

const char* notcurses_detected_terminal(const notcurses* nc){
  return nc->tcache.termname;
}

bool notcurses_cantruecolor(const notcurses* nc){
  return nc->tcache.caps.rgb;
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
  ncchannels_fg_rgb8(c1, &ur, &ug, &ub);
  r1 = ur; g1 = ug; b1 = ub;
  ncchannels_fg_rgb8(c2, &ur, &ug, &ub);
  r2 = ur; g2 = ug; b2 = ub;
  ncchannels_bg_rgb8(c1, &ur, &ug, &ub);
  br1 = ur; bg1 = ug; bb1 = ub;
  ncchannels_bg_rgb8(c2, &ur, &ug, &ub);
  br2 = ur; bg2 = ug; bb2 = ub;
  int deltr = r2 - r1;
  int deltg = g2 - g1;
  int deltb = b2 - b1;
  int deltbr = br2 - br1;
  int deltbg = bg2 - bg1;
  int deltbb = bb2 - bb1;
  int ret;
  nccell dupc = CELL_TRIVIAL_INITIALIZER;
  if(nccell_duplicate(n, &dupc, c) < 0){
    return -1;
  }
  bool fgdef = false, bgdef = false;
  if(ncchannels_fg_default_p(c1) && ncchannels_fg_default_p(c2)){
    fgdef = true;
  }
  if(ncchannels_bg_default_p(c1) && ncchannels_bg_default_p(c2)){
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
      nccell_set_fg_rgb8(&dupc, r, g, b);
    }
    if(!bgdef){
      nccell_set_bg_rgb8(&dupc, br, bg, bb);
    }
    if(ncplane_putc(n, &dupc) <= 0){
      break;
    }
  }
  nccell_release(n, &dupc);
  return ret;
}

int ncplane_vline_interp(ncplane* n, const nccell* c, int len,
                         uint64_t c1, uint64_t c2){
  unsigned ur, ug, ub;
  int r1, g1, b1, r2, g2, b2;
  int br1, bg1, bb1, br2, bg2, bb2;
  ncchannels_fg_rgb8(c1, &ur, &ug, &ub);
  r1 = ur; g1 = ug; b1 = ub;
  ncchannels_fg_rgb8(c2, &ur, &ug, &ub);
  r2 = ur; g2 = ug; b2 = ub;
  ncchannels_bg_rgb8(c1, &ur, &ug, &ub);
  br1 = ur; bg1 = ug; bb1 = ub;
  ncchannels_bg_rgb8(c2, &ur, &ug, &ub);
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
  if(nccell_duplicate(n, &dupc, c) < 0){
    return -1;
  }
  bool fgdef = false, bgdef = false;
  if(ncchannels_fg_default_p(c1) && ncchannels_fg_default_p(c2)){
    fgdef = true;
  }
  if(ncchannels_bg_default_p(c1) && ncchannels_bg_default_p(c2)){
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
      nccell_set_fg_rgb8(&dupc, r1, g1, b1);
    }
    if(!bgdef){
      nccell_set_bg_rgb8(&dupc, br1, bg1, bb1);
    }
    if(ncplane_putc(n, &dupc) <= 0){
      break;
    }
  }
  nccell_release(n, &dupc);
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
    logerror("ystop (%d) insufficient for yoff (%d)\n", ystop, yoff);
    return -1;
  }
  if(xstop < xoff + 1){
    logerror("xstop (%d) insufficient for xoff (%d)\n", xstop, xoff);
    return -1;
  }
  ncplane_dim_yx(n, &ymax, &xmax);
  // must be within the ncplane
  if(xstop >= xmax || ystop >= ymax){
    logerror("Boundary (%dx%d) beyond plane (%dx%d)\n", ystop, xstop, ymax, xmax);
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
    if(n->sprite){
      sprixel_movefrom(n->sprite, n->absy, n->absx);
    }
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
  if(n->boundto == n){
    dy = y - n->absy;
    dx = x - n->absx;
  }else{
    dy = (n->boundto->absy + y) - n->absy;
    dx = (n->boundto->absx + x) - n->absx;
  }
  if(dy || dx){ // don't want to trigger sprixel_movefrom() if unneeded
    if(n->sprite){
      sprixel_movefrom(n->sprite, n->absy, n->absx);
    }
    n->absx += dx;
    n->absy += dy;
    move_bound_planes(n->blist, dy, dx);
  }
  return 0;
}

int ncplane_y(const ncplane* n){
  if(n->boundto == n){
    return n->absy;
  }
  return n->absy - n->boundto->absy;
}

int ncplane_x(const ncplane* n){
  if(n->boundto == n){
    return n->absx;
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
  if(n->sprite){
    sprixel_hide(n->sprite);
  }
  // we must preserve the background, but a pure nccell_duplicate() would be
  // wiped out by the egcpool_dump(). do a duplication (to get the stylemask
  // and channels), and then reload.
  char* egc = nccell_strdup(n, &n->basecell);
  memset(n->fb, 0, sizeof(*n->fb) * n->leny * n->lenx);
  egcpool_dump(&n->pool);
  egcpool_init(&n->pool);
  // we need to zero out the EGC before handing this off to cell_load, but
  // we don't want to lose the channels/attributes, so explicit gcluster load.
  n->basecell.gcluster = 0;
  nccell_load(n, &n->basecell, egc);
  free(egc);
  n->y = n->x = 0;
}

int ncplane_erase_region(ncplane* n, int ystart, int xstart, int ylen, int xlen){
  if(ylen < 0 || xlen < 0){
    logerror("Won't erase section of negative length (%d, %d)\n", ylen, xlen);
    return -1;
  }
  if(ystart < 0 || xstart < 0){
    logerror("Illegal start of erase (%d, %d)\n", ystart, xstart);
    return -1;
  }
  if(ystart >= ncplane_dim_y(n) || ystart + ylen > ncplane_dim_y(n)){
    logerror("Illegal y spec for erase (%d, %d)\n", ystart, ylen);
    return -1;
  }
  if(ylen == 0){
    ylen = ncplane_dim_y(n) - ystart;
  }
  if(xstart >= ncplane_dim_x(n) || xstart + xlen > ncplane_dim_x(n)){
    logerror("Illegal x spec for erase (%d, %d)\n", xstart, xlen);
    return -1;
  }
  if(xlen == 0){
    xlen = ncplane_dim_x(n) - ystart;
  }
  for(int y = ystart ; y < ystart + ylen ; ++y){
    for(int x = xstart ; x < xstart + xlen ; ++x){
      nccell_release(n, &n->fb[nfbcellidx(n, y, x)]);
      nccell_init(&n->fb[nfbcellidx(n, y, x)]);
    }
  }
  return 0;
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
    return tty_emit(ESC "[?" SET_BTN_EVENT_MOUSE ";"
                    /*SET_FOCUS_EVENT_MOUSE ";" */SET_SGR_MODE_MOUSE "h",
                    n->ttyfd);
  }
  return 0;
}

// this seems to work (note difference in suffix, 'l' vs 'h'), but what about
// the sequences 1000 etc?
int notcurses_mouse_disable(notcurses* n){
  if(n->ttyfd >= 0){
    return tty_emit(ESC "[?" SET_BTN_EVENT_MOUSE ";"
                    /*SET_FOCUS_EVENT_MOUSE ";" */SET_SGR_MODE_MOUSE "l",
                    n->ttyfd);
  }
  return 0;
}

bool notcurses_canutf8(const notcurses* nc){
  return nc->tcache.caps.utf8;
}

bool notcurses_canhalfblock(const notcurses* nc){
  return nc->tcache.caps.utf8;
}

bool notcurses_canquadrant(const notcurses* nc){
  return nc->tcache.caps.quadrants && nc->tcache.caps.utf8;
}

bool notcurses_cansextant(const notcurses* nc){
  return nc->tcache.caps.sextants && nc->tcache.caps.utf8;
}

bool notcurses_canbraille(const notcurses* nc){
  return nc->tcache.caps.braille && nc->tcache.caps.utf8;
}

bool notcurses_canfade(const notcurses* nc){
  return nc->tcache.caps.can_change_colors || nc->tcache.caps.rgb;
}

bool notcurses_canchangecolor(const notcurses* nc){
  return nccapability_canchangecolor(&nc->tcache.caps);
}

ncpalette* ncpalette_new(notcurses* nc){
  ncpalette* p = malloc(sizeof(*p));
  if(p){
    memcpy(p, &nc->palette, sizeof(*p));
  }
  return p;
}

ncpalette* palette256_new(notcurses* nc){
  return ncpalette_new(nc);
}

int ncpalette_use(notcurses* nc, const ncpalette* p){
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

int palette256_use(notcurses* nc, const ncpalette* p){
  return ncpalette_use(nc, p);
}

void ncpalette_free(ncpalette* p){
  free(p);
}

void palette256_free(ncpalette* p){
  ncpalette_free(p);
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

notcurses* ncplane_notcurses(const ncplane* n){
  return ncplane_pile(n)->nc;
}

const notcurses* ncplane_notcurses_const(const ncplane* n){
  return ncplane_pile_const(n)->nc;
}

int ncplane_abs_y(const ncplane* n){
  return n->absy;
}

int ncplane_abs_x(const ncplane* n){
  return n->absx;
}

void ncplane_abs_yx(const ncplane* n, int* RESTRICT y, int* RESTRICT x){
  if(y){
    *y = ncplane_abs_y(n);
  }
  if(x){
    *x = ncplane_abs_x(n);
  }
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

int ncplane_resize_marginalized(ncplane* n){
  const ncplane* parent = ncplane_parent_const(n);
  // a marginalized plane cannot be larger than its oppressor plane =]
  int maxy, maxx;
  if(parent == n){ // root plane, need to use pile size
    ncpile* p = ncplane_pile(n);
    maxy = p->dimy;
    maxx = p->dimx;
  }else{
    ncplane_dim_yx(parent, &maxy, &maxx);
  }
  if((maxy -= (n->margin_b + (n->absy - n->boundto->absy))) < 1){
    maxy = 1;
  }
  if((maxx -= (n->margin_r + (n->absx - n->boundto->absx))) < 1){
    maxx = 1;
  }
  int oldy, oldx;
  ncplane_dim_yx(n, &oldy, &oldx); // current dimensions of 'n'
  int keepleny = oldy > maxy ? maxy : oldy;
  int keeplenx = oldx > maxx ? maxx : oldx;
  // FIXME place it according to top/left
  return ncplane_resize_internal(n, 0, 0, keepleny, keeplenx, 0, 0, maxy, maxx);
}

int ncplane_resize_maximize(ncplane* n){
  const ncpile* pile = ncplane_pile(n); // FIXME should be taken against parent
  const int rows = pile->dimy;
  const int cols = pile->dimx;
  int oldy, oldx;
  ncplane_dim_yx(n, &oldy, &oldx); // current dimensions of 'n'
  int keepleny = oldy > rows ? rows : oldy;
  int keeplenx = oldx > cols ? cols : oldx;
  return ncplane_resize_internal(n, 0, 0, keepleny, keeplenx, 0, 0, rows, cols);
}

int ncplane_resize_realign(ncplane* n){
  const ncplane* parent = ncplane_parent_const(n);
  if(parent == n){
    logerror("Can't realign a root plane\n");
    return 0;
  }
  if(n->halign == NCALIGN_UNALIGNED && n->valign == NCALIGN_UNALIGNED){
    logerror("Passed a non-aligned plane\n");
    return -1;
  }
  int xpos = ncplane_x(n);
  if(n->halign != NCALIGN_UNALIGNED){
    xpos = ncplane_halign(parent, n->halign, ncplane_dim_x(n));
  }
  int ypos = ncplane_y(n);
  if(n->valign != NCALIGN_UNALIGNED){
    ypos = ncplane_valign(parent, n->valign, ncplane_dim_y(n));
  }
  return ncplane_move_yx(n, ypos, xpos);
}

// The standard plane cannot be reparented; we return NULL in that case.
// If provided |newparent|==|n|, we are moving |n| to its own pile. If |n|
// is already bound to |newparent|, this is a no-op, and we return |n|.
// This is essentially a wrapper around ncplane_reparent_family() that first
// reparents any children to the parent of 'n', or makes them root planes if
// 'n' is a root plane.
ncplane* ncplane_reparent(ncplane* n, ncplane* newparent){
  const notcurses* nc = ncplane_notcurses_const(n);
  if(n == nc->stdplane){
    logerror("Won't reparent standard plane\n");
    return NULL; // can't reparent standard plane
  }
  if(n->boundto == newparent){
    loginfo("Won't reparent plane to itself\n");
    return n;
  }
//notcurses_debug(ncplane_notcurses(n), stderr);
  if(n->blist){
    if(n->boundto == n){ // children become new root planes
      ncplane* lastlink;
      ncplane* child = n->blist;
      do{
        child->boundto = child;
        lastlink = child;
        child = child->bnext;
      }while(child); // n->blist != NULL -> lastlink != NULL
      if( (lastlink->bnext = ncplane_pile(n)->roots) ){
        lastlink->bnext->bprev = &lastlink->bnext;
      }
      n->blist->bprev = &ncplane_pile(n)->roots;
      ncplane_pile(n)->roots = n->blist;
    }else{ // children are rebound to current parent
      ncplane* lastlink;
      ncplane* child = n->blist;
      do{
        child->boundto = n->boundto;
        lastlink = child;
        child = child->bnext;
      }while(child); // n->blist != NULL -> lastlink != NULL
      if( (lastlink->bnext = n->boundto->blist) ){
        lastlink->bnext->bprev = &lastlink->bnext;
      }
      n->blist->bprev = &n->boundto->blist;
      n->boundto->blist = n->blist;
    }
    n->blist = NULL;
  }
  // FIXME would be nice to skip ncplane_descendant_p() on this call...:/
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

// unsplice our sprixel from the pile's sprixellist, and then unsplice all
// children, recursively. call before unbinding. returns a doubly-linked
// list of any sprixels found.
static sprixel*
unsplice_sprixels_recursive(ncplane* n, sprixel* prev){
  sprixel* s = n->sprite;
  if(s){
    if(s->prev){
      s->prev->next = s->next;
    }else{
      ncplane_pile(n)->sprixelcache = s->next;
    }
    if(s->next){
      s->next->prev = s->prev;
    }
    if( (s->prev = prev) ){
      prev->next = s;
    }
    s->next = NULL;
    prev = s;
  }
  for(ncplane* child = n->blist ; child ; child = child->bnext){
    unsplice_sprixels_recursive(child, prev);
    while(prev->next){ // FIXME lame
      prev = prev->next;
    }
  }
  return prev;
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
//notcurses_debug(ncplane_notcurses(n), stderr);
  if(n->boundto == newparent){ // no-op
    return n;
  }
  if(n->bprev){ // extract from sibling list
    if( (*n->bprev = n->bnext) ){
      n->bnext->bprev = n->bprev;
    }
  }
  // ncplane_notcurses() goes through ncplane_pile(). since we're possibly
  // destroying piles below, get the notcurses reference early on.
  notcurses* nc = ncplane_notcurses(n);
  // if leaving a pile, extract n from the old zaxis, and also any sprixel
  sprixel* s = NULL;
  if(n == newparent || ncplane_pile(n) != ncplane_pile(newparent)){
    unsplice_zaxis_recursive(n);
    s = unsplice_sprixels_recursive(n, NULL);
  }
  n->boundto = newparent;
  if(n == n->boundto){ // we're a new root plane
    n->bnext = NULL;
    n->bprev = NULL;
    splice_zaxis_recursive(n);
    pthread_mutex_lock(&nc->pilelock);
    if(ncplane_pile(n)->top == NULL){ // did we just empty our pile?
      ncpile_destroy(ncplane_pile(n));
    }
    make_ncpile(ncplane_notcurses(n), n);
    pthread_mutex_unlock(&nc->pilelock);
  }else{ // establish ourselves as a sibling of new parent's children
    if( (n->bnext = newparent->blist) ){
      n->bnext->bprev = &n->bnext;
    }
    n->bprev = &newparent->blist;
    newparent->blist = n;
    // place it immediately above the new binding plane if crossing piles
    if(n->pile != ncplane_pile(n->boundto)){
      splice_zaxis_recursive(n);
      pthread_mutex_lock(&nc->pilelock);
      if(ncplane_pile(n)->top == NULL){ // did we just empty our pile?
        ncpile_destroy(ncplane_pile(n));
      }
      n->pile = ncplane_pile(n->boundto);
      pthread_mutex_unlock(&nc->pilelock);
    }
  }
  if(s){ // must be on new plane, with sprixels to donate
    sprixel* lame = s;
    while(lame->next){
      lame = lame->next;
    }
    if( (lame->next = n->pile->sprixelcache) ){
      n->pile->sprixelcache->prev = lame;
    }
    n->pile->sprixelcache = s;
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
  return n->tcache.input.infd;
}

int ncdirect_inputready_fd(ncdirect* n){
  return n->tcache.input.infd;
}

// FIXME speed this up, PoC
// given an egc, get its index in the blitter's EGC set
static int
get_blitter_egc_idx(const struct blitset* bset, const char* egc){
  wchar_t wc;
  mbstate_t mbs = {};
  size_t sret = mbrtowc(&wc, egc, strlen(egc), &mbs);
  if(sret == (size_t)-1 || sret == (size_t)-2){
    return -1;
  }
  wchar_t* wptr = wcsrchr(bset->egcs, wc);
  if(wptr == NULL){
//fprintf(stderr, "FAILED TO FIND [%s] (%lc) in [%ls]\n", egc, wc, bset->egcs);
    return -1;
  }
//fprintf(stderr, "FOUND [%s] (%lc) in [%ls] (%zu)\n", egc, wc, bset->egcs, wptr - bset->egcs);
  return wptr - bset->egcs;
}

static bool
is_bg_p(int idx, int py, int px, int width){
  // bit increases to the right, and down
  const int bpos = py * width + px; // bit corresponding to pixel, 0..|egcs|-1
  const unsigned mask = 1u << bpos;
  if(idx & mask){
    return false;
  }
  return true;
}

static inline uint32_t*
ncplane_as_rgba_internal(const ncplane* nc, ncblitter_e blit,
                         int begy, int begx, int leny, int lenx,
                         int* pxdimy, int* pxdimx){
  const notcurses* ncur = ncplane_notcurses_const(nc);
  if(begy < 0 || begx < 0){
    logerror("Nil offset (%d,%d)\n", begy, begx);
    return NULL;
  }
  if(begx >= nc->lenx || begy >= nc->leny){
    logerror("Invalid offset (%d,%d)\n", begy, begx);
    return NULL;
  }
  if(lenx == -1){ // -1 means "to the end"; use all space available
    lenx = nc->lenx - begx;
  }
  if(leny == -1){
    leny = nc->leny - begy;
  }
  if(lenx <= 0 || leny <= 0){ // no need to draw zero-size object, exit
    logerror("Nil geometry (%dx%d)\n", leny, lenx);
    return NULL;
  }
//fprintf(stderr, "sum: %d/%d avail: %d/%d\n", begy + leny, begx + lenx, nc->leny, nc->lenx);
  if(begx + lenx > nc->lenx || begy + leny > nc->leny){
    logerror("Invalid specs %d + %d > %d or %d + %d > %d\n",
             begx, lenx, nc->lenx, begy, leny, nc->leny);
    return NULL;
  }
  if(blit == NCBLIT_PIXEL){ // FIXME extend this to support sprixels
    logerror("Pixel blitter %d not yet supported\n", blit);
    return NULL;
  }
  if(blit == NCBLIT_DEFAULT){
    logerror("Must specify exact blitter, not NCBLIT_DEFAULT\n");
    return NULL;
  }
  const struct blitset* bset = lookup_blitset(&ncur->tcache, blit, false);
  if(bset == NULL){
    logerror("Blitter %d invalid in current environment\n", blit);
    return NULL;
  }
//fprintf(stderr, "ALLOCATING %u %d %d %p\n", 4u * lenx * leny * 2, leny, lenx, bset);
  if(pxdimy){
    *pxdimy = leny * bset->height;
  }
  if(pxdimx){
    *pxdimx = lenx * bset->width;
  }
  uint32_t* ret = malloc(sizeof(*ret) * lenx * bset->width * leny * bset->height);
//fprintf(stderr, "GEOM: %d/%d %d/%d ret: %p\n", bset->height, bset->width, *pxdimy, *pxdimx, ret);
  if(ret){
    for(int y = begy, targy = 0 ; y < begy + leny ; ++y, targy += bset->height){
      for(int x = begx, targx = 0 ; x < begx + lenx ; ++x, targx += bset->width){
        uint16_t stylemask;
        uint64_t channels;
        char* c = ncplane_at_yx(nc, y, x, &stylemask, &channels);
        if(c == NULL){
          free(ret);
          return NULL;
        }
        int idx = get_blitter_egc_idx(bset, c);
        if(idx < 0){
          free(ret);
          free(c);
          return NULL;
        }
        unsigned fr, fg, fb, br, bg, bb, fa, ba;
        ncchannels_fg_rgb8(channels, &fr, &fb, &fg);
        fa = ncchannels_fg_alpha(channels);
        ncchannels_bg_rgb8(channels, &br, &bb, &bg);
        ba = ncchannels_bg_alpha(channels);
        // handle each destination pixel from this cell
        for(int py = 0 ; py < bset->height ; ++py){
          for(int px = 0 ; px < bset->width ; ++px){
            uint32_t* p = &ret[(targy + py) * (lenx * bset->width) + (targx + px)];
            bool background = is_bg_p(idx, py, px, bset->width);
            if(background){
              if(ba){
                *p = 0;
              }else{
                ncpixel_set_a(p, 0xff);
                ncpixel_set_r(p, br);
                ncpixel_set_g(p, bb);
                ncpixel_set_b(p, bg);
              }
            }else{
              if(fa){
                *p = 0;
              }else{
                ncpixel_set_a(p, 0xff);
                ncpixel_set_r(p, fr);
                ncpixel_set_g(p, fb);
                ncpixel_set_b(p, fg);
              }
            }
          }
        }
        free(c);
      }
    }
  }
  return ret;
}

uint32_t* ncplane_as_rgba(const ncplane* nc, ncblitter_e blit,
                          int begy, int begx, int leny, int lenx,
                          int* pxdimy, int* pxdimx){
  int px, py;
  if(!pxdimy){
    pxdimy = &py;
  }
  if(!pxdimx){
    pxdimx = &px;
  }
  return ncplane_as_rgba_internal(nc, blit, begy, begx, leny, lenx, pxdimy, pxdimx);
}

// return a heap-allocated copy of the contents
char* ncplane_contents(ncplane* nc, int begy, int begx, int leny, int lenx){
  if(begy < 0 || begx < 0){
    logerror("Beginning coordinates (%d/%d) below 0\n", begy, begx);
    return NULL;
  }
  if(begx >= nc->lenx || begy >= nc->leny){
    logerror("Beginning coordinates (%d/%d) exceeded lengths (%d/%d)\n",
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
    logerror("Lengths (%d/%d) below 0\n", leny, lenx);
    return NULL;
  }
  if(begx + lenx > nc->lenx || begy + leny > nc->leny){
    logerror("Ending coordinates (%d/%d) exceeded lengths (%d/%d)\n",
             begy + leny, begx + lenx, nc->leny, nc->lenx);
    return NULL;
  }
  size_t retlen = 1;
  char* ret = malloc(retlen);
  if(ret){
    for(int y = begy, targy = 0 ; y < begy + leny ; ++y, targy += 2){
      for(int x = begx, targx = 0 ; x < begx + lenx ; ++x, ++targx){
        nccell ncl = CELL_TRIVIAL_INITIALIZER;
        // we need ncplane_at_yx_cell() here instead of ncplane_at_yx(),
        // because we should only have one copy of each wide EGC.
        int clen;
        if((clen = ncplane_at_yx_cell(nc, y, x, &ncl)) < 0){
          free(ret);
          return NULL;
        }
        const char* c = nccell_extended_gcluster(nc, &ncl);
        if(clen){
          char* tmp = realloc(ret, retlen + clen);
          if(!tmp){
            free(ret);
            return NULL;
          }
          ret = tmp;
          memcpy(ret + retlen - 1, c, clen);
          retlen += clen;
        }
      }
    }
    ret[retlen - 1] = '\0';
  }
  return ret;
}

int nccells_double_box(ncplane* n, uint32_t attr, uint64_t channels,
                       nccell* ul, nccell* ur, nccell* ll, nccell* lr, nccell* hl, nccell* vl){
  if(notcurses_canutf8(ncplane_notcurses(n))){
    return nccells_load_box(n, attr, channels, ul, ur, ll, lr, hl, vl, NCBOXDOUBLE);
  }
  return nccells_ascii_box(n, attr, channels, ul, ur, ll, lr, hl, vl);
}

int cells_double_box(ncplane* n, uint32_t attr, uint64_t channels,
                     nccell* ul, nccell* ur, nccell* ll, nccell* lr, nccell* hl, nccell* vl){
  return nccells_double_box(n, attr, channels, ul, ur, ll, lr, hl, vl);
}

int nccells_rounded_box(ncplane* n, uint32_t attr, uint64_t channels,
                        nccell* ul, nccell* ur, nccell* ll, nccell* lr, nccell* hl, nccell* vl){
  if(notcurses_canutf8(ncplane_notcurses(n))){
    return nccells_load_box(n, attr, channels, ul, ur, ll, lr, hl, vl, NCBOXROUND);
  }
  return nccells_ascii_box(n, attr, channels, ul, ur, ll, lr, hl, vl);
}

int cells_rounded_box(ncplane* n, uint32_t attr, uint64_t channels,
                      nccell* ul, nccell* ur, nccell* ll, nccell* lr, nccell* hl, nccell* vl){
  return nccells_rounded_box(n, attr, channels, ul, ur, ll, lr, hl, vl);
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

void ncplane_pixelgeom(const ncplane* n, int* RESTRICT pxy, int* RESTRICT pxx,
                       int* RESTRICT celldimy, int* RESTRICT celldimx,
                       int* RESTRICT maxbmapy, int* RESTRICT maxbmapx){
  notcurses* nc = ncplane_notcurses(n);
  if(celldimy){
    *celldimy = nc->tcache.cellpixy;
  }
  if(celldimx){
    *celldimx = nc->tcache.cellpixx;
  }
  if(pxy){
    *pxy = nc->tcache.cellpixy * ncplane_dim_y(n);
  }
  if(pxx){
    *pxx = nc->tcache.cellpixx * ncplane_dim_x(n);
  }
  if(notcurses_check_pixel_support(nc) > 0){
    if(maxbmapy){
      *maxbmapy = nc->tcache.cellpixy * ncplane_dim_y(n);
      if(*maxbmapy > nc->tcache.sixel_maxy && nc->tcache.sixel_maxy){
        *maxbmapy = nc->tcache.sixel_maxy;
      }
    }
    if(maxbmapx){
      *maxbmapx = nc->tcache.cellpixx * ncplane_dim_x(n);
      if(*maxbmapx > nc->tcache.sixel_maxx && nc->tcache.sixel_maxx){
        *maxbmapx = nc->tcache.sixel_maxx;
      }
    }
  }else{
    if(maxbmapy){
      *maxbmapy = 0;
    }
    if(maxbmapx){
      *maxbmapx = 0;
    }
  }
}
