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
#include <ncpp/Direct.hh>
#include <ncpp/Visual.hh>
#include <ncpp/NotCurses.hh>
#include "compat/compat.h"

using namespace ncpp;

static void usage(std::ostream& os, const char* name, int exitcode)
  __attribute__ ((noreturn));

void usage(std::ostream& o, const char* name, int exitcode){
  o << "usage: " << name << " [ -h ] [ -q ] [ -m margins ] [ -l loglevel ] [ -d mult ] [ -s scaletype ] [ -k ] [ -L ] [ -t seconds ] files" << '\n';
  o << " -h: display help and exit with success\n";
  o << " -V: print program name and version\n";
  o << " -q: be quiet (no frame/timing information along top of screen)\n";
  o << " -k: use direct mode (cannot be used with -L or -d)\n";
  o << " -L: loop frames\n";
  o << " -t seconds: delay t seconds after each file\n";
  o << " -l loglevel: integer between 0 and 9, goes to stderr'\n";
  o << " -s scaling: one of 'none', 'hires', 'scale', 'scalehi', or 'stretch'\n";
  o << " -b blitter: one of 'ascii', 'half', 'quad', 'sex', 'braille', or 'pixel'\n";
  o << " -m margins: margin, or 4 comma-separated margins\n";
  o << " -d mult: non-negative floating point scale for frame time" << std::endl;
  exit(exitcode);
}

struct marshal {
  struct ncplane* subtitle_plane;
  int framecount;
  bool quiet;
  ncblitter_e blitter; // can be changed while streaming, must propagate out
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
  marsh->blitter = vopts->blitter;
  if(marsh->blitter == NCBLIT_DEFAULT){
    marsh->blitter = ncvisual_media_defblitter(nc, vopts->scaling);
  }
  if(!marsh->quiet){
    stdn->printf(0, NCAlign::Left, "frame %06d (%s)", marsh->framecount,
                 notcurses_str_blitter(vopts->blitter));
  }
  char* subtitle = ncvisual_subtitle(ncv);
  if(subtitle){
    if(!marsh->subtitle_plane){
      int dimx, dimy;
      ncplane_dim_yx(vopts->n, &dimy, &dimx);
      struct ncplane_options nopts = {
        .y = dimy - 1,
        .x = 0,
        .rows = 1,
        .cols = dimx,
        .userptr = nullptr,
        .name = "subt",
        .resizecb = nullptr,
        .flags = 0,
      };
      marsh->subtitle_plane = ncplane_create(notcurses_stdplane(nc), &nopts);
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
  for( ; ; ){
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
    }else if(keyp == 'L'){ // FIXME check for ctrl+l
      nc.refresh(nullptr, nullptr);
      continue;
    }else if(keyp >= '0' && keyp <= '6'){ // FIXME eliminate ctrl/alt
      marsh->blitter = static_cast<ncblitter_e>(keyp - '0');
      vopts->blitter = marsh->blitter;
      if(vopts->blitter == NCBLIT_PIXEL){
        notcurses_check_pixel_support(nc);
        vopts->y = 1;
      }else{
        vopts->y = 0;
      }
      continue;
    }else if(keyp == NCKey::Up){
      // FIXME
      continue;
    }else if(keyp == NCKey::Down){
      // FIXME
      continue;
    }else if(keyp == NCKey::Right){
      // FIXME
      continue;
    }else if(keyp == NCKey::Left){
      // FIXME
      continue;
    }
    return 1;
  }
  return 0;
}

// can exit() directly. returns index in argv of first non-option param.
auto handle_opts(int argc, char** argv, notcurses_options& opts, bool* quiet,
                 float* timescale, ncscale_e* scalemode, ncblitter_e* blitter,
                 float* displaytime, bool* loop)
                 -> int {
  *timescale = 1.0;
  *scalemode = NCSCALE_STRETCH;
  *displaytime = -1;
  int c;
  while((c = getopt(argc, argv, "Vhql:d:s:b:t:m:kL")) != -1){
    switch(c){
      case 'h':
        usage(std::cout, argv[0], EXIT_SUCCESS);
        break;
      case 'V':
        printf("ncplayer version %s\n", notcurses_version());
        exit(EXIT_SUCCESS);
      case 'q':
        *quiet = true;
        break;
      case 's':
        if(notcurses_lex_scalemode(optarg, scalemode)){
          std::cerr << "Scaling type should be one of stretch, scale, scalehi, hires, none (got "
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
      case 'k':{ // actually engages direct mode
        opts.flags |= NCOPTION_NO_ALTERNATE_SCREEN;
        if(*loop || *timescale != 1.0){
          std::cerr << "-k cannot be used with -L or -d" << std::endl;
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
        break;
      }case 'L':{
        if(opts.flags & NCOPTION_NO_ALTERNATE_SCREEN){
          std::cerr << "-L cannot be used with -k" << std::endl;
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
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
      }case 't':{
        std::stringstream ss;
        ss << optarg;
        float ts;
        ss >> ts;
        if(ts < 0){
          std::cerr << "Invalid displaytime [" << optarg << "] (wanted (0..))\n";
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
        *displaytime = ts;
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
        if(opts.flags & NCOPTION_NO_ALTERNATE_SCREEN){
          std::cerr << "-d cannot be used with -k" << std::endl;
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
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

// argc/argv ought already be reduced to only the media arguments
int direct_mode_player(int argc, char** argv, ncscale_e scalemode,
                       ncblitter_e blitter, int lmargin, int rmargin){
  Direct dm{};
  if(!dm.canopen_images()){
    std::cerr << "Notcurses was compiled without multimedia support\n";
    return -1;
  }
  bool failed = false;
  if(blitter == NCBLIT_PIXEL){
    dm.check_pixel_support();
  }
  for(auto i = 0 ; i < argc ; ++i){
    auto faken = dm.prep_image(argv[i], blitter, scalemode, -1,
                               dm.get_dim_x() - (lmargin + rmargin));
    if(!faken){
      failed = true;
      break;
    }
    // FIXME we want to honor the different left and right margins, but that
    // would require raster_image() knowing how far over we were starting for
    // multiline cellular blittings...
    ncpp::NCAlign a;
    if(blitter == NCBLIT_PIXEL){
      printf("%*.*s", lmargin, lmargin, "");
      a = NCAlign::Left;
    }else{
      a = NCAlign::Center;
    }
    if(dm.raster_image(faken, a)){
      failed = true;
      break;
    }
    printf("\n");
  }
  return failed ? -1 : 0;
}

int rendered_mode_player_inner(NotCurses& nc, int argc, char** argv,
                               ncscale_e scalemode, ncblitter_e blitter,
                               bool quiet, bool loop,
                               double timescale, double displaytime){
  int dimy, dimx;
  std::unique_ptr<Plane> stdn(nc.get_stdplane(&dimy, &dimx));
  for(auto i = 0 ; i < argc ; ++i){
    std::unique_ptr<Visual> ncv;
    ncv = std::make_unique<Visual>(argv[i]);
    stdn->erase();
    struct ncvisual_options vopts{};
    int r;
    vopts.n = *stdn;
    vopts.scaling = scalemode;
    vopts.blitter = blitter;
    if(vopts.blitter == NCBLIT_PIXEL){
      notcurses_check_pixel_support(nc);
      vopts.y = 1;
    }else{
      vopts.y = 0;
    }
    do{
      struct marshal marsh = {
        .subtitle_plane = nullptr,
        .framecount = 0,
        .quiet = quiet,
        .blitter = vopts.blitter,
      };
      r = ncv->stream(&vopts, timescale, perframe, &marsh);
      free(stdn->get_userptr());
      stdn->set_userptr(nullptr);
      if(r == 0){
        vopts.blitter = marsh.blitter;
        if(!loop){
          if(displaytime < 0){
            stdn->printf(0, NCAlign::Center, "press key to advance");
            if(!nc.render()){
              return -1;
            }
            char32_t ie = nc.getc(true);
            if(ie == (char32_t)-1){
              return -1;
            }else if(ie == 'q'){
              return 0;
            }else if(ie == 'L'){
              --i;
              nc.refresh(nullptr, nullptr);
            }else if(ie >= '0' && ie <= '6'){
              --i; // rerun same input with the new blitter
              vopts.blitter = blitter = static_cast<ncblitter_e>(ie - '0');
            }else if(ie == NCKey::Resize){
              --i; // rerun with the new size
              if(!nc.refresh(&dimy, &dimx)){
                return -1;
              }
            }
          }else{
            // FIXME do we still want to honor keybindings when timing out?
            struct timespec ts;
            ts.tv_sec = displaytime;
            ts.tv_nsec = (displaytime - ts.tv_sec) * NANOSECS_IN_SEC;
            clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
          }
        }else{
          ncv->decode_loop();
        }
      }
    }while(loop && r == 0);
    if(r < 0){ // positive is intentional abort
      std::cerr << "Error decoding " << argv[i] << std::endl;
      return -1;
    }
  }
  return 0;
}

int rendered_mode_player(int argc, char** argv, ncscale_e scalemode,
                         ncblitter_e blitter, notcurses_options& ncopts,
                         bool quiet, bool loop,
                         double timescale, double displaytime){
  // no -k, we're using full rendered mode (and the alternate screen).
  ncopts.flags |= NCOPTION_INHIBIT_SETLOCALE;
  if(quiet){
    ncopts.flags |= NCOPTION_SUPPRESS_BANNERS;
  }
  int r;
  try{
    NotCurses nc{ncopts};
    if(!nc.can_open_images()){
      nc.stop();
      std::cerr << "Notcurses was compiled without multimedia support\n";
      return EXIT_FAILURE;
    }
    try{
      r = rendered_mode_player_inner(nc, argc, argv, scalemode, blitter,
                                    quiet, loop, timescale, displaytime);
    }catch(std::exception& e){
      nc.stop();
      std::cerr << e.what() << "\n";
      return -1;
    }
    if(!nc.stop()){
      return -1;
    }
  }catch(ncpp::init_error* e){
    std::cerr << e->what() << "\n";
    return -1;
  }
  return r;
}

auto main(int argc, char** argv) -> int {
  if(setlocale(LC_ALL, "") == nullptr){
    std::cerr << "Couldn't set locale based off LANG\n";
    return EXIT_FAILURE;
  }
  float timescale, displaytime;
  ncscale_e scalemode;
  notcurses_options ncopts{};
  ncblitter_e blitter = NCBLIT_DEFAULT;
  bool quiet = false;
  bool loop = false;
  auto nonopt = handle_opts(argc, argv, ncopts, &quiet, &timescale, &scalemode,
                            &blitter, &displaytime, &loop);
  int r;
  // if -k was provided, we now use direct mode rather than simply not using the
  // alternate screen, so that output is inline with the shell.
  if(ncopts.flags & NCOPTION_NO_ALTERNATE_SCREEN){
    r = direct_mode_player(argc - nonopt, argv + nonopt, scalemode, blitter, ncopts.margin_l, ncopts.margin_r);
  }else{
    r = rendered_mode_player(argc - nonopt, argv + nonopt, scalemode, blitter, ncopts,
                             quiet, loop, timescale, displaytime);
  }
  if(r){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
