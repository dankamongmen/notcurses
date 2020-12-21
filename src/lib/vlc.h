#ifndef NOTCURSES_VLC
#define NOTCURSES_VLC

#include <string.h>
#include "version.h"
#ifdef USE_VLC

#include <vlc/libvlc.h>
#include <vlc/libvlc_media.h>
#include <vlc/libvlc_events.h>
#include <vlc/libvlc_media_player.h>

typedef struct ncvisual_details {
  libvlc_media_t* media;
  libvlc_event_manager_t* manager;
  libvlc_media_player_t* player;
} ncvisual_details;

static inline auto
ncvisual_details_init(ncvisual_details* deets) -> int {
  memset(deets, 0, sizeof(*deets));
  return 0;
}

static inline auto
ncvisual_details_destroy(ncvisual_details* deets) -> void {
  if(deets->media){
    libvlc_media_player_release(deets->player);
    libvlc_media_release(deets->media);
  }
}

#endif

#endif
