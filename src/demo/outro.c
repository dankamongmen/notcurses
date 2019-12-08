#include "demo.h"

static int
outro_message(struct notcurses* nc, int rows, int cols){
  const char str0[] = " ATL, baby! ATL! ";
  const char str1[] = " much, much more is coming ";
  const char str2[] = " hack on! —dank❤ ";
  struct ncplane* on = notcurses_newplane(nc, 5, strlen(str1) + 4, rows - 6,
                                         (cols - (strlen(str1) + 4)) / 2, NULL);
  if(on == NULL){
    return -1;
  }
  cell bgcell = CELL_TRIVIAL_INITIALIZER;
  notcurses_bg_prep(&bgcell.channels, 0x58, 0x36, 0x58);
  ncplane_set_background(on, &bgcell);
  ncplane_dim_yx(on, &rows, &cols);
  int ybase = 0;
  // bevel the upper corners
  uint64_t channel = 0;
  if(notcurses_bg_set_alpha(&channel, 3)){
    return -1;
  }
  if(ncplane_cursor_move_yx(on, ybase, 0)){
    return -1;
  }
  if(ncplane_putsimple(on, ' ', 0, channel) < 0 || ncplane_putsimple(on, ' ', 0, channel) < 0){
    return -1;
  }
  if(ncplane_cursor_move_yx(on, ybase, cols - 2)){
    return -1;
  }
  if(ncplane_putsimple(on, ' ', 0, channel) < 0 || ncplane_putsimple(on, ' ', 0, channel) < 0){
    return -1;
  }
  // ...and now the lower corners
  if(ncplane_cursor_move_yx(on, rows - 1, 0)){
    return -1;
  }
  if(ncplane_putsimple(on, ' ', 0, channel) < 0 || ncplane_putsimple(on, ' ', 0, channel) < 0){
    return -1;
  }
  if(ncplane_cursor_move_yx(on, rows - 1, cols - 2)){
    return -1;
  }
  if(ncplane_putsimple(on, ' ', 0, channel) < 0 || ncplane_putsimple(on, ' ', 0, channel) < 0){
    return -1;
  }
  if(ncplane_set_fg_rgb(on, 0, 0, 0)){
    return -1;
  }
  if(ncplane_set_bg_rgb(on, 0, 180, 180)){
    return -1;
  }
  if(ncplane_cursor_move_yx(on, ++ybase, (cols - strlen(str0)) / 2)){
    return -1;
  }
  if(ncplane_putstr(on, str0) < 0){
    return -1;
  }
  if(ncplane_cursor_move_yx(on, ++ybase, (cols - strlen(str1)) / 2)){
    return -1;
  }
  if(ncplane_putstr(on, str1) < 0){
    return -1;
  }
  if(ncplane_cursor_move_yx(on, ++ybase, (cols - (strlen(str2) - 4)) / 2)){
    return -1;
  }
  if(ncplane_putstr(on, str2) < 0){
    return -1;
  }
  if(notcurses_render(nc)){
    return -1;
  }
  cell_release(on, &bgcell);
  return 0;
}

int outro(struct notcurses* nc){
  struct ncplane* ncp;
  if((ncp = notcurses_stdplane(nc)) == NULL){
    return -1;
  }
  int rows, cols;
  ncplane_erase(ncp);
  ncplane_dim_yx(ncp, &rows, &cols);
  int averr = 0;
  struct ncvisual* ncv = ncplane_visual_open(ncp, "../tests/changes.jpg", &averr);
  if(ncv == NULL){
    return -1;
  }
  if(ncvisual_decode(ncv, &averr) == NULL){
    ncvisual_destroy(ncv);
    return -1;
  }
  if(ncvisual_render(ncv)){
    ncvisual_destroy(ncv);
    return -1;
  }
  int ret = outro_message(nc, rows, cols);
  if(ret == 0){
    struct timespec fade = { .tv_sec = 5, .tv_nsec = 0, };
    ret |= ncplane_fadeout(ncp, &fade);
  }
  ncvisual_destroy(ncv);
  return ret;
}
