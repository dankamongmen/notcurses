#include "demo.h"

static int
view_video_demo(struct notcurses* nc){
  int dimy, dimx;
  struct ncplane* ncp = notcurses_stddim_yx(nc, &dimy, &dimx);
  nc_err_e err;
  struct ncvisual* ncv;
  char* fm6 = find_data("fm6.mkv");
  ncv = ncplane_visual_open(ncp, fm6, &err);
  if(!ncv){
    free(fm6);
    return -1;
  }
  free(fm6);
  int ret = ncvisual_stream(nc, ncv, &err, 2.0/3.0 * delaymultiplier,
                            demo_simple_streamer, NULL);
  ncvisual_destroy(ncv);
  return ret;
}

static struct ncplane*
legend(struct notcurses* nc, int dimy, int dimx){
  struct ncplane* n = ncplane_new(nc, 4, 25, dimy * 7 / 8 - 1, (dimx - 25) / 2, NULL);
  ncplane_set_bg_alpha(n, CELL_ALPHA_TRANSPARENT);
  uint64_t channels = 0;
  channels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
  ncplane_set_base(n, " ", 0, channels);
  ncplane_styles_set(n, NCSTYLE_BOLD);
  ncplane_set_fg_rgb(n, 0xff, 0xff, 0xff);
  ncplane_set_fg_alpha(n, CELL_ALPHA_HIGHCONTRAST);
  if(ncplane_putstr_aligned(n, 0, NCALIGN_CENTER, "target launch") <= 0){
    ncplane_destroy(n);
    return NULL;
  }
  ncplane_set_fg_rgb(n, 0, 0, 0);
  if(ncplane_putstr_aligned(n, 1, NCALIGN_CENTER, "2003-12-11 FM-6") <= 0){
    ncplane_destroy(n);
    return NULL;
  }
  if(ncplane_putstr_aligned(n, 2, NCALIGN_CENTER, "RIM-161 SM-3 v. Aries TTV") <= 0){
    ncplane_destroy(n);
    return NULL;
  }
  ncplane_set_fg_rgb(n, 0x80, 0xc0, 0x80);
  if(ncplane_putstr_aligned(n, 3, NCALIGN_CENTER, "exo-atmospheric intercept") <= 0){
    ncplane_destroy(n);
    return NULL;
  }
  ncplane_styles_off(n, NCSTYLE_BOLD);
  return n;
}

// first, images scaled to the rendering area's size. we first put up the
// DSSCAW logo (opaquely), then the purple drinking menace (opaquely), then
// the DSSAW logo atop it (transparently). the purple fellow has no
// transparency, so he's always opaque.
static int
view_images(struct notcurses* nc, struct ncplane* nstd, int dimy, int dimx){
  ncplane_erase(nstd);
  // standard plane gets PurpleDrank (which will cover the plane), but first
  // serves as a blocker behind dsplane, which gets the DSSCAW logo.
  struct ncplane* dsplane = ncplane_new(nc, dimy, dimx, 0, 0, NULL);
  if(dsplane == NULL){
    return -1;
  }
  nc_err_e err = NCERR_SUCCESS;
  char* pic = find_data("dsscaw-purp.png");
  struct ncvisual* ncv2 = ncplane_visual_open(dsplane, pic, &err);
  if(ncv2 == NULL){
    free(pic);
    ncplane_destroy(dsplane);
    return -1;
  }
  free(pic);
  if((err = ncvisual_decode(ncv2)) != NCERR_SUCCESS){
    ncvisual_destroy(ncv2);
    ncplane_destroy(dsplane);
    return -1;
  }
  if(ncvisual_render(ncv2, 0, 0, -1, -1) <= 0){
    ncvisual_destroy(ncv2);
    ncplane_destroy(dsplane);
    return -1;
  }
  uint64_t channels = 0;
  channels_set_fg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
  channels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
  ncplane_set_base(ncvisual_plane(ncv2), "", 0, channels);
  ncvisual_destroy(ncv2);
  demo_render(nc);
  demo_nanosleep(nc, &demodelay);
  // now we open PurpleDrank on the standard plane, and hide DSSAW
  ncplane_move_bottom(dsplane);
  pic = find_data("PurpleDrank.jpg");
  struct ncvisual* ncv = ncplane_visual_open(nstd, pic, &err);
  if(ncv == NULL){
    ncplane_destroy(dsplane);
    free(pic);
    return -1;
  }
  free(pic);
  if((err = ncvisual_decode(ncv)) != NCERR_SUCCESS){
    ncvisual_destroy(ncv);
    ncplane_destroy(dsplane);
    return -1;
  }
  if(ncvisual_render(ncv, 0, 0, -1, -1) <= 0){
    ncvisual_destroy(ncv);
    ncplane_destroy(dsplane);
    return -1;
  }
  ncvisual_destroy(ncv);
  demo_render(nc);
  demo_nanosleep(nc, &demodelay);
  // bring DSSCAW back up to the top
  ncplane_move_top(dsplane);
  demo_render(nc);
  demo_nanosleep(nc, &demodelay);
  ncplane_destroy(dsplane);
  return 0;
}

int view_demo(struct notcurses* nc){
  if(!notcurses_canopen_images(nc)){
    return 0;
  }
  int dimy, dimx;
  struct ncplane* nstd = notcurses_stddim_yx(nc, &dimy, &dimx);
  int ret = view_images(nc, nstd, dimy, dimx);
  if(ret){
    return ret;
  }
  struct ncplane* ncpl = legend(nc, dimy, dimx);
  if(ncpl == NULL){
    return -1;
  }
  if(notcurses_canopen_videos(nc)){
    ret |= view_video_demo(nc);
  }
  ncplane_destroy(ncpl);
  return ret;
}
