#include "internal.h"

ncplot* ncplot_create(ncplane* n, const ncplot_options* opts){
  ncplot* ret = malloc(sizeof(*ret));
  if(ret){
    ret->ncp = n;
    ret->maxchannel = opts->maxchannel;
    ret->minchannel = opts->minchannel;
    // FIXME
  }
  return ret;
}

ncplane* ncplot_plane(ncplot* n){
  return n->ncp;
}

void ncplot_destroy(ncplot* n){
  if(n){
    free(n);
  }
}
