#ifndef NOTCURSES_VISUAL_DETAILS
#define NOTCURSES_VISUAL_DETAILS

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "builddef.h"

struct blitset;
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

static inline void
ncvisual_set_data(ncvisual* ncv, void* data, bool owned){
  if(ncv->owndata){
    free(ncv->data);
  }
  ncv->data = (uint32_t*)data;
  ncv->owndata = owned;
}

static inline void
scale_visual(const ncvisual* ncv, int* disprows, int* dispcols){
  float xratio = (float)(*dispcols) / ncv->cols;
  if(xratio * ncv->rows > *disprows){
    xratio = (float)(*disprows) / ncv->rows;
  }
  *disprows = xratio * (ncv->rows);
  *dispcols = xratio * (ncv->cols);
}

#ifdef __cplusplus
}
#endif

#endif
