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
  const long total = vy * vx;
  const long threshold_painted = total * 4 / 5;
  timespec_div(&demodelay, 128, &scaled);
  long tfilled = 0;
  while(tfilled < threshold_painted){
//fprintf(stderr, "tfilled: %ld thresh: %ld total: %ld\n", tfilled, threshold_painted, total);
    int pfilled = 0;
    do{
      int x = random() % (vx);
      int y = random() % (vy);
      uint32_t pixel = 0;
      if(ncvisual_at_yx(wmv, y, x, &pixel) < 0){
        ncvisual_destroy(wmv);
        return -1;
      }
      if(ncpixel_a(pixel) != 0xff){
        continue;
      }
      ncpixel_set_a(&pixel, 0xfe);
      ncpixel_set_rgb(&pixel, random() % 256, 0, ncpixel_b(pixel) / 2);
      pfilled = ncvisual_polyfill_yx(wmv, y, x, pixel);
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
  }

  ncvisual_destroy(wmv);
  return 0;
}
