#include "demo.h"

int yield_demo(struct notcurses* nc){
  if(!notcurses_canopen_images(nc)){
    return 0;
  }
  int dimy, dimx;
  struct ncplane* std = notcurses_stddim_yx(nc, &dimy, &dimx);
  char* pic = find_data("worldmap.png");
  struct ncvisual* wmv = ncvisual_from_file(pic);
  free(pic);
  if(wmv == NULL){
    return -1;
  }
  struct ncvisual_options vopts = {
    .n = std,
    .y = 1,
    .scaling = NCSCALE_STRETCH,
  };
  if(ncvisual_render(nc, wmv, &vopts) == NULL){
    ncvisual_destroy(wmv);
    return -1;
  }
  
  DEMO_RENDER(nc);
  demo_nanosleep(nc, &demodelay);

  ncplane_erase(std);

  int vy, vx, vscaley, vscalex;
  vopts.scaling = NCSCALE_NONE;
  ncvisual_geom(nc, wmv, &vopts, &vy, &vx, &vscaley, &vscalex);
  vopts.scaling = NCSCALE_STRETCH;
  struct timespec scaled;
  const long total = vy * vx;
  // less than this, and we exit almost immediately. more than this, and we
  // run closer to twenty seconds. 11/50 it is, then.
  const long threshold_painted = total * 11 / 50;
  const int MAXITER = 512;
  timespec_div(&demodelay, MAXITER, &scaled);
  long tfilled = 0;
  int iters = 0;
  while(tfilled < threshold_painted && iters < MAXITER){
//fprintf(stderr, "%d/%d x %d/%d tfilled: %ld thresh: %ld total: %ld\n", vy, vx, vscaley, vscalex, tfilled, threshold_painted, total);
    int pfilled = 0;
    do{
      int x = random() % (vx);
      int y = random() % (vy);
      uint32_t pixel = 0;
      if(ncvisual_at_yx(wmv, y, x, &pixel) < 0){
        ncvisual_destroy(wmv);
        return -1;
      }
      if(ncpixel_a(pixel) != 0xff){ // don't do areas we've already done
        continue;
      }
      if(ncpixel_g(pixel) < 0x80){ // only do land, which is whiter than blue
        continue;
      }
      ncpixel_set_a(&pixel, 0xfe);
      ncpixel_set_rgb8(&pixel, (random() % 128) + 128, 0, ncpixel_b(pixel) / 4);
      pfilled = ncvisual_polyfill_yx(wmv, y, x, pixel);
      if(pfilled < 0){
        ncvisual_destroy(wmv);
        return -1;
      }
      // it's possible that nothing changed (pfilled == 0), but render anyway
      // so that it never looks like we locked up
      DEMO_RENDER(nc);
    }while(pfilled == 0);
    tfilled += pfilled;
    if(ncvisual_render(nc, wmv, &vopts) == NULL){
      ncvisual_destroy(wmv);
      return -1;
    }
    ncplane_set_bg_rgb8(std, 0x10, 0x10, 0x10);
    ncplane_set_fg_rgb8(std, 0xf0, 0x20, 0x20);
    ncplane_set_styles(std, NCSTYLE_BOLD);
    if(tfilled > threshold_painted){
      tfilled = threshold_painted; // don't allow printing of 100.1% etc
    }
    ncplane_printf_aligned(std, 3, NCALIGN_CENTER, "Yield: %3.1f%%", ((double)tfilled * 100) / threshold_painted);
    ncplane_set_styles(std, NCSTYLE_NONE);
    DEMO_RENDER(nc);
    demo_nanosleep(nc, &scaled);
    ++iters;
  }
  ncvisual_destroy(wmv);
  ncplane_erase(std);
  return 0;
}
