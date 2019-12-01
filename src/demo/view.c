#include <notcurses.h>
#include "demo.h"

int view_demo(struct notcurses* nc){
  struct ncplane* ncp = notcurses_stdplane(nc);
  int dimy, dimx;
  ncplane_dim_yx(ncp, &dimy, &dimx);
  int averr;
  struct ncvisual* ncv = ncplane_visual_open(ncp, "../tests/dsscaw-purp.png", &averr);
  if(ncv == NULL){
    return -1;
  }
  struct AVFrame* frame = ncvisual_decode(ncv, &averr);
  if(frame == NULL){
    ncvisual_destroy(ncv);
    return -1;
  }
  if(ncvisual_render(ncv)){
    ncvisual_destroy(ncv);
    return -1;
  }
  if(notcurses_render(nc)){
    ncvisual_destroy(ncv);
    return -1;
  }
  ncvisual_destroy(ncv);
  nanosleep(&demodelay, NULL);
  return 0;
}
