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

  ncplane_erase(std);

  int vy, vx, vscaley, vscalex;
  ncvisual_geom(nc, wmv, vopts.blitter, &vy, &vx, &vscaley, &vscalex);
  struct timespec scaled;
  int threshold_painted = vy * vx * 10 / 9;
  const int ITER = 128;
  timespec_div(&demodelay, ITER, &scaled);
  int tfilled = 0;
  for(int i = 0 ; i < ITER ; ++i){
    int pfilled;
    do{
      int x = random() % (vx);
      int y = random() % (vy);
      uint32_t pixel = 0;
      ncvisual_at_yx(wmv, y, x, &pixel);
      uint32_t channel = 0;
      channel_set_rgb(&channel, 0x80, channel_g(pixel), channel_b(pixel));
//fprintf(stderr, "POLY: %d/%d\n", y, x);
      pfilled = ncvisual_polyfill_yx(wmv, y, x, channel);
      if(pfilled < 0){
        ncvisual_destroy(wmv);
        return -1;
      }
    }while(pfilled == 0);
    tfilled += pfilled;
    if(ncvisual_render(nc, wmv, &vopts) == NULL){
      ncvisual_destroy(wmv);
      return -1;
    }
    DEMO_RENDER(nc);
    demo_nanosleep(nc, &scaled);
    if(tfilled >= threshold_painted){
      break;
    }
  }

  ncvisual_destroy(wmv);
  return 0;
}
