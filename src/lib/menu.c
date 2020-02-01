#include "internal.h"

ncmenu* ncmenu_create(notcurses* nc, const menu_options* opts){
  int totalheight = 0;
  int totalwidth = 0;
  // FIXME calaculate maximum dimensions
  ncmenu* ret = malloc(sizeof(*ret));
  ret->sectioncount = opts->sectioncount;
  ret->sections = NULL;
  int dimy = ncplane_dim_y(notcurses_stdplane(nc));
  int ypos = opts->bottom ? dimy - 1 : 0;
  if(ret){
    ret->ncp = ncplane_new(nc, totalheight, totalwidth, ypos, 0, NULL);
    // FIXME
  }
  return ret;
}

int ncmenu_unroll(ncmenu* n, int sectionidx){
  if(sectionidx < 0 || sectionidx >= n->sectioncount){
    return -1;
  }
  if(ncmenu_rollup(n)){ // roll up any unroled section
    return -1;
  }
  // FIXME
  return 0;
}

int ncmenu_rollup(ncmenu* n){
  if(n->unrolledsection < 0){
    return 0;
  }
  // FIXME
  return 0;
}

int ncmenu_destroy(ncmenu* n){
  int ret = 0;
  if(n){
  }
  return ret;
}
