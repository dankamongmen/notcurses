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
  o << "usage: " << name << " [ -h ] [ -q ] [ -m margins ] [ -l loglevel ] [ -d mult ] [ -s scaletype ] [ -k ] [ -L ] files" << '\n';
  o << " -h: display help and exit with success\n";
  o << " -q: be quiet (no frame/timing information along top of screen)\n";
  o << " -k: don't use the alternate screen\n";
  o << " -L: loop frames\n";
  o << " -l loglevel: integer between 0 and 9, goes to stderr'\n";
  o << " -s scaletype: one of 'none', 'scale', or 'stretch'\n";
  o << " -b blitter: one of 'ascii', 'halfblock', 'quadblitter' or 'braille'\n";
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

struct marshal {
  struct ncplane* subtitle_plane;
  int framecount;
  bool quiet;
};

// frame count is in the curry. original time is kept in n's userptr.
auto perframe(struct ncvisual* ncv, struct ncvisual_options* vopts,
              const struct timespec* abstime, void* vmarshal) -> int {
  struct marshal* marsh = static_cast<struct marshal*>(vmarshal);
  NotCurses &nc = NotCurses::get_instance ();
  auto start = static_cast<struct timespec*>(ncplane_userptr(vopts->n));
  if(!start){
    start = static_cast<struct timespec*>(malloc(sizeof(struct timespec)));
    clock_gettime(CLOCK_MONOTONIC, start);
    ncplane_set_userptr(vopts->n, start);
  }
  std::unique_ptr<Plane> stdn(nc.get_stdplane());
  // negative framecount means don't print framecount/timing (quiet mode)
  if(marsh->framecount >= 0){
    ++marsh->framecount;
  }
  stdn->set_fg_rgb(0x80c080);
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  intmax_t ns = timespec_to_ns(&now) - timespec_to_ns(start);
  auto blitter = vopts->blitter;
  if(blitter == NCBLIT_DEFAULT){
    blitter = ncvisual_default_blitter(notcurses_canutf8(nc), vopts->scaling);
    vopts->blitter = blitter;
  }
  if(!marsh->quiet){
    stdn->printf(0, NCAlign::Left, "frame %06d\u2026 (%s)", marsh->framecount,
                 notcurses_str_blitter(blitter));
  }
  char* subtitle = ncvisual_subtitle(ncv);
  if(subtitle){
    if(!marsh->subtitle_plane){
      int dimx, dimy;
      ncplane_dim_yx(vopts->n, &dimy, &dimx);
      marsh->subtitle_plane = ncplane_new(notcurses_stdplane(nc), 1, dimx,
                                          dimy - 1, 0, nullptr, "subt");
      uint64_t channels = 0;
      channels_set_fg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
      channels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
      ncplane_set_base(marsh->subtitle_plane, "", 0, channels);
      ncplane_set_fg_rgb(marsh->subtitle_plane, 0x00ffff);
      ncplane_set_fg_alpha(marsh->subtitle_plane, CELL_ALPHA_HIGHCONTRAST);
      ncplane_set_bg_alpha(marsh->subtitle_plane, CELL_ALPHA_TRANSPARENT);
    }else{
      ncplane_erase(marsh->subtitle_plane);
    }
    ncplane_printf_yx(marsh->subtitle_plane, 0, 0, "%s", subtitle);
    free(subtitle);
  }
  const intmax_t h = ns / (60 * 60 * NANOSECS_IN_SEC);
  ns -= h * (60 * 60 * NANOSECS_IN_SEC);
  const intmax_t m = ns / (60 * NANOSECS_IN_SEC);
  ns -= m * (60 * NANOSECS_IN_SEC);
  const intmax_t s = ns / NANOSECS_IN_SEC;
  ns -= s * NANOSECS_IN_SEC;
  if(!marsh->quiet){
    stdn->printf(0, NCAlign::Right, "%02jd:%02jd:%02jd.%04jd",
                 h, m, s, ns / 1000000);
  }
  if(!nc.render()){
    return -1;
  }
  int dimx, dimy, oldx, oldy;
  nc.get_term_dim(&dimy, &dimx);
  ncplane_dim_yx(vopts->n, &oldy, &oldx);
  uint64_t absnow = timespec_to_ns(abstime);
  char32_t keyp;
  bool gotinput;
  do{
    gotinput = false;
    struct timespec interval;
    clock_gettime(CLOCK_MONOTONIC, &interval);
    uint64_t nsnow = timespec_to_ns(&interval);
    if(absnow > nsnow){
      ns_to_timespec(absnow - nsnow, &interval);
      keyp = nc.getc(&interval, nullptr, nullptr);
    }else{
      keyp = nc.getc();
    }
    if(keyp == (char32_t)-1){
      break;
    }
    gotinput = true;
    if(keyp == ' '){
      if((keyp = nc.getc(true)) == (char32_t)-1){
        return -1;
      }
    }
    // if we just hit a non-space character to unpause, interpret it
    if(keyp == ' '){ // space for unpause
      continue;
    }
    if(keyp == NCKey::Resize){
      return 0;
    }else if(keyp >= '0' && keyp <= '8'){ // FIXME eliminate ctrl/alt
      vopts->blitter = static_cast<ncblitter_e>(keyp - '0');
      continue;
    }
    return 1;
  }while(gotinput);
  return 0;
}

// can exit() directly. returns index in argv of first non-option param.
auto handle_opts(int argc, char** argv, notcurses_options& opts, bool* quiet,
                 float* timescale, ncscale_e* scalemode, ncblitter_e* blitter,
                 bool* loop)
                 -> int {
  *timescale = 1.0;
  *scalemode = NCSCALE_STRETCH;
  int c;
  while((c = getopt(argc, argv, "hql:d:s:b:m:kL")) != -1){
    switch(c){
      case 'h':
        usage(std::cout, argv[0], EXIT_SUCCESS);
        break;
      case 'q':
        *quiet = true;
        break;
      case 's':
        if(notcurses_lex_scalemode(optarg, scalemode)){
          std::cerr << "Scaling type should be one of stretch, scale, none (got "
                    << optarg << ")" << std::endl;
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
        break;
      case 'b':
        if(notcurses_lex_blitter(optarg, blitter)){
          std::cerr << "Invalid blitter specification (got "
                    << optarg << ")" << std::endl;
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
        break;
      case 'k':{
        opts.flags |= NCOPTION_NO_ALTERNATE_SCREEN;
        break;
      }case 'L':{
        *loop = true;
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
  if(setlocale(LC_ALL, "") == nullptr){
    std::cerr << "Couldn't set locale based off LANG\n";
    return EXIT_FAILURE;
  }
  float timescale;
  ncscale_e scalemode;
  notcurses_options nopts{};
  ncblitter_e blitter = NCBLIT_DEFAULT;
  bool quiet = false;
  bool loop = false;
  auto nonopt = handle_opts(argc, argv, nopts, &quiet, &timescale, &scalemode,
                            &blitter, &loop);
  nopts.flags |= NCOPTION_INHIBIT_SETLOCALE;
  NotCurses nc{nopts};
  if(!nc.can_open_images()){
    nc.stop();
    std::cerr << "Notcurses was compiled without multimedia support\n";
    return EXIT_FAILURE;
  }
  int dimy, dimx;
  bool failed = false;
  {
    std::unique_ptr<Plane> stdn(nc.get_stdplane(&dimy, &dimx));
    for(auto i = nonopt ; i < argc ; ++i){
      std::unique_ptr<Visual> ncv;
      try{
        ncv = std::make_unique<Visual>(argv[i]);
      }catch(std::exception& e){
        // FIXME want to stop nc first :/ can't due to stdn, ugh
        std::cerr << argv[i] << ": " << e.what() << "\n";
        failed = true;
        break;
      }
      if(!(nopts.flags & NCOPTION_NO_ALTERNATE_SCREEN)){
        stdn->erase();
      }
      struct ncvisual_options vopts{};
      int r;
      vopts.n = *stdn;
      vopts.scaling = scalemode;
      vopts.blitter = blitter;
      do{
        struct marshal marsh = {
          .subtitle_plane = nullptr, .framecount = 0, .quiet = quiet,
        };
        r = ncv->stream(&vopts, timescale, perframe, &marsh);
        free(stdn->get_userptr());
        stdn->set_userptr(nullptr);
        if(r == 0){
          if(!loop){
            stdn->printf(0, NCAlign::Center, "press any key to advance");
            if(!nc.render()){
              failed = true;
              break;
            }
            char32_t ie = nc.getc(true);
            if(ie == (char32_t)-1){
              failed = true;
              break;
            }else if(ie == 'q'){
              break;
            }else if(ie >= '0' && ie <= '8'){
              --i; // rerun same input with the new blitter
              blitter = static_cast<ncblitter_e>(ie - '0');
            }else if(ie == NCKey::Resize){
              --i; // rerun with the new size
              if(!nc.refresh(&dimy, &dimx)){
                failed = true;
                break;
              }
            }
          }else{
            ncv->decode_loop();
          }
        }
      }while(loop && r == 0);
      if(r < 0){ // positive is intentional abort
        std::cerr << "Error decoding " << argv[i] << std::endl;
        failed = true;
        break;
      }
    }
  }
  if(!nc.stop()){
    return EXIT_FAILURE;
  }
  return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}
