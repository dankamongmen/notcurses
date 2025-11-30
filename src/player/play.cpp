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
#include <cstdio>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/packet.h>
#include <libavutil/frame.h>
#include <libavutil/samplefmt.h>
}
#include <ncpp/Direct.hh>
#include <ncpp/Visual.hh>
#include <ncpp/NotCurses.hh>
#include "compat/compat.h"
#include "media/audio-output.h"

using namespace ncpp;

// Forward declarations for audio API functions from ffmpeg.c
extern "C" {
bool ffmpeg_has_audio(ncvisual* ncv);
int ffmpeg_get_decoded_audio_frame(ncvisual* ncv);
int ffmpeg_resample_audio(ncvisual* ncv, uint8_t** out_data, int* out_samples);
AVFrame* ffmpeg_get_audio_frame(ncvisual* ncv);
int ffmpeg_init_audio_resampler(ncvisual* ncv, int out_sample_rate, int out_channels);
int ffmpeg_get_audio_sample_rate(ncvisual* ncv);
int ffmpeg_get_audio_channels(ncvisual* ncv);
void ffmpeg_audio_request_packets(ncvisual* ncv);
double ffmpeg_get_video_position_seconds(const ncvisual* ncv);
}

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
  o << " -b blitter: one of 'ascii', 'half', 'quad', 'sex', 'oct', 'braille', or 'pixel'\n";
  o << " -m margins: margin, or 4 comma-separated margins\n";
  o << " -a color: replace color with a transparent channel\n";
  o << " -n: force non-interpolative scaling\n";
  o << " -d mult: non-negative floating point scale for frame time" << std::endl;
  exit(exitcode);
}

enum class PlaybackRequest {
  None,
  Quit,
  NextFile,
  PrevFile,
  RestartFile,
  Seek,
};

struct marshal {
  int framecount;
  bool quiet;
  ncblitter_e blitter; // can be changed while streaming, must propagate out
  uint64_t last_abstime_ns;
  uint64_t avg_frame_ns;
  uint64_t dropped_frames;
  bool show_fps;
  double current_fps;
  double current_drop_pct;
  PlaybackRequest request;
  double seek_delta;
};

static constexpr double kNcplayerSeekSeconds = 5.0;
static constexpr double kNcplayerSeekMinutes = 2.0 * 60.0;
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
  static auto last_fps_sample = std::chrono::steady_clock::now();
  static int frames_since_sample = 0;
  // negative framecount means don't print framecount/timing (quiet mode)
  if(marsh->framecount >= 0){
    ++frames_since_sample;
    int64_t idx = ncvisual_frame_index(ncv);
    if(idx >= 0){
      marsh->framecount = idx;
    }else{
      ++marsh->framecount;
    }
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_fps_sample).count();
    if(elapsed >= 1000){
      if(elapsed > 0){
        marsh->current_fps = frames_since_sample * 1000.0 / elapsed;
        const uint64_t total_attempted = marsh->framecount + marsh->dropped_frames;
        marsh->current_drop_pct = total_attempted ? (100.0 * marsh->dropped_frames / total_attempted) : 0.0;
      }
      frames_since_sample = 0;
      last_fps_sample = now;
    }
  }
  stdn->set_fg_rgb(0x80c080);
  int64_t display_ns;
  double media_seconds = ffmpeg_get_video_position_seconds(ncv);
  if(media_seconds >= 0.0){
    display_ns = (int64_t)(media_seconds * (double)NANOSECS_IN_SEC);
  }else{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    display_ns = timespec_to_ns(&now) - timespec_to_ns(start);
  }
  if(display_ns < 0){
    display_ns = 0;
  }
  marsh->blitter = vopts->blitter;
  if(marsh->blitter == NCBLIT_DEFAULT){
    marsh->blitter = ncvisual_media_defblitter(nc, vopts->scaling);
  }
  if(!marsh->quiet){
    // FIXME put this on its own plane if we're going to be erase()ing it
    stdn->erase();
    if(marsh->show_fps){
      stdn->printf(0, NCAlign::Left, "frame %06d FPS %.2f drops %.1f%% (%s)",
                   marsh->framecount, marsh->current_fps, marsh->current_drop_pct,
                   notcurses_str_blitter(vopts->blitter));
    }else{
      stdn->printf(0, NCAlign::Left, "frame %06d (%s)", marsh->framecount,
                   notcurses_str_blitter(vopts->blitter));
    }
  }

  const uint64_t target_ns = timespec_to_ns(abstime);
  const uint64_t prev_ns = marsh->last_abstime_ns;
  if(prev_ns > 0 && target_ns > prev_ns){
    uint64_t delta = target_ns - prev_ns;
    if(delta < NANOSECS_IN_SEC * 10){
      if(marsh->avg_frame_ns == 0){
        marsh->avg_frame_ns = delta;
      }else{
        marsh->avg_frame_ns = (marsh->avg_frame_ns * 7 + delta) / 8;
      }
    }
  }
  marsh->last_abstime_ns = target_ns;
  const uint64_t default_frame_ns = 41666667ull; // ~24fps
  const uint64_t expected_frame_ns = marsh->avg_frame_ns ? marsh->avg_frame_ns : default_frame_ns;

  struct timespec nowts;
  clock_gettime(CLOCK_MONOTONIC, &nowts);
  uint64_t now_ns = timespec_to_ns(&nowts);
  int64_t lag_ns = (int64_t)now_ns - (int64_t)target_ns;
  const uint64_t drop_threshold_ns = expected_frame_ns * 3 / 2; // drop once 1.5 frames behind
  if(lag_ns > (int64_t)drop_threshold_ns){
    marsh->dropped_frames++;
    return 0;
  }
  if(lag_ns < 0){
    struct timespec sleep_ts;
    ns_to_timespec((uint64_t)(-lag_ns), &sleep_ts);
    clock_nanosleep(CLOCK_MONOTONIC, 0, &sleep_ts, NULL);
  }

  auto recreate_subtitle_plane = [&]() -> struct ncplane* {
    return ncvisual_subtitle_plane(*stdn, ncv);
  };
  struct ncplane* subp = recreate_subtitle_plane();
  int64_t remaining = display_ns;
  const int64_t h = remaining / (60 * 60 * NANOSECS_IN_SEC);
  remaining -= h * (60 * 60 * NANOSECS_IN_SEC);
  const int64_t m = remaining / (60 * NANOSECS_IN_SEC);
  remaining -= m * (60 * NANOSECS_IN_SEC);
  const int64_t s = remaining / NANOSECS_IN_SEC;
  remaining -= s * NANOSECS_IN_SEC;
  if(!marsh->quiet){
    stdn->printf(0, NCAlign::Right, "%02" PRId64 ":%02" PRId64 ":%02" PRId64 ".%04" PRId64,
                 h, m, s, remaining / 1000000);
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
      // Timeout - check if we should continue
      break;
    }
    // Check for 'q' key immediately to allow quitting
    if((keyp == 'q' || keyp == 'Q') && ni.evtype != EvType::Release){
      marsh->request = PlaybackRequest::Quit;
      ncplane_destroy(subp);
      return 1;
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
      if(!nc.refresh(&dimy, &dimx)){
        ncplane_destroy(subp);
        return -1;
      }
      if(subp){
        ncplane_destroy(subp);
      }
      subp = recreate_subtitle_plane();
      continue;
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
    }else if(keyp == 'f' || keyp == 'F'){
      marsh->show_fps = !marsh->show_fps;
      continue;
    }else if(keyp == NCKey::Up){
      marsh->request = PlaybackRequest::Seek;
      marsh->seek_delta = -kNcplayerSeekMinutes;
      ncplane_destroy(subp);
      return 2;
    }else if(keyp == NCKey::Down){
      marsh->request = PlaybackRequest::Seek;
      marsh->seek_delta = kNcplayerSeekMinutes;
      ncplane_destroy(subp);
      return 2;
    }else if(keyp == NCKey::Right){
      marsh->request = PlaybackRequest::Seek;
      marsh->seek_delta = kNcplayerSeekSeconds;
      ncplane_destroy(subp);
      return 2;
    }else if(keyp == NCKey::Left){
      marsh->request = PlaybackRequest::Seek;
      marsh->seek_delta = -kNcplayerSeekSeconds;
      ncplane_destroy(subp);
      return 2;
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

// Audio thread data structure
struct audio_thread_data {
  ncvisual* ncv;
  audio_output* ao;
  std::atomic<bool>* running;
  std::mutex* mutex;
};

// Audio thread function - processes decoded frames (video decoder reads packets)
static void audio_thread_func(audio_thread_data* data) {
  ncvisual* ncv = data->ncv;
  audio_output* ao = data->ao;
  std::atomic<bool>* running = data->running;

  if(!ffmpeg_has_audio(ncv) || !ao){
    return;
  }

  // Initialize resampler - convert FROM codec format TO output format
  int out_sample_rate = audio_output_get_sample_rate(ao);
  if(out_sample_rate <= 0){
    out_sample_rate = 44100;
  }
  int out_channels = audio_output_get_channels(ao);
  if(out_channels <= 0){
    out_channels = ffmpeg_get_audio_channels(ncv);
  }
  // Limit to stereo max for output
  if(out_channels > 2){
    out_channels = 2;
  }

  if(ffmpeg_init_audio_resampler(ncv, out_sample_rate, out_channels) < 0){
    return;
  }

  int frame_count = 0;
  int consecutive_eagain = 0;
  auto last_log = std::chrono::steady_clock::now();
  int frames_since_log = 0;
  while(*running){
    ffmpeg_audio_request_packets(ncv);
    // Get decoded audio frame (packets are read by video decoder)
    // IMPORTANT: avcodec_receive_frame can only return each frame once
    // So we must process the frame immediately and not call get_decoded_audio_frame again
    // until we've finished processing
    int samples = ffmpeg_get_decoded_audio_frame(ncv);
    if(samples > 0){
      do{
        consecutive_eagain = 0;
        uint8_t* out_data = nullptr;
        int out_samples = 0;
        int bytes = ffmpeg_resample_audio(ncv, &out_data, &out_samples);
        if(bytes > 0 && out_data){
          audio_output_write(ao, out_data, bytes);
          frame_count++;
          frames_since_log++;
          auto now = std::chrono::steady_clock::now();
          auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_log).count();
          if(elapsed >= 1000){
            frames_since_log = 0;
            last_log = now;
          }
          free(out_data);
        }
        samples = ffmpeg_get_decoded_audio_frame(ncv);
      }while(samples > 0 && audio_output_needs_data(ao));
      if(!audio_output_needs_data(ao)){
        usleep(1000);
      }
      continue;
    }else if(samples == 1){
      // EOF - don't break, just wait for more data (video might still be playing)
      consecutive_eagain = 0;
      usleep(10000); // 10ms delay on EOF
    }else if(samples < 0){
      // Error
      break;
    }else{
      consecutive_eagain++;
      ffmpeg_audio_request_packets(ncv);
      if(!audio_output_needs_data(ao)){
        usleep(1000);
      }
    }
  }

  // Flush any remaining frames at EOF
  int flush_count = 0;
  while(*running){
    int samples = ffmpeg_get_decoded_audio_frame(ncv);
    if(samples > 0){
      uint8_t* out_data = nullptr;
      int out_samples = 0;
      int bytes = ffmpeg_resample_audio(ncv, &out_data, &out_samples);
      if(bytes > 0 && out_data){
        audio_output_write(ao, out_data, bytes);
        free(out_data);
        flush_count++;
      }
    }else{
      break; // No more frames
    }
  }
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

  bool show_fps_overlay = false;
  // Audio-related variables (declared at function scope for cleanup)
  audio_output* ao = nullptr;
  std::thread* audio_thread = nullptr;
  std::atomic<bool> audio_running(false);
  std::mutex audio_mutex;
  audio_thread_data* audio_data = nullptr;

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

    PlaybackRequest pending_request = PlaybackRequest::None;
    while(true){
      bool restart_stream = false;
      if(ffmpeg_has_audio(*ncv)){
        int sample_rate = 44100;
        int channels = ffmpeg_get_audio_channels(*ncv);
        if(channels > 2){
          channels = 2;
        }
        ao = audio_output_init(sample_rate, channels, AV_SAMPLE_FMT_S16);
        if(ao){
          audio_running = true;
          audio_data = new audio_thread_data{*ncv, ao, &audio_running, &audio_mutex};
          audio_thread = new std::thread(audio_thread_func, audio_data);
          audio_output_start(ao);
        }
      }

      struct marshal marsh = {
        .framecount = 0,
        .quiet = quiet,
        .blitter = vopts.blitter,
        .last_abstime_ns = 0,
        .avg_frame_ns = 0,
        .dropped_frames = 0,
        .show_fps = show_fps_overlay,
        .current_fps = 0.0,
        .current_drop_pct = 0.0,
        .request = PlaybackRequest::None,
        .seek_delta = 0.0,
      };
      r = ncv->stream(&vopts, timescale, perframe, &marsh);
      pending_request = marsh.request;
      show_fps_overlay = marsh.show_fps;
      if(audio_thread){
        audio_running = false;
        audio_thread->join();
        delete audio_thread;
        audio_thread = nullptr;
      }
      if(audio_data){
        delete audio_data;
        audio_data = nullptr;
      }
      if(ao){
        audio_output_destroy(ao);
        ao = nullptr;
      }

      free(stdn->get_userptr());
      stdn->set_userptr(nullptr);
      restart_stream = false;
      if(pending_request == PlaybackRequest::Seek){
        double delta = marsh.seek_delta;
        if(ncvisual_seek(*ncv, delta) == 0){
          pending_request = PlaybackRequest::None;
          restart_stream = true;
        }else{
          pending_request = PlaybackRequest::None;
        }
      }

      if(!restart_stream && r == 0){
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
                --i;
                break;
              }else if(ni.id >= '7' && ni.id <= '9'){
                --i;
                break;
              }else if(ni.id == NCKey::Resize){
                --i;
                if(!nc.refresh(&dimy, &dimx)){
                  goto err;
                }
                break;
              }
            }while(ni.id != ' ');
          }else{
            struct timespec ts;
            ts.tv_sec = displaytime;
            ts.tv_nsec = (displaytime - ts.tv_sec) * NANOSECS_IN_SEC;
            clock_nanosleep(CLOCK_MONOTONIC, 0, &ts, NULL);
          }
        }else{
          ncv->decode_loop();
        }

        if(loop && pending_request == PlaybackRequest::None){
          restart_stream = true;
        }
      }

      if(!restart_stream){
        break;
      }
    }
    if(clin){
      ncplane_destroy(clin);
      clin = nullptr;
    }
    if(r < 0){ // positive is intentional abort
      std::cerr << "Error while playing " << argv[i] << std::endl;
      goto err;
    }
    free(ncplane_userptr(n));
    ncplane_destroy(n);
    if(pending_request == PlaybackRequest::Quit){
      return 0;
    }
    if(pending_request == PlaybackRequest::PrevFile){
      if(i >= 1){
        i -= 2;
      }else{
        i = -1;
      }
      continue;
    }
    if(pending_request == PlaybackRequest::RestartFile){
      if(i >= 0){
        --i;
      }
      continue;
    }
    if(pending_request == PlaybackRequest::Quit){
      return 0;
    }else if(pending_request == PlaybackRequest::PrevFile){
      if(i >= 1){
        i -= 2;
      }else{
        i = -1;
      }
      continue;
    }else if(pending_request == PlaybackRequest::RestartFile){
      --i;
      continue;
    }
  }
  return 0;

err:
  // Cleanup audio resources
  if(audio_thread){
    audio_running = false;
    audio_thread->join();
    delete audio_thread;
    audio_thread = nullptr;
  }
  if(audio_data){
    delete audio_data;
    audio_data = nullptr;
  }
  if(ao){
    audio_output_destroy(ao);
    ao = nullptr;
  }
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
