#include "internal.h"

#ifndef __MINGW32__
#define API __attribute__((visibility("default")))
#else
#define API __declspec(dllexport)
#endif

API ncloglevel_e loglevel = NCLOGLEVEL_SILENT;

void notcurses_debug(const notcurses* nc, FILE* debugfp){
  fbuf f;
  if(fbuf_init_small(&f)){
    return;
  }
  notcurses_debug_fbuf(nc, &f);
  fbuf_finalize(&f, debugfp);
}
