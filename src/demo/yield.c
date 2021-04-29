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
  // can we do bitmaps?
  const bool bitmaps = (notcurses_check_pixel_support(nc) > 0);
  struct ncplane_options nopts = {
    .y = 1,
    .rows = dimy - 1 - bitmaps,
    .cols = dimx,
    .name = "wmap",
  };
  struct ncvisual_options vopts = {
    .scaling = NCSCALE_STRETCH,
    .blitter = NCBLIT_PIXEL,
  };
  vopts.n = ncplane_create(std, &nopts);
  if(vopts.n == NULL){
    ncvisual_destroy(wmv);
    return -1;
  }
  int maxy, maxx;
  if(ncvisual_blitter_geom(nc, wmv, &vopts, &maxy, &maxx, NULL, NULL, NULL)){
    ncvisual_destroy(wmv);
    ncplane_destroy(vopts.n);
    return -1;
  }
  if(ncvisual_render(nc, wmv, &vopts) == NULL){
    ncvisual_destroy(wmv);
    ncplane_destroy(vopts.n);
    return -1;
  }
  struct timespec scaled;
  const long total = maxy * maxx;
  // less than this, and we exit almost immediately. more than this, and we
  // run closer to twenty seconds. 11/50 it is, then. pixels are different.
  const long threshold_painted = total * 11 / 50;
  const int MAXITER = 1024;
  timespec_div(&demodelay, 10, &scaled);
  long tfilled = 0;
  struct ncplane_options labopts = {
    .y = 3,
    .x = NCALIGN_CENTER,
    .rows = 1,
    .cols = 13,
    .name = "pcnt",
    .flags = NCPLANE_OPTION_HORALIGNED,
  };
  struct ncplane* label = ncplane_create(std, &labopts);
  if(label == NULL){
    ncvisual_destroy(wmv);
    ncplane_destroy(vopts.n);
    return -1;
  }
  uint64_t basechan = 0;
  ncchannels_set_bg_alpha(&basechan, CELL_ALPHA_TRANSPARENT);
  ncchannels_set_fg_alpha(&basechan, CELL_ALPHA_TRANSPARENT);
  ncplane_set_base(label, "", 0, basechan);
  ncplane_set_bg_alpha(label, CELL_ALPHA_TRANSPARENT);
  ncplane_set_fg_rgb8(label, 0xff, 0xff, 0xff);
  ncplane_set_styles(label, NCSTYLE_BOLD);
  ncplane_printf_aligned(label, 0, NCALIGN_CENTER, "Yield: %03.1f%%", ((double)tfilled * 100) / threshold_painted);

  DEMO_RENDER(nc);
  demo_nanosleep(nc, &demodelay);

  int iters = 0;
  while(tfilled < threshold_painted && iters < MAXITER){
//fprintf(stderr, "%d/%d tfilled: %ld thresh: %ld total: %ld\n", maxy, maxx, tfilled, threshold_painted, total);
    int pfilled = 0;
    do{
      ++iters;
      int x = random() % maxx;
      int y = random() % maxy;
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
    if(!bitmaps){
      demo_nanosleep(nc, &scaled);
    }
  }
  ncplane_destroy(label);
  ncvisual_destroy(wmv);
  ncplane_destroy(vopts.n);
  ncplane_erase(std);
  return 0;
}
