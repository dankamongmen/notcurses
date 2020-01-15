#include "demo.h"

int fallin_demo(struct notcurses* nc){
  // make a copy of the standard plane so that we don't need rederive the
  // world in the event of a resize event
  struct ncplane* ndup = ncplane_dup(notcurses_stdplane(nc), NULL);
  if(!ndup){
    return -1;
  }
  ncplane_erase(notcurses_stdplane(nc));
  int dimx, dimy, atotal;
  ncplane_dim_yx(ndup, &dimy, &dimx);
  atotal = dimy * dimx;
  // FIXME shuffle up a list of all coordinates, then walk through them. each
  // one ought be turned into its own small ncplane, erased from ndup, and the
  // plane set falling.
  while(atotal--){
    if(notcurses_render(nc)){
      ncplane_destroy(ndup);
      return -1;
    }
  }
  if(ncplane_destroy(ndup)){
    return -1;
  }
  return 0;
}
