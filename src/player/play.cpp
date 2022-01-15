#include <array>
#include <memory>
#include <cinttypes>
#include <cstring>
#include <cstdlib>
#include <clocale>
#include <sstream>
#include <getopt.h>
#include <libgen.h>
#include <unistd.h>
#include <iostream>
#include <inttypes.h>
#include <ncpp/Direct.hh>
#include <ncpp/Visual.hh>
#include <ncpp/NotCurses.hh>
#include "compat/compat.h"

using namespace ncpp;

static void usage(std::ostream& os, const char* name, int exitcode)
  __attribute__ ((noreturn));

void usage(std::ostream& o, const char* name, int exitcode){
  o << "usage: " << name << " [ -h ] [ -q ] [ -m margins ] [ -l loglevel ] [ -d mult ] [ -s scaletype ] [ -k ] [ -L ] [ -t seconds ] [ -n ] [ -a color ] files" << '\n';
  o << " -h: display help and exit with success\n";
  o << " -V: print program name and version\n";
  o << " -q: be quiet (no frame/timing information along top of screen)\n";
  o << " -k: use direct mode (cannot be used with -L or -d)\n";
  o << " -L: loop frames\n";
  o << " -t seconds: delay t seconds after each file\n";
  o << " -l loglevel: integer between 0 and 7, goes to stderr\n";
  o << " -s scaling: one of 'none', 'hires', 'scale', 'scalehi', or 'stretch'\n";
  o << " -b blitter: one of 'ascii', 'half', 'quad', 'sex', 'braille', or 'pixel'\n";
  o << " -m margins: margin, or 4 comma-separated margins\n";
  o << " -a color: replace color with a transparent channel\n";
  o << " -n: force non-interpolative scaling\n";
  o << " -d mult: non-negative floating point scale for frame time" << std::endl;
  exit(exitcode);
}

struct marshal {
  int framecount;
  bool quiet;
  ncblitter_e blitter; // can be changed while streaming, must propagate out
};

// frame count is in the curry. original time is kept in n's userptr.
auto perframe(struct ncvisual* ncv, struct ncvisual_options* vopts,
              const struct timespec* abstime, void* vmarshal) -> int {
  struct marshal* marsh = static_cast<struct marshal*>(vmarshal);
  NotCurses &nc = NotCurses::get_instance();
  auto start = static_cast<struct timespec*>(ncplane_userptr(vopts->n));
  if(!start){
    // FIXME how do we get this free()d at the end?
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
  int64_t ns = timespec_to_ns(&now) - timespec_to_ns(start);
  marsh->blitter = vopts->blitter;
  if(marsh->blitter == NCBLIT_DEFAULT){
    marsh->blitter = ncvisual_media_defblitter(nc, vopts->scaling);
  }
  if(!marsh->quiet){
    // FIXME put this on its own plane if we're going to be erase()ing it
    stdn->erase();
    stdn->printf(0, NCAlign::Left, "frame %06d (%s)", marsh->framecount,
                 notcurses_str_blitter(vopts->blitter));
  }
  struct ncplane* subp = ncvisual_subtitle_plane(*stdn, ncv);
  const int64_t h = ns / (60 * 60 * NANOSECS_IN_SEC);
  ns -= h * (60 * 60 * NANOSECS_IN_SEC);
  const int64_t m = ns / (60 * NANOSECS_IN_SEC);
  ns -= m * (60 * NANOSECS_IN_SEC);
  const int64_t s = ns / NANOSECS_IN_SEC;
  ns -= s * NANOSECS_IN_SEC;
  if(!marsh->quiet){
    stdn->printf(0, NCAlign::Right, "%02" PRId64 ":%02" PRId64 ":%02" PRId64 ".%04" PRId64,
                 h, m, s, ns / 1000000);
  }
  if(!nc.render()){
    return -1;
  }
  unsigned dimx, dimy, oldx, oldy;
  nc.get_term_dim(&dimy, &dimx);
  ncplane_dim_yx(vopts->n, &oldy, &oldx);
  uint64_t absnow = timespec_to_ns(abstime);
  for( ; ; ){
    struct timespec interval;
    clock_gettime(CLOCK_MONOTONIC, &interval);
    uint64_t nsnow = timespec_to_ns(&interval);
    uint32_t keyp;
    ncinput ni;
    if(absnow > nsnow){
      ns_to_timespec(absnow - nsnow, &interval);
      keyp = nc.get(&interval, &ni);
    }else{
      keyp = nc.get(false, &ni);
    }
    if(keyp == 0){
      break;
    }
    // we don't care about key release events, especially the enter
    // release that starts so many interactive programs under Kitty
    if(ni.evtype == EvType::Release){
      continue;
    }
    if(keyp == ' '){
      do{
        if((keyp = nc.get(true, &ni)) == (uint32_t)-1){
          ncplane_destroy(subp);
          return -1;
        }
      }while(ni.id != 'q' && (ni.evtype == EvType::Release || ni.id != ' '));
    }
    // if we just hit a non-space character to unpause, ignore it
    if(keyp == NCKey::Resize){
      return 0;
    }else if(keyp == ' '){ // space for unpause
      continue;
    }else if(keyp == 'L' && ncinput_ctrl_p(&ni) && !ncinput_alt_p(&ni)){
      nc.refresh(nullptr, nullptr);
      continue;
    }else if(keyp >= '0' && keyp <= '6' && !ncinput_alt_p(&ni) && !ncinput_ctrl_p(&ni)){
      marsh->blitter = static_cast<ncblitter_e>(keyp - '0');
      vopts->blitter = marsh->blitter;
      continue;
    }else if(keyp >= '7' && keyp <= '9' && !ncinput_alt_p(&ni) && !ncinput_ctrl_p(&ni)){
      continue; // don't error out
    }else if(keyp == NCKey::Up){
      // FIXME move backwards significantly
      continue;
    }else if(keyp == NCKey::Down){
      // FIXME move forwards significantly
      continue;
    }else if(keyp == NCKey::Right){
      // FIXME move forwards
      continue;
    }else if(keyp == NCKey::Left){
      // FIXME move backwards
      continue;
    }else if(keyp != 'q'){
      continue;
    }
    ncplane_destroy(subp);
    return 1;
  }
  ncplane_destroy(subp);
  return 0;
}

// can exit() directly. returns index in argv of first non-option param.
auto handle_opts(int argc, char** argv, notcurses_options& opts, bool* quiet,
                 float* timescale, ncscale_e* scalemode, ncblitter_e* blitter,
                 float* displaytime, bool* loop, bool* noninterp,
                 uint32_t* transcolor, bool* climode)
                 -> int {
  *timescale = 1.0;
  *scalemode = NCSCALE_STRETCH;
  *displaytime = -1;
  int c;
  while((c = getopt(argc, argv, "Vhql:d:s:b:t:m:kLa:n")) != -1){
    switch(c){
      case 'h':
        usage(std::cout, argv[0], EXIT_SUCCESS);
        break;
      case 'V':
        printf("ncplayer version %s\n", notcurses_version());
        exit(EXIT_SUCCESS);
      case 'n':
        if(*noninterp){
          std::cerr <<  "Provided -n twice!" << std::endl;
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
        *noninterp = true;
        break;
      case 'a':
        if(*transcolor){
          std::cerr <<  "Provided -a twice!" << std::endl;
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
        if(sscanf(optarg, "%x", transcolor) != 1){
          std::cerr <<  "Invalid RGB color:" << optarg << std::endl;
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
        if(*transcolor > 0xfffffful){
          std::cerr <<  "Invalid RGB color:" << optarg << std::endl;
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
        *transcolor |= 0x1000000ull;
        break;
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
          std::cerr << "Invalid blitter specification (got " << optarg << ")" << std::endl;
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
        break;
      case 'k':{ // actually engages direct mode
        opts.flags |= NCOPTION_NO_ALTERNATE_SCREEN
                      | NCOPTION_PRESERVE_CURSOR
                      | NCOPTION_NO_CLEAR_BITMAPS;
        *displaytime = 0;
        *quiet = true;
        *climode = true;
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
          std::cerr << "Invalid log level [" << optarg << "] (wanted [-1..7])\n";
          usage(std::cerr, argv[0], EXIT_FAILURE);
        }
        if(ll == 0 && strcmp(optarg, "0")){
          std::cerr << "Invalid log level [" << optarg << "] (wanted [-1..7])\n";
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
  if(*blitter == NCBLIT_DEFAULT){
    *blitter = NCBLIT_PIXEL;
  }
  return optind;
}

int rendered_mode_player_inner(NotCurses& nc, int argc, char** argv,
                               ncscale_e scalemode, ncblitter_e blitter,
                               bool quiet, bool loop,
                               double timescale, double displaytime,
                               bool noninterp, uint32_t transcolor,
                               bool climode){
  unsigned dimy, dimx;
  std::unique_ptr<Plane> stdn(nc.get_stdplane(&dimy, &dimx));
  if(climode){
    stdn->set_scrolling(true);
  }
  uint64_t transchan = 0;
  ncchannels_set_fg_alpha(&transchan, NCALPHA_TRANSPARENT);
  ncchannels_set_bg_alpha(&transchan, NCALPHA_TRANSPARENT);
  stdn->set_base("", 0, transchan);
  struct ncplane_options nopts{};
  nopts.name = "play";
  nopts.resizecb = ncplane_resize_marginalized;
  nopts.flags = NCPLANE_OPTION_MARGINALIZED;
  ncplane* n = nullptr;
  ncplane* clin = nullptr;
  for(auto i = 0 ; i < argc ; ++i){
    std::unique_ptr<Visual> ncv;
    ncv = std::make_unique<Visual>(argv[i]);
    if((n = ncplane_create(*stdn, &nopts)) == nullptr){
      return -1;
    }
    ncplane_move_bottom(n);
    struct ncvisual_options vopts{};
    int r;
    if(noninterp){
      vopts.flags |= NCVISUAL_OPTION_NOINTERPOLATE;
    }
    if(transcolor){
      vopts.flags |= NCVISUAL_OPTION_ADDALPHA;
    }
    vopts.transcolor = transcolor & 0xffffffull;
    vopts.n = n;
    vopts.scaling = scalemode;
    vopts.blitter = blitter;
    if(!climode){
      vopts.flags |= NCVISUAL_OPTION_HORALIGNED | NCVISUAL_OPTION_VERALIGNED;
      vopts.y = NCALIGN_CENTER;
      vopts.x = NCALIGN_CENTER;
    }else{
      ncvgeom geom;
      if(ncvisual_geom(nc, *ncv, &vopts, &geom)){
        return -1;
      }
      struct ncplane_options cliopts{};
      cliopts.y = stdn->cursor_y();
      cliopts.x = stdn->cursor_x();
      cliopts.rows = geom.rcelly;
      cliopts.cols = geom.rcellx;
      clin = ncplane_create(n, &cliopts);
      if(!clin){
        return -1;
      }
      vopts.n = clin;
      ncplane_scrollup_child(*stdn, clin);
    }
    ncplane_erase(n);
    do{
      struct marshal marsh = {
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
            stdn->printf(0, NCAlign::Center, "press space to advance");
            if(!nc.render()){
              goto err;
            }
            ncinput ni;
            do{
              do{
                nc.get(true, &ni);
              }while(ni.evtype == EvType::Release);
              if(ni.id == (uint32_t)-1){
                return -1;
              }else if(ni.id == 'q'){
                return 0;
              }else if(ni.id == 'L'){
                nc.refresh(nullptr, nullptr);
              }else if(ni.id >= '0' && ni.id <= '6'){
                blitter = vopts.blitter = static_cast<ncblitter_e>(ni.id - '0');
                --i; // rerun same input with the new blitter
                break;
              }else if(ni.id >= '7' && ni.id <= '9'){
                --i; // just absorb the input
                break;
              }else if(ni.id == NCKey::Resize){
                --i; // rerun with the new size
                if(!nc.refresh(&dimy, &dimx)){
                  goto err;
                }
                break;
              }
            }while(ni.id != ' ');
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
        ncplane_destroy(clin);
      }
    }while(loop && r == 0);
    if(r < 0){ // positive is intentional abort
      std::cerr << "Error while playing " << argv[i] << std::endl;
      goto err;
    }
    free(ncplane_userptr(n));
    ncplane_destroy(n);
  }
  return 0;

err:
  free(ncplane_userptr(n));
  ncplane_destroy(n);
  return -1;
}

int rendered_mode_player(int argc, char** argv, ncscale_e scalemode,
                         ncblitter_e blitter, notcurses_options& ncopts,
                         bool quiet, bool loop,
                         double timescale, double displaytime,
                         bool noninterp, uint32_t transcolor,
                         bool climode){
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
    r = rendered_mode_player_inner(nc, argc, argv, scalemode, blitter,
                                   quiet, loop, timescale, displaytime,
                                   noninterp, transcolor, climode);
    if(!nc.stop()){
      return -1;
    }
  }catch(ncpp::init_error& e){
    std::cerr << e.what() << "\n";
    return -1;
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
  uint32_t transcolor = 0;
  bool quiet = false;
  bool loop = false;
  bool noninterp = false;
  bool climode = false;
  auto nonopt = handle_opts(argc, argv, ncopts, &quiet, &timescale, &scalemode,
                            &blitter, &displaytime, &loop, &noninterp, &transcolor,
                            &climode);
  // if -k was provided, we use CLI mode rather than simply not using the
  // alternate screen, so that output is inline with the shell.
  if(rendered_mode_player(argc - nonopt, argv + nonopt, scalemode, blitter, ncopts,
                          quiet, loop, timescale, displaytime, noninterp,
                          transcolor, climode)){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
