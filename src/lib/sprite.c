#include "internal.h"
#include "visual-details.h"

// FIXME needs be atomic
static uint32_t sprixelid_nonce;

void sprixel_free(sprixel* s){
  if(s){
    if(s->n){
      s->n->sprite = NULL;
    }
    if(s->ncv){
      s->ncv->spx = NULL;
    }
    free(s->glyph);
    free(s);
  }
}

sprixel* sprixel_recycle(ncplane* n, ncvisual* ncv){
  const notcurses* nc = ncplane_notcurses_const(n);
  if(nc->tcache.pixel_destroy == sprite_kitty_annihilate){
    sprixel* hides = n->sprite ? n->sprite : ncv->spx;
    assert(hides);
    int dimy = hides->dimy;
    int dimx = hides->dimx;
    sprixel_hide(hides);
    return sprixel_alloc(n, ncv, dimy, dimx);
  }
  return n->sprite;
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
//fprintf(stderr, "HIDING %d\n", s->id);
    s->invalidated = SPRIXEL_HIDE;
    s->movedfromy = ncplane_abs_y(s->n);
    s->movedfromx = ncplane_abs_x(s->n);
    if(s->ncv){
      s->ncv->spx = NULL;
      s->ncv = NULL;
    }
    if(s->n){
      s->n->sprite = NULL;
      s->n = NULL;
    }
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

sprixel* sprixel_alloc(ncplane* n, ncvisual* ncv, int dimy, int dimx){
  sprixel* ret = malloc(sizeof(sprixel));
  if(ret){
    memset(ret, 0, sizeof(*ret));
    ret->n = n;
    ret->ncv = ncv;
    ret->dimy = dimy;
    ret->dimx = dimx;
    ret->id = ++sprixelid_nonce;
//fprintf(stderr, "LOOKING AT %p (p->n = %p)\n", ret, ret->n);
    if(ncplane_pile(ret->n)){
      notcurses* nc = ncplane_notcurses(ret->n);
      ret->next = nc->sprixelcache;
      nc->sprixelcache = ret;
//fprintf(stderr, "%p %p %p\n", nc->sprixelcache, ret, nc->sprixelcache->next);
    }
  }
  return ret;
}

// 'y' and 'x' are the cell geometry, not the pixel geometry. takes
// ownership of 's' on success.
int sprixel_load(sprixel* spx, char* s, int bytes, int placey, int placex,
                 int pixy, int pixx, int parse_start){
  assert(spx->n);
  free(spx->glyph);
  spx->glyph = s;
  spx->glyphlen = bytes;
  spx->invalidated = SPRIXEL_INVALIDATED;
  spx->pixx = pixx;
  spx->pixy = pixy;
  spx->y = placey;
  spx->x = placex;
  spx->parse_start = parse_start;
  return 0;
}

// returns 1 if already annihilated, 0 if we successfully annihilated the cell,
// or -1 if we could not annihilate the cell (i.e. we're sixel).
int sprite_wipe_cell(const notcurses* nc, sprixel* s, int ycell, int xcell){
  if(s->invalidated == SPRIXEL_HIDE){ // no need to do work if we're killing it
    return 0;
  }
//fprintf(stderr, "ANNIHILATED %p %d\n", s->n->tacache, s->dimx * ycell + xcell);
  int r = nc->tcache.pixel_cell_wipe(nc, s, ycell, xcell);
//fprintf(stderr, "WIPED %d %d/%d ret=%d\n", s->id, ycell, xcell, r);
  // mark the cell as annihilated whether we actually scrubbed it or not,
  // so that we use this fact should we move to another frame
  s->n->tacache[s->dimx * ycell + xcell] = SPRIXCELL_ANNIHILATED;
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
