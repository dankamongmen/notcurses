#include "internal.h"

int loglevel = NCLOGLEVEL_SILENT;

void notcurses_debug_caps(const notcurses* nc, FILE* debugfp){
  // FIXME deprecated, remove for ABI3
  (void)nc;
  (void)debugfp;
}

void notcurses_debug(const notcurses* nc, FILE* debugfp){
  fbuf f;
  if(fbuf_init_small(&f)){
    return;
  }
  notcurses_debug_fbuf(nc, &f);
  fbuf_finalize(&f, debugfp, true);
  fbuf_free(&f);
}
