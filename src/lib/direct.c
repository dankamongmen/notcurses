#include "version.h"
#include "builddef.h"
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "version.h"
#include "visual-details.h"
#include "notcurses/direct.h"
#include "internal.h"
#ifdef USE_READLINE
#include <readline/readline.h>
#endif

// conform to the foreground and background channels of 'channels'
static int
activate_channels(ncdirect* nc, uint64_t channels){
  if(ncchannels_fg_default_p(channels)){
    if(ncdirect_set_fg_default(nc)){
      return -1;
    }
  }else if(ncchannels_fg_palindex_p(channels)){
    if(ncdirect_set_fg_palindex(nc, ncchannels_fg_palindex(channels))){
      return -1;
    }
  }else if(ncdirect_set_fg_rgb(nc, ncchannels_fg_rgb(channels))){
    return -1;
  }
  if(ncchannels_bg_default_p(channels)){
    if(ncdirect_set_bg_default(nc)){
      return -1;
    }
  }else if(ncchannels_bg_palindex_p(channels)){
    if(ncdirect_set_bg_palindex(nc, ncchannels_bg_palindex(channels))){
      return -1;
    }
  }else if(ncdirect_set_bg_rgb(nc, ncchannels_bg_rgb(channels))){
    return -1;
  }
  return 0;
}

int ncdirect_putstr(ncdirect* nc, uint64_t channels, const char* utf8){
  if(activate_channels(nc, channels)){
    return -1;
  }
  return ncfputs(utf8, nc->ttyfp);
}

int ncdirect_putegc(ncdirect* nc, uint64_t channels, const char* utf8,
                    int* sbytes){
  int cols;
  int bytes = utf8_egc_len(utf8, &cols);
  if(bytes < 0){
    return -1;
  }
  if(sbytes){
    *sbytes = bytes;
  }
  if(activate_channels(nc, channels)){
    return -1;
  }
  if(fprintf(nc->ttyfp, "%.*s", bytes, utf8) < 0){
    return -1;
  }
  return cols;
}

int ncdirect_cursor_up(ncdirect* nc, int num){
  if(num < 0){
    return -1;
  }
  if(num == 0){
    return 0;
  }
  const char* cuu = get_escape(&nc->tcache, ESCAPE_CUU);
  if(cuu){
    return term_emit(tiparm(cuu, num), nc->ttyfp, false);
  }
  return -1;
}

int ncdirect_cursor_left(ncdirect* nc, int num){
  if(num < 0){
    return -1;
  }
  if(num == 0){
    return 0;
  }
  const char* cub = get_escape(&nc->tcache, ESCAPE_CUB);
  if(cub){
    return term_emit(tiparm(cub, num), nc->ttyfp, false);
  }
  return -1;
}

int ncdirect_cursor_right(ncdirect* nc, int num){
  if(num < 0){
    return -1;
  }
  if(num == 0){
    return 0;
  }
  const char* cuf = get_escape(&nc->tcache, ESCAPE_CUF);
  if(cuf){
    return term_emit(tiparm(cuf, num), nc->ttyfp, false);
  }
  return -1; // FIXME fall back to cuf1?
}

// if we're on the last line, we need some scrolling action. rather than
// merely using cud (which doesn't reliably scroll), we emit vertical tabs.
// this has the peculiar property (in all terminals tested) of scrolling when
// necessary but performing no carriage return -- a pure line feed.
int ncdirect_cursor_down(ncdirect* nc, int num){
  if(num < 0){
    return -1;
  }
  if(num == 0){
    return 0;
  }
  int ret = 0;
  while(num--){
    if(ncfputc('\v', nc->ttyfp) == EOF){
      ret = -1;
      break;
    }
  }
  return ret;
}

int ncdirect_clear(ncdirect* nc){
  const char* clearscr = get_escape(&nc->tcache, ESCAPE_CLEAR);
  if(clearscr){
    return term_emit(clearscr, nc->ttyfp, true);
  }
  return -1;
}

int ncdirect_dim_x(ncdirect* nc){
  int x;
  if(nc->tcache.ttyfd >= 0){
    if(update_term_dimensions(NULL, &x, &nc->tcache, 0) == 0){
      return x;
    }
  }else{
    return 80; // lol
  }
  return -1;
}

int ncdirect_dim_y(ncdirect* nc){
  int y;
  if(nc->tcache.ttyfd >= 0){
    if(update_term_dimensions(&y, NULL, &nc->tcache, 0) == 0){
      return y;
    }
  }else{
    return 24; // lol
  }
  return -1;
}

int ncdirect_cursor_enable(ncdirect* nc){
  const char* cnorm = get_escape(&nc->tcache, ESCAPE_CNORM);
  if(cnorm){
    return term_emit(cnorm, nc->ttyfp, true);
  }
  return -1;
}

int ncdirect_cursor_disable(ncdirect* nc){
  const char* cinvis = get_escape(&nc->tcache, ESCAPE_CIVIS);
  if(cinvis){
    return term_emit(cinvis, nc->ttyfp, true);
  }
  return -1;
}

static int
cursor_yx_get(int ttyfd, const char* u7, int* y, int* x){
  if(tty_emit(u7, ttyfd)){
    return -1;
  }
  bool done = false;
  enum { // what we expect now
    CURSOR_ESC, // 27 (0x1b)
    CURSOR_LSQUARE,
    CURSOR_ROW, // delimited by a semicolon
    CURSOR_COLUMN,
    CURSOR_R,
  } state = CURSOR_ESC;
  int row = 0, column = 0;
  int r;
  char in;
  do{
    while((r = read(ttyfd, &in, 1)) == 1){
      bool valid = false;
      switch(state){
        case CURSOR_ESC: valid = (in == NCKEY_ESC); state = CURSOR_LSQUARE; break;
        case CURSOR_LSQUARE: valid = (in == '['); state = CURSOR_ROW; break;
        case CURSOR_ROW:
          if(isdigit(in)){
            row *= 10;
            row += in - '0';
            valid = true;
          }else if(in == ';'){
            state = CURSOR_COLUMN;
            valid = true;
          }
          break;
        case CURSOR_COLUMN:
          if(isdigit(in)){
            column *= 10;
            column += in - '0';
            valid = true;
          }else if(in == 'R'){
            state = CURSOR_R;
            valid = true;
          }
          break;
        case CURSOR_R: default: // logical error, whoops
          break;
      }
      if(!valid){
        logerror("Unexpected result (%c, %d) from terminal\n", in, in);
        break;
      }
      if(state == CURSOR_R){
        done = true;
        break;
      }
    }
    // need to loop 0 to handle slow terminals, see for instance screen =[
  }while(!done && (r >= 0 || (errno == EINTR || errno == EAGAIN || errno == EBUSY)));
  if(!done){
    logerror("Error reading cursor location\n");
    return -1;
  }
  if(y){
    *y = row;
  }
  if(x){
    *x = column;
  }
  return 0;
}

// if we're lacking hpa/vpa, *and* -1 is passed for one of x/y, *and* we've
// not got a real ttyfd, we're pretty fucked. we just punt and substitute
// 0 for that case, which hopefully only happens when running headless unit
// tests under TERM=vt100. if we need to truly rigourize things, we could
// cub/cub1 the width or cuu/cuu1 the height, then cuf/cub back? FIXME
int ncdirect_cursor_move_yx(ncdirect* n, int y, int x){
  const char* hpa = get_escape(&n->tcache, ESCAPE_HPA);
  const char* vpa = get_escape(&n->tcache, ESCAPE_VPA);
  const char* u7 = get_escape(&n->tcache, ESCAPE_U7);
  if(y == -1){ // keep row the same, horizontal move only
    if(hpa){
      return term_emit(tiparm(hpa, x), n->ttyfp, false);
    }else if(n->tcache.ttyfd >= 0 && u7){
      if(cursor_yx_get(n->tcache.ttyfd, u7, &y, NULL)){
        return -1;
      }
    }else{
      y = 0;
    }
  }else if(x == -1){ // keep column the same, vertical move only
    if(!vpa){
      return term_emit(tiparm(vpa, y), n->ttyfp, false);
    }else if(n->tcache.ttyfd >= 0 && u7){
      if(cursor_yx_get(n->tcache.ttyfd, u7, NULL, &x)){
        return -1;
      }
    }else{
      x = 0;
    }
  }
  const char* cup = get_escape(&n->tcache, ESCAPE_CUP);
  if(cup){
    return term_emit(tiparm(cup, y, x), n->ttyfp, false);
  }else if(vpa && hpa){
    if(term_emit(tiparm(hpa, x), n->ttyfp, false) == 0 &&
       term_emit(tiparm(vpa, y), n->ttyfp, false) == 0){
      return 0;
    }
  }
  return -1; // we will not be moving the cursor today
}

// an algorithm to detect inverted cursor reporting on terminals 2x2 or larger:
//  * get initial cursor position / push cursor position
//  * move right using cursor-independent routines
//  * move up using cursor-independent routines
//  * get cursor position
//  * if cursor position is unchanged, either cursor reporting is broken, or
//    we started in the upper-right corner. determine the latter by checking
//    terminal dimensions. if we were in the upper-right corner, move somewhere
//    else and retry.
//  * if cursor coordinate changed in only one dimension, we were either on the
//    right side, or along the top row, but not both. determine which one, and
//    determine whether we're inverted.
//  * if both dimensions changed, determine whether we're inverted by checking
//    the change. the row ought have decreased; the column ought have increased.
//  * move back to intiial position / pop cursor position
static int
detect_cursor_inversion(ncdirect* n, const char* u7, int rows, int cols, int* y, int* x){
  if(rows <= 1 || cols <= 1){ // FIXME can this be made to work in 1 dimension?
    return -1;
  }
  if(cursor_yx_get(n->tcache.ttyfd, u7, y, x)){
    return -1;
  }
  // do not use normal ncdirect_cursor_*() commands, because those go to ttyfp
  // instead of tcache.ttyfd. since we always talk directly to the terminal, we need
  // to move the cursor directly via the terminal.
  const char* cuu = get_escape(&n->tcache, ESCAPE_CUU);
  const char* cuf = get_escape(&n->tcache, ESCAPE_CUF);
  const char* cub = get_escape(&n->tcache, ESCAPE_CUB);
  // FIXME do we want to use cud here, or \v like above?
  const char* cud = get_escape(&n->tcache, ESCAPE_CUD);
  if(!cud || !cub || !cuf || !cuu){
    return -1;
  }
  int movex;
  int movey;
  if(*x == cols && *y == 1){
    if(tty_emit(tiparm(cud, 1), n->tcache.ttyfd)){
      return -1;
    }
    if(tty_emit(tiparm(cub, 1), n->tcache.ttyfd)){
      return -1;
    }
    movex = 1;
    movey = -1;
  }else{
    if(tty_emit(tiparm(cuu, 1), n->tcache.ttyfd)){
      return -1;
    }
    if(tty_emit(tiparm(cuf, 1), n->tcache.ttyfd)){
      return -1;
    }
    movex = -1;
    movey = 1;
  }
  int newy, newx;
  if(cursor_yx_get(n->tcache.ttyfd, u7, &newy, &newx)){
    return -1;
  }
  if(*x == cols && *y == 1){ // need to swap values, since we moved opposite
    *x = newx;
    newx = cols;
    *y = newy;
    newy = 1;
  }
  if(tty_emit(tiparm(movex == 1 ? cuf : cub, 1), n->tcache.ttyfd)){
    return -1;
  }
  if(tty_emit(tiparm(movey == 1 ? cud : cuu, 1), n->tcache.ttyfd)){
    return -1;
  }
  if(*y == newy && *x == newx){
    return -1; // hopelessly broken
  }else if(*x == newx){
    // we only changed one, supposedly the number of rows. if we were on the
    // top row before, the reply is inverted.
    if(*y == 0){
      n->tcache.inverted_cursor = true;
    }
  }else if(*y == newy){
    // we only changed one, supposedly the number of columns. if we were on the
    // rightmost column before, the reply is inverted.
    if(*x == cols){
      n->tcache.inverted_cursor = true;
    }
  }else{
    // the row ought have decreased, and the column ought have increased. if it
    // went the other way, the reply is inverted.
    if(newy > *y && newx < *x){
      n->tcache.inverted_cursor = true;
    }
  }
  n->tcache.detected_cursor_inversion = true;
  return 0;
}

static int
detect_cursor_inversion_wrapper(ncdirect* n, const char* u7, int* y, int* x){
  // if we're not on a real terminal, there's no point in running this
  if(n->tcache.ttyfd < 0){
    return 0;
  }
  const int toty = ncdirect_dim_y(n);
  const int totx = ncdirect_dim_x(n);
  // there's an argument to be made that this ought be wrapped in sc/rc
  // (push/pop cursor), rather than undoing itself. problem is, some
  // terminals lack sc/rc (they need cursor moves to run the detection
  // algorithm in the first place), and our versions go to ttyfp instead
  // of ttyfd, as needed by cursor interrogation.
  return detect_cursor_inversion(n, u7, toty, totx, y, x);
}

// no terminfo capability for this. dangerous--it involves writing controls to
// the terminal, and then reading a response. many things can distupt this
// non-atomic procedure, leading to unexpected results. a garbage function.
int ncdirect_cursor_yx(ncdirect* n, int* y, int* x){
#ifndef __MINGW64__ // FIXME
  struct termios termio, oldtermios;
  // this is only meaningful for real terminals
  if(n->tcache.ttyfd < 0){
    return -1;
  }
  const char* u7 = get_escape(&n->tcache, ESCAPE_U7);
  if(u7 == NULL){
    fprintf(stderr, "Terminal doesn't support cursor reporting\n");
    return -1;
  }
  if(tcgetattr(n->tcache.ttyfd, &termio)){
    fprintf(stderr, "Couldn't get terminal info from %d (%s)\n",
            n->tcache.ttyfd, strerror(errno));
    return -1;
  }
  memcpy(&oldtermios, &termio, sizeof(termio));
  // we might already be in cbreak mode from ncdirect_init(), but just in case
  // it got changed by the client code since then, duck into cbreak mode anew.
  termio.c_lflag &= ~(ICANON | ECHO);
  if(tcsetattr(n->tcache.ttyfd, TCSAFLUSH, &termio)){
    fprintf(stderr, "Couldn't put terminal into cbreak mode via %d (%s)\n",
            n->tcache.ttyfd, strerror(errno));
    return -1;
  }
  int ret, yval, xval;
  if(!y){
    y = &yval;
  }
  if(!x){
    x = &xval;
  }
  if(!n->tcache.detected_cursor_inversion){
    ret = detect_cursor_inversion_wrapper(n, u7, y, x);
  }else{
    ret = cursor_yx_get(n->tcache.ttyfd, u7, y, x);
  }
  if(ret == 0){
    if(n->tcache.inverted_cursor){
      int tmp = *y;
      *y = *x;
      *x = tmp;
    }else{
      // we use 0-based coordinates, but known terminals use 1-based
      // coordinates. the only known exception is kmscon, which is
      // incidentally the only one which inverts its response.
      --*y;
      --*x;
    }
  }
  if(tcsetattr(n->tcache.ttyfd, TCSANOW, &oldtermios)){
    fprintf(stderr, "Couldn't restore terminal mode on %d (%s)\n",
            n->tcache.ttyfd, strerror(errno)); // don't return error for this
  }
  return ret;
#else
  return -1; // FIXME
#endif
}

int ncdirect_cursor_push(ncdirect* n){
  const char* sc = get_escape(&n->tcache, ESCAPE_SC);
  if(sc){
    return term_emit(sc, n->ttyfp, false);
  }
  return -1;
}

int ncdirect_cursor_pop(ncdirect* n){
  const char* rc = get_escape(&n->tcache, ESCAPE_RC);
  if(rc){
    return term_emit(rc, n->ttyfp, false);
  }
  return -1;
}

static inline int
ncdirect_align(struct ncdirect* n, ncalign_e align, int c){
  if(align == NCALIGN_LEFT){
    return 0;
  }
  int cols = ncdirect_dim_x(n);
  if(c > cols){
    return 0;
  }
  if(align == NCALIGN_CENTER){
    return (cols - c) / 2;
  }else if(align == NCALIGN_RIGHT){
    return cols - c;
  }
  return INT_MAX;
}

static int
ncdirect_dump_plane(ncdirect* n, const ncplane* np, int xoff){
  const int toty = ncdirect_dim_y(n);
  int dimy, dimx;
  ncplane_dim_yx(np, &dimy, &dimx);
  if(np->sprite){
    if(xoff){
      // doing an x-move without specifying the y coordinate requires asking
      // the terminal where the cursor currently is.
      if(ncdirect_cursor_move_yx(n, -1, xoff)){
        return -1;
      }
    }
    // flush our FILE*, as we're about to use UNIX I/O (since we can't rely on
    // stdio to transfer large amounts at once).
    if(ncdirect_flush(n)){
      return -1;
    }
    fbuf f = {};
    if(fbuf_init(&f)){
      return -1;
    }
    if(sprite_draw(&n->tcache, NULL, np->sprite, &f, 0, xoff) < 0){
      return -1;
    }
    if(sprite_commit(&n->tcache, &f, np->sprite, true)){
      fbuf_free(&f);
      return -1;
    }
    if(fbuf_finalize(&f, n->ttyfp)){
      return -1;
    }
    return 0;
  }
//fprintf(stderr, "rasterizing %dx%d+%d\n", dimy, dimx, xoff);
  // save the existing style and colors
  const bool fgdefault = ncdirect_fg_default_p(n);
  const bool bgdefault = ncdirect_bg_default_p(n);
  const uint32_t fgrgb = ncchannels_fg_rgb(n->channels);
  const uint32_t bgrgb = ncchannels_bg_rgb(n->channels);
  for(int y = 0 ; y < dimy ; ++y){
    if(xoff){
      if(ncdirect_cursor_move_yx(n, -1, xoff)){
        return -1;
      }
    }
    for(int x = 0 ; x < dimx ; ++x){
      uint16_t stylemask;
      uint64_t channels;
      char* egc = ncplane_at_yx(np, y, x, &stylemask, &channels);
      if(egc == NULL){
        return -1;
      }
      if(ncchannels_fg_alpha(channels) == NCALPHA_TRANSPARENT){
        ncdirect_set_fg_default(n);
      }else{
        ncdirect_set_fg_rgb(n, ncchannels_fg_rgb(channels));
      }
      if(ncchannels_bg_alpha(channels) == NCALPHA_TRANSPARENT){
        ncdirect_set_bg_default(n);
      }else{
        ncdirect_set_bg_rgb(n, ncchannels_bg_rgb(channels));
      }
//fprintf(stderr, "%03d/%03d [%s] (%03dx%03d)\n", y, x, egc, dimy, dimx);
      if(fprintf(n->ttyfp, "%s", strlen(egc) == 0 ? " " : egc) < 0){
        free(egc);
        return -1;
      }
      free(egc);
    }
    // yes, we want to reset colors and emit an explicit new line following
    // each line of output; this is necessary if our output is lifted out and
    // used in something e.g. paste(1).
    // FIXME replace with a SGR clear
    ncdirect_set_fg_default(n);
    ncdirect_set_bg_default(n);
    if(ncfputc('\n', n->ttyfp) == EOF){
      return -1;
    }
    if(y == toty){
      if(ncdirect_cursor_down(n, 1)){
        return -1;
      }
    }
  }
  // restore the previous colors
  if(fgdefault){
    ncdirect_set_fg_default(n);
  }else{
    ncdirect_set_fg_rgb(n, fgrgb);
  }
  if(bgdefault){
    ncdirect_set_bg_default(n);
  }else{
    ncdirect_set_bg_rgb(n, bgrgb);
  }
  return 0;
}

int ncdirect_raster_frame(ncdirect* n, ncdirectv* ncdv, ncalign_e align){
  int lenx = ncplane_dim_x(ncdv);
  int xoff = ncdirect_align(n, align, lenx);
  if(ncdirect_dump_plane(n, ncdv, xoff)){
    free_plane(ncdv);
    return -1;
  }
  int r = ncdirect_flush(n);
  free_plane(ncdv);
  return r;
}

static ncdirectv*
ncdirect_render_visual(ncdirect* n, ncvisual* ncv,
                       const struct ncvisual_options* vopts){
  struct ncvisual_options defvopts = {};
  if(!vopts){
    vopts = &defvopts;
  }
  if(vopts->leny < 0 || vopts->lenx < 0){
    fprintf(stderr, "Invalid render geometry %d/%d\n", vopts->leny, vopts->lenx);
    return NULL;
  }
//fprintf(stderr, "OUR DATA: %p rows/cols: %d/%d outsize: %d/%d %d/%d\n", ncv->data, ncv->pixy, ncv->pixx, dimy, dimx, ymax, xmax);
//fprintf(stderr, "render %d/%d to scaling: %d\n", ncv->pixy, ncv->pixx, vopts->scaling);
  const struct blitset* bset = rgba_blitter_low(&n->tcache, vopts->scaling,
                                                !(vopts->flags & NCVISUAL_OPTION_NODEGRADE),
                                                vopts->blitter);
  if(!bset){
    return NULL;
  }
  int ymax = vopts->leny / bset->height;
  int xmax = vopts->lenx / bset->width;
  int dimy = vopts->leny > 0 ? ymax : ncdirect_dim_y(n);
  int dimx = vopts->lenx > 0 ? xmax : ncdirect_dim_x(n);
  int disprows, dispcols, outy;
  if(vopts->scaling != NCSCALE_NONE && vopts->scaling != NCSCALE_NONE_HIRES){
    if(bset->geom != NCBLIT_PIXEL){
      dispcols = dimx * encoding_x_scale(&n->tcache, bset);
      disprows = dimy * encoding_y_scale(&n->tcache, bset) - 1;
      outy = disprows;
    }else{
      dispcols = dimx * n->tcache.cellpixx;
      disprows = dimy * n->tcache.cellpixy;
      clamp_to_sixelmax(&n->tcache, &disprows, &dispcols, &outy, vopts->scaling);
    }
    if(vopts->scaling == NCSCALE_SCALE || vopts->scaling == NCSCALE_SCALE_HIRES){
      scale_visual(ncv, &disprows, &dispcols);
      outy = disprows;
      if(bset->geom == NCBLIT_PIXEL){
        clamp_to_sixelmax(&n->tcache, &disprows, &dispcols, &outy, vopts->scaling);
      }
    }
  }else{
    disprows = ncv->pixy;
    dispcols = ncv->pixx;
    if(bset->geom == NCBLIT_PIXEL){
      clamp_to_sixelmax(&n->tcache, &disprows, &dispcols, &outy, vopts->scaling);
    }else{
      outy = disprows;
    }
  }
  if(bset->geom == NCBLIT_PIXEL){
    while((outy + n->tcache.cellpixy - 1) / n->tcache.cellpixy > dimy){
      outy -= n->tcache.sprixel_scale_height;
      disprows = outy;
    }
  }
//fprintf(stderr, "max: %d/%d out: %d/%d\n", ymax, xmax, outy, dispcols);
//fprintf(stderr, "render: %d/%d stride %u %p\n", ncv->pixy, ncv->pixx, ncv->rowstride, ncv->data);
  ncplane_options nopts = {
    .y = 0,
    .x = 0,
    .rows = outy / encoding_y_scale(&n->tcache, bset),
    .cols = dispcols / encoding_x_scale(&n->tcache, bset),
    .userptr = NULL,
    .name = "fake",
    .resizecb = NULL,
    .flags = 0,
  };
  if(bset->geom == NCBLIT_PIXEL){
    nopts.rows = outy / n->tcache.cellpixy + !!(outy % n->tcache.cellpixy);
    nopts.cols = dispcols / n->tcache.cellpixx + !!(dispcols % n->tcache.cellpixx);
  }
  if(ymax && nopts.rows > ymax){
    nopts.rows = ymax;
  }
  if(xmax && nopts.cols > xmax){
    nopts.cols = xmax;
  }
  struct ncplane* ncdv = ncplane_new_internal(NULL, NULL, &nopts);
  if(!ncdv){
    return NULL;
  }
  blitterargs bargs = {};
  bargs.flags = vopts->flags;
  if(vopts->flags & NCVISUAL_OPTION_ADDALPHA){
    bargs.transcolor = vopts->transcolor | 0x1000000ull;
  }
  if(bset->geom == NCBLIT_PIXEL){
    bargs.u.pixel.colorregs = n->tcache.color_registers;
    if((bargs.u.pixel.spx = sprixel_alloc(&n->tcache, ncdv, nopts.rows, nopts.cols)) == NULL){
      free_plane(ncdv);
      return NULL;
    }
    ncdv->sprite = bargs.u.pixel.spx;
  }
  if(ncvisual_blit(ncv, disprows, dispcols, ncdv, bset, &bargs)){
    free_plane(ncdv);
    return NULL;
  }
  return ncdv;
}

ncdirectv* ncdirect_render_frame(ncdirect* n, const char* file,
                                 ncblitter_e blitfxn, ncscale_e scale,
                                 int ymax, int xmax){
  if(ymax < 0 || xmax < 0){
    return NULL;
  }
  ncdirectf* ncv = ncdirectf_from_file(n, file);
  if(ncv == NULL){
    return NULL;
  }
  struct ncvisual_options vopts = {};
  const struct blitset* bset = rgba_blitter_low(&n->tcache, scale, true, blitfxn);
  if(!bset){
    return NULL;
  }
  vopts.blitter = bset->geom;
  vopts.flags = NCVISUAL_OPTION_NODEGRADE;
  vopts.scaling = scale;
  if(ymax > 0){
    if((vopts.leny = ymax * bset->height) > ncv->pixy){
      vopts.leny = 0;
    }
  }
  if(xmax > 0){
    if((vopts.lenx = xmax * bset->width) > ncv->pixx){
      vopts.lenx = 0;
    }
  }
  ncdirectv* v = ncdirectf_render(n, ncv, &vopts);
  ncvisual_destroy(ncv);
  return v;
}

int ncdirect_render_image(ncdirect* n, const char* file, ncalign_e align,
                          ncblitter_e blitfxn, ncscale_e scale){
  ncdirectv* faken = ncdirect_render_frame(n, file, blitfxn, scale, 0, 0);
  if(!faken){
    return -1;
  }
  return ncdirect_raster_frame(n, faken, align);
}

int ncdirect_set_fg_palindex(ncdirect* nc, int pidx){
  const char* setaf = get_escape(&nc->tcache, ESCAPE_SETAF);
  if(!setaf){
    return -1;
  }
  if(ncchannels_set_fg_palindex(&nc->channels, pidx) < 0){
    return -1;
  }
  return term_emit(tiparm(setaf, pidx), nc->ttyfp, false);
}

int ncdirect_set_bg_palindex(ncdirect* nc, int pidx){
  const char* setab = get_escape(&nc->tcache, ESCAPE_SETAB);
  if(!setab){
    return -1;
  }
  if(ncchannels_set_bg_palindex(&nc->channels, pidx) < 0){
    return -1;
  }
  return term_emit(tiparm(setab, pidx), nc->ttyfp, false);
}

int ncdirect_vprintf_aligned(ncdirect* n, int y, ncalign_e align, const char* fmt, va_list ap){
  char* r = ncplane_vprintf_prep(fmt, ap);
  if(r == NULL){
    return -1;
  }
  const size_t len = ncstrwidth(r);
  const int x = ncdirect_align(n, align, len);
  if(ncdirect_cursor_move_yx(n, y, x)){
    free(r);
    return -1;
  }
  int ret = puts(r);
  free(r);
  if(ret == EOF){
    return -1;
  }
  return ret;
}

int ncdirect_printf_aligned(ncdirect* n, int y, ncalign_e align, const char* fmt, ...){
  va_list va;
  va_start(va, fmt);
  int ret = ncdirect_vprintf_aligned(n, y, align, fmt, va);
  va_end(va);
  return ret;
}

static int
ncdirect_stop_minimal(void* vnc){
  ncdirect* nc = vnc;
  int ret = drop_signals(nc);
  if(nc->initialized_readline){
#ifdef USE_READLINE
    rl_deprep_terminal();
#endif
  }
  fbuf f = {};
  if(fbuf_init_small(&f) == 0){
    if(nc->tcache.pixel_shutdown){
      ret |= nc->tcache.pixel_shutdown(&f);
    }
    ret |= reset_term_attributes(&nc->tcache, &f);
  }
  if(nc->tcache.ttyfd >= 0){
    const char* cnorm = get_escape(&nc->tcache, ESCAPE_CNORM);
    if(cnorm && tty_emit(cnorm, nc->tcache.ttyfd)){
      ret = -1;
    }
    ret |= tcsetattr(nc->tcache.ttyfd, TCSANOW, &nc->tcache.tpreserved);
    ret |= close(nc->tcache.ttyfd);
  }
  ret |= ncdirect_flush(nc);
  free_terminfo_cache(&nc->tcache);
  return ret;
}

ncdirect* ncdirect_core_init(const char* termtype, FILE* outfp, uint64_t flags){
  if(outfp == NULL){
    outfp = stdout;
  }
  if(flags > (NCDIRECT_OPTION_VERY_VERBOSE << 1)){ // allow them through with warning
    logwarn("Passed unsupported flags 0x%016jx\n", (uintmax_t)flags);
  }
  ncdirect* ret = malloc(sizeof(ncdirect));
  if(ret == NULL){
    return ret;
  }
  memset(ret, 0, sizeof(*ret));
  ret->flags = flags;
  ret->ttyfp = outfp;
  if(!(flags & NCDIRECT_OPTION_INHIBIT_SETLOCALE)){
    init_lang();
  }
  const char* encoding = nl_langinfo(CODESET);
  bool utf8 = false;
  if(encoding && strcmp(encoding, "UTF-8") == 0){
    utf8 = true;
  }
  if(setup_signals(ret, (flags & NCDIRECT_OPTION_NO_QUIT_SIGHANDLERS),
                   true, ncdirect_stop_minimal)){
    free(ret);
    return NULL;
  }
  // don't set the loglevel until we've locked in signal handling, lest we
  // change the loglevel out from under a running instance.
  if(flags & NCDIRECT_OPTION_VERY_VERBOSE){
    loglevel = NCLOGLEVEL_TRACE;
  }else if(flags & NCDIRECT_OPTION_VERBOSE){
    loglevel = NCLOGLEVEL_WARNING;
  }else{
    loglevel = NCLOGLEVEL_SILENT;
  }
  int cursor_y = -1;
  int cursor_x = -1;
  if(interrogate_terminfo(&ret->tcache, termtype, ret->ttyfp, utf8,
                          1, flags & NCDIRECT_OPTION_INHIBIT_CBREAK,
                          TERMINAL_UNKNOWN, &cursor_y, &cursor_x, NULL)){
    goto err;
  }
  if(cursor_y >= 0){
    // the u7 led the queries so that we would get a cursor position
    // unaffected by any query spill (unconsumed control sequences). move
    // us back to that location, in case there was any such spillage.
    if(ncdirect_cursor_move_yx(ret, cursor_y, cursor_x)){
      goto err;
    }
  }
  if(ncvisual_init(loglevel)){
    goto err;
  }
  update_term_dimensions(NULL, NULL, &ret->tcache, 0);
  ncdirect_set_styles(ret, 0);
  return ret;

err:
  if(ret->tcache.ttyfd >= 0){
    (void)tcsetattr(ret->tcache.ttyfd, TCSANOW, &ret->tcache.tpreserved);
  }
  drop_signals(ret);
  free(ret);
  return NULL;
}

int ncdirect_stop(ncdirect* nc){
  int ret = 0;
  if(nc){
    ret |= ncdirect_stop_minimal(nc);
    free(nc);
  }
  return ret;
}

char* ncdirect_readline(ncdirect* n, const char* prompt){
#ifdef USE_READLINE
  if(!n->initialized_readline){
    rl_outstream = n->ttyfp;
    rl_instream = stdin;
    rl_prep_terminal(1); // 1 == read 8-bit input
    n->initialized_readline = true;
  }
  return readline(prompt);
#else
  (void)n;
  (void)prompt;
  return NULL;
#endif
}

static inline int
ncdirect_style_emit(ncdirect* n, unsigned stylebits, fbuf* f){
  unsigned normalized = 0;
  int r = coerce_styles(f, &n->tcache, &n->stylemask, stylebits, &normalized);
  // sgr0 resets colors, so set them back up if not defaults and it was used
  if(normalized){
    // emitting an sgr resets colors. if we want to be default, that's no
    // problem, and our channels remain correct. otherwise, clear our
    // channel, and set them back up.
    if(!ncdirect_fg_default_p(n)){
      if(!ncdirect_fg_palindex_p(n)){
        uint32_t fg = ncchannels_fg_rgb(n->channels);
        ncchannels_set_fg_default(&n->channels);
        r |= ncdirect_set_fg_rgb(n, fg);
      }else{ // palette-indexed
        uint32_t fg = ncchannels_fg_palindex(n->channels);
        ncchannels_set_fg_default(&n->channels);
        r |= ncdirect_set_fg_palindex(n, fg);
      }
    }
    if(!ncdirect_bg_default_p(n)){
      if(!ncdirect_bg_palindex_p(n)){
        uint32_t bg = ncchannels_bg_rgb(n->channels);
        ncchannels_set_bg_default(&n->channels);
        r |= ncdirect_set_bg_rgb(n, bg);
      }else{ // palette-indexed
        uint32_t bg = ncchannels_bg_palindex(n->channels);
        ncchannels_set_bg_default(&n->channels);
        r |= ncdirect_set_bg_palindex(n, bg);
      }
    }
  }
  return r;
}

// turn on any specified stylebits
int ncdirect_styles_on(ncdirect* n, unsigned stylebits){
  return ncdirect_on_styles(n, stylebits);
}

int ncdirect_on_styles(ncdirect* n, unsigned stylebits){
  if((stylebits & n->tcache.supported_styles) < stylebits){ // unsupported styles
    return -1;
  }
  uint32_t stylemask = n->stylemask | stylebits;
  fbuf f = {};
  if(fbuf_init_small(&f)){
    return -1;
  }
  if(ncdirect_style_emit(n, stylemask, &f)){
    fbuf_free(&f);
    return -1;
  }
  if(fbuf_finalize(&f, n->ttyfp)){
    return -1;
  }
  return 0;
}

int ncdirect_styles_off(ncdirect* n, unsigned stylebits){
  return ncdirect_off_styles(n, stylebits);
}

unsigned ncdirect_styles(ncdirect* n){
  return n->stylemask;
}

// turn off any specified stylebits
int ncdirect_off_styles(ncdirect* n, unsigned stylebits){
  uint32_t stylemask = n->stylemask & ~stylebits;
  fbuf f = {};
  if(fbuf_init_small(&f)){
    return -1;
  }
  if(ncdirect_style_emit(n, stylemask, &f)){
    fbuf_free(&f);
    return -1;
  }
  if(fbuf_finalize(&f, n->ttyfp)){
    return -1;
  }
  return 0;
}

int ncdirect_styles_set(ncdirect* n, unsigned stylebits){
  return ncdirect_set_styles(n, stylebits);
}

// set the current stylebits to exactly those provided
int ncdirect_set_styles(ncdirect* n, unsigned stylebits){
  if((stylebits & n->tcache.supported_styles) < stylebits){ // unsupported styles
    return -1;
  }
  uint32_t stylemask = stylebits;
  fbuf f = {};
  if(fbuf_init_small(&f)){
    return -1;
  }
  if(ncdirect_style_emit(n, stylemask, &f)){
    fbuf_free(&f);
    return -1;
  }
  if(fbuf_finalize(&f, n->ttyfp)){
    return -1;
  }
  return 0;
}

unsigned ncdirect_palette_size(const ncdirect* nc){
  return ncdirect_capabilities(nc)->colors;
}

int ncdirect_set_fg_default(ncdirect* nc){
  if(ncdirect_fg_default_p(nc)){
    return 0;
  }
  const char* esc;
  if((esc = get_escape(&nc->tcache, ESCAPE_FGOP)) != NULL){
    if(term_emit(esc, nc->ttyfp, false)){
      return -1;
    }
  }else if((esc = get_escape(&nc->tcache, ESCAPE_OP)) != NULL){
    if(term_emit(esc, nc->ttyfp, false)){
      return -1;
    }
    if(!ncdirect_bg_default_p(nc)){
      if(ncdirect_set_bg_rgb(nc, ncchannels_bg_rgb(nc->channels))){
        return -1;
      }
    }
  }
  ncchannels_set_fg_default(&nc->channels);
  return 0;
}

int ncdirect_set_bg_default(ncdirect* nc){
  if(ncdirect_bg_default_p(nc)){
    return 0;
  }
  const char* esc;
  if((esc = get_escape(&nc->tcache, ESCAPE_BGOP)) != NULL){
    if(term_emit(esc, nc->ttyfp, false)){
      return -1;
    }
  }else if((esc = get_escape(&nc->tcache, ESCAPE_OP)) != NULL){
    if(term_emit(esc, nc->ttyfp, false)){
      return -1;
    }
    if(!ncdirect_fg_default_p(nc)){
      if(ncdirect_set_fg_rgb(nc, ncchannels_fg_rgb(nc->channels))){
        return -1;
      }
    }
  }
  ncchannels_set_bg_default(&nc->channels);
  return 0;
}

int ncdirect_hline_interp(ncdirect* n, const char* egc, int len,
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
  bool fgdef = false, bgdef = false;
  if(ncchannels_fg_default_p(c1) && ncchannels_fg_default_p(c2)){
    if(ncdirect_set_fg_default(n)){
      return -1;
    }
    fgdef = true;
  }
  if(ncchannels_bg_default_p(c1) && ncchannels_bg_default_p(c2)){
    if(ncdirect_set_bg_default(n)){
      return -1;
    }
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
      ncdirect_set_fg_rgb8(n, r, g, b);
    }
    if(!bgdef){
      ncdirect_set_bg_rgb8(n, br, bg, bb);
    }
    if(fprintf(n->ttyfp, "%s", egc) < 0){
      break;
    }
  }
  return ret;
}

int ncdirect_vline_interp(ncdirect* n, const char* egc, int len,
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
  int ret;
  bool fgdef = false, bgdef = false;
  if(ncchannels_fg_default_p(c1) && ncchannels_fg_default_p(c2)){
    if(ncdirect_set_fg_default(n)){
      return -1;
    }
    fgdef = true;
  }
  if(ncchannels_bg_default_p(c1) && ncchannels_bg_default_p(c2)){
    if(ncdirect_set_bg_default(n)){
      return -1;
    }
    bgdef = true;
  }
  for(ret = 0 ; ret < len ; ++ret){
    r1 += deltr;
    g1 += deltg;
    b1 += deltb;
    br1 += deltbr;
    bg1 += deltbg;
    bb1 += deltbb;
    uint64_t channels = 0;
    if(!fgdef){
      ncchannels_set_fg_rgb8(&channels, r1, g1, b1);
    }
    if(!bgdef){
      ncchannels_set_bg_rgb8(&channels, br1, bg1, bb1);
    }
    if(ncdirect_putstr(n, channels, egc) <= 0){
      break;
    }
    if(len - ret > 1){
      if(ncdirect_cursor_down(n, 1) || ncdirect_cursor_left(n, 1)){
        break;
      }
    }
  }
  return ret;
}

//  wchars: wchar_t[6] mapping to UL, UR, BL, BR, HL, VL.
//  they cannot be complex EGCs, but only a single wchar_t, alas.
int ncdirect_box(ncdirect* n, uint64_t ul, uint64_t ur,
                 uint64_t ll, uint64_t lr, const wchar_t* wchars,
                 int ylen, int xlen, unsigned ctlword){
  if(xlen < 2 || ylen < 2){
    return -1;
  }
  char hl[WCHAR_MAX_UTF8BYTES + 1];
  char vl[WCHAR_MAX_UTF8BYTES + 1];
  unsigned edges;
  edges = !(ctlword & NCBOXMASK_TOP) + !(ctlword & NCBOXMASK_LEFT);
  if(edges >= box_corner_needs(ctlword)){
    if(activate_channels(n, ul)){
      return -1;
    }
    if(fprintf(n->ttyfp, "%lc", wchars[0]) < 0){
      return -1;
    }
  }else{
    ncdirect_cursor_right(n, 1);
  }
  mbstate_t ps = {};
  size_t bytes;
  if((bytes = wcrtomb(hl, wchars[4], &ps)) == (size_t)-1){
    return -1;
  }
  hl[bytes] = '\0';
  memset(&ps, 0, sizeof(ps));
  if((bytes = wcrtomb(vl, wchars[5], &ps)) == (size_t)-1){
    return -1;
  }
  vl[bytes] = '\0';
  if(!(ctlword & NCBOXMASK_TOP)){ // draw top border, if called for
    if(xlen > 2){
      if(ncdirect_hline_interp(n, hl, xlen - 2, ul, ur) < 0){
        return -1;
      }
    }
  }else{
    ncdirect_cursor_right(n, xlen - 2);
  }
  edges = !(ctlword & NCBOXMASK_TOP) + !(ctlword & NCBOXMASK_RIGHT);
  if(edges >= box_corner_needs(ctlword)){
    if(activate_channels(n, ur)){
      return -1;
    }
    if(fprintf(n->ttyfp, "%lc", wchars[1]) < 0){
      return -1;
    }
    ncdirect_cursor_left(n, xlen);
  }else{
    ncdirect_cursor_left(n, xlen - 1);
  }
  ncdirect_cursor_down(n, 1);
  // middle rows (vertical lines)
  if(ylen > 2){
    if(!(ctlword & NCBOXMASK_LEFT)){
      if(ncdirect_vline_interp(n, vl, ylen - 2, ul, ll) < 0){
        return -1;
      }
      ncdirect_cursor_right(n, xlen - 2);
      ncdirect_cursor_up(n, ylen - 3);
    }else{
      ncdirect_cursor_right(n, xlen - 1);
    }
    if(!(ctlword & NCBOXMASK_RIGHT)){
      if(ncdirect_vline_interp(n, vl, ylen - 2, ur, lr) < 0){
        return -1;
      }
      ncdirect_cursor_left(n, xlen);
    }else{
      ncdirect_cursor_left(n, xlen - 1);
    }
  }
  ncdirect_cursor_down(n, 1);
  // bottom line
  edges = !(ctlword & NCBOXMASK_BOTTOM) + !(ctlword & NCBOXMASK_LEFT);
  if(edges >= box_corner_needs(ctlword)){
    if(activate_channels(n, ll)){
      return -1;
    }
    if(fprintf(n->ttyfp, "%lc", wchars[2]) < 0){
      return -1;
    }
  }else{
    ncdirect_cursor_right(n, 1);
  }
  if(!(ctlword & NCBOXMASK_BOTTOM)){
    if(xlen > 2){
      if(ncdirect_hline_interp(n, hl, xlen - 2, ll, lr) < 0){
        return -1;
      }
    }
  }else{
    ncdirect_cursor_right(n, xlen - 2);
  }
  edges = !(ctlword & NCBOXMASK_BOTTOM) + !(ctlword & NCBOXMASK_RIGHT);
  if(edges >= box_corner_needs(ctlword)){
    if(activate_channels(n, lr)){
      return -1;
    }
    if(fprintf(n->ttyfp, "%lc", wchars[3]) < 0){
      return -1;
    }
  }
  return 0;
}

int ncdirect_rounded_box(ncdirect* n, uint64_t ul, uint64_t ur,
                         uint64_t ll, uint64_t lr,
                         int ylen, int xlen, unsigned ctlword){
  return ncdirect_box(n, ul, ur, ll, lr, NCBOXROUNDW, ylen, xlen, ctlword);
}

int ncdirect_double_box(ncdirect* n, uint64_t ul, uint64_t ur,
                         uint64_t ll, uint64_t lr,
                         int ylen, int xlen, unsigned ctlword){
  return ncdirect_box(n, ul, ur, ll, lr, NCBOXDOUBLEW, ylen, xlen, ctlword);
}

// Can we load images? This requires being built against FFmpeg/OIIO.
bool ncdirect_canopen_images(const ncdirect* n __attribute__ ((unused))){
  return notcurses_canopen_images(NULL);
}

// Is our encoding UTF-8? Requires LANG being set to a UTF8 locale.
bool ncdirect_canutf8(const ncdirect* n){
  return n->tcache.caps.utf8;
}

int ncdirect_flush(const ncdirect* nc){
  return ncflush(nc->ttyfp);
}

int ncdirect_check_pixel_support(const ncdirect* n){
  if(n->tcache.pixel_draw){
    return 1;
  }
  return 0;
}

int ncdirect_stream(ncdirect* n, const char* filename, ncstreamcb streamer,
                    struct ncvisual_options* vopts, void* curry){
  ncvisual* ncv = ncvisual_from_file(filename);
  if(ncv == NULL){
    return -1;
  }
  // starting position *after displaying one frame* so as to effect any
  // necessary scrolling.
  int y = -1, x = -1;
  int lastid = -1;
  int thisid = -1;
  do{
    if(y > 0){
      if(x == ncdirect_dim_x(n)){
        x = 0;
        ++y;
      }
      ncdirect_cursor_up(n, y - 1);
    }
    if(x > 0){
      ncdirect_cursor_left(n, x);
    }
    ncdirectv* v = ncdirect_render_visual(n, ncv, vopts);
    if(v == NULL){
      ncvisual_destroy(ncv);
      return -1;
    }
    ncplane_dim_yx(v, &y, &x);
    if(v->sprite){
      thisid = v->sprite->id;
    }
    ncdirect_raster_frame(n, v, (vopts->flags & NCVISUAL_OPTION_HORALIGNED) ? vopts->x : 0);
    if(lastid > -1){
      if(n->tcache.pixel_remove){
        fbuf f = {};
        fbuf_init_small(&f);
        if(n->tcache.pixel_remove(lastid, &f) || fwrite(f.buf, f.used, 1, n->ttyfp) != 1){
          fbuf_free(&f);
          ncvisual_destroy(ncv);
          return -1;
        }
        fbuf_free(&f);
      }
    }
    streamer(ncv, vopts, NULL, curry);
    lastid = thisid;
  }while(ncvisual_decode(ncv) == 0);
  ncdirect_flush(n);
  ncvisual_destroy(ncv);
  return 0;
}

ncdirectf* ncdirectf_from_file(ncdirect* n __attribute__ ((unused)),
                               const char* filename){
  return ncvisual_from_file(filename);
}

void ncdirectf_free(ncdirectf* frame){
  ncvisual_destroy(frame);
}

ncdirectv* ncdirectf_render(ncdirect* n, ncdirectf* frame, const struct ncvisual_options* vopts){
  return ncdirect_render_visual(n, frame, vopts);
}

int ncdirectf_geom(ncdirect* n, ncdirectf* frame,
                   const struct ncvisual_options* vopts, ncvgeom* geom){
  geom->cdimy = n->tcache.cellpixy;
  geom->cdimx = n->tcache.cellpixx;
  geom->maxpixely = n->tcache.sixel_maxy;
  geom->maxpixelx = n->tcache.sixel_maxx;
  const struct blitset* bset;
  int r = ncvisual_blitset_geom(NULL, &n->tcache, frame, vopts,
                                &geom->pixy, &geom->pixx,
                                &geom->scaley, &geom->scalex,
                                &geom->rpixy, &geom->rpixx, &bset);
  if(r == 0){
    // FIXME ncvisual_blitset_geom() ought calculate these two for us; until
    // then, derive them ourselves. the row count might be short by one if
    // we're using sixel, and we're not a multiple of 6
    geom->rcelly = geom->pixy / geom->scaley;
    geom->rcellx = geom->pixx / geom->scalex;
    geom->blitter = bset->geom;
  }
  return r;
}

unsigned ncdirect_supported_styles(const ncdirect* nc){
  return term_supported_styles(&nc->tcache);
}

char* ncdirect_detected_terminal(const ncdirect* nc){
  return termdesc_longterm(&nc->tcache);
}

const nccapabilities* ncdirect_capabilities(const ncdirect* n){
  return &n->tcache.caps;
}

bool ncdirect_canget_cursor(const ncdirect* n){
  if(get_escape(&n->tcache, ESCAPE_U7) == NULL){
    return false;
  }
  if(n->tcache.ttyfd < 0){
    return false;
  }
  return true;
}
