#include <array>
#include <cstring>
#include <cstdlib>
#include <clocale>
#include <sstream>
#include <getopt.h>
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
  o << "usage: " << name << " [ -h ] [ -l loglevel ] [ -d mult ] [ -s scaletype ] files" << '\n';
  o << " -l loglevel: integer between 0 and 9, goes to stderr'\n";
  o << " -s scaletype: one of 'none', 'scale', or 'stretch'\n";
  o << " -d mult: positive floating point scale for frame time" << std::endl;
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
  char32_t keyp;
  while((keyp = notcurses_getc_nblock(nc, nullptr)) != (char32_t)-1){
    if(keyp == NCKEY_RESIZE){
      return ncplane_resize(ncvisual_plane(ncv), 0, 0, keepy, keepx, 0, 0, dimy, dimx);
    }
    return 1;
  }
  return 0;
}

// can exit() directly. returns index in argv of first non-option param.
int handle_opts(int argc, char** argv, notcurses_options* opts, float* timescale,
                ncscale_e *scalemode) {
  *timescale = 1.0;
  *scalemode = NCSCALE_SCALE;
  int c;
  while((c = getopt(argc, argv, "hl:d:s:")) != -1){
    switch(c){
      case 'h':
        usage(std::cout, argv[0], EXIT_SUCCESS);
        break;
      case 's':
        if(strcmp(optarg, "stretch") == 0){
          *scalemode = NCSCALE_STRETCH;
        }else if(strcmp(optarg, "scale") == 0){
          *scalemode = NCSCALE_SCALE;
        }else if(strcmp(optarg, "none") == 0){
          *scalemode = NCSCALE_NONE;
        }
        break;
      case 'd':{
        std::stringstream ss;
        ss << optarg;
        float ts;
        ss >> ts;
        if(ts <= 0){
          std::cerr << "Invalid timescale [" << optarg << "] (wanted (0..))\n";
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
        *timescale = ts;
        break;
      }case 'l':{
        std::stringstream ss;
        ss << optarg;
        int ll;
        ss >> ll;
        if(ll < NCLOGLEVEL_SILENT || ll > NCLOGLEVEL_TRACE){
          std::cerr << "Invalid log level [" << optarg << "] (wanted [0..8])\n";
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
        if(ll == 0 && strcmp(optarg, "0")){
          std::cerr << "Invalid log level [" << optarg << "] (wanted [0..8])\n";
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
        opts->loglevel = static_cast<ncloglevel_e>(ll);
        break;
      }default:
        usage(std::cerr, argv[0], EXIT_FAILURE);
        break;
    }
  }
  // we require at least one free parameter
  if(argv[optind] == nullptr){
    usage(std::cerr, argv[0], EXIT_FAILURE);
  }
  return optind;
}

int main(int argc, char** argv){
  setlocale(LC_ALL, "");
  notcurses_options opts{};
  float timescale;
  ncscale_e stretchmode;
  auto nonopt = handle_opts(argc, argv, &opts, &timescale, &stretchmode);
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
  for(int i = nonopt ; i < argc ; ++i){
    std::array<char, 128> errbuf;
    int frames = 0;
    int averr;
    auto ncv = ncvisual_open_plane(nc, argv[i], &averr,
                                   0, 0, stretchmode);
    if(ncv == nullptr){
      av_make_error_string(errbuf.data(), errbuf.size(), averr);
      notcurses_stop(nc);
      std::cerr << "Error opening " << argv[i] << ": " << errbuf.data() << std::endl;
      return EXIT_FAILURE;
    }
    clock_gettime(CLOCK_MONOTONIC, &start);
    int r = ncvisual_stream(nc, ncv, &averr, timescale, perframe, &frames);
    if(r < 0){ // positive is intentional abort
      av_make_error_string(errbuf.data(), errbuf.size(), averr);
      notcurses_stop(nc);
      std::cerr << "Error decoding " << argv[i] << ": " << errbuf.data() << std::endl;
      return EXIT_FAILURE;
    }else if(r == 0){
      char32_t ie = notcurses_getc_blocking(nc, nullptr);
      if(ie == (char32_t)-1){
        break;
      }else if(ie == 'q'){
        break;
      }else if(ie == NCKEY_RESIZE){
        --i; // rerun with the new size
        if(notcurses_resize(nc, &dimy, &dimx)){
          notcurses_stop(nc);
          return EXIT_FAILURE;
        }
        if(ncplane_resize_simple(ncvisual_plane(ncv), dimy, dimx)){
          notcurses_stop(nc);
          return EXIT_FAILURE;
        }
      }
    }
    ncvisual_destroy(ncv);
    ncplane_erase(ncp);
  }
  if(notcurses_stop(nc)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
