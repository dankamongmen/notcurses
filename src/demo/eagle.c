#include "demo.h"

static struct ncvisual*
outzoomed_map(struct notcurses* nc){
  int averr;
  struct ncvisual* ncv = ncvisual_open_plane(nc, "../tests/eagles.png", &averr,
                                             0, 0, NCSCALE_SCALE);
  if(ncv == NULL){
    return NULL;
  }
  if(ncvisual_decode(ncv, &averr) == NULL){
    return NULL;
  }
  if(ncvisual_render(ncv)){
    return NULL;
  }
  if(notcurses_render(nc)){
    return NULL;
  }
  nanosleep(&demodelay, NULL);
  return ncv;
}

// motherfucking eagles!
int eagle_demo(struct notcurses* nc){
  struct ncvisual* zo;
  if((zo = outzoomed_map(nc)) == NULL){
    return -1;
  }
  int averr;
  // FIXME determine size that will be represented on screen at once, and how
  // large that section has been rendered in the outzoomed map. take the map
  // and begin opening it on larger and larger planes that fit on the screen
  // less and less. eventually, reach our natural NCSCALE_NONE size and begin
  // scrolling through the map, whooooooooosh.
  struct ncvisual* ncv = ncvisual_open_plane(nc, "../tests/eagles.png", &averr,
                                             0, 0, NCSCALE_NONE);
  if(ncv == NULL){
    return -1;
  }
  if(ncvisual_decode(ncv, &averr) == NULL){
    return -1;
  }
  if(ncvisual_render(ncv)){
    return -1;
  }
  if(notcurses_render(nc)){
    return -1;
  }
  nanosleep(&demodelay, NULL);
  ncvisual_destroy(ncv);
  ncvisual_destroy(zo);
  return 0;
}
