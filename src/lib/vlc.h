#ifndef NOTCURSES_VLC
#define NOTCURSES_VLC

#include <string.h>
#include "version.h"
#ifdef USE_VLC

#include <vlc/libvlc.h>
#include <vlc/libvlc_media.h>

typedef struct ncvisual_details {
  libvlc_media_t *media;
} ncvisual_details;

static inline auto
ncvisual_details_init(ncvisual_details* deets) -> int {
  memset(deets, 0, sizeof(*deets));
  return 0;
}

static inline auto
ncvisual_details_destroy(ncvisual_details* deets) -> void {
  if(deets->media){
    libvlc_media_release(deets->media);
  }
}

#endif

#endif
