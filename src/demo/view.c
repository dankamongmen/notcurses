#include <notcurses.h>
#include "demo.h"

int view_demo(struct notcurses* nc){
  struct ncplane* ncp = notcurses_stdplane(nc);
  int dimy, dimx;
  ncplane_dim_yx(ncp, &dimy, &dimx);
  int averr;
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
  return 0;
}
