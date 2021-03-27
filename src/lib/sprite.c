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
  s->n = NULL;
}

void sprixel_invalidate(sprixel* s){
  if(s->invalidated != SPRIXEL_HIDE){
    s->invalidated = SPRIXEL_INVALIDATED;
  }
}

sprixel* sprixel_by_id(notcurses* nc, uint32_t id){
  for(sprixel* cur = nc->sprixelcache ; cur ; cur = cur->next){
    if(cur->id == id){
      return cur;
    }
  }
  return NULL;
}

// s ought already have been scrubbed according to the T-A matrix
sprixel* sprixel_update(sprixel* s, char* g, int bytes){
  free(s->glyph);
  s->glyph = g;
  s->glyphlen = bytes;
  s->invalidated = SPRIXEL_INVALIDATED;
  return s;
}

// 'y' and 'x' are the cell geometry, not the pixel geometry. takes
// ownership of 's' on success.
sprixel* sprixel_create(ncplane* n, char* s, int bytes, int placey, int placex,
                        int sprixelid, int dimy, int dimx, int pixy, int pixx,
                        int parse_start, sprixcell_e* tacache){
  sprixel* ret = malloc(sizeof(sprixel));
  if(ret){
    ret->glyph = s;
    ret->glyphlen = bytes;
    ret->tacache = tacache;
    ret->invalidated = SPRIXEL_INVALIDATED;
    ret->n = n;
    ret->dimy = dimy;
    ret->dimx = dimx;
    ret->pixx = pixx;
    ret->pixy = pixy;
    ret->y = placey;
    ret->x = placex;
    ret->parse_start = parse_start;
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
  if(s->invalidated == SPRIXEL_HIDE){ // no need to do work if we're killing it
    return 0;
  }
  if(!nc->tcache.pixel_cell_wipe){
    return 0;
  }
  if(ycell >= s->dimy || ycell < 0){
    logerror(nc, "Bad y coordinate %d (%d)\n", ycell, s->dimy);
    return -1;
  }
  if(xcell >= s->dimx || xcell < 0){
    logerror(nc, "Bad x coordinate %d (%d)\n", xcell, s->dimx);
    return -1;
  }
  if(s->tacache[s->dimx * ycell + xcell] == 2){
fprintf(stderr, "CACHED WIPE %d %d/%d\n", s->id, ycell, xcell);
    return 0; // already annihilated
  }
  s->tacache[s->dimx * ycell + xcell] = 2;
fprintf(stderr, "WIPING %d %d/%d\n", s->id, ycell, xcell);
  int r = nc->tcache.pixel_cell_wipe(nc, s, ycell, xcell);
  if(r == 0){
    s->invalidated = SPRIXEL_INVALIDATED;
  }
  return r;
}

int sprite_init(const notcurses* nc){
  if(!nc->tcache.pixel_init){
    return 0;
  }
  return nc->tcache.pixel_init(nc->ttyfd);
}
