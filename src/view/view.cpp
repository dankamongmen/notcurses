#include <array>
#include <cstdlib>
#include <libgen.h>
#include <iostream>

extern "C" {
#include <libavutil/pixdesc.h>
#include <libavutil/avconfig.h>
#include <libavcodec/avcodec.h>
}

#include "notcurses.h"

static void usage(std::ostream& os, const char* name, int exitcode)
  __attribute__ ((noreturn));

void usage(std::ostream& o, const char* name, int exitcode){
  o << "usage: " << name << " files" << '\n';
  exit(exitcode);
}

int ncview(struct notcurses* nc, struct ncvisual* ncv, int* averr){
  struct ncplane* n = notcurses_stdplane(nc);
  int frame = 0;
  AVFrame* avf;
  while( (avf = ncvisual_decode(ncv, averr)) ){
    ncplane_cursor_move_yx(n, 0, 0);
    ncplane_printf(n, "Got frame %05d\u2026", frame);
    ++frame;
  }
  return *averr;
}

int main(int argc, char** argv){
  if(argc == 1){
    usage(std::cerr, argv[0], EXIT_FAILURE);
  }
  notcurses_options opts{};
  opts.outfp = stdout;
  bool success = true;
  auto nc = notcurses_init(&opts);
  if(nc == nullptr){
    return -1;
  }
  auto ncp = notcurses_stdplane(nc);
  for(int i = 1 ; i < argc ; ++i){
    std::array<char, 128> errbuf;
    int averr;
    auto ncv = ncplane_visual_open(ncp, argv[i], &averr);
    if(ncv == nullptr){
      av_make_error_string(errbuf.data(), errbuf.size(), averr);
      std::cerr << "Error opening " << argv[i] << ": " << errbuf.data() << std::endl;
      success = false;
      continue;
    }
    if(ncview(nc, ncv, &averr)){
      av_make_error_string(errbuf.data(), errbuf.size(), averr);
      std::cerr << "Error decoding " << argv[i] << ": " << errbuf.data() << std::endl;
      success = false;
    }
    ncvisual_destroy(ncv);
  }
  if(notcurses_stop(nc)){
    success = false;
  }
  if(!success){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
