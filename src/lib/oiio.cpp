#include "version.h"
#ifdef USE_OIIO
#include <OpenImageIO/version.h>
#include <OpenImageIO/imageio.h>
#include "internal.h"

typedef struct ncvisual {
  int packet_outstanding;
  int dstwidth, dstheight;
  float timescale;         // scale frame duration by this value
  std::unique_ptr<OIIO::ImageInput> image;  // must be close()d
  std::unique_ptr<uint32_t[]> frame;
  ncplane* ncp;
  // if we're creating the plane based off the first frame's dimensions, these
  // describe where the plane ought be placed, and how it ought be sized. this
  // path sets ncobj. ncvisual_destroy() ought in that case kill the ncplane.
  int placex, placey;
  ncscale_e style;         // none, scale, or stretch
  struct notcurses* ncobj; // set iff this ncvisual "owns" its ncplane
} ncvisual;

extern "C" {

bool notcurses_canopen(const notcurses* nc __attribute__ ((unused))){
  return true;
}

static ncvisual*
ncvisual_create(float timescale){
  auto ret = new ncvisual;
  if(ret == nullptr){
    return nullptr;
  }
  ret->packet_outstanding = 0;
  ret->dstwidth = ret->dstheight = 0;
  ret->timescale = timescale;
  ret->image = nullptr;
  ret->ncp = nullptr;
  ret->placex = ret->placey = 0;
  ret->style = NCSCALE_NONE;
  ret->ncobj = nullptr;
  ret->frame = nullptr;
  return ret;
}

static ncvisual*
ncvisual_open(const char* filename, nc_err_e* err){
  ncvisual* ncv = ncvisual_create(1);
  if(ncv == nullptr){
    *err = NCERR_NOMEM;
    return nullptr;
  }
  ncv->image = OIIO::ImageInput::open(filename);
  if(!ncv->image){
    // fprintf(stderr, "Couldn't create %s (%s)\n", filename, strerror(errno));
    *err = NCERR_DECODE;
    return nullptr;
  }
  return ncv;
}

ncvisual* ncplane_visual_open(ncplane* nc, const char* filename, nc_err_e* ncerr){
  ncvisual* ncv = ncvisual_open(filename, ncerr);
  if(ncv == nullptr){
    *ncerr = NCERR_NOMEM;
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
  const auto &spec = nc->image->spec();
  auto pixels = spec.width * spec.height;
  nc->frame = std::make_unique<uint32_t[]>(pixels);
  if(!nc->image->read_image(OIIO::TypeDesc::UINT32, nc->frame.get())){
    return NCERR_DECODE;
  }
  return NCERR_SUCCESS;
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

void ncvisual_destroy(ncvisual* ncv){
  if(ncv){
    ncv->image->close();
    delete ncv;
  }
}

const char* oiio_version(void){
  return OIIO_VERSION_STRING;
}

} // extern "C"

#endif
