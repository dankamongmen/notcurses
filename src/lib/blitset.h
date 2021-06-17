#ifndef NOTCURSES_BLITSET
#define NOTCURSES_BLITSET

#include "notcurses/notcurses.h"

// number of pixels that map to a single cell, height-wise
static inline int
encoding_y_scale(const tinfo* tcache, const struct blitset* bset) {
  if(bset->geom == NCBLIT_PIXEL){
    return tcache->cellpixy;
  }
  return bset->height;
}

// number of pixels that map to a single cell, width-wise
static inline int
encoding_x_scale(const tinfo* tcache, const struct blitset* bset) {
  if(bset->geom == NCBLIT_PIXEL){
    return tcache->cellpixx;
  }
  return bset->width;
}

// Expand NCBLIT_DEFAULT for media blitting, based on environment. We never
// use NCBLIT_PIXEL for NCBLIT_DEFAULT, though maybe this ought change.
static inline ncblitter_e 
rgba_blitter_default(const tinfo* tcache, ncscale_e scale){
  if(!tcache->caps.utf8){
    return NCBLIT_1x1; // only one that works in ASCII
  }
  if(scale == NCSCALE_NONE || scale == NCSCALE_SCALE){
    return NCBLIT_2x1;
  }
  if(tcache->caps.sextants){
    return NCBLIT_3x2;
  }
  if(tcache->caps.quadrants){
    return NCBLIT_2x2;
  }
  return NCBLIT_2x1;
}

static inline ncblitter_e
ncplot_defblitter(const notcurses* nc){
  if(notcurses_canutf8(nc)){
    return NCBLIT_8x1;
  }
  return NCBLIT_1x1;
}

void set_pixel_blitter(ncblitter blitfxn);

#endif
