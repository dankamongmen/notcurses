#include "notcurses/direct.h"
#include "internal.h"

ncdirect* ncdirect_init(const char* termtype, FILE* outfp, uint64_t flags){
  return ncdirect_core_init(termtype, outfp, flags);
}

notcurses* notcurses_init(const notcurses_options* opts, FILE* outfp){
  return notcurses_core_init(opts, outfp);
}
