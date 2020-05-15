#include <array>
#include <memory>
#include <cstring>
#include <cstdlib>
#include <clocale>
#include <sstream>
#include <getopt.h>
#include <libgen.h>
#include <unistd.h>
#include <iostream>
#include <ncpp/Visual.hh>
#include <ncpp/NotCurses.hh>

using namespace ncpp;

static void usage(std::ostream& os, const char* name, int exitcode)
  __attribute__ ((noreturn));

void usage(std::ostream& o, const char* name, int exitcode){
  o << "usage: " << name << " [ -h ] [ -m margins ] [ -l loglevel ] [ -d mult ] [ -s scaletype ] [ -k ] files" << '\n';
  o << " -k: don't use the alternate screen\n";
  o << " -l loglevel: integer between 0 and 9, goes to stderr'\n";
  o << " -s scaletype: one of 'none', 'scale', or 'stretch'\n";
  o << " -m margins: margin, or 4 comma-separated margins\n";
  o << " -d mult: non-negative floating point scale for frame time" << std::endl;
  exit(exitcode);
}

constexpr auto NANOSECS_IN_SEC = 1000000000ll;

static inline auto
timespec_to_ns(const struct timespec* ts) -> uint64_t {
  return ts->tv_sec * NANOSECS_IN_SEC + ts->tv_nsec;
}

static inline struct timespec*
ns_to_timespec(uint64_t ns, struct timespec* ts){
  ts->tv_sec = ns / NANOSECS_IN_SEC;
  ts->tv_nsec = ns % NANOSECS_IN_SEC;
  return ts;
}

// FIXME internalize this via complex curry
static struct ncplane* subtitle_plane = nullptr;

// frame count is in the curry. original time is in the ncvisual's ncplane's userptr.
auto perframe([[maybe_unused]] struct notcurses* _nc, struct ncvisual* ncv,
              const struct timespec* abstime, void* vframecount) -> int {
  NotCurses &nc = NotCurses::get_instance ();
  auto start = static_cast<struct timespec*>(ncplane_userptr(ncvisual_plane(ncv)));
  if(!start){
    start = new struct timespec;
    clock_gettime(CLOCK_MONOTONIC, start);
    ncplane_set_userptr(ncvisual_plane(ncv), start);
  }
  std::unique_ptr<Plane> stdn(nc.get_stdplane());
  int* framecount = static_cast<int*>(vframecount);
  ++*framecount;
  stdn->set_fg(0x80c080);
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  intmax_t ns = timespec_to_ns(&now) - timespec_to_ns(start);
  stdn->erase();
  stdn->printf(0, NCAlign::Left, "frame %06d\u2026", *framecount);
  char* subtitle = ncvisual_subtitle(ncv);
  if(subtitle){
    if(!subtitle_plane){
      int dimx, dimy;
      notcurses_term_dim_yx(_nc, &dimy, &dimx);
      subtitle_plane = ncplane_new(_nc, 1, dimx, dimy - 1, 0, nullptr);
      uint64_t channels = 0;
      channels_set_fg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
      channels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
      ncplane_set_base(subtitle_plane, "", 0, channels);
      ncplane_set_fg(subtitle_plane, 0x00ffff);
      ncplane_set_fg_alpha(subtitle_plane, CELL_ALPHA_HIGHCONTRAST);
      ncplane_set_bg_alpha(subtitle_plane, CELL_ALPHA_TRANSPARENT);
    }else{
      ncplane_erase(subtitle_plane);
    }
    ncplane_printf_yx(subtitle_plane, 0, 0, "%s", subtitle);
    free(subtitle);
  }
  const intmax_t h = ns / (60 * 60 * NANOSECS_IN_SEC);
  ns -= h * (60 * 60 * NANOSECS_IN_SEC);
  const intmax_t m = ns / (60 * NANOSECS_IN_SEC);
  ns -= m * (60 * NANOSECS_IN_SEC);
  const intmax_t s = ns / NANOSECS_IN_SEC;
  ns -= s * NANOSECS_IN_SEC;
  stdn->printf(0, NCAlign::Right, "%02jd:%02jd:%02jd.%04jd",
               h, m, s, ns / 1000000);
  if(!nc.render()){
    return -1;
  }
  int dimx, dimy, oldx, oldy, keepy, keepx;
  nc.get_term_dim(&dimy, &dimx);
  ncplane_dim_yx(ncvisual_plane(ncv), &oldy, &oldx);
  keepy = oldy > dimy ? dimy : oldy;
  keepx = oldx > dimx ? dimx : oldx;
  struct timespec interval;
  clock_gettime(CLOCK_MONOTONIC, &interval);
  uint64_t nsnow = timespec_to_ns(&interval);
  uint64_t absnow = timespec_to_ns(abstime);
  if(absnow > nsnow){
    ns_to_timespec(absnow - nsnow, &interval);
    char32_t keyp;
    while((keyp = nc.getc(&interval, nullptr, nullptr)) != (char32_t)-1){
      if(keyp == NCKEY_RESIZE){
        return ncplane_resize(ncvisual_plane(ncv), 0, 0, keepy, keepx, 0, 0, dimy, dimx);
      }
      return 1;
    }
  }
  return 0;
}

// can exit() directly. returns index in argv of first non-option param.
auto handle_opts(int argc, char** argv, notcurses_options& opts, float* timescale,
                 NCScale* scalemode) -> int {
  *timescale = 1.0;
  *scalemode = NCScale::Scale;
  int c;
  while((c = getopt(argc, argv, "hl:d:s:m:k")) != -1){
    switch(c){
      case 'h':
        usage(std::cout, argv[0], EXIT_SUCCESS);
        break;
      case 's':
        if(strcmp(optarg, "stretch") == 0){
          *scalemode = NCScale::Stretch;
        }else if(strcmp(optarg, "scale") == 0){
          *scalemode = NCScale::Scale;
        }else if(strcmp(optarg, "none") == 0){
          *scalemode = NCScale::None;
        }else{
          std::cerr <<  "Scaling type should be one of stretch, scale, none" << std::endl;
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
        break;
      case 'k':{
        opts.inhibit_alternate_screen = true;
        break;
      }case 'm':{
        if(opts.margin_t || opts.margin_r || opts.margin_b || opts.margin_l){
          std::cerr <<  "Provided margins twice!" << std::endl;
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
        if(notcurses_lex_margins(optarg, &opts)){
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
        break;
      }case 'd':{
        std::stringstream ss;
        ss << optarg;
        float ts;
        ss >> ts;
        if(ts < 0){
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
        if(ll < NCLogLevel::Silent || ll > NCLogLevel::Trace){
          std::cerr << "Invalid log level [" << optarg << "] (wanted [0..8])\n";
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
        if(ll == 0 && strcmp(optarg, "0")){
          std::cerr << "Invalid log level [" << optarg << "] (wanted [0..8])\n";
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
        opts.loglevel = static_cast<ncloglevel_e>(ll);
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

auto main(int argc, char** argv) -> int {
  setlocale(LC_ALL, "");
  float timescale;
  NCScale stretchmode;
  auto nonopt = handle_opts(argc, argv, NotCurses::default_notcurses_options, &timescale, &stretchmode);
  NotCurses nc;
  if(!nc.can_open_images()){
    nc.stop();
    std::cerr << "Notcurses was compiled without multimedia support\n";
    return EXIT_FAILURE;
  }
  int dimy, dimx;
  nc.get_term_dim(&dimy, &dimx);
  for(auto i = nonopt ; i < argc ; ++i){
    int frames = 0;
    nc_err_e err;
    std::unique_ptr<Visual> ncv;
    try{
      ncv = std::make_unique<Visual>(argv[i], &err, 1, 0, stretchmode);
    }catch(std::exception& e){
      nc.stop();
      std::cerr << argv[i] << ": " << e.what() << "\n";
      return EXIT_FAILURE;
    }
    int r = ncv->stream(&err, timescale, perframe, &frames);
    if(r < 0){ // positive is intentional abort
      nc.stop();
      std::cerr << "Error decoding " << argv[i] << ": " << nc_strerror(err) << std::endl;
      return EXIT_FAILURE;
    }else if(r == 0){
      std::unique_ptr<Plane> stdn(nc.get_stdplane());
      stdn->printf(0, NCAlign::Center, "press any key to advance");
      nc.render();
      char32_t ie = nc.getc(true);
      if(ie == (char32_t)-1){
        break;
      }else if(ie == 'q'){
        break;
      }else if(ie == NCKey::Resize){
        --i; // rerun with the new size
        if(!nc.refresh(&dimy, &dimx)){
          return EXIT_FAILURE;
        }
        if(!ncv->get_plane()->resize(dimy, dimx)){
          nc.stop();
          return EXIT_FAILURE;
        }
      }
    }
  }
  if(!nc.stop()){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
