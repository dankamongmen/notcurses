#include "builddef.h"
#ifdef USE_VLC
#include "vlc.h"
#include "internal.h"
#include "visual-details.h"
#include <vlc/libvlc.h>
#include <vlc/libvlc_media.h>

static libvlc_instance_t* vlcctx;

bool notcurses_canopen_images(const notcurses* nc __attribute__ ((unused))) {
  return true;
}

bool notcurses_canopen_videos(const notcurses* nc __attribute__ ((unused))) {
  return true;
}

auto ncvisual_subtitle(const ncvisual* ncv) -> char* {
  return nullptr;
}

int ncvisual_decode(ncvisual* nc){
  return -1;
}

// resize frame to oframe, converting to RGBA (if necessary) along the way
int ncvisual_resize(ncvisual* nc, int rows, int cols) {
  return -1;
}

ncvisual* ncvisual_from_file(const char* filename) {
  ncvisual* ret = new ncvisual;
  if(ret == nullptr){
    return nullptr;
  }
  ret->details.media = libvlc_media_new_path(vlcctx, filename);
  if(ret->details.media == nullptr){
    delete ret;
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
  return -1;
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
  vlcctx = vlc;
  return 0;
}
#endif
