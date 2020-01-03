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

constexpr auto NANOSECS_IN_SEC = 1000000000ll;

static inline uint64_t
timespec_to_ns(const struct timespec* ts){
  return ts->tv_sec * NANOSECS_IN_SEC + ts->tv_nsec;
}

// frame count is in the curry. original time is in the ncplane's userptr.
int perframe(struct notcurses* nc, struct ncvisual* ncv, void* vframecount){
  const struct timespec* start = static_cast<struct timespec*>(ncplane_userptr(ncvisual_plane(ncv)));
  struct ncplane* stdn = notcurses_stdplane(nc);
  int* framecount = static_cast<int*>(vframecount);
  ++*framecount;
  ncplane_set_fg(stdn, 0x80c080);
  ncplane_cursor_move_yx(stdn, 0, 0);
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  int64_t ns = timespec_to_ns(&now) - timespec_to_ns(start);
  ncplane_printf(stdn, "Got frame %05d\u2026", *framecount);
  const int64_t h = ns / (60 * 60 * NANOSECS_IN_SEC);
  ns -= h * (60 * 60 * NANOSECS_IN_SEC);
  const int64_t m = ns / (60 * NANOSECS_IN_SEC);
  ns -= m * (60 * NANOSECS_IN_SEC);
  const int64_t s = ns / NANOSECS_IN_SEC;
  ns -= s * NANOSECS_IN_SEC;
  ncplane_printf_aligned(stdn, 0, NCALIGN_RIGHT, "%02ld:%02ld:%02ld.%04ld",
                         h, m, s, ns / 1000000);
  if(notcurses_render(nc)){
    return -1;
  }
  int dimx, dimy, oldx, oldy, keepy, keepx;
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  ncplane_dim_yx(ncvisual_plane(ncv), &oldy, &oldx);
  keepy = oldy > dimy ? dimy : oldy;
  keepx = oldx > dimx ? dimx : oldx;
  return ncplane_resize(ncvisual_plane(ncv), 0, 0, keepy, keepx, 0, 0, dimy, dimx);
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
  struct timespec start;
  auto ncp = ncplane_new(nc, dimy - 1, dimx, 1, 0, &start);
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
    clock_gettime(CLOCK_MONOTONIC, &start);
    if(ncvisual_stream(nc, ncv, &averr, perframe, &frames)){
      av_make_error_string(errbuf.data(), errbuf.size(), averr);
      notcurses_stop(nc);
      std::cerr << "Error decoding " << argv[i] << ": " << errbuf.data() << std::endl;
      return EXIT_FAILURE;
    }
    char32_t ie = notcurses_getc_blocking(nc, nullptr);
    if(ie == (char32_t)-1){
      break;
    }
    ncvisual_destroy(ncv);
  }
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
