#include "demo.h"

int yield_demo(struct notcurses* nc){
  if(!notcurses_canopen_images(nc)){
    return 0;
  }
  int dimy, dimx;
  struct ncplane* std = notcurses_stddim_yx(nc, &dimy, &dimx);
  char* pic = find_data("worldmap.png");
  nc_err_e err;
  struct ncvisual* wmv = ncvisual_from_file(pic, &err);
  free(pic);
  if(wmv == NULL){
    return -1;
  }
  struct ncvisual_options vopts = {
    .n = std,
    .scaling = NCSCALE_STRETCH,
    .blitter = NCBLIT_2x2,
  };
  if(ncvisual_render(nc, wmv, &vopts) == NULL){
    ncvisual_destroy(wmv);
    return -1;
  }
  DEMO_RENDER(nc);
  demo_nanosleep(nc, &demodelay);

  cell c = CELL_SIMPLE_INITIALIZER('*');
  cell_set_fg_rgb(&c, 0xff, 0, 0);
  cell_set_bg_rgb(&c, 0xff, 0, 0);
  for(int i = 0 ; i < 128 ; ++i){
    // FIXME
    // don't try to use polyfill; work directly on the ncvisual instead
  }
  cell_release(std, &c);

  ncvisual_destroy(wmv);
  return 0;
}
