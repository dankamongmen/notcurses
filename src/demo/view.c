#include <notcurses.h>
#include "demo.h"

static int
watch_for_keystroke(struct notcurses* nc, struct ncvisual* ncv __attribute__ ((unused))){
  wchar_t w;
  // we don't want a keypress, but should handle NCKEY_RESIZE
  if((w = notcurses_getc_nblock(nc)) != (wchar_t)-1){
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
  ncv = ncplane_visual_open(ncp, "../tests/fm6.mkv", &averr);
  if(!ncv){
    return -1;
  }
  if(ncvisual_stream(nc, ncv, &averr, watch_for_keystroke) < 0){
    ncvisual_destroy(ncv);
    return -1;
  }
  ncvisual_destroy(ncv);
  return 0;
}

static struct ncplane*
legend(struct notcurses* nc, int dimy, int dimx){
  struct ncplane* n = notcurses_newplane(nc, 3, dimx / 5 + 10, dimy / 8, dimx / 10, NULL);
  ncplane_set_bg_alpha(n, CELL_ALPHA_TRANS);
  uint64_t channels = 0;
  channels_set_bg_alpha(&channels, CELL_ALPHA_TRANS);
  cell c = CELL_INITIALIZER(' ', 0, channels);
  ncplane_set_default(n, &c);
  ncplane_set_fg_rgb(n, 0, 0, 0);
  if(ncplane_putstr_aligned(n, 0, "target launch", NCALIGN_LEFT) <= 0){
    ncplane_destroy(n);
    return NULL;
  }
  ncplane_set_fg_rgb(n, 0xff, 0xff, 0xff);
  if(ncplane_putstr_aligned(n, 1, "2003-12-11 FM-6", NCALIGN_RIGHT) <= 0){
    ncplane_destroy(n);
    return NULL;
  }
  if(ncplane_putstr_aligned(n, 2, "RIM-161 SM-3 v. Aries TTV", NCALIGN_RIGHT) <= 0){
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
  struct ncvisual* ncv = ncplane_visual_open(ncp, "../tests/PurpleDrank.jpg", &averr);
  if(ncv == NULL){
    return -1;
  }
  struct ncvisual* ncv2 = ncplane_visual_open(ncp, "../tests/dsscaw-purp.png", &averr);
  if(ncv2 == NULL){
    ncvisual_destroy(ncv);
    return -1;
  }
  if(ncvisual_decode(ncv, &averr) == NULL){
    ncvisual_destroy(ncv);
    ncvisual_destroy(ncv2);
    return -1;
  }
  if(ncvisual_decode(ncv2, &averr) == NULL){
    ncvisual_destroy(ncv);
    ncvisual_destroy(ncv2);
    return -1;;
  }
  if(ncvisual_render(ncv)){
    ncvisual_destroy(ncv);
    ncvisual_destroy(ncv2);
    return -1;
  }
  if(notcurses_render(nc)){
    ncvisual_destroy(ncv);
    ncvisual_destroy(ncv2);
    return -1;
  }
  nanosleep(&demodelay, NULL);
  if(ncvisual_render(ncv2)){
    ncvisual_destroy(ncv);
    ncvisual_destroy(ncv2);
    return -1;
  }
  ncvisual_destroy(ncv);
  ncvisual_destroy(ncv2);
  if(notcurses_render(nc)){
    ncvisual_destroy(ncv);
    ncvisual_destroy(ncv2);
    return -1;
  }
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
