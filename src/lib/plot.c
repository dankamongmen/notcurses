#include "internal.h"

ncplot* ncplot_create(ncplane* n, const ncplot_options* opts){
  // detectrange requires that miny == maxy
  if(opts->detectrange && opts->miny != opts->maxy){
    return NULL;
  }
  if(opts->maxy < opts->miny){
    return NULL;
  }
  ncplot* ret = malloc(sizeof(*ret));
  if(ret){
    ret->ncp = n;
    ret->maxchannel = opts->maxchannel;
    ret->minchannel = opts->minchannel;
    ret->rangex = opts->rangex;
    ret->miny = opts->miny;
    ret->maxy = opts->maxy;
    ret->vertical_indep = opts->vertical_indep;
    ret->gridtype = opts->gridtype;
    ret->exponentialy = opts->exponentialy;
    ret->windowbase = 0;
    ret->detectrange = opts->detectrange;
  }
  return ret;
}

ncplane* ncplot_plane(ncplot* n){
  return n->ncp;
}

static inline bool
invalid_y(ncplot* n, int64_t y){
  if(y > n->maxy || y < n->miny){
    return true;
  }
  return false;
}

// Add to or set the value corresponding to this x. If x is beyond the current
// x window, the x window is advanced to include x, and values passing beyond
// the window are lost. The first call will place the initial window. The plot
// will be redrawn, but notcurses_render() is not called.
int ncplot_add_sample(ncplot* n, uint64_t x, int64_t y){
  if(invalid_y(n, y)){
    return -1;
  }
  return 0;
}

int ncplot_set_sample(ncplot* n, uint64_t x, int64_t y){
  if(invalid_y(n, y)){
    return -1;
  }
  return 0;
}

void ncplot_destroy(ncplot* n){
  if(n){
    free(n);
  }
}
