#include "internal.h"

static const struct {
  ncgridgeom_e geom;
  int width;
  int height;
  // the EGCs which form the various levels of a given geometry. if the geometry
  // is wide, things are arranged with the rightmost side increasing most
  // quickly, i.e. it can be indexed as height arrays of 1 + height glyphs. i.e.
  // the first five braille EGCs are all 0 on the left, [0..4] on the right.
  const wchar_t* egcs;
  bool fill;
} geomdata[] = {
  { .geom = NCPLOT_1x1,   .width = 1, .height = 2, .egcs = L" █",                        .fill = false, },
  { .geom = NCPLOT_2x1,   .width = 1, .height = 3, .egcs = L" ▄█",                       .fill = false, },
  { .geom = NCPLOT_1x1x4, .width = 1, .height = 5, .egcs = L" ▒░▓█",                     .fill = false, },
  { .geom = NCPLOT_2x2,   .width = 2, .height = 3, .egcs = L" ▗▐▖▄▟▌▙█",                 .fill = false, },
  { .geom = NCPLOT_4x1,   .width = 1, .height = 5, .egcs = L" ▂▄▆█",                     .fill = false, },
  { .geom = NCPLOT_4x2,   .width = 2, .height = 5, .egcs = L"⠀⡀⡄⡆⡇⢀⣀⣄⣆⣇⢠⣠⣤⣦⣧⢰⣰⣴⣶⣷⢸⣸⣼⣾⣿", .fill = true,  },
  { .geom = NCPLOT_8x1,   .width = 1, .height = 9, .egcs = L" ▁▂▃▄▅▆▇█",                 .fill = false, },
};

static int
redraw_plot(ncplot* n){
  ncplane_erase(ncplot_plane(n));
  const int scale = geomdata[n->gridtype].width;
  int dimy, dimx;
  ncplane_dim_yx(ncplot_plane(n), &dimy, &dimx);
  const int scaleddim = dimx * scale;
  // each transition is worth this much change in value
  const size_t states = geomdata[n->gridtype].height;
  // FIXME can we not rid ourselves of this meddlesome double?
  double interval = n->maxy < n->miny ? 0 : (n->maxy - n->miny) / ((double)dimy * states);
  const int startx = n->labelaxisd ? PREFIXSTRLEN : 0; // plot cols begin here
  // if we want fewer slots than there are available columns, our final column
  // will be other than the plane's final column. most recent x goes here.
  const int finalx = (n->slotcount < scaleddim - 1 - (startx * scale) ? startx + (n->slotcount / scale) - 1 : dimx - 1);
  if(n->labelaxisd){
    // show the *top* of each interval range
    for(int y = 0 ; y < dimy ; ++y){
      char buf[PREFIXSTRLEN + 1];
      ncmetric(interval * states * (y + 1) * 100, 100, buf, 0, 1000, '\0');
      ncplane_putstr_yx(ncplot_plane(n), dimy - y - 1, PREFIXSTRLEN - strlen(buf), buf);
    }
  }
  if(finalx < startx){ // exit on pathologically narrow planes
    return 0;
  }
  if(!interval){
    interval = 1;
  }
  #define MAXWIDTH 2
  int idx = n->slotstart; // idx holds the real slot index; we move backwards
  for(int x = finalx ; x >= startx ; --x){
    uint64_t gvals[MAXWIDTH];
    // load it retaining the same ordering we have in the actual array
    for(int i = scale - 1 ; i >= 0 ; --i){
      gvals[i] = n->slots[idx]; // clip the value at the limits of the graph
      if(gvals[i] < n->miny){
        gvals[i] = n->miny;
      }
      if(gvals[i] > n->maxy){
        gvals[i] = n->maxy;
      }
      // FIXME if there are an odd number, only go up through the valid ones...
      if(--idx < 0){
        idx = n->slotcount - 1;
      }
    }
    // starting from the least-significant row, progress in the more significant
    // direction, drawing egcs from the grid specification, aborting early if
    // we can't draw anything in a given cell.
    double intervalbase = n->miny;
    const wchar_t* egc = geomdata[n->gridtype].egcs;
    for(int y = 0 ; y < dimy ; ++y){
      size_t egcidx, sumidx = 0;
      // if we've got at least one interval's worth on the number of positions
      // times the number of intervals per position plus the starting offset,
      // we're going to print *something*
      bool done = !geomdata[n->gridtype].fill;
      for(int i = 0 ; i < scale ; ++i){
        sumidx *= states;
        if(intervalbase < gvals[i]){
          egcidx = (gvals[i] - intervalbase) / interval;
          if(egcidx >= states){
            egcidx = states - 1;
          }
          done = false;
          sumidx += egcidx;
        }else{
          egcidx = 0;
        }
      }
      if(done){
        break;
      }
      if(ncplane_putwc_yx(ncplot_plane(n), dimy - y - 1, x, egc[sumidx]) <= 0){
        return -1;
      }
      intervalbase += (states * interval);
    }
  }
  if(ncplane_cursor_move_yx(ncplot_plane(n), 0, 0)){
    return -1;
  }
  if(ncplane_stain(ncplot_plane(n), dimy - 1, dimx - 1, n->maxchannel,
                   n->maxchannel, n->minchannel, n->minchannel) <= 0){
    return -1;
  }
  return 0;
}

ncplot* ncplot_create(ncplane* n, const ncplot_options* opts){
  // if miny == maxy, they both must be equal to 0
  if(opts->miny == opts->maxy && opts->miny){
    return NULL;
  }
  if(opts->rangex < 0){
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
  int dimx = sdimx;
  ncplot* ret = malloc(sizeof(*ret));
  if(ret){
    ret->rangex = opts->rangex;
    // if we're sizing the plot based off the plane dimensions, scale it by the
    // plot geometry's width for all calculations
    const int scaleddim = dimx * geomdata[opts->gridtype].width;
    const int scaledprefixlen = PREFIXSTRLEN * geomdata[opts->gridtype].width;
    if((ret->slotcount = ret->rangex) == 0){
      ret->slotcount = scaleddim;
    }
    if(dimx < ret->rangex){
      ret->slotcount = scaleddim;
    }
    if( (ret->labelaxisd = opts->labelaxisd) ){
      if(ret->slotcount + scaledprefixlen > scaleddim){
        if(scaleddim > scaledprefixlen){
          ret->slotcount = scaleddim - scaledprefixlen;
        }
      }
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
      if( (ret->detectdomain = (opts->miny == opts->maxy)) ){
        ret->maxy = 0;
        ret->miny = ~(uint64_t)0ull;
      }
      ret->slotstart = 0;
      ret->slotx = 0;
      redraw_plot(ret);
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
  const uint64_t val = n->slots[x % n->slotcount];
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

// if x is less than the window, return -1, as the sample will be thrown away.
// if the x is within the current window, find the proper slot and update it.
// otherwise, the x is the newest sample. if it is obsoletes all existing slots,
// reset them, and write the new sample anywhere. otherwise, write it to the
// proper slot based on the current newest slot.
static inline int
window_slide(ncplot* n, int64_t x){
  if(x < n->slotx - (n->slotcount - 1)){ // x is behind window, won't be counted
    return -1;
  }else if(x <= n->slotx){ // x is within window, do nothing
    return 0;
  } // x is newest; we might be keeping some, might not
  int64_t xdiff = x - n->slotx; // the raw amount we're advancing
  n->slotx = x;
  if(xdiff >= n->slotcount){ // we're throwing away all old samples, write to 0
    memset(n->slots, 0, sizeof(*n->slots) * n->slotcount);
    n->slotstart = 0;
    return 0;
  }
  // we're throwing away only xdiff slots, which is less than n->slotcount.
  // first, we'll try to clear to the right...number to reset on the right of
  // the circular buffer. min of (available at current or to right, xdiff)
  int slotsreset = n->slotcount - n->slotstart - 1;
  if(slotsreset > xdiff){
    slotsreset = xdiff;
  }
  if(slotsreset){
    memset(n->slots + n->slotstart + 1, 0, slotsreset * sizeof(*n->slots));
  }
  n->slotstart = (n->slotstart + xdiff) % n->slotcount;
  xdiff -= slotsreset;
  if(xdiff){ // throw away some at the beginning
    memset(n->slots, 0, xdiff * sizeof(*n->slots));
  }
  return 0;
}

// x must be within n's window at this point
static inline void
update_sample(ncplot* n, int64_t x, uint64_t y, bool reset){
  const int64_t diff = n->slotx - x; // amount behind
  const int idx = (n->slotstart + n->slotcount - diff) % n->slotcount;
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
int ncplot_add_sample(ncplot* n, uint64_t x, uint64_t y){
  if(window_slide(n, x)){
    return -1;
  }
  update_sample(n, x, y, false);
  if(update_domain(n, x)){
    return -1;
  }
  return redraw_plot(n);
}

int ncplot_set_sample(ncplot* n, uint64_t x, uint64_t y){
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
