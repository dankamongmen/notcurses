#include <string.h>
#include "internal.h"

ncprogbar* ncprogbar_create(ncplane* n, const ncprogbar_options* opts){
  ncprogbar_options default_opts;
  if(opts == NULL){
    memset(&default_opts, 0, sizeof(default_opts));
    opts = &default_opts;
  }
  if(opts->flags > (NCPROGBAR_OPTION_RETROGRADE << 1u)){
    logwarn(ncplane_notcurses(n), "Invalid flags %016lx\n", opts->flags);
  }
  ncprogbar* ret = malloc(sizeof(*ret));
  if(ret){
    ret->ncp = n;
    ret->ulchannel = opts->ulchannel;
    ret->urchannel = opts->urchannel;
    ret->blchannel = opts->blchannel;
    ret->brchannel = opts->brchannel;
    ret->retrograde = opts->flags & NCPROGBAR_OPTION_RETROGRADE;
  }
  return ret;
}

ncplane* ncprogbar_plane(ncprogbar* n){
  return n->ncp;
}

static const char right_egcs[8][5] = {
  "🮇", "🮇", "🮈", "▐", "🮉", "🮊", "🮋", "█",
};

static const char left_egcs[8][5] = {
  "▏", "▎", "▍", "▌", "▋", "▊", "▉", "█",
};

static const char down_egcs[8][5] = {
  "▔", "🮂", "🮃", "▀", "🮄", "🮅", "🮆", "█",
};

static const char up_egcs[8][5] = {
  "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█",
};

static int
progbar_redraw(ncprogbar* n){
  struct ncplane* ncp = ncprogbar_plane(n);
  // get current dimensions; they might have changed
  int dimy, dimx;
  ncplane_dim_yx(ncp, &dimy, &dimx);
  const bool horizontal = dimx > dimy;
  int range, delt, pos;
  const char* egcs;
  uint32_t ul, ur, bl, br;
  if(horizontal){
    range = dimx;
    delt = 1;
    pos = 0;
    if(n->retrograde){
      egcs = *right_egcs;
      ul = n->urchannel; ur = n->brchannel;
      bl = n->ulchannel; br = n->blchannel;
    }else{
      egcs = *left_egcs;
      ul = n->blchannel; ur = n->ulchannel;
      bl = n->brchannel; br = n->urchannel;
    }
  }else{
    range = dimy;
    delt = -1;
    pos = range - 1;
    if(n->retrograde){
      egcs = *down_egcs;
      ul = n->brchannel; ur = n->blchannel;
      bl = n->urchannel; br = n->ulchannel;
    }else{
      egcs = *up_egcs;
      ul = n->ulchannel; ur = n->urchannel;
      bl = n->blchannel; br = n->brchannel;
    }
  }
  ncplane_home(ncp);
  if(notcurses_canutf8(ncplane_notcurses(ncp))){
    if(ncplane_highgradient(ncp, ul, ur, bl, br, dimy - 1, dimx - 1) <= 0){
      return -1;
    }
  }else{
    if(ncplane_gradient(ncp, " ", 0, ul, ur, bl, br, dimy - 1, dimx - 1) <= 0){
      return -1;
    }
  }
  double progress = n->progress * range;
  if(n->retrograde){
    delt *= -1;
    if(pos){
      pos = 0;
    }else{
      pos = range - 1;
    }
    if(horizontal){
      progress = range - progress;
    }
  }else if(!horizontal){
    progress = range - progress;
  }
  double eachcell = (1.0 / range); // how much each cell is worth
  double chunk = n->progress;
  const int chunks = n->progress / eachcell;
  chunk -= eachcell * chunks;
  pos += delt * chunks;
  const int egcidx = (int)(chunk / (eachcell / 8));
  const char* egc = egcs + egcidx * 5;
  if(horizontal){
    for(int freepos = 0 ; freepos < dimy ; ++freepos){
      if(notcurses_canutf8(ncplane_notcurses(ncp))){
        nccell* c = ncplane_cell_ref_yx(ncp, freepos, pos);
        if(pool_blit_direct(&ncp->pool, c, egc, strlen(egc), 1) <= 0){
          return -1;
        }
        cell_set_bchannel(c, 0);
      }else{
        if(ncplane_putchar_yx(ncp, freepos, pos, ' ') <= 0){
          return -1;
        }
      }
    }
  }else{
    for(int freepos = 0 ; freepos < dimx ; ++freepos){
      if(notcurses_canutf8(ncplane_notcurses(ncp))){
        nccell* c = ncplane_cell_ref_yx(ncp, pos, freepos);
        if(pool_blit_direct(&ncp->pool, c, egc, strlen(egc), 1) <= 0){
          return -1;
        }
        cell_set_bchannel(c, 0);
      }else{
        if(ncplane_putchar_yx(ncp, pos, freepos, ' ') <= 0){
          return -1;
        }
      }
    }
  }
  pos += delt;
  while(pos >= 0 && pos < range){
    if(horizontal){
      for(int freepos = 0 ; freepos < dimy ; ++freepos){
        nccell* c = ncplane_cell_ref_yx(ncp, freepos, pos);
        cell_release(ncp, c);
        cell_init(c);
      }
    }else{
      for(int freepos = 0 ; freepos < dimx ; ++freepos){
        nccell* c = ncplane_cell_ref_yx(ncp, pos, freepos);
        cell_release(ncp, c);
        cell_init(c);
      }
    }
    pos += delt;
  }
  return 0;
}

int ncprogbar_set_progress(ncprogbar* n, double p){
//fprintf(stderr, "PROGRESS: %g\n", p);
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
  if(n){
    ncplane_destroy(n->ncp);
    free(n);
  }
}
