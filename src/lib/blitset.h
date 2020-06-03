#ifndef NOTCURSES_BLITSET
#define NOTCURSES_BLITSET

#include "notcurses/notcurses.h"

static inline const struct blitset*
lookup_blitset(const struct notcurses* nc, ncblitter_e setid, bool may_degrade) {
  // the only viable blitter in ASCII is NCBLIT_1x1
  if(!notcurses_canutf8(nc) && setid != NCBLIT_1x1){
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

#endif
