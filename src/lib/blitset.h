#ifndef NOTCURSES_BLITSET
#define NOTCURSES_BLITSET

static inline const struct blitset*
lookup_blitset(ncblitter_e setid) {
  const struct blitset* bset = geomdata;
  while(bset->egcs){
    if(bset->geom == setid){
      return bset;
    }
    ++bset;
  }
  return NULL;
}

#endif
