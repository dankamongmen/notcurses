#include "demo.h"

int yield_demo(struct notcurses* nc){
  if(!notcurses_canopen_images(nc)){
    return 0;
  }
  int dimy, dimx;
  struct ncplane* std = notcurses_stddim_yx(nc, &dimy, &dimx);
  // in sixel-based implementation, if we redraw each cycle, the underlying
  // material will be redrawn, taking time. erasing won't eliminate the
  // flicker, but it does minimize it.
  ncplane_erase(std);
  char* pic = find_data("worldmap.png");
  struct ncvisual* wmv = ncvisual_from_file(pic);
  free(pic);
  if(wmv == NULL){
    return -1;
  }
  ncscale_e scale = NCSCALE_STRETCH;
  struct ncplane_options nopts = {
    .y = 1,
    .rows = dimy - 2,
    .cols = dimx,
  };
  struct ncvisual_options vopts = {
    .scaling = scale,
    .blitter = NCBLIT_PIXEL,
  };
  vopts.n = ncplane_create(std, &nopts);
  if(vopts.n == NULL){
    ncvisual_destroy(wmv);
    return -1;
  }
  if(ncvisual_render(nc, wmv, &vopts) == NULL){
    ncplane_destroy(vopts.n);
    ncvisual_destroy(wmv);
    return -1;
  }
  
  int vy, vx, vscaley, vscalex;
  vopts.scaling = NCSCALE_NONE;
  ncvisual_geom(nc, wmv, &vopts, &vy, &vx, &vscaley, &vscalex);
  vopts.scaling = scale;
  struct timespec scaled;
  const long total = vy * vx;
  // less than this, and we exit almost immediately. more than this, and we
  // run closer to twenty seconds. 11/50 it is, then. pixels are different.
  // it would be nice to hit this all with a rigor stick. yes, the 1 makes
  // all the difference in cells v pixels. FIXME
  const long threshold_painted = total * (10 + (notcurses_check_pixel_support(nc) <= 0)) / 50;
  const int MAXITER = 256;
  timespec_div(&demodelay, MAXITER, &scaled);
  long tfilled = 0;

  struct ncplane_options labopts = {
    .y = 3,
    .x = NCALIGN_CENTER,
    .rows = 1,
    .cols = 12,
    .flags = NCPLANE_OPTION_HORALIGNED,
  };
  struct ncplane* label = ncplane_create(std, &labopts);
  if(label == NULL){
    ncvisual_destroy(wmv);
    ncplane_destroy(vopts.n);
    return -1;
  }
  uint64_t basechan = 0;
  channels_set_bg_alpha(&basechan, CELL_ALPHA_TRANSPARENT);
  channels_set_fg_alpha(&basechan, CELL_ALPHA_TRANSPARENT);
  ncplane_set_base(label, " ", NCSTYLE_BOLD, basechan);
  ncplane_set_bg_alpha(label, CELL_ALPHA_TRANSPARENT);
  ncplane_set_fg_rgb8(label, 0xff, 0xff, 0xff);
  ncplane_set_styles(label, NCSTYLE_BOLD);
  ncplane_printf_aligned(label, 0, NCALIGN_CENTER, "Yield: %03.1f%%", ((double)tfilled * 100) / threshold_painted);

  DEMO_RENDER(nc);
  struct timespec delay;
  timespec_div(&demodelay, 2, &delay);
  demo_nanosleep(nc, &delay);

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
        ncplane_destroy(vopts.n);
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
        ncplane_destroy(vopts.n);
        return -1;
      }
      // it's possible that nothing changed (pfilled == 0), but render anyway
      // so that it never looks like we locked up
      DEMO_RENDER(nc);
    }while(pfilled == 0);
    tfilled += pfilled;
    if(ncvisual_render(nc, wmv, &vopts) == NULL){
      ncvisual_destroy(wmv);
      ncplane_destroy(vopts.n);
      return -1;
    }
    if(tfilled > threshold_painted){
      tfilled = threshold_painted; // don't allow printing of 100.1% etc
    }
    ncplane_printf_aligned(label, 0, NCALIGN_CENTER, "Yield: %3.1f%%", ((double)tfilled * 100) / threshold_painted);
    DEMO_RENDER(nc);
    demo_nanosleep(nc, &scaled);
    ++iters;
  }
  ncplane_destroy(label);
  ncvisual_destroy(wmv);
  ncplane_destroy(vopts.n);
  ncplane_erase(std);
  return 0;
}
