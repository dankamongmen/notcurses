#ifndef NOTCURSES_BLITSET
#define NOTCURSES_BLITSET

#include "notcurses/notcurses.h"

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
rgba_blitter_default(const tinfo* tcache, ncscale_e scale){
  if(!tcache->utf8){
    return NCBLIT_1x1;
  }
  if(scale == NCSCALE_NONE || scale == NCSCALE_SCALE){
    return NCBLIT_2x1;
  }
  if(!tcache->sextants){
    return NCBLIT_2x2;
  }
  return NCBLIT_3x2;
}

static inline ncblitter_e
ncplot_defblitter(const notcurses* nc){
  if(notcurses_canutf8(nc)){
    return NCBLIT_8x1;
  }
  return NCBLIT_1x1;
}

#endif
