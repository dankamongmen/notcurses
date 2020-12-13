#include <string.h>
#include "internal.h"

ncprogbar* ncprogbar_create(ncplane* n, const ncprogbar_options* opts){
  ncprogbar_options default_opts;
  if(opts == NULL){
    memset(&default_opts, 0, sizeof(default_opts));
    default_opts.minchannels = CHANNELS_RGB_INITIALIZER(0x3d, 0x3d, 0x3d, 0, 0, 0);
    default_opts.maxchannels = CHANNELS_RGB_INITIALIZER(0xe0, 0xee, 0xe0, 0, 0, 0);
    opts = &default_opts;
  }
  if(opts->flags > (NCPROGBAR_OPTION_FORCE_VERTICAL << 1u)){
    logwarn(ncplane_notcurses(n), "Invalid flags %016lx\n", opts->flags);
  }
  ncprogbar* ret = malloc(sizeof(*ret));
  ret->ncp = n;
  ret->maxchannels = opts->maxchannels;
  ret->minchannels = opts->minchannels;
  if(opts->flags & NCPROGBAR_OPTION_LOCK_ORIENTATION){
    if(opts->flags & NCPROGBAR_OPTION_FORCE_VERTICAL){
      ret->direction = PROGRESS_VERT;
    }else{
      ret->direction = PROGRESS_HORIZ;
    }
  }else{
    ret->direction = PROGRESS_DYNAMIC;
  }
  ret->retrograde = opts->flags & NCPROGBAR_OPTION_RETROGRADE;
  return ret;
}

ncplane* ncprogbar_plane(ncprogbar* n){
  return n->ncp;
}

static int
progbar_redraw(ncprogbar* n){
  enum {
    DIR_UP,
    DIR_RIGHT,
    DIR_DOWN,
    DIR_LEFT
  } direction;
  // get current dimensions; they might have changed
  int dimy, dimx;
  ncplane_dim_yx(ncprogbar_plane(n), &dimy, &dimx);
  if(n->direction == PROGRESS_DYNAMIC){
    if(dimx > dimy){
      direction = n->retrograde ? DIR_LEFT : DIR_RIGHT;
    }else{
      direction = n->retrograde ? DIR_DOWN : DIR_UP;
    }
  }else if(n->direction == PROGRESS_VERT){
    direction = n->retrograde ? DIR_DOWN : DIR_UP;
  }else{ // PROGRESS_HORIZ
    direction = n->retrograde ? DIR_LEFT : DIR_RIGHT;
  }
  int range;
  if(direction == DIR_UP || direction == DIR_DOWN){
    range = dimy;
  }else{
    range = dimx;
  }
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
