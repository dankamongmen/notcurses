#include "internal.h"

void sprixel_free(sprixel* s){
  if(s){
    free(s->tacache);
    free(s->glyph);
    free(s);
  }
}

void sprixel_hide(sprixel* s){
  s->n->sprite = NULL;
  s->invalidated = SPRIXEL_HIDE;
  ncplane_yx(s->n, &s->y, &s->x);
  ncplane_dim_yx(s->n, &s->dimy, &s->dimx);
  s->n = NULL;
}

// y and x are the cell geometry, not the pixel geometry
sprixel* sprixel_create(ncplane* n, const char* s, int bytes, int placey, int placex,
                        int sprixelid, int dimy, int dimx, int pixy, int pixx){
  sprixel* ret = malloc(sizeof(sprixel));
  if(ret){
    if((ret->glyph = memdup(s, bytes + 1)) == NULL){
      free(ret);
      return NULL;
    }
    const size_t tasize = sizeof(*ret->tacache) * dimy * dimx;
    if((ret->tacache = malloc(tasize)) == NULL){
      free(ret->glyph);
      free(ret);
      return NULL;
    }
    // FIXME set up transparency cache when generating sprixel;
    // we manage only annihilation cache
    memset(ret->tacache, 0, tasize);
    ret->invalidated = SPRIXEL_INVALIDATED;
    ret->n = n;
    ret->dimy = dimy;
    ret->dimx = dimx;
    ret->pixx = pixx;
    ret->pixy = pixy;
    ret->y = placey;
    ret->x = placex;
    if(ncplane_pile(n)){
      notcurses* nc = ncplane_notcurses(n);
      ret->next = nc->sprixelcache;
      nc->sprixelcache = ret;
      ret->id = sprixelid;
    }
  }
  return ret;
}

int sprite_wipe_cell(const notcurses* nc, sprixel* s, int ycell, int xcell){
  if(!nc->tcache.pixel_cell_wipe){
    return 0;
  }
  if(ycell >= s->dimy){
    return -1;
  }
  if(xcell >= s->dimx){
    return -1;
  }
  if(s->tacache[s->dimx * ycell + xcell] == 2){
    return 0; // already annihilated
  }
  s->tacache[s->dimx * ycell + xcell] = 2;
  int r = nc->tcache.pixel_cell_wipe(nc, s, ycell, xcell);
  if(r == 0){
    s->invalidated = SPRIXEL_INVALIDATED;
  }
  return r;
}
