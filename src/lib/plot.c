#include "internal.h"

static const struct {
  ncgridgeom_e geom;
  const wchar_t* egcs;
} geomdata[] = {
  { .geom = NCPLOT_1x1,   .egcs = L"█",    },
  { .geom = NCPLOT_1x1x4, .egcs = L"▒░▓█", },
  { .geom = NCPLOT_2x1,   .egcs = L"▄█",   },
};

/* FIXME
  NCPLOT_2x1TB, // full/upper blocks         █▀
  NCPLOT_2x1BT, // full/lower blocks
  NCPLOT_1x2LR, // left/full blocks          ▌█
  NCPLOT_1x2RL, // right/full blocks         █▐
  NCPLOT_2x2,   // quadrants                 ▖▘▝▗
  NCPLOT_4x1,   // four vert levels          █▆▄▂
  NCPLOT_1x4,   // four horizontal levels    ▎▌▊█
  NCPLOT_8x1,   // eight vert levels         █▇▆▅▄▃▂▁
  NCPLOT_1x8,   // eight horizontal levels   ▏▎▍▌▋▊▉█
  NCPLOT_4x2,   // 4 rows, 2 cols (braille)  ...etc...
*/

ncplot* ncplot_create(ncplane* n, const ncplot_options* opts){
  // detectdomain requires that miny == maxy
  if(opts->detectdomain && opts->miny != opts->maxy){
    return NULL;
  }
  if(opts->maxy < opts->miny){
    return NULL;
  }
  if(opts->gridtype < 0 || opts->gridtype >= sizeof(geomdata) / sizeof(*geomdata)){
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

// if we're doing domain detection, update the domain to reflect the value we
// just set. if we're not, check the result against the known ranges, and
// return -1 if the value is outside of that range.
static inline int
update_domain(ncplot* n, uint64_t x){
  const int64_t val = n->slots[x % n->slotcount];
  if(n->detectdomain){
    if(val > n->maxy){
      n->maxy = val;
    }
    if(val < n->miny){
      n->miny = val;
    }
    return 0;
  }
  if(val > n->maxy || val < n->miny){
    return -1;
  }
  return 0;
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
  uint64_t idx = x/*(n->slotstart + delta)*/ % n->slotcount;
  if(reset){
    n->slots[idx] = y;
  }else{
    n->slots[idx] += y;
  }
}

static int
redraw_plot(ncplot* n){
  ncplane_erase(ncplot_plane(n)); // FIXME shouldn't need this
  const int dimy = ncplane_dim_y(ncplot_plane(n));
  // each transition is worth this much change in value
  const size_t states = wcslen(geomdata[n->gridtype].egcs);
  double interval = (n->maxy - n->miny + 1) / ((double)dimy * states);
  int idx = n->slotstart;
  for(uint64_t x = 0 ; x < n->slotcount ; ++x){
    int64_t gval = n->slots[idx]; // clip the value at the limits of the graph
    if(gval < n->miny){
      gval = n->miny;
    }
    if(gval > n->maxy){
      gval = n->maxy;
    }
    // starting from the least-significant row, progress in the more significant
    // direction, drawing egcs from the grid specification, aborting early if
    // we can't draw anything in a given cell.
    for(int y = 0 ; y < dimy ; ++y){
      // if we've got at least one interval's worth on the number of positions
      // times the number of intervals per position plus the starting offset,
      // we're going to print *something*
      if(n->miny + (interval * states) * y + interval > gval){
        break;
      }
      size_t egcidx = (gval - n->miny) - (y * interval * states) - 1;
      if(egcidx >= states){
        egcidx = states - 1;
      }
      if(ncplane_putwc_yx(ncplot_plane(n), dimy - y - 1, x, geomdata[n->gridtype].egcs[egcidx]) <= 0){
        return -1;
      }
      // FIXME this ought fall out naturally. that it does not indicates, i
      // think, an error above...
      if(egcidx != states - 1){
        break;
      }
    }
    idx = (idx + 1) % n->slotcount;
  }
  return 0;
}

// Add to or set the value corresponding to this x. If x is beyond the current
// x window, the x window is advanced to include x, and values passing beyond
// the window are lost. The first call will place the initial window. The plot
// will be redrawn, but notcurses_render() is not called.
int ncplot_add_sample(ncplot* n, uint64_t x, int64_t y){
  if(window_slide(n, x)){
    return -1;
  }
  update_sample(n, x, y, false);
  if(update_domain(n, x)){
    return -1;
  }
  return redraw_plot(n);
}

int ncplot_set_sample(ncplot* n, uint64_t x, int64_t y){
  if(window_slide(n, x)){
    return -1;
  }
  update_sample(n, x, y, true);
  if(update_domain(n, x)){
    return -1;
  }
  return redraw_plot(n);
}

void ncplot_destroy(ncplot* n){
  if(n){
    free(n->slots);
    free(n);
  }
}
