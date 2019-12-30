#include <array>
#include <cstdlib>
#include <clocale>
#include <libgen.h>
#include <unistd.h>
#include <iostream>
#include "notcurses.h"

extern "C" {
#include <libavutil/pixdesc.h>
#include <libavutil/avconfig.h>
#include <libavcodec/avcodec.h> // ffmpeg doesn't reliably "C"-guard itself
}

static void usage(std::ostream& os, const char* name, int exitcode)
  __attribute__ ((noreturn));

void usage(std::ostream& o, const char* name, int exitcode){
  o << "usage: " << name << " files" << '\n';
  exit(exitcode);
}

// frame count is in the ncplane's user pointer
int perframe(struct notcurses* nc, struct ncvisual* ncv, void* vframecount){
  struct ncplane* stdn = notcurses_stdplane(nc);
  int* framecount = static_cast<int*>(vframecount);
  ++*framecount;
  ncplane_set_fg(stdn, 0x80c080);
  ncplane_cursor_move_yx(stdn, 0, 0);
  ncplane_printf(stdn, "Got frame %05d\u2026", *framecount);
  if(ncvisual_render(ncv, 0, 0, 0, 0)){
    return -1;
  }
  if(notcurses_render(nc)){
    return -1;
  }
  return 0;
}

int main(int argc, char** argv){
  setlocale(LC_ALL, "");
  if(argc == 1){
    usage(std::cerr, argv[0], EXIT_FAILURE);
  }
  notcurses_options opts{};
  auto nc = notcurses_init(&opts, stdout);
  if(nc == nullptr){
    return EXIT_FAILURE;
  }
  int dimy, dimx;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  auto ncp = notcurses_newplane(nc, dimy - 1, dimx, 1, 0, nullptr);
  if(ncp == nullptr){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  for(int i = 1 ; i < argc ; ++i){
    std::array<char, 128> errbuf;
    int frames = 0;
    int averr;
    auto ncv = ncplane_visual_open(ncp, argv[i], &averr);
    if(ncv == nullptr){
      av_make_error_string(errbuf.data(), errbuf.size(), averr);
      notcurses_stop(nc);
      std::cerr << "Error opening " << argv[i] << ": " << errbuf.data() << std::endl;
      return EXIT_FAILURE;
    }
    if(ncvisual_stream(nc, ncv, &averr, perframe, &frames)){
      av_make_error_string(errbuf.data(), errbuf.size(), averr);
      notcurses_stop(nc);
      std::cerr << "Error decoding " << argv[i] << ": " << errbuf.data() << std::endl;
      return EXIT_FAILURE;
    }
    notcurses_getc_blocking(nc, nullptr);
    ncvisual_destroy(ncv);
  }
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
