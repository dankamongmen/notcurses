#include "internal.h"

#ifdef USE_OIIO
bool notcurses_canopen(const notcurses* nc __attribute__ ((unused))){
  return false;
}

struct AVFrame* ncvisual_decode(ncvisual* nc, int* averr){
  (void)nc;
  (void)averr;
  return NULL;
}

int ncvisual_render(const ncvisual* ncv, int begy, int begx, int leny, int lenx){
  (void)ncv;
  (void)begy;
  (void)begx;
  (void)leny;
  (void)lenx;
  return -1;
}

int ncvisual_stream(struct notcurses* nc, struct ncvisual* ncv, int* averr,
                    float timespec, streamcb streamer, void* curry){
  (void)nc;
  (void)ncv;
  (void)averr;
  (void)timespec;
  (void)streamer;
  (void)curry;
  return -1;
}

ncvisual* ncplane_visual_open(ncplane* nc, const char* filename, int* averr){
  (void)nc;
  (void)filename;
  (void)averr;
  return NULL;
}

ncvisual* ncvisual_open_plane(notcurses* nc, const char* filename,
                              int* averr, int y, int x, ncscale_e style){
  (void)nc;
  (void)filename;
  (void)averr;
  (void)y;
  (void)x;
  (void)style;
  return NULL;
}

char* ncvisual_subtitle(const ncvisual* ncv){
  (void)ncv;
  return NULL;
}

int ncvisual_init(int loglevel){
  (void)loglevel;
  return 0; // allow success here
}
#endif
