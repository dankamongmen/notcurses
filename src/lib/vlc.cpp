#include "builddef.h"
#ifdef USE_VLC
#include <vlc/libvlc.h>
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_events.h>
#include <vlc/libvlc_media_player.h>
#include "vlc.h"
#include "internal.h"
#include "visual-details.h"

static libvlc_instance_t* vlcctx;

bool notcurses_canopen_images(const notcurses* nc __attribute__ ((unused))) {
  return true;
}

bool notcurses_canopen_videos(const notcurses* nc __attribute__ ((unused))) {
  return true;
}

auto ncvisual_subtitle(const ncvisual* ncv) -> char* {
  (void)ncv; // FIXME
  return nullptr;
}

static void
media_callback(const struct libvlc_event_t* p_event, void* p_data) {
  ncvisual* nc = static_cast<ncvisual*>(p_data);
  fprintf(stderr, "CALLBACK! %p\n", nc);
  libvlc_media_player_stop(nc->details.player);
}

int ncvisual_decode(ncvisual* nc){
  libvlc_media_player_play(nc->details.player); // FIXME retcode
  return 0;
}

// resize frame to oframe, converting to RGBA (if necessary) along the way
int ncvisual_resize(ncvisual* nc, int rows, int cols) {
  (void)nc; // FIXME
  (void)rows;
  (void)cols;
  return -1;
}

ncvisual* ncvisual_from_file(const char* filename) {
  ncvisual* ret = ncvisual_create();
  if(ret == nullptr){
    return nullptr;
  }
  ret->details.media = libvlc_media_new_path(vlcctx, filename);
  if(ret->details.media == nullptr){
    ncvisual_destroy(ret);
    return nullptr;
  }
  ret->details.manager = libvlc_media_event_manager(ret->details.media);
  if(ret->details.manager == nullptr){
    ncvisual_destroy(ret);
    return nullptr;
  }
  if(libvlc_event_attach(ret->details.manager, libvlc_MediaPlayerOpening,
                         media_callback, ret)){
    ncvisual_destroy(ret);
    return nullptr;
  }
  ret->details.player = libvlc_media_player_new_from_media(ret->details.media);
  if(!ret->details.player){
    ncvisual_destroy(ret);
    return nullptr;
  }
  if(ncvisual_decode(ret)){
    ncvisual_destroy(ret);
    return nullptr;
  }
  return ret;
}

// iterate over the decoded frames, calling streamer() with curry for each.
// frames carry a presentation time relative to the beginning, so we get an
// initial timestamp, and check each frame against the elapsed time to sync
// up playback.
int ncvisual_stream(notcurses* nc, ncvisual* ncv, float timescale,
                    streamcb streamer, const struct ncvisual_options* vopts,
                    void* curry) {
  int frame = 1;
  struct timespec begin; // time we started
  clock_gettime(CLOCK_MONOTONIC, &begin);
  uint64_t nsbegin = timespec_to_ns(&begin);
  bool usets = false;
  // each frame has a pkt_duration in milliseconds. keep the aggregate, in case
  // we don't have PTS available.
  uint64_t sum_duration = 0;
  ncplane* newn = NULL;
  ncvisual_options activevopts;
  memcpy(&activevopts, vopts, sizeof(*vopts));
  int ncerr;
  do{
    // FIXME do timing
    if(activevopts.n){
      ncplane_erase(activevopts.n); // new frame could be partially transparent
    }
    if((newn = ncvisual_render(nc, ncv, &activevopts)) == NULL){
      if(activevopts.n != vopts->n){
        ncplane_destroy(activevopts.n);
      }
      return -1;
    }
    if(activevopts.n != newn){
      activevopts.n = newn;
    }
    ++frame;
    double schedns = nsbegin;
    /*
    uint64_t duration = ncv->details.frame->pkt_duration * tbase * NANOSECS_IN_SEC;
//fprintf(stderr, "use: %u dur: %ju ts: %ju cctx: %f fctx: %f\n", usets, duration, ts, av_q2d(ncv->details.codecctx->time_base), av_q2d(ncv->details.fmtctx->streams[ncv->stream_index]->time_base));
    if(usets){
      if(tbase == 0){
        tbase = duration;
      }
      schedns += ts * (tbase * timescale) * NANOSECS_IN_SEC;
    }else{
      sum_duration += (duration * timescale);
      schedns += sum_duration;
    }
    */
    struct timespec abstime;
    ns_to_timespec(schedns, &abstime);
    int r;
    if(streamer){
      r = streamer(ncv, &activevopts, &abstime, curry);
    }else{
      r = ncvisual_simple_streamer(ncv, &activevopts, &abstime, curry);
    }
    if(r){
      if(activevopts.n != vopts->n){
        ncplane_destroy(activevopts.n);
      }
      return r;
    }
  }while((ncerr = ncvisual_decode(ncv)) == 0);
  if(activevopts.n != vopts->n){
    ncplane_destroy(activevopts.n);
  }
  if(ncerr == 1){ // 1 indicates reaching EOF
    ncerr = 0;
  }
  return ncerr;
}

int ncvisual_decode_loop(ncvisual* ncv){
  return -1;
}

int ncvisual_blit(ncvisual* ncv, int rows, int cols, ncplane* n,
                  const struct blitset* bset, int placey, int placex,
                  int begy, int begx, int leny, int lenx,
                  bool blendcolors) {
  return -1;
}

const char* vlc_version() {
  return libvlc_get_version();
}

auto ncvisual_details_seed(ncvisual* ncv) -> void {
}

int ncvisual_init(int loglevel) {
  const char * const argv[] = { NULL, };
  libvlc_instance_t* vlc = libvlc_new(0, argv);
  if(vlc == NULL){
    return -1;
  }
  // FIXME set up libvlc logging?
  vlcctx = vlc;
  return 0;
}
#endif
