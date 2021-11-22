#include "notcurses/direct.h"
#include "lib/internal.h"

extern ncvisual_implementation local_visual_implementation;

ncdirect* ncdirect_init(const char* termtype, FILE* outfp, uint64_t flags){
  visual_implementation = &local_visual_implementation;
  return ncdirect_core_init(termtype, outfp, flags);
}

notcurses* notcurses_init(const notcurses_options* opts, FILE* outfp){
  visual_implementation = &local_visual_implementation;
  return notcurses_core_init(opts, outfp);
}
