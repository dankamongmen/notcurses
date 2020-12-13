#include <string.h>
#include "internal.h"

ncprogbar* ncprogbar_create(ncplane* n, const ncprogbar_options* opts){
  ncprogbar_options default_opts;
  if(opts == NULL){
    memset(&default_opts, 0, sizeof(default_opts));
    opts = &default_opts;
    // FIXME need sensible default channels
  }
  if(opts->flags > (NCPROGBAR_OPTION_FORCE_VERTICAL << 1u)){
    logwarn(ncplane_notcurses(n), "Invalid flags %016lx\n", opts->flags);
  }
  ncprogbar* ret = malloc(sizeof(*ret));
  ret->ncp = n;
  ret->maxchannels = opts->maxchannels;
  ret->minchannels = opts->minchannels;
  return ret;
}

ncplane* ncprogbar_plane(ncprogbar* n){
  return n->ncp;
}

static int
progbar_redraw(ncprogbar* n){
  // FIXME
  return 0;
}

int ncprogbar_set_progress(ncprogbar* n, double p){
  if(p < 0 || p > 1){
    logerror(ncplane_notcurses(ncprogbar_plane(n)), "Invalid progress %g\n", p);
    return -1;
  }
  n->progress = p;
  return progbar_redraw(n);
}

double ncprogbar_progress(const ncprogbar* n){
  return n->progress;
}

void ncprogbar_destroy(ncprogbar* n){
  ncplane_destroy(n->ncp);
  free(n);
}
