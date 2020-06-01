#include "internal.h"

typedef struct ncfadectx {
  int rows;                     // number of rows when allocated
  int cols;                     // number of columns when allocated
  int maxsteps;                 // maximum number of iterations
  unsigned maxr, maxg, maxb;    // maxima across foreground channels
  unsigned maxbr, maxbg, maxbb; // maxima across background channels
  uint64_t* channels;           // all channels from the framebuffer
} ncfadectx;

// These arrays are too large to be safely placed on the stack. Get an atomic
// snapshot of all channels on the plane. While copying the snapshot, determine
// the maxima across each of the six components.
static int
alloc_ncplane_palette(ncplane* n, ncfadectx* pp){
  ncplane_dim_yx(n, &pp->rows, &pp->cols);
  // add an additional element for the background cell
  int size = pp->rows * pp->cols + 1;
  if((pp->channels = malloc(sizeof(*pp->channels) * size)) == NULL){
    return -1;
  }
  pp->maxr = pp->maxg = pp->maxb = 0;
  pp->maxbr = pp->maxbg = pp->maxbb = 0;
  unsigned r, g, b, br, bg, bb;
  uint64_t channels;
  int y, x;
  for(y = 0 ; y < pp->rows ; ++y){
    for(x = 0 ; x < pp->cols ; ++x){
      channels = n->fb[nfbcellidx(n, y, x)].channels;
      pp->channels[y * pp->cols + x] = channels;
      channels_fg_rgb(channels, &r, &g, &b);
      if(r > pp->maxr){
        pp->maxr = r;
      }
      if(g > pp->maxg){
        pp->maxg = g;
      }
      if(b > pp->maxb){
        pp->maxb = b;
      }
      channels_bg_rgb(channels, &br, &bg, &bb);
      if(br > pp->maxbr){
        pp->maxbr = br;
      }
      if(bg > pp->maxbg){
        pp->maxbg = bg;
      }
      if(bb > pp->maxbb){
        pp->maxbb = bb;
      }
    }
  }
  // FIXME factor this duplication out
  channels = n->basecell.channels;
  pp->channels[y * pp->cols] = channels;
  channels_fg_rgb(channels, &r, &g, &b);
  if(r > pp->maxr){
    pp->maxr = r;
  }
  if(g > pp->maxg){
    pp->maxg = g;
  }
  if(b > pp->maxb){
    pp->maxb = b;
  }
  channels_bg_rgb(channels, &br, &bg, &bb);
  if(br > pp->maxbr){
    pp->maxbr = br;
  }
  if(bg > pp->maxbg){
    pp->maxbg = bg;
  }
  if(bb > pp->maxbb){
    pp->maxbb = bb;
  }
  int maxfsteps = pp->maxg > pp->maxr ? (pp->maxb > pp->maxg ? pp->maxb : pp->maxg) :
                  (pp->maxb > pp->maxr ? pp->maxb : pp->maxr);
  int maxbsteps = pp->maxbg > pp->maxbr ? (pp->maxbb > pp->maxbg ? pp->maxbb : pp->maxbg) :
                  (pp->maxbb > pp->maxbr ? pp->maxbb : pp->maxbr);
  pp->maxsteps = maxfsteps > maxbsteps ? maxfsteps : maxbsteps;
  if(pp->maxsteps == 0){
    pp->maxsteps = 1;
  }
  return 0;
}

static int
ncplane_fadein_internal(ncplane* n, const struct timespec* ts,
                        fadecb fader, ncfadectx* pp, void* curry){
  uint64_t nanosecs_total = ts->tv_sec * NANOSECS_IN_SEC + ts->tv_nsec;
  uint64_t nanosecs_step = nanosecs_total / pp->maxsteps;
  if(nanosecs_step == 0){
    nanosecs_step = 1;
  }
  struct timespec times;
  clock_gettime(CLOCK_MONOTONIC, &times);
  // Start time in absolute nanoseconds
  uint64_t startns = times.tv_sec * NANOSECS_IN_SEC + times.tv_nsec;
  // Current time, sampled each iteration
  uint64_t curns;
  int ret = 0;
  do{
    clock_gettime(CLOCK_MONOTONIC, &times);
    curns = times.tv_sec * NANOSECS_IN_SEC + times.tv_nsec;
    int iter = (curns - startns) / nanosecs_step + 1;
    if(iter > pp->maxsteps){
      break;
    }
    int y, x;
    // each time through, we need look each cell back up, due to the
    // possibility of a resize event :/
    int dimy, dimx;
    ncplane_dim_yx(n, &dimy, &dimx);
    for(y = 0 ; y < pp->rows && y < dimy ; ++y){
      for(x = 0 ; x < pp->cols && x < dimx; ++x){
        unsigned r, g, b;
        channels_fg_rgb(pp->channels[pp->cols * y + x], &r, &g, &b);
        unsigned br, bg, bb;
        channels_bg_rgb(pp->channels[pp->cols * y + x], &br, &bg, &bb);
        cell* c = &n->fb[dimx * y + x];
        if(!cell_fg_default_p(c)){
          r = r * iter / pp->maxsteps;
          g = g * iter / pp->maxsteps;
          b = b * iter / pp->maxsteps;
          cell_set_fg_rgb(c, r, g, b);
        }
        if(!cell_bg_default_p(c)){
          br = br * iter / pp->maxsteps;
          bg = bg * iter / pp->maxsteps;
          bb = bb * iter / pp->maxsteps;
          cell_set_bg_rgb(c, br, bg, bb);
        }
      }
    }
    uint64_t nextwake = (iter + 1) * nanosecs_step + startns;
    struct timespec sleepspec;
    sleepspec.tv_sec = nextwake / NANOSECS_IN_SEC;
    sleepspec.tv_nsec = nextwake % NANOSECS_IN_SEC;
    if(fader){
      ret |= fader(n->nc, n, &sleepspec, curry);
    }else{
      ret |= notcurses_render(n->nc);
      // clock_nanosleep() has no love for CLOCK_MONOTONIC_RAW, at least as
      // of Glibc 2.29 + Linux 5.3 (or FreeBSD 12) :/.
      ret |= clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &sleepspec, NULL);
    }
    if(ret){
      break;
    }
  }while(true);
  return ret;
}

int ncplane_fadeout(ncplane* n, const struct timespec* ts, fadecb fader, void* curry){
  ncfadectx pp;
  if(!n->nc->tcache.RGBflag && !n->nc->tcache.CCCflag){ // terminal can't fade
    return -1;
  }
  if(alloc_ncplane_palette(n, &pp)){
    return -1;
  }
  uint64_t nanosecs_total = timespec_to_ns(ts);
  uint64_t nanosecs_step = nanosecs_total / pp.maxsteps;
  if(nanosecs_step == 0){
    nanosecs_step = 1;
  }
  struct timespec times;
  clock_gettime(CLOCK_MONOTONIC, &times);
  // Start time in absolute nanoseconds
  uint64_t startns = timespec_to_ns(&times);
  int ret = 0;
  do{
    unsigned br, bg, bb;
    unsigned r, g, b;
    uint64_t curns = times.tv_sec * NANOSECS_IN_SEC + times.tv_nsec;
    int iter = (curns - startns) / nanosecs_step + 1;
    if(iter > pp.maxsteps){
      break;
    }
    int y, x;
    // each time through, we need look each cell back up, due to the
    // possibility of a resize event :/
    int dimy, dimx;
    ncplane_dim_yx(n, &dimy, &dimx);
    for(y = 0 ; y < pp.rows && y < dimy ; ++y){
      for(x = 0 ; x < pp.cols && x < dimx; ++x){
        cell* c = &n->fb[dimx * y + x];
        if(!cell_fg_default_p(c)){
          channels_fg_rgb(pp.channels[pp.cols * y + x], &r, &g, &b);
          r = r * (pp.maxsteps - iter) / pp.maxsteps;
          g = g * (pp.maxsteps - iter) / pp.maxsteps;
          b = b * (pp.maxsteps - iter) / pp.maxsteps;
          cell_set_fg_rgb(c, r, g, b);
        }
        if(!cell_bg_default_p(c)){
          channels_bg_rgb(pp.channels[pp.cols * y + x], &br, &bg, &bb);
          br = br * (pp.maxsteps - iter) / pp.maxsteps;
          bg = bg * (pp.maxsteps - iter) / pp.maxsteps;
          bb = bb * (pp.maxsteps - iter) / pp.maxsteps;
          cell_set_bg_rgb(c, br, bg, bb);
        }
      }
    }
    cell* c = &n->basecell;
    if(!cell_fg_default_p(c)){
      channels_fg_rgb(pp.channels[pp.cols * y], &r, &g, &b);
      r = r * (pp.maxsteps - iter) / pp.maxsteps;
      g = g * (pp.maxsteps - iter) / pp.maxsteps;
      b = b * (pp.maxsteps - iter) / pp.maxsteps;
      cell_set_fg_rgb(&n->basecell, r, g, b);
    }
    if(!cell_bg_default_p(c)){
      channels_bg_rgb(pp.channels[pp.cols * y], &br, &bg, &bb);
      br = br * (pp.maxsteps - iter) / pp.maxsteps;
      bg = bg * (pp.maxsteps - iter) / pp.maxsteps;
      bb = bb * (pp.maxsteps - iter) / pp.maxsteps;
      cell_set_bg_rgb(&n->basecell, br, bg, bb);
    }
    uint64_t nextwake = (iter + 1) * nanosecs_step + startns;
    struct timespec sleepspec;
    sleepspec.tv_sec = nextwake / NANOSECS_IN_SEC;
    sleepspec.tv_nsec = nextwake % NANOSECS_IN_SEC;
    int rsleep;
    if(fader){
      ret = fader(n->nc, n, &sleepspec, curry);
    }else{
      ret = notcurses_render(n->nc);
    }
    if(ret){
      break;
    }
    // clock_nanosleep() has no love for CLOCK_MONOTONIC_RAW, at least as
    // of Glibc 2.29 + Linux 5.3 (or FreeBSD 12) :/.
    rsleep = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &sleepspec, NULL);
    if(rsleep){
      break;
    }
    clock_gettime(CLOCK_MONOTONIC, &times);
  }while(true);
  free(pp.channels);
  return ret;
}

int ncplane_fadein(ncplane* n, const struct timespec* ts, fadecb fader, void* curry){
  ncfadectx pp;
  if(!n->nc->tcache.RGBflag && !n->nc->tcache.CCCflag){ // terminal can't fade
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    if(fader){
      fader(n->nc, n, &now, curry);
    }else{
      notcurses_render(n->nc);
    }
    return -1;
  }
  if(alloc_ncplane_palette(n, &pp)){
    return -1;
  }
  int ret = ncplane_fadein_internal(n, ts, fader, &pp, curry);
  free(pp.channels);
  return ret;
}

int ncplane_pulse(ncplane* n, const struct timespec* ts, fadecb fader, void* curry){
  ncfadectx pp;
  int ret;
  if(!n->nc->tcache.RGBflag && !n->nc->tcache.CCCflag){ // terminal can't fade
    return -1;
  }
  if(alloc_ncplane_palette(n, &pp)){
    return -1;
  }
  for(;;){
    ret = ncplane_fadein_internal(n, ts, fader, &pp, curry);
    if(ret){
      break;
    }
    ret = ncplane_fadeout(n, ts, fader, curry);
    if(ret){
      break;
    }
  }
  free(pp.channels);
  return ret;
}
