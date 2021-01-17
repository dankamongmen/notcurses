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

typedef struct ncvisual_implementation {
  int (*ncvisual_init)(int loglevel);
  int (*ncvisual_decode)(ncvisual*);
  int (*ncvisual_blit)(ncvisual* ncv, int rows, int cols, ncplane* n,
                       const struct blitset* bset, int placey, int placex,
                       int begy, int begx, int leny, int lenx,
                       bool blendcolors);
  ncvisual* (*ncvisual_create)(void);
  ncvisual* (*ncvisual_from_file)(const char* s);
  // ncv constructors other than ncvisual_from_file() need to set up the
  // AVFrame* 'frame' according to their own data, which is assumed to
  // have been prepared already in 'ncv'.
  void (*ncvisual_details_seed)(ncvisual* ncv);
  void (*ncvisual_details_destroy)(ncvisual_details* deets);
  bool canopen_images;
  bool canopen_videos;
} ncvisual_implementation;

__attribute__((visibility("default")))
int notcurses_set_ncvisual_implementation(const ncvisual_implementation* imp);

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
