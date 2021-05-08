#include "internal.h"
#include "visual-details.h"
#include <stdatomic.h>

static atomic_uint_fast32_t sprixelid_nonce;

void sprixel_debug(FILE* out, const sprixel* s){
  fprintf(out, "Sprixel %d (%p) %dB %dx%d (%dx%d) @%d/%d state: %d\n",
          s->id, s, s->glyphlen, s->dimy, s->dimx, s->pixy, s->pixx,
          s->n ? s->n->absy : 0, s->n ? s->n->absx : 0,
          s->invalidated);
  if(s->n){
    int idx = 0;
    for(int y = 0 ; y < s->dimy ; ++y){
      for(int x = 0 ; x < s->dimx ; ++x){
        fprintf(out, "%d", s->n->tam[idx].state);
        ++idx;
      }
      fprintf(out, "\n");
    }
    idx = 0;
    for(int y = 0 ; y < s->dimy ; ++y){
      for(int x = 0 ; x < s->dimx ; ++x){
        if(s->n->tam[idx].state == SPRIXCELL_ANNIHILATED){
          if(s->n->tam[idx].auxvector){
            fprintf(out, "%03d] ", idx);
            for(int p = 0 ; p < s->cellpxx * s->cellpxy ; ++p){
              fprintf(out, "%02x ", s->n->tam[idx].auxvector[p]);
            }
            fprintf(out, "\n");
          }else{
            fprintf(out, "%03d] missing!\n", idx);
          }
        }
        ++idx;
      }
    }
  }
}

// doesn't splice us out of any lists, just frees
void sprixel_free(sprixel* s){
  if(s){
    if(s->n){
      s->n->sprite = NULL;
    }
    sixelmap_free(s->smap);
    free(s->glyph);
    free(s);
  }
}

sprixel* sprixel_recycle(ncplane* n){
  assert(n->sprite);
  const notcurses* nc = ncplane_notcurses_const(n);
  if(nc->tcache.pixel_shutdown == kitty_shutdown){
    sprixel* hides = n->sprite;
    int dimy = hides->dimy;
    int dimx = hides->dimx;
    sprixel_hide(hides);
    return sprixel_alloc(n, dimy, dimx);
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
  if(ncplane_pile(s->n) == NULL){ // ncdirect case; destroy now
fprintf(stderr, "DESTROY IMMEDIATELY\n");
    sprixel_free(s);
    return;
  }
  // otherwise, it'll be killed in the next rendering cycle.
  // guard so that a double call doesn't drop core on /s->n->sprite
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
  if(s->invalidated == SPRIXEL_QUIESCENT && s->n){
    int localy = y - s->n->absy;
    int localx = x - s->n->absx;
//fprintf(stderr, "INVALIDATING AT %d/%d (%d/%d) TAM: %d\n", y, x, localy, localx, s->n->tam[localy * s->dimx + localx].state);
    if(s->n->tam[localy * s->dimx + localx].state != SPRIXCELL_TRANSPARENT &&
       s->n->tam[localy * s->dimx + localx].state != SPRIXCELL_ANNIHILATED &&
       s->n->tam[localy * s->dimx + localx].state != SPRIXCELL_ANNIHILATED_TRANS){
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

sprixel* sprixel_alloc(ncplane* n, int dimy, int dimx){
  sprixel* ret = malloc(sizeof(sprixel));
  if(ret){
    memset(ret, 0, sizeof(*ret));
    ret->n = n;
    ret->dimy = dimy;
    ret->dimx = dimx;
    ret->id = ++sprixelid_nonce;
    if(ret->id >= 0x1000000){
      ret->id = sprixelid_nonce = 1;
    }
//fprintf(stderr, "LOOKING AT %p (p->n = %p)\n", ret, ret->n);
    if(ncplane_pile(ret->n)){
      ncpile* np = ncplane_pile(ret->n);
      if( (ret->next = np->sprixelcache) ){
        ret->next->prev = ret;
      }
      np->sprixelcache = ret;
      ret->prev = NULL;
      const notcurses* nc = ncplane_notcurses_const(ret->n);
      ret->cellpxy = nc->tcache.cellpixy;
      ret->cellpxx = nc->tcache.cellpixx;
//fprintf(stderr, "%p %p %p\n", nc->sprixelcache, ret, nc->sprixelcache->next);
    }else{ // ncdirect case
      ret->next = ret->prev = NULL;
      ret->cellpxy = ret->cellpxx = -1;
    }
  }
  return ret;
}

// |pixy| and |pixx| are the output pixel geometry (i.e. |pixy| must be a
// multiple of 6 for sixel). output coverage ought already have been loaded.
// takes ownership of 's' on success. frees any existing glyph.
int sprixel_load(sprixel* spx, char* s, int bytes, int pixy, int pixx,
                 int parse_start){
  assert(spx->n);
  if(spx->cellpxy > 0){ // don't explode on ncdirect case
    if((pixy + spx->cellpxy - 1) / spx->cellpxy != spx->dimy){
      return -1;
    }
    if((pixx + spx->cellpxx - 1) / spx->cellpxx != spx->dimx){
      return -1;
    }
  }
  free(spx->glyph);
  spx->glyph = s;
  spx->glyphlen = bytes;
  spx->invalidated = SPRIXEL_INVALIDATED;
  spx->pixx = pixx;
  spx->pixy = pixy;
  spx->parse_start = parse_start;
  return 0;
}

// returns 1 if already annihilated, 0 if we successfully annihilated the cell,
// or -1 if we could not annihilate the cell (i.e. we're sixel).
int sprite_wipe(const notcurses* nc, sprixel* s, int ycell, int xcell){
  int idx = s->dimx * ycell + xcell;
  if(s->n->tam[idx].state == SPRIXCELL_TRANSPARENT){
    s->n->tam[idx].state = SPRIXCELL_ANNIHILATED_TRANS;
    return 1;
  }
  if(s->n->tam[idx].state == SPRIXCELL_ANNIHILATED_TRANS){
    // both handle this correctly; one day, we will also check for ANNIHILATED
    // here, and return 0 (sixel currently must return 1) FIXME
    return 0;
  }
//fprintf(stderr, "ANNIHILATING %p %d\n", s->n->tam, idx);
  int r = nc->tcache.pixel_wipe(s, ycell, xcell);
//fprintf(stderr, "WIPED %d %d/%d ret=%d\n", s->id, ycell, xcell, r);
  // mark the cell as annihilated whether we actually scrubbed it or not,
  // so that we use this fact should we move to another frame
  s->n->tam[idx].state = SPRIXCELL_ANNIHILATED;
  return r;
}

int sprite_clear_all(const tinfo* t, int fd){
  if(t->pixel_clear_all == NULL){
    return 0;
  }
  return t->pixel_clear_all(fd);
}

int sprite_init(const tinfo* t, int fd){
  sprixelid_nonce = random() % 0xffffffu;
  if(t->pixel_init == NULL){
    return 0;
  }
  return t->pixel_init(fd);
}

uint8_t* sprixel_auxiliary_vector(const sprixel* s){
  int pixels = s->cellpxy * s->cellpxx;
  // for now we just do two bytes per pixel. we ought squeeze the transparency
  // vector down to a bit per pixel, rather than a byte FIXME.
  uint8_t* ret = malloc(sizeof(*ret) * pixels * 2);
  memset(ret, 0, sizeof(*ret) * pixels);
  return ret;
}
