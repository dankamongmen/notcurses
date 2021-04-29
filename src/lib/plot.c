#include <math.h>
#include <float.h>
#include <limits.h>
#include <string.h>
#include "internal.h"

#define MAXWIDTH 2
#define CREATE(T, X) \
typedef struct nc##X##plot { \
  T* slots; \
  T miny, maxy; \
  ncplane* ncp; \
  /* sloutcount-element circular buffer of samples. the newest one (rightmost) \
     is at slots[slotstart]; they get older as you go back (and around). \
     elements. slotcount is max(columns, rangex), less label room. */ \
  int64_t slotx; /* x value corresponding to slots[slotstart] (newest x) */ \
  uint64_t maxchannels; \
  uint64_t minchannels; \
  uint16_t legendstyle; \
  bool vertical_indep; /* not yet implemented FIXME */ \
  const struct blitset* bset; \
  char* title; \
  /* requested number of slots. 0 for automatically setting the number of slots \
     to span the horizontal area. if there are more slots than there are \
     columns, we prefer showing more recent slots to less recent. if there are \
     fewer slots than there are columns, they prefer the left side. */ \
  int rangex; \
  /* domain minimum and maximum. if detectdomain is true, these are \
     progressively enlarged/shrunk to fit the sample set. if not, samples \
     outside these bounds are counted, but the displayed range covers only this. */ \
  int slotcount; \
  int slotstart; /* index of most recently-written slot */ \
  bool labelaxisd; /* label dependent axis (consumes PREFIXCOLUMNS columns) */ \
  bool exponentiali; /* exponential independent axis */ \
  bool detectdomain; /* is domain detection in effect (stretch the domain)? */ \
  bool detectonlymax; /* domain detection applies only to max, not min */ \
  bool printsample; /* print the most recent sample */ \
} nc##X##plot; \
\
int redraw_plot_##T(nc##X##plot* ncp){ \
  ncplane_erase(ncp->ncp); \
  const int scale = ncp->bset->width; \
  int dimy, dimx; \
  ncplane_dim_yx(ncp->ncp, &dimy, &dimx); \
  const int scaleddim = dimx * scale; \
  /* each transition is worth this much change in value */ \
  const size_t states = ncp->bset->height + 1; \
  /* FIXME can we not rid ourselves of this meddlesome double? either way, the \
     interval is one row's range (for linear plots), or the base (base^slots== \
     maxy-miny) of the range (for exponential plots). */ \
  double interval; \
  if(ncp->exponentiali){ \
    if(ncp->maxy > ncp->miny){ \
      interval = pow(ncp->maxy - ncp->miny, (double)1 / (dimy * states)); \
/* fprintf(stderr, "miny: %ju maxy: %ju dimy: %d states: %zu\n", miny, maxy, dimy, states); */ \
    }else{ \
      interval = 0; \
    } \
  }else{ \
    interval = ncp->maxy < ncp->miny ? 0 : (ncp->maxy - ncp->miny) / ((double)dimy * states); \
  } \
  const int startx = ncp->labelaxisd ? PREFIXCOLUMNS : 0; /* plot cols begin here */ \
  /* if we want fewer slots than there are available columns, our final column \
     will be other than the plane's final column. most recent x goes here. */ \
  const int finalx = (ncp->slotcount < scaleddim - 1 - (startx * scale) ? startx + (ncp->slotcount / scale) - 1 : dimx - 1); \
  ncplane_set_styles(ncp->ncp, ncp->legendstyle); \
  if(ncp->labelaxisd){ \
    /* show the *top* of each interval range */ \
    for(int y = 0 ; y < dimy ; ++y){ \
      uint64_t channels = 0; \
      calc_gradient_channels(&channels, ncp->minchannels, ncp->minchannels, \
                             ncp->maxchannels, ncp->maxchannels, y, 0, dimy, dimx); \
      ncplane_set_channels(ncp->ncp, channels); \
      char buf[PREFIXSTRLEN + 1]; \
      if(ncp->exponentiali){ \
        if(y == dimy - 1){ /* we cheat on the top row to exactly match maxy */ \
          ncmetric(ncp->maxy * 100, 100, buf, 0, 1000, '\0'); \
        }else{ \
          ncmetric(pow(interval, (y + 1) * states) * 100, 100, buf, 0, 1000, '\0'); \
        } \
      }else{ \
        ncmetric((ncp->maxy - interval * states * (dimy - y - 1)) * 100, 100, buf, 0, 1000, '\0'); \
      } \
      if(y == dimy - 1 && strlen(ncp->title)){ \
        ncplane_printf_yx(ncp->ncp, dimy - y - 1, PREFIXCOLUMNS - strlen(buf), "%s %s", buf, ncp->title); \
      }else{ \
        ncplane_printf_yx(ncp->ncp, dimy - y - 1, PREFIXCOLUMNS - strlen(buf), "%s", buf); \
      } \
    } \
  }else if(strlen(ncp->title)){ \
    uint64_t channels = 0; \
    calc_gradient_channels(&channels, ncp->minchannels, ncp->minchannels, \
                           ncp->maxchannels, ncp->maxchannels, dimy - 1, 0, dimy, dimx); \
    ncplane_set_channels(ncp->ncp, channels); \
    ncplane_printf_yx(ncp->ncp, 0, PREFIXCOLUMNS - strlen(ncp->title), "%s", ncp->title); \
  } \
  ncplane_set_styles(ncp->ncp, NCSTYLE_NONE); \
  if(finalx < startx){ /* exit on pathologically narrow planes */ \
    return 0; \
  } \
  if(!interval){ \
    interval = 1; \
  } \
  int idx = ncp->slotstart; /* idx holds the real slot index; we move backwards */ \
  for(int x = finalx ; x >= startx ; --x){ \
    /* a single column might correspond to more than 1 ('scale', up to \
       MAXWIDTH) slot's worth of samples. prepare the working gval set. */ \
    T gvals[MAXWIDTH]; \
    /* load it retaining the same ordering we have in the actual array */ \
    for(int i = scale - 1 ; i >= 0 ; --i){ \
      gvals[i] = ncp->slots[idx]; /* clip the value at the limits of the graph */ \
      if(gvals[i] < ncp->miny){ \
        gvals[i] = ncp->miny; \
      } \
      if(gvals[i] > ncp->maxy){ \
        gvals[i] = ncp->maxy; \
      } \
      /* FIXME if there are an odd number, only go up through the valid ones... */ \
      if(--idx < 0){ \
        idx = ncp->slotcount - 1; \
      } \
    } \
    /* starting from the least-significant row, progress in the more significant \
       direction, drawing egcs from the grid specification, aborting early if \
       we can't draw anything in a given cell. */ \
    T intervalbase = ncp->miny; \
    const wchar_t* egc = ncp->bset->egcs; \
    bool done = !ncp->bset->fill; \
    for(int y = 0 ; y < dimy ; ++y){ \
      uint64_t channels = 0; \
      calc_gradient_channels(&channels, ncp->minchannels, ncp->minchannels, \
                             ncp->maxchannels, ncp->maxchannels, y, x, dimy, dimx); \
      ncplane_set_channels(ncp->ncp, channels); \
      size_t egcidx = 0, sumidx = 0; \
      /* if we've got at least one interval's worth on the number of positions \
         times the number of intervals per position plus the starting offset, \
         we're going to print *something* */ \
      for(int i = 0 ; i < scale ; ++i){ \
        sumidx *= states; \
        if(intervalbase < gvals[i]){ \
          if(ncp->exponentiali){ \
            /* we want the log-base-interval of gvals[i] */ \
            double scaled = log(gvals[i] - ncp->miny) / log(interval); \
            double sival = intervalbase ? log(intervalbase) / log(interval) : 0; \
            egcidx = scaled - sival; \
          }else{ \
            egcidx = (gvals[i] - intervalbase) / interval; \
          } \
          if(egcidx >= states){ \
            egcidx = states - 1; \
            done = false; \
          } \
          sumidx += egcidx; \
        }else{ \
          egcidx = 0; \
        } \
/* printf(stderr, "y: %d i(scale): %d gvals[%d]: %ju egcidx: %zu sumidx: %zu interval: %f intervalbase: %ju\n", y, i, i, gvals[i], egcidx, sumidx, interval, intervalbase); */ \
      } \
      /* if we're not UTF8, we can only arrive here via NCBLIT_1x1 (otherwise \
         we would have errored out during construction). even then, however, \
         we need handle ASCII differently, since it can't print full block. \
         in ASCII mode, sumidx != 0 means swap colors and use space. in all \
         modes, sumidx == 0 means don't do shit, since we erased earlier. */ \
/* if(sumidx)fprintf(stderr, "dimy: %d y: %d x: %d sumidx: %zu egc[%zu]: %lc\n", dimy, y, x, sumidx, sumidx, egc[sumidx]); */ \
      if(sumidx){ \
        if(notcurses_canutf8(ncplane_notcurses(ncp->ncp))){ \
          char utf8[MB_CUR_MAX + 1]; \
          int bytes = wctomb(utf8, egc[sumidx]); \
          if(bytes < 0){ \
            return -1; \
          } \
          utf8[bytes] = '\0'; \
          nccell* c = ncplane_cell_ref_yx(ncp->ncp, dimy - y - 1, x); \
          cell_set_bchannel(c, ncchannels_bchannel(channels)); \
          cell_set_fchannel(c, ncchannels_fchannel(channels)); \
          nccell_set_styles(c, NCSTYLE_NONE); \
          if(pool_blit_direct(&ncp->ncp->pool, c, utf8, bytes, 1) <= 0){ \
            return -1; \
          } \
        }else{ \
          const uint64_t swapbg = ncchannels_bchannel(channels); \
          const uint64_t swapfg = ncchannels_fchannel(channels); \
          ncchannels_set_bchannel(&channels, swapfg); \
          ncchannels_set_fchannel(&channels, swapbg); \
          ncplane_set_channels(ncp->ncp, channels); \
          if(ncplane_putchar_yx(ncp->ncp, dimy - y - 1, x, ' ') <= 0){ \
            return -1; \
          } \
          ncchannels_set_bchannel(&channels, swapbg); \
          ncchannels_set_fchannel(&channels, swapfg); \
          ncplane_set_channels(ncp->ncp, channels); \
        } \
      } \
      if(done){ \
        break; \
      } \
      if(ncp->exponentiali){ \
        intervalbase = ncp->miny + pow(interval, (y + 1) * states - 1); \
      }else{ \
        intervalbase += (states * interval); \
      } \
    } \
  } \
  if(ncp->printsample){ \
    int lastslot = ncp->slotstart ? ncp->slotstart - 1 : ncp->slotcount - 1; \
    ncplane_set_styles(ncp->ncp, ncp->legendstyle); \
    ncplane_printf_aligned(ncp->ncp, 0, NCALIGN_RIGHT, "%ju", (uintmax_t)ncp->slots[lastslot]); \
  } \
  ncplane_home(ncp->ncp); \
  return 0; \
} \
\
static bool \
create_##T(nc##X##plot* ncpp, ncplane* n, const ncplot_options* opts, const T miny, const T maxy, \
           const T trueminy, const T truemaxy){ \
  ncplot_options zeroed = {}; \
  if(!opts){ \
    opts = &zeroed; \
  } \
  if(opts->flags >= (NCPLOT_OPTION_PRINTSAMPLE << 1u)){ \
    logwarn(ncplane_notcurses(n), "Provided unsupported flags %016jx\n", (uintmax_t)opts->flags); \
  } \
  const notcurses* nc = ncplane_notcurses_const(n); \
  /* if miny == maxy (enabling domain detection), they both must be equal to 0 */ \
  if(miny == maxy && miny){ \
    ncplane_destroy(n); \
    return false; \
  } \
  if(opts->rangex < 0){ \
    logerror(nc, "Supplied negative independent range %d\n", opts->rangex); \
    ncplane_destroy(n); \
    return false; \
  } \
  if(maxy < miny){ \
    ncplane_destroy(n); \
    return false; \
  } \
  /* DETECTMAXONLY can't be used without domain detection */ \
  if(opts->flags & NCPLOT_OPTION_DETECTMAXONLY && (miny != maxy)){ \
    logerror(nc, "Supplied DETECTMAXONLY without domain detection"); \
    ncplane_destroy(n); \
    return false; \
  } \
  const notcurses* notc = ncplane_notcurses(n); \
  ncblitter_e blitfxn = opts ? opts->gridtype : NCBLIT_DEFAULT; \
  if(blitfxn == NCBLIT_DEFAULT){ \
    blitfxn = ncplot_defblitter(notc); \
  } \
  bool degrade_blitter = !(opts && (opts->flags & NCPLOT_OPTION_NODEGRADE)); \
  const struct blitset* bset = lookup_blitset(&notc->tcache, blitfxn, degrade_blitter); \
  if(bset == NULL){ \
    ncplane_destroy(n); \
    return false; \
  } \
  int sdimy, sdimx; \
  ncplane_dim_yx(n, &sdimy, &sdimx); \
  if(sdimx <= 0){ \
    ncplane_destroy(n); \
    return false; \
  } \
  int dimx = sdimx; \
  ncpp->title = strdup(opts->title ? opts->title : ""); \
  ncpp->rangex = opts->rangex; \
  /* if we're sizing the plot based off the plane dimensions, scale it by the \
     plot geometry's width for all calculations */ \
  const int scaleddim = dimx * bset->width; \
  const int scaledprefixlen = PREFIXCOLUMNS * bset->width; \
  if((ncpp->slotcount = ncpp->rangex) == 0){ \
    ncpp->slotcount = scaleddim; \
  } \
  if(dimx < ncpp->rangex){ \
    ncpp->slotcount = scaleddim; \
  } \
  ncpp->legendstyle = opts->legendstyle; \
  if( (ncpp->labelaxisd = opts->flags & NCPLOT_OPTION_LABELTICKSD) ){ \
    if(ncpp->slotcount + scaledprefixlen > scaleddim){ \
      if(scaleddim > scaledprefixlen){ \
        ncpp->slotcount = scaleddim - scaledprefixlen; \
      } \
    } \
  } \
  size_t slotsize = sizeof(*ncpp->slots) * ncpp->slotcount; \
  ncpp->slots = malloc(slotsize); \
  if(ncpp->slots == NULL){ \
    ncplane_destroy(n); \
    return false; \
  } \
  memset(ncpp->slots, 0, slotsize); \
  ncpp->ncp = n; \
  ncpp->maxchannels = opts->maxchannels; \
  ncpp->minchannels = opts->minchannels; \
  ncpp->bset = bset; \
  ncpp->miny = miny; \
  ncpp->maxy = maxy; \
  ncpp->vertical_indep = opts->flags & NCPLOT_OPTION_VERTICALI; \
  ncpp->exponentiali = opts->flags & NCPLOT_OPTION_EXPONENTIALD; \
  ncpp->detectonlymax = opts->flags & NCPLOT_OPTION_DETECTMAXONLY; \
  ncpp->printsample = opts->flags & NCPLOT_OPTION_PRINTSAMPLE; \
  if( (ncpp->detectdomain = (miny == maxy)) ){ \
    ncpp->maxy = trueminy; \
    ncpp->miny = truemaxy; \
  } \
  ncpp->slotstart = 0; \
  ncpp->slotx = 0; \
  redraw_plot_##T(ncpp); \
  return true; \
} \
/* if x is less than the window, return -1, as the sample will be thrown away. \
   if the x is within the current window, find the proper slot and update it. \
   otherwise, the x is the newest sample. if it is obsoletes all existing slots, \
   reset them, and write the new sample anywhere. otherwise, write it to the \
   proper slot based on the current newest slot. */ \
int window_slide_##T(nc##X##plot* ncp, int64_t x){ \
  if(x < ncp->slotx - (ncp->slotcount - 1)){ /* x is behind window, won't be counted */ \
    return -1; \
  }else if(x <= ncp->slotx){ /* x is within window, do nothing */ \
    return 0; \
  } /* x is newest; we might be keeping some, might not */ \
  int64_t xdiff = x - ncp->slotx; /* the raw amount we're advancing */ \
  ncp->slotx = x; \
  if(xdiff >= ncp->slotcount){ /* we're throwing away all old samples, write to 0 */ \
    memset(ncp->slots, 0, sizeof(*ncp->slots) * ncp->slotcount); \
    ncp->slotstart = 0; \
    return 0; \
  } \
  /* we're throwing away only xdiff slots, which is less than slotcount. \
     first, we'll try to clear to the right...number to reset on the right of \
     the circular buffer. min of (available at current or to right, xdiff) */ \
  int slotsreset = ncp->slotcount - ncp->slotstart - 1; \
  if(slotsreset > xdiff){ \
    slotsreset = xdiff; \
  } \
  if(slotsreset){ \
    memset(ncp->slots + ncp->slotstart + 1, 0, slotsreset * sizeof(*ncp->slots)); \
  } \
  ncp->slotstart = (ncp->slotstart + xdiff) % ncp->slotcount; \
  xdiff -= slotsreset; \
  if(xdiff){ /* throw away some at the beginning */ \
    memset(ncp->slots, 0, xdiff * sizeof(*ncp->slots)); \
  } \
  return 0; \
} \
\
/* x must be within n's window at this point */ \
void update_sample_##T(nc##X##plot* ncp, int64_t x, T y, bool reset){ \
  const int64_t diff = ncp->slotx - x; /* amount behind */ \
  const int idx = (ncp->slotstart + ncp->slotcount - diff) % ncp->slotcount; \
  if(reset){ \
    ncp->slots[idx] = y; \
  }else{ \
    ncp->slots[idx] += y; \
  } \
} \
\
/* if we're doing domain detection, update the domain to reflect the value we \
   just set. if we're not, check the result against the known ranges, and \
   return -1 if the value is outside of that range. */ \
int update_domain_##T(nc##X##plot* ncp, uint64_t x){ \
  const T val = ncp->slots[x % ncp->slotcount]; \
  if(ncp->detectdomain){ \
    if(val > ncp->maxy){ \
      ncp->maxy = val; \
    } \
    if(!ncp->detectonlymax){ \
      if(val < ncp->miny){ \
        ncp->miny = val; \
      } \
    } \
    return 0; \
  } \
  if(val > ncp->maxy || val < ncp->miny){ \
    return -1; \
  } \
  return 0; \
} \
int set_sample_##T(nc##X##plot* ncpp, uint64_t x, T y){ \
  if(window_slide_##T(ncpp, x)){ \
    return -1; \
  } \
  update_sample_##T(ncpp, x, y, true); \
  if(update_domain_##T(ncpp, x)){ \
    return -1; \
  } \
  return redraw_plot_##T(ncpp); \
} \
/* Add to or set the value corresponding to this x. If x is beyond the current \
   x window, the x window is advanced to include x, and values passing beyond \
   the window are lost. The first call will place the initial window. The plot \
   will be redrawn, but notcurses_render() is not called. */ \
int add_sample_##T(nc##X##plot* ncpp, uint64_t x, T y){ \
  if(window_slide_##T(ncpp, x)){ \
    return -1; \
  } \
  update_sample_##T(ncpp, x, y, false); \
  if(update_domain_##T(ncpp, x)){ \
    return -1; \
  } \
  return redraw_plot_##T(ncpp); \
} \
int sample_##T(const nc##X##plot* ncp, int64_t x, T* y){ \
  if(x < ncp->slotx - (ncp->slotcount - 1)){ /* x is behind window */ \
    return -1; \
  }else if(x > ncp->slotx){ /* x is ahead of window */ \
    return -1; \
  } \
  *y = ncp->slots[x % ncp->slotcount]; \
  return 0; \
} \
void destroy_##T(nc##X##plot* ncpp){ \
  free(ncpp->title); \
  free(ncpp->slots); \
  ncplane_destroy(ncpp->ncp); \
}

CREATE(uint64_t, u)
CREATE(double, d)

ncuplot* ncuplot_create(ncplane* n, const ncplot_options* opts, uint64_t miny, uint64_t maxy){
  ncuplot* ret = malloc(sizeof(*ret));
  if(ret){
    memset(ret, 0, sizeof(*ret));
    if(create_uint64_t(ret, n, opts, miny, maxy, 0, UINT64_MAX)){
      return ret;
    }
    free(ret);
    ret = NULL;
  }
  return ret;
}

ncplane* ncuplot_plane(ncuplot* n){
  return n->ncp;
}

int ncuplot_add_sample(ncuplot* n, uint64_t x, uint64_t y){
  return add_sample_uint64_t(n, x, y);
}

int ncuplot_set_sample(ncuplot* n, uint64_t x, uint64_t y){
  return set_sample_uint64_t(n, x, y);
}

void ncuplot_destroy(ncuplot* n){
  if(n){
    destroy_uint64_t(n);
    free(n);
  }
}

ncdplot* ncdplot_create(ncplane* n, const ncplot_options* opts, double miny, double maxy){
  ncdplot* ret = malloc(sizeof(*ret));
  if(ret){
    memset(ret, 0, sizeof(*ret));
    if(create_double(ret, n, opts, miny, maxy, -DBL_MAX, DBL_MAX)){
      return ret;
    }
    free(ret);
    ret = NULL;
  }
  return ret;
}

ncplane* ncdplot_plane(ncdplot* n){
  return n->ncp;
}

int ncdplot_add_sample(ncdplot* n, uint64_t x, double y){
  return add_sample_double(n, x, y);
}

int ncdplot_set_sample(ncdplot* n, uint64_t x, double y){
  return set_sample_double(n, x, y);
}

int ncuplot_sample(const ncuplot* n, uint64_t x, uint64_t* y){
  return sample_uint64_t(n, x, y);
}

int ncdplot_sample(const ncdplot* n, uint64_t x, double* y){
  return sample_double(n, x, y);
}

void ncdplot_destroy(ncdplot* n) {
  if(n){
    destroy_double(n);
    free(n);
  }
}
