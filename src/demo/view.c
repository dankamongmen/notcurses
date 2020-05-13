#include "demo.h"

static int
watch_for_keystroke(struct notcurses* nc, struct ncvisual* ncv __attribute__ ((unused)),
                    void* curry __attribute__ ((unused))){
  wchar_t w;
  // we don't want a keypress, but allow the ncvisual to handle
  // NCKEY_RESIZE for us
  if((w = demo_getc_nblock(nc, NULL)) != (wchar_t)-1){
    if(w == 'q'){
      return 1;
    }
  }
  return demo_render(nc);
}

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
  int ret = ncvisual_stream(nc, ncv, &err, 2.0/3.0 * delaymultiplier, watch_for_keystroke, NULL);
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

int view_demo(struct notcurses* nc){
  if(!notcurses_canopen_images(nc)){
    return 0;
  }
  int dimy, dimx;
  struct ncplane* ncp = notcurses_stddim_yx(nc, &dimy, &dimx);
  nc_err_e err = 0;
  char* pic = find_data("PurpleDrank.jpg");
  ncplane_erase(ncp);
  struct ncvisual* ncv = ncplane_visual_open(ncp, pic, &err);
  if(ncv == NULL){
    free(pic);
    return -1;
  }
  free(pic);
  struct ncplane* dsplane = ncplane_new(nc, dimy, dimx, 0, 0, NULL);
  if(dsplane == NULL){
    return -1;
  }
  pic = find_data("dsscaw-purp.png");
  struct ncvisual* ncv2 = ncplane_visual_open(dsplane, pic, &err);
  if(ncv2 == NULL){
    free(pic);
    ncvisual_destroy(ncv);
    ncplane_destroy(dsplane);
    return -1;
  }
  free(pic);
  if((err = ncvisual_decode(ncv)) != NCERR_SUCCESS){
    ncvisual_destroy(ncv);
    ncvisual_destroy(ncv2);
    ncplane_destroy(dsplane);
    return -1;
  }
  if((err = ncvisual_decode(ncv2)) != NCERR_SUCCESS){
    ncvisual_destroy(ncv);
    ncvisual_destroy(ncv2);
    ncplane_destroy(dsplane);
    return -1;
  }
  if(ncvisual_render(ncv2, 0, 0, -1, -1) <= 0){
    ncvisual_destroy(ncv);
    ncvisual_destroy(ncv2);
    ncplane_destroy(dsplane);
    return -1;
  }
  demo_render(nc);
  demo_nanosleep(nc, &demodelay);
  ncplane_move_bottom(dsplane);
  if(ncvisual_render(ncv, 0, 0, -1, -1) <= 0){
    ncvisual_destroy(ncv);
    ncvisual_destroy(ncv2);
    ncplane_destroy(dsplane);
    return -1;
  }
  uint64_t channels = 0;
  channels_set_fg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
  channels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
  ncplane_set_base(ncvisual_plane(ncv2), "", 0, channels);
  demo_render(nc);
  demo_nanosleep(nc, &demodelay);
  ncvisual_destroy(ncv);
  ncvisual_destroy(ncv2);
  ncplane_move_top(dsplane);
  demo_render(nc);
  demo_nanosleep(nc, &demodelay);
  ncplane_destroy(dsplane);
  struct ncplane* ncpl = legend(nc, dimy, dimx);
  if(ncpl == NULL){
    return -1;
  }
  int ret = 0;
  if(notcurses_canopen_videos(nc)){
    ret |= view_video_demo(nc);
  }
  ncplane_destroy(ncpl);
  return ret;
}
