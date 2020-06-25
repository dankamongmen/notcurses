#ifndef NOTCURSES_BLITSET
#define NOTCURSES_BLITSET

#include "notcurses/notcurses.h"

static inline const struct blitset*
lookup_blitset(bool utf8, ncblitter_e setid, bool may_degrade) {
  if(setid == NCBLIT_DEFAULT){
    setid = NCBLIT_2x1;
    may_degrade = true;
  }
  // the only viable blitter in ASCII is NCBLIT_1x1
  if(!utf8 && setid != NCBLIT_1x1){
    if(may_degrade){
      setid = NCBLIT_1x1;
    }else{
      return NULL;
    }
  }
  const struct blitset* bset = notcurses_blitters;
  while(bset->egcs){
    if(bset->geom == setid){
      return bset;
    }
    ++bset;
  }
  return NULL;
}

static inline ncblitter_e
ncvisual_default_blitter(bool canutf8) {
  if(canutf8){
    // FIXME probably want 2x2, at least for images larger than plane
    return NCBLIT_2x1;
  }
  return NCBLIT_1x1;
}

// RGBA visuals all use NCBLIT_2x1 by default (or NCBLIT_1x1 if not in
// UTF-8 mode), but an alternative can be specified.
static inline const struct blitset*
rgba_blitter(bool utf8, ncblitter_e setid, bool maydegrade) {
  const struct blitset* bset;
  if(setid != NCBLIT_DEFAULT){
    bset = lookup_blitset(utf8, setid, maydegrade);
  }else{
    bset = lookup_blitset(utf8, ncvisual_default_blitter(utf8), maydegrade);
  }
  if(bset && !bset->blit){ // FIXME remove this once all blitters are enabled
    bset = NULL;
  }
  return bset;
}

// convenience wrapper around rgba_blitter()
static inline const struct blitset*
ncv_rgba_blitter(bool utf8, const struct ncvisual_options* opts) {
  const bool maydegrade = !(opts && (opts->flags & NCVISUAL_OPTION_NODEGRADE));
  return rgba_blitter(utf8, opts ? opts->blitter : NCBLIT_DEFAULT, maydegrade);
}

// number of pixels that map to a single cell, height-wise
static inline int
encoding_y_scale(const struct blitset* bset) {
  return bset->height;
}

// number of pixels that map to a single cell, width-wise
static inline int
encoding_x_scale(const struct blitset* bset) {
  return bset->width;
}

#endif
