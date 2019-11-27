#include <cstdlib>
#include <libgen.h>
#include <iostream>
#include "notcurses.h"

static void usage(std::ostream& os, const char* name, int exitcode)
  __attribute__ ((noreturn));

void usage(std::ostream& o, const char* name, int exitcode){
  o << "usage: " << name << " files" << '\n';
  exit(exitcode);
}

int ncview(struct ncvisual* ncv, const notcurses_options* opts){
  ncvisual_decode(ncv);
  auto nc = notcurses_init(opts);
  if(nc == nullptr){
    return -1;
  }
  ncvisual_destroy(ncv);
  return notcurses_stop(nc);
}

int main(int argc, char** argv){
  if(argc == 1){
    usage(std::cerr, argv[0], EXIT_FAILURE);
  }
  notcurses_options opts{};
  opts.outfp = stdout;
  bool success = true;
  for(int i = 1 ; i < argc ; ++i){
    auto ncv = notcurses_visual_open(nullptr, argv[i]);
    if(ncv == nullptr){
      success = false;
      continue;
    }
    if(ncview(ncv, &opts)){
      success = false;
    }
  }
  if(!success){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
