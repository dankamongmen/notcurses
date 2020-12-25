#ifndef NOTCURSES_BLITSET
#define NOTCURSES_BLITSET

#include "notcurses/notcurses.h"

static inline const struct blitset*
lookup_blitset(unsigned utf8, ncblitter_e setid, bool may_degrade) {
  if(setid == NCBLIT_DEFAULT){ // ought have resolved NCBLIT_DEFAULT before now
    return NULL;
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

// Expand NCBLIT_DEFAULT for media blitting, based on environment.
static inline ncblitter_e 
rgba_blitter_default(bool utf8, ncscale_e scale, bool sextants){
  if(!utf8){
    return NCBLIT_1x1;
  }
  if(scale != NCSCALE_STRETCH){
    return NCBLIT_2x1;
  }
  if(!sextants){
    return NCBLIT_2x2;
  }
  return NCBLIT_3x2;
}

static inline const struct blitset*
rgba_blitter_low(bool utf8, bool sextants, ncscale_e scale, bool maydegrade,
                 ncblitter_e blitrec) {
  const struct blitset* bset;
  if(blitrec != NCBLIT_DEFAULT){
    bset = lookup_blitset(utf8, blitrec, maydegrade);
  }else{
    bset = lookup_blitset(utf8, rgba_blitter_default(utf8, scale, sextants), maydegrade);
  }
  if(bset && !bset->blit){ // FIXME remove this once all blitters are enabled
    bset = NULL;
  }
  return bset;
}

// RGBA visuals all use NCBLIT_2x1 by default (or NCBLIT_1x1 if not in
// UTF-8 mode), but an alternative can be specified.
static inline const struct blitset*
rgba_blitter(const struct notcurses* nc, const struct ncvisual_options* opts) {
  const bool maydegrade = !(opts && (opts->flags & NCVISUAL_OPTION_NODEGRADE));
  const ncscale_e scale = opts ? opts->scaling : NCSCALE_NONE;
  return rgba_blitter_low(notcurses_canutf8(nc), notcurses_cansextant(nc),
                          scale, maydegrade, opts ? opts->blitter : NCBLIT_DEFAULT);
}

#endif
