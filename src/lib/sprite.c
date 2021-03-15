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

sprixel* sprixel_create(ncplane* n, const char* s, int bytes){
  sprixel* ret = malloc(sizeof(sprixel));
  if(ret){
    if((ret->glyph = memdup(s, bytes + 1)) == NULL){
      free(ret);
      return NULL;
    }
    ret->invalidated = SPRIXEL_INVALIDATED;
    ret->n = n;
    if(ncplane_pile(n)){
      notcurses* nc = ncplane_notcurses(n);
      ret->next = nc->sprixelcache;
      nc->sprixelcache = ret;
      ret->id = nc->sprixelnonce++; // FIXME should be atomic
    }
  }
  return ret;
}
