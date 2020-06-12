#include "demo.h"

static void
set_colors(uint64_t* tl, uint64_t* tr, uint64_t* bl, uint64_t* br){
  channels_set_fg_rgb(tl, random() % 256, random() % 256, random() % 256);
  channels_set_bg_rgb(tl, 0, 0, 0);
  *tr = *tl;
  *bl = *tl;
  *br = *tl;
}

// start with a line. on each iteration, rotate a copy of what we have 90deg
// cw, and attach it to the end of what we drew. very easy in notcurses!
int dragon_demo(struct notcurses* nc){
  const int ITERATIONS = 10;
  int dimy, dimx;
  struct ncplane* n = notcurses_stddim_yx(nc, &dimy, &dimx);
  if(n == NULL){
    return -1;
  }
  cell c = CELL_TRIVIAL_INITIALIZER;
  if(cell_load(n, &c, "█") <= 0){
    return -1;
  }
  cell_set_fg_rgb(&c, 0, 0xff, 0);
  ncplane_cursor_move_yx(n, dimy / 2, dimx / 2);
  if(ncplane_vline(n, &c, 2) < 2){
    cell_release(n, &c);
    return -1;
  }
  cell_release(n, &c);
  DEMO_RENDER(nc);
  for(int iter = 0 ; iter < ITERATIONS ; ++iter){
    struct ncplane* newn = ncplane_dup(n, NULL);
    if(NULL == newn){
      return -1;
    }
    uint64_t channels = 0;
    channels_set_fg(&channels, 0);
    channels_set_bg(&channels, 0);
    channels_set_fg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
    channels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
    if(ncplane_set_base(newn, "", 0, channels) < 0){
      return -1;
    }
    /*uint64_t tl = 0, tr = 0, bl = 0, br = 0;
    set_colors(&tl, &tr, &bl, &br);
    ncplane_dim_yx(newn, &dimy, &dimx);
    ncplane_cursor_move_yx(newn, 0, 0);
    if(ncplane_stain(newn, dimy - 1, dimx - 1, tl, tr, bl, br) < 0){
      return -1;
    }*/
    if(ncplane_rotate_cw(newn)){
      return -1;
    }
    if(ncplane_resize_simple(newn, dimy, dimx) < 0){
      return -1;
    }
    int y, x;
    ncplane_yx(newn, &y, &x);
    ncplane_move_yx(newn, y - 1, x- 1);
    if(ncplane_mergedown(newn, n) < 0){
      return -1;
    }
    ncplane_destroy(newn);
    demo_nanosleep(nc, &demodelay);
    DEMO_RENDER(nc);
notcurses_debug(nc, stderr);
  }
  return 0;
}
