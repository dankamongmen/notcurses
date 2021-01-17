#include "builddef.h"
#ifndef NOTCURSES_USE_MULTIMEDIA
#include <stdlib.h>
#include "internal.h"
#include "visual-details.h"

void inject_implementation(void) __attribute__ ((constructor));

typedef struct ncvisual_details {
  bool null;
} ncvisual_details;

static ncvisual*
null_create(void){
  return new ncvisual{};
}

static const ncvisual_implementation ffmpeg_impl = {
  .ncvisual_init = NULL,
  .ncvisual_decode = NULL,
  .ncvisual_blit = NULL,
  .ncvisual_create = null_create,
  .ncvisual_from_file = NULL,
  .ncvisual_printbanner = NULL,
  .ncvisual_details_seed = NULL,
  .ncvisual_details_destroy = NULL,
  .canopen_images = false,
  .canopen_videos = false,
};

void inject_implementation(void){
  notcurses_set_ncvisual_implementation(&ffmpeg_impl);
}

#endif
