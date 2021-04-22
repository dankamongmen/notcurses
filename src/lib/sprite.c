#include "internal.h"
#include "visual-details.h"

// FIXME needs be atomic
static uint32_t sprixelid_nonce;

static inline void
sprixel_debug(FILE* out, const sprixel* s){
  fprintf(out, "Sprixel %d (%p) %dx%d (%dx%d) @%d/%d state: %d\n",
          s->id, s, s->dimy, s->dimx, s->pixy, s->pixx,
          s->n ? s->n->absy : 0, s->n ? s->n->absx : 0,
          s->invalidated);
  if(s->n){
    int idx = 0;
    for(int y = 0 ; y < s->dimy ; ++y){
      for(int x = 0 ; x < s->dimx ; ++x){
        fprintf(out, "%d", s->n->tacache[idx]);
        ++idx;
      }
      fprintf(out, "\n");
    }
  }
}

void sprixel_free(sprixel* s){
  if(s){
    if(s->n){
      s->n->sprite = NULL;
    }
    free(s->glyph);
    free(s);
  }
}

sprixel* sprixel_recycle(ncplane* n){
  assert(n->sprite);
  const notcurses* nc = ncplane_notcurses_const(n);
  if(nc->tcache.pixel_destroy == kitty_delete){
    sprixel* hides = n->sprite;
    int dimy = hides->dimy;
    int dimx = hides->dimx;
    int y = hides->y;
    int x = hides->x;
    sprixel_hide(hides);
    return sprixel_alloc(n, dimy, dimx, y, x);
  }
  return n->sprite;
}

// store the original (absolute) coordinates from which we moved, so that
// we can invalidate them in sprite_draw().
void sprixel_movefrom(sprixel* s, int y, int x){
  if(s->invalidated != SPRIXEL_HIDE){
    if(s->invalidated != SPRIXEL_MOVED){
    // FIXME if we're Sixel, we need to effect any wipes that were run
    // (we normally don't because redisplaying sixel doesn't change
    // what's there--you can't "write transparency"). this is probably
    // best done by conditionally reblitting the sixel(?).
//fprintf(stderr, "SETTING TO MOVE: %d/%d was: %d\n", y, x, s->invalidated);
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
    if(s->n){
      s->n->sprite = NULL;
      s->n = NULL;
    }
  }
}

// y and x are absolute coordinates.
void sprixel_invalidate(sprixel* s, int y, int x){
//fprintf(stderr, "INVALIDATING AT %d/%d\n", y, x);
  if(s->invalidated != SPRIXEL_HIDE && s->n){
    int localy = y - s->n->absy;
    int localx = x - s->n->absx;
//fprintf(stderr, "INVALIDATING AT %d/%d (%d/%d) TAM: %d\n", y, x, localy, localx, s->n->tacache[localy * s->dimx + localx]);
    if(s->n->tacache[localy * s->dimx + localx] != SPRIXCELL_TRANSPARENT &&
       s->n->tacache[localy * s->dimx + localx] != SPRIXCELL_ANNIHILATED){
      s->invalidated = SPRIXEL_INVALIDATED;
    }
  }
}

sprixel* sprixel_by_id(const ncpile* n, uint32_t id){
  for(sprixel* cur = n->sprixelcache ; cur ; cur = cur->next){
    if(cur->id == id){
      return cur;
    }
  }
  return NULL;
}

sprixel* sprixel_alloc(ncplane* n, int dimy, int dimx, int placey, int placex){
  sprixel* ret = malloc(sizeof(sprixel));
  if(ret){
    memset(ret, 0, sizeof(*ret));
    ret->n = n;
    ret->dimy = dimy;
    ret->dimx = dimx;
    ret->y = placey;
    ret->x = placex;
    ret->id = ++sprixelid_nonce;
    ret->wipes_outstanding = false;
//fprintf(stderr, "LOOKING AT %p (p->n = %p)\n", ret, ret->n);
    if(ncplane_pile(ret->n)){
      ncpile* np = ncplane_pile(ret->n);
      ret->next = np->sprixelcache;
      np->sprixelcache = ret;
      const notcurses* nc = ncplane_notcurses_const(ret->n);
      ret->cellpxy = nc->tcache.cellpixy;
      ret->cellpxx = nc->tcache.cellpixx;
//fprintf(stderr, "%p %p %p\n", nc->sprixelcache, ret, nc->sprixelcache->next);
    }else{
      ret->next = NULL;
      ret->cellpxy = ret->cellpxx = -1;
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
int sprite_wipe(const notcurses* nc, sprixel* s, int ycell, int xcell){
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
//sprixel_debug(stderr, s);
  int r = n->tcache.pixel_draw(n, p, s, out);
  return r;
}

int sprite_init(const notcurses* nc){
  if(!nc->tcache.pixel_init){
    return 0;
  }
  return nc->tcache.pixel_init(nc->ttyfd);
}
