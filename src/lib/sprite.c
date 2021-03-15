#include "internal.h"

void sprixel_free(sprixel* s){
  if(s){
    free(s->glyph);
    free(s);
  }
}

sprixel* sprixel_create(ncplane* n, const char* s, int bytes){
  sprixel* ret = malloc(sizeof(sprixel));
  if(ret){
    if((ret->glyph = memdup(s, bytes + 1)) == NULL){
      free(ret);
      return NULL;
    }
    ret->invalidated = 1;
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
