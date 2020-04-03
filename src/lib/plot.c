#include "internal.h"

ncplot* ncplot_create(ncplane* n, const ncplot_options* opts){
  // detectdomain requires that miny == maxy
  if(opts->detectdomain && opts->miny != opts->maxy){
    return NULL;
  }
  if(opts->maxy < opts->miny){
    return NULL;
  }
  int sdimy, sdimx;
  ncplane_dim_yx(n, &sdimy, &sdimx);
  if(sdimx <= 0){
    return NULL;
  }
  uint64_t dimx = sdimx;
  ncplot* ret = malloc(sizeof(*ret));
  if(ret){
    if((ret->rangex = opts->rangex) == 0){
      ret->rangex = dimx;
    }
    ret->slotcount = ret->rangex;
    if(dimx < ret->rangex){
      ret->slotcount = dimx;
    }
    size_t slotsize = sizeof(*ret->slots) * ret->slotcount;
    ret->slots = malloc(slotsize);
    if(ret->slots){
      memset(ret->slots, 0, slotsize);
      ret->ncp = n;
      ret->maxchannel = opts->maxchannel;
      ret->minchannel = opts->minchannel;
      ret->miny = opts->miny;
      ret->maxy = opts->maxy;
      ret->vertical_indep = opts->vertical_indep;
      ret->gridtype = opts->gridtype;
      ret->exponentialy = opts->exponentialy;
      ret->windowbase = 0;
      ret->detectdomain = opts->detectdomain;
      ret->slotstart = 0;
      ret->slotx = 0;
      return ret;
    }
    free(ret);
  }
  return NULL;
}

ncplane* ncplot_plane(ncplot* n){
  return n->ncp;
}

static inline bool
invalid_y(ncplot* n, int64_t y){
  if(n->detectdomain){
    if(y > n->maxy){
      n->maxy = y;
    }
    if(y < n->miny){
      n->miny = y;
    }
  }else if(y > n->maxy || y < n->miny){
    return true;
  }
  return false;
}

// if x is less than the window, return -1
static inline int
window_slide(ncplot* n, uint64_t x){
  if(x < n->slotx){ // x is behind window, won't be counted
    return -1;
  }else if(x < n->slotx + n->rangex){ // x is within window, do nothing
    return 0;
  } // x is beyond window; we might be keeping some, might not
  uint64_t newslotx = x - n->rangex + 1; // the new value of slotx
  uint64_t slotdiff = newslotx - n->slotx; // the raw amount we're advancing
  if(slotdiff > n->rangex){
    slotdiff = n->rangex;
  } // slotdiff is the number of slots to reset, and amount to advance slotstart
  n->slotx = newslotx;
  // number to reset on the right of the circular buffer. min of (available at
  // current or to right, slotdiff)
  uint64_t slotsreset = n->slotcount - n->slotstart;
  if(slotsreset > slotdiff){
    slotsreset = slotdiff;
  }
  if(slotsreset){
    memset(n->slots + n->slotstart, 0, slotsreset * sizeof(*n->slots));
    n->slotstart += slotsreset;
    n->slotstart %= n->slotcount;
    slotdiff -= slotsreset;
  }
  if(slotdiff){
    memset(n->slots, 0, slotdiff * sizeof(*n->slots));
    n->slotstart = slotdiff - 1;
  }
  return 0;
}

// x must be within n's window
static inline void
update_sample(ncplot* n, uint64_t x, int64_t y, bool reset){
  uint64_t delta = x - n->slotx;
  uint64_t idx = (n->slotstart + delta) % n->slotcount;
  if(reset){
    n->slots[idx] = y;
  }else{
    n->slots[idx] += y;
  }
}

// Add to or set the value corresponding to this x. If x is beyond the current
// x window, the x window is advanced to include x, and values passing beyond
// the window are lost. The first call will place the initial window. The plot
// will be redrawn, but notcurses_render() is not called.
int ncplot_add_sample(ncplot* n, uint64_t x, int64_t y){
  if(invalid_y(n, y)){
    return -1;
  }
  if(window_slide(n, x)){
    return -1;
  }
  update_sample(n, x, y, false);
  // FIXME redraw plot
  return 0;
}

int ncplot_set_sample(ncplot* n, uint64_t x, int64_t y){
  if(invalid_y(n, y)){
    return -1;
  }
  if(window_slide(n, x)){
    return -1;
  }
  update_sample(n, x, y, true);
  // FIXME redraw plot
  return 0;
}

void ncplot_destroy(ncplot* n){
  if(n){
    free(n->slots);
    free(n);
  }
}
