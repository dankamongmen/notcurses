#include "demo.h"

// motherfucking eagles!
int eagle_demo(struct notcurses* nc){
  int averr;
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
  return 0;
}
