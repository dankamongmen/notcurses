#include <notcurses.h>
#include "demo.h"

static int
watch_for_keystroke(struct notcurses* nc, struct ncvisual* ncv __attribute__ ((unused))){
  wchar_t w;
  // we don't want a keypress, but should handle NCKEY_RESIZE
  if((w = notcurses_getc_nblock(nc, NULL)) != (wchar_t)-1){
    if(w == NCKEY_RESIZE){
      // FIXME resize that sumbitch
    }else{
      return 1;
    }
  }
  return notcurses_render(nc);
}

static int
view_video_demo(struct notcurses* nc){
  struct ncplane* ncp = notcurses_stdplane(nc);
  int dimy, dimx;
  ncplane_dim_yx(ncp, &dimy, &dimx);
  int averr;
  struct ncvisual* ncv;
  char* fm6 = find_data("fm6.mkv");
  ncv = ncplane_visual_open(ncp, fm6, &averr);
  if(!ncv){
    free(fm6);
    return -1;
  }
  free(fm6);
  if(ncvisual_stream(nc, ncv, &averr, watch_for_keystroke) < 0){
    ncvisual_destroy(ncv);
    return -1;
  }
  ncvisual_destroy(ncv);
  return 0;
}

static struct ncplane*
legend(struct notcurses* nc, int dimy, int dimx){
  struct ncplane* n = notcurses_newplane(nc, 4, 25, dimy * 7 / 8, (dimx - 25) / 2, NULL);
  ncplane_set_bg_alpha(n, CELL_ALPHA_TRANSPARENT);
  uint64_t channels = 0;
  channels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
  cell c = CELL_INITIALIZER(' ', 0, channels);
  ncplane_set_default(n, &c);
  ncplane_styles_set(n, CELL_STYLE_BOLD);
  ncplane_set_fg_rgb(n, 0xff, 0xff, 0xff);
  if(ncplane_putstr_aligned(n, 0, "target launch", NCALIGN_CENTER) <= 0){
    ncplane_destroy(n);
    return NULL;
  }
  ncplane_set_fg_rgb(n, 0, 0, 0);
  ncplane_styles_off(n, CELL_STYLE_BOLD);
  if(ncplane_putstr_aligned(n, 1, "2003-12-11 FM-6", NCALIGN_CENTER) <= 0){
    ncplane_destroy(n);
    return NULL;
  }
  if(ncplane_putstr_aligned(n, 2, "RIM-161 SM-3 v. Aries TTV", NCALIGN_LEFT) <= 0){
    ncplane_destroy(n);
    return NULL;
  }
  ncplane_set_fg_rgb(n, 0x80, 0xc0, 0x80);
  if(ncplane_putstr_aligned(n, 3, "exo-atmospheric intercept", NCALIGN_LEFT) <= 0){
    ncplane_destroy(n);
    return NULL;
  }
  return n;
}

int view_demo(struct notcurses* nc){
  struct ncplane* ncp = notcurses_stdplane(nc);
  int dimy, dimx;
  ncplane_dim_yx(ncp, &dimy, &dimx);
  int averr = 0;
  char* pic = find_data("PurpleDrank.jpg");
  ncplane_erase(ncp);
  struct ncvisual* ncv = ncplane_visual_open(ncp, pic, &averr);
  if(ncv == NULL){
    free(pic);
    return -1;
  }
  free(pic);
  struct ncplane* dsplane = notcurses_newplane(nc, dimy, dimx, 0, 0, NULL);
  if(dsplane == NULL){
    return -1;
  }
  pic = find_data("dsscaw-purp.png");
  struct ncvisual* ncv2 = ncplane_visual_open(dsplane, pic, &averr);
  if(ncv2 == NULL){
    free(pic);
    ncvisual_destroy(ncv);
    ncplane_destroy(dsplane);
    return -1;
  }
  free(pic);
  if(ncvisual_decode(ncv, &averr) == NULL){
    ncvisual_destroy(ncv);
    ncvisual_destroy(ncv2);
    ncplane_destroy(dsplane);
    return -1;
  }
  if(ncvisual_decode(ncv2, &averr) == NULL){
    ncvisual_destroy(ncv);
    ncvisual_destroy(ncv2);
    ncplane_destroy(dsplane);
    return -1;
  }
  if(ncvisual_render(ncv2, 0, 0, 0, 0)){
    ncvisual_destroy(ncv);
    ncvisual_destroy(ncv2);
    ncplane_destroy(dsplane);
    return -1;
  }
  notcurses_render(nc);
  ncplane_move_bottom(dsplane);
  nanosleep(&demodelay, NULL);
  if(ncvisual_render(ncv, 0, 0, 0, 0)){
    ncvisual_destroy(ncv);
    ncvisual_destroy(ncv2);
    ncplane_destroy(dsplane);
    return -1;
  }
  ncvisual_destroy(ncv);
  ncvisual_destroy(ncv2);
  notcurses_render(nc);
  nanosleep(&demodelay, NULL);
  ncplane_move_top(dsplane);
  notcurses_render(nc);
  ncplane_destroy(dsplane);
  nanosleep(&demodelay, NULL);
  struct ncplane* ncpl = legend(nc, dimy, dimx);
  if(ncpl == NULL){
    return -1;
  }
  if(view_video_demo(nc)){
    return -1;
  }
  ncplane_destroy(ncpl);
  return 0;
}
