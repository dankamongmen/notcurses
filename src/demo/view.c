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
  if(view_video_demo(nc)){
    return -1;
  }
  return 0;
}
