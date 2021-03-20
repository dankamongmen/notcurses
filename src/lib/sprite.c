#include "internal.h"

void sprixel_free(sprixel* s){
  if(s){
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
sprixel* sprixel_create(ncplane* n, const char* s, int bytes, int sprixelid,
                        int dimy, int dimx, int pixy, int pixx){
  sprixel* ret = malloc(sizeof(sprixel));
  if(ret){
    if((ret->glyph = memdup(s, bytes + 1)) == NULL){
      free(ret);
      return NULL;
    }
    ret->invalidated = SPRIXEL_INVALIDATED;
    ret->n = n;
    ret->dimy = dimy;
    ret->dimx = dimx;
    ret->pixx = pixx;
    ret->pixy = pixy;
    if(ncplane_pile(n)){
      notcurses* nc = ncplane_notcurses(n);
      ret->next = nc->sprixelcache;
      nc->sprixelcache = ret;
      ret->id = sprixelid;
    }
  }
  return ret;
}
