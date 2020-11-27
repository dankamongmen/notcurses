#ifndef NOTCURSES_VISUAL_DETAILS
#define NOTCURSES_VISUAL_DETAILS

#include "builddef.h"
#include "notcurses/notcurses.h"

#ifdef USE_FFMPEG
#include "ffmpeg.h"
#else
#ifdef USE_OIIO
#include "oiio.h"
#else

typedef struct ncvisual_details {
} ncvisual_details;

static inline auto ncvisual_details_init(ncvisual_details* deets) -> int {
  (void)deets;
  return 0;
}

static inline auto
ncvisual_details_destroy(ncvisual_details* deets) -> void {
  (void)deets;
}
#endif
#endif

struct ncplane;

typedef struct ncvisual {
  int cols, rows;
  // lines are sometimes padded. this many true bytes per row in data.
  int rowstride;
  ncvisual_details details;// implementation-specific details
  uint32_t* data; // (scaled) RGBA image data, rowstride bytes per row
  bool owndata; // we own data iff owndata == true
} ncvisual;

static inline auto
ncvisual_create(void) -> ncvisual* {
  auto ret = new ncvisual{};
  if(ret == nullptr){
    return nullptr;
  }
  ncvisual_details_init(&ret->details);
  return ret;
}

static inline auto
ncvisual_set_data(ncvisual* ncv, uint32_t* data, bool owned) -> void {
  if(ncv->owndata){
    free(ncv->data);
  }
  ncv->data = data;
  ncv->owndata = owned;
}

static inline void
scale_visual(const ncvisual* ncv, int* disprows, int* dispcols) {
  float xratio = (float)(*dispcols) / ncv->cols;
  if(xratio * ncv->rows > *disprows){
    xratio = (float)(*disprows) / ncv->rows;
  }
  *disprows = xratio * (ncv->rows);
  *dispcols = xratio * (ncv->cols);
}

#endif
