#ifndef NOTCURSES_OIIO
#define NOTCURSES_OIIO

// OpenImageIO implementation of ncvisual_details
#include "version.h"
#ifdef USE_OIIO
#include <OpenImageIO/filter.h>
#include <OpenImageIO/version.h>
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>

typedef struct ncvisual_details {
  std::unique_ptr<OIIO::ImageInput> image;  // must be close()d
  std::unique_ptr<uint32_t[]> frame;
  std::unique_ptr<OIIO::ImageBuf> ibuf;
  uint64_t framenum;
} ncvisual_details;

static inline auto
ncvisual_details_init(ncvisual_details *deets) -> int {
  deets->image = nullptr;
  deets->frame = nullptr;
  deets->ibuf = nullptr;
  deets->framenum = 0;
  return 0;
}

static inline auto
ncvisual_details_destroy(ncvisual_details* deets) -> void {
  if(deets->image){
    deets->image->close();
  }
}

#endif

#endif
