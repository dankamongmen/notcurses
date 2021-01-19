#ifndef NOTCURSES_VISUAL_DETAILS
#define NOTCURSES_VISUAL_DETAILS

#include "builddef.h"
#include "notcurses/notcurses.h"

struct ncplane;
struct ncvisual_details;

typedef struct ncvisual {
  struct ncvisual_details* details;// implementation-specific details
  uint32_t* data; // (scaled) RGBA image data, rowstride bytes per row
  int cols, rows;
  // lines are sometimes padded. this many true bytes per row in data.
  int rowstride;
  bool owndata; // we own data iff owndata == true
} ncvisual;

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
