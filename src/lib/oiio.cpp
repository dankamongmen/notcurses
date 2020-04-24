#include "version.h"
#ifdef USE_OIIO
#include <OpenImageIO/imageio.h>
#include "internal.h"

extern "C" {

typedef struct ncvisual {
  int packet_outstanding;
  int dstwidth, dstheight;
  float timescale;         // scale frame duration by this value
  ncplane* ncp;
  // if we're creating the plane based off the first frame's dimensions, these
  // describe where the plane ought be placed, and how it ought be sized. this
  // path sets ncobj. ncvisual_destroy() ought in that case kill the ncplane.
  int placex, placey;
  ncscale_e style;         // none, scale, or stretch
  struct notcurses* ncobj; // set iff this ncvisual "owns" its ncplane
} ncvisual;

bool notcurses_canopen(const notcurses* nc __attribute__ ((unused))){
  return true;
}

static ncvisual*
ncvisual_create(float timescale){
  auto ret = new ncvisual;
  if(ret == nullptr){
    return nullptr;
  }
  memset(ret, 0, sizeof(*ret));
  ret->timescale = timescale;
  return ret;
}

static ncvisual*
ncvisual_open(const char* filename, nc_err_e* err){
  auto in = OIIO::ImageInput::open(filename);
  if(!in){
    return nullptr;
  }
  ncvisual* ncv = ncvisual_create(1);
  if(ncv == nullptr){
    // fprintf(stderr, "Couldn't create %s (%s)\n", filename, strerror(errno));
    *err = NCERR_NOMEM;
    return nullptr;
  }
  memset(ncv, 0, sizeof(*ncv));
  return ncv;
}

ncvisual* ncplane_visual_open(ncplane* nc, const char* filename, nc_err_e* ncerr){
  ncvisual* ncv = ncvisual_open(filename, ncerr);
  if(ncv == nullptr){
    return nullptr;
  }
  ncplane_dim_yx(nc, &ncv->dstheight, &ncv->dstwidth);
  ncv->dstheight *= 2;
  ncv->ncp = nc;
  ncv->style = NCSCALE_STRETCH;
  return ncv;
}

ncvisual* ncvisual_open_plane(notcurses* nc, const char* filename,
                              nc_err_e* ncerr, int y, int x, ncscale_e style){
  ncvisual* ncv = ncvisual_open(filename, ncerr);
  if(ncv == nullptr){
    return nullptr;
  }
  ncv->placey = y;
  ncv->placex = x;
  ncv->style = style;
  ncv->ncobj = nc;
  ncv->ncp = nullptr;
  return ncv;
}

nc_err_e ncvisual_decode(ncvisual* nc){
  (void)nc; // FIXME
  return NCERR_DECODE;
}

int ncvisual_render(const ncvisual* ncv, int begy, int begx, int leny, int lenx){
  (void)ncv;
  (void)begy;
  (void)begx;
  (void)leny;
  (void)lenx;
  return -1;
}

int ncvisual_stream(struct notcurses* nc, struct ncvisual* ncv, nc_err_e* ncerr,
                    float timespec, streamcb streamer, void* curry){
  (void)nc;
  (void)ncv;
  (void)ncerr;
  (void)timespec;
  (void)streamer;
  (void)curry;
  return -1;
}

char* ncvisual_subtitle(const ncvisual* ncv){ // no support in OIIO
  (void)ncv;
  return nullptr;
}

int ncvisual_init(int loglevel){
  (void)loglevel;
  return 0; // allow success here
}

} // extern "C"

#endif
