#include "internal.h"

ncplot* ncplot_create(ncplane* n, const ncplot_options* opts){
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
  }
  return ret;
}

ncplane* ncplot_plane(ncplot* n){
  return n->ncp;
}

// Add to or set the value corresponding to this x. If x is beyond the current
// x window, the x window is advanced to include x, and values passing beyond
// the window are lost. The first call will place the initial window. The plot
// will be redrawn, but notcurses_render() is not called.
int ncplot_add_sample(struct ncplot* n, uint64_t x, int64_t y){
}

int ncplot_set_sample(struct ncplot* n, uint64_t x, int64_t y){
}

void ncplot_destroy(ncplot* n){
  if(n){
    free(n);
  }
}
