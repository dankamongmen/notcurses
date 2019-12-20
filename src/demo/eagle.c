#include "demo.h"

static struct ncvisual*
outzoomed_map(struct notcurses* nc, const char* map){
  int averr;
  struct ncvisual* ncv = ncvisual_open_plane(nc, map, &averr, 0, 0, NCSCALE_SCALE);
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

static int
zoom_map(struct notcurses* nc, const char* map){
  int averr;
  // FIXME determine size that will be represented on screen at once, and how
  // large that section has been rendered in the outzoomed map. take the map
  // and begin opening it on larger and larger planes that fit on the screen
  // less and less. eventually, reach our natural NCSCALE_NONE size and begin
  // scrolling through the map, whooooooooosh.
  struct ncvisual* ncv = ncvisual_open_plane(nc, map, &averr, 0, 0, NCSCALE_NONE);
  if(ncv == NULL){
    return -1;
  }
  if(ncvisual_decode(ncv, &averr) == NULL){
    ncvisual_destroy(ncv);
    return -1;
  }
  // we start at the lower left corner of the outzoomed map
  int truex, truey; // dimensions of true display
  notcurses_term_dim_yx(nc, &truey, &truex);
  struct ncplane* ncp = ncvisual_plane(ncv);
  int vx, vy; // dimensions of unzoomed map
  ncplane_dim_yx(ncp, &vy, &vx);
  ncplane_move_yx(ncp, truey - vy, truex - vx);
  if(ncvisual_render(ncv)){
    ncvisual_destroy(ncv);
    return -1;
  }
  ncplane_move_bottom(ncp);
  int zoomy = truey;
  int zoomx = truex;
  while(zoomy < vy && zoomx < vx){
    zoomy += 2;
    zoomx += 2;
    struct ncplane* zncp = notcurses_newplane(nc, zoomy, zoomx, truey - zoomy, 0, NULL);
    if(zncp == NULL){
      ncvisual_destroy(ncv);
      return -1;
    }
    struct ncvisual* zncv = ncplane_visual_open(zncp, map, &averr);
    if(zncv == NULL){
      ncplane_destroy(zncp);
      ncvisual_destroy(ncv);
      return -1;
    }
    if(ncvisual_decode(zncv, &averr) == NULL){
      ncvisual_destroy(zncv);
      ncplane_destroy(zncp);
      ncvisual_destroy(ncv);
      return -1;
    }
    if(ncvisual_render(zncv)){
      ncvisual_destroy(zncv);
      ncplane_destroy(zncp);
      ncvisual_destroy(ncv);
      return -1;
    }
    if(notcurses_render(nc)){
      ncvisual_destroy(zncv);
      ncplane_destroy(zncp);
      ncvisual_destroy(ncv);
      return -1;
    }
    ncvisual_destroy(zncv);
    ncplane_destroy(zncp);
  }
  nanosleep(&demodelay, NULL);
  ncvisual_destroy(ncv);
  return 0;
}

// motherfucking eagles!
int eagle_demo(struct notcurses* nc){
  const char* map = "../tests/eagles.png";
  struct ncvisual* zo;
  if((zo = outzoomed_map(nc, map)) == NULL){
    return -1;
  }
  ncvisual_destroy(zo);
  if(zoom_map(nc, map)){
    return -1;
  }
  return 0;
}
