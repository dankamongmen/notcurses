#include "internal.h"

void sprixel_free(sprixel* s){
  if(s){
    free(s->glyph);
    free(s);
  }
}

// store the original (absolute) coordinates from which we moved, so that
// we can invalidate them in sprite_draw().
void sprixel_movefrom(sprixel* s, int y, int x){
  if(s->invalidated != SPRIXEL_HIDE){
    if(s->invalidated != SPRIXEL_MOVED){
      s->invalidated = SPRIXEL_MOVED;
      s->movedfromy = y;
      s->movedfromx = x;
    }
  }
}

void sprixel_hide(sprixel* s){
  // guard so that a double call doesn't drop core on s->n->sprite
  if(s->invalidated != SPRIXEL_HIDE){
    s->invalidated = SPRIXEL_HIDE;
    s->n->sprite = NULL;
    s->n = NULL;
  }
}

void sprixel_invalidate(sprixel* s){
  if(s->invalidated != SPRIXEL_HIDE){
    s->invalidated = SPRIXEL_INVALIDATED;
  }
}

sprixel* sprixel_by_id(const notcurses* nc, uint32_t id){
  for(sprixel* cur = nc->sprixelcache ; cur ; cur = cur->next){
    if(cur->id == id){
      return cur;
    }
  }
  return NULL;
}

// 'y' and 'x' are the cell geometry, not the pixel geometry. takes
// ownership of 's' on success.
sprixel* sprixel_create(ncplane* n, char* s, int bytes, int placey, int placex,
                        int sprixelid, int dimy, int dimx, int pixy, int pixx,
                        int parse_start){
  sprixel* ret = malloc(sizeof(sprixel));
  if(ret){
    ret->glyph = s;
    ret->glyphlen = bytes;
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
  if(ycell >= s->dimy || ycell < 0){
    logerror(nc, "Bad y coordinate %d (%d)\n", ycell, s->dimy);
    return -1;
  }
  if(xcell >= s->dimx || xcell < 0){
    logerror(nc, "Bad x coordinate %d (%d)\n", xcell, s->dimx);
    return -1;
  }
  if(s->n->tacache[s->dimx * ycell + xcell] == SPRIXCELL_ANNIHILATED){
//fprintf(stderr, "CACHED WIPE %d %d/%d\n", s->id, ycell, xcell);
    return 0; // already annihilated
  }
  // mark the cell as annihilated whether we actually scrubbed it or not,
  // so that we use this fact should we move to another frame
  s->n->tacache[s->dimx * ycell + xcell] = SPRIXCELL_ANNIHILATED;
  if(!nc->tcache.pixel_cell_wipe){ // sixel has no cell wiping
    return -1;
  }
//fprintf(stderr, "WIPING %d %d/%d\n", s->id, ycell, xcell);
  int r = nc->tcache.pixel_cell_wipe(nc, s, ycell, xcell);
  if(r == 0){
    s->invalidated = SPRIXEL_INVALIDATED;
  }
  return r;
}

// precondition: s->invalidated is SPRIXEL_INVALIDATED or SPRIXEL_MOVED.
int sprite_draw(const notcurses* n, const ncpile* p, sprixel* s, FILE* out){
  int r = n->tcache.pixel_draw(n, p, s, out);
  return r;
}

int sprite_init(const notcurses* nc){
  if(!nc->tcache.pixel_init){
    return 0;
  }
  return nc->tcache.pixel_init(nc->ttyfd);
}

int sprite_destroy(const notcurses* nc, const ncpile* p, FILE* out, sprixel* s){
  if(!nc->tcache.pixel_destroy){
    return 0;
  }
  return nc->tcache.pixel_destroy(nc, p, out, s);
}
