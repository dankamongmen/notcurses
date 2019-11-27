#include <cstdlib>
#include <libgen.h>
#include <iostream>
#include <libavutil/pixdesc.h>
#include <libavutil/avconfig.h>
#include <libavcodec/avcodec.h>
#include "notcurses.h"

static void usage(std::ostream& os, const char* name, int exitcode)
  __attribute__ ((noreturn));

void usage(std::ostream& o, const char* name, int exitcode){
  o << "usage: " << name << " files" << '\n';
  exit(exitcode);
}

int ncview(struct ncvisual* ncv){
  AVFrame* avf;
  if((avf = ncvisual_decode(ncv)) == nullptr){
    return -1;
  }
  printf("%s: %dx%d aspect %d:%d %d\n", avf->key_frame ? "Keyframe" : "Frame",
         avf->height, avf->width, avf->sample_aspect_ratio.num,
         avf->sample_aspect_ratio.den, avf->format);
  return 0;
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
    auto ncv = ncplane_visual_open(ncp, argv[i]);
    if(ncv == nullptr){
      success = false;
      continue;
    }
    if(ncview(ncv)){
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
