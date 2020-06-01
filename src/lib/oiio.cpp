#include "version.h"
#ifdef USE_OIIO
#include "oiio.h"
#include "internal.h"
#include "visual-details.h"

bool notcurses_canopen_images(const notcurses* nc __attribute__ ((unused))) {
  return true;
}

bool notcurses_canopen_videos(const notcurses* nc __attribute__ ((unused))) {
  return false; // too slow for reliable use at the moment
}

ncvisual* ncvisual_from_file(const char* filename, nc_err_e* err) {
  *err = NCERR_SUCCESS;
  ncvisual* ncv = ncvisual_create();
  if(ncv == nullptr){
    *err = NCERR_NOMEM;
    return nullptr;
  }
  ncv->details.image = OIIO::ImageInput::open(filename);
  if(!ncv->details.image){
    // fprintf(stderr, "Couldn't create %s (%s)\n", filename, strerror(errno));
    *err = NCERR_DECODE;
    ncvisual_destroy(ncv);
    return nullptr;
  }
/*const auto &spec = ncv->details.image->spec_dimensions();
std::cout << "Opened " << filename << ": " << spec.height << "x" <<
spec.width << "@" << spec.nchannels << " (" << spec.format << ")" << std::endl;*/
  return ncv;
}

nc_err_e ncvisual_decode(ncvisual* nc) {
//fprintf(stderr, "current subimage: %d frame: %p\n", nc->details.image->current_subimage(), nc->details.frame.get());
  const auto &spec = nc->details.image->spec_dimensions(nc->details.framenum);
  if(nc->details.frame){
//fprintf(stderr, "seeking subimage: %d\n", nc->details.image->current_subimage() + 1);
    OIIO::ImageSpec newspec;
    if(!nc->details.image->seek_subimage(nc->details.image->current_subimage() + 1, 0, newspec)){
       return NCERR_EOF;
    }
    // FIXME check newspec vis-a-vis image->spec()?
  }
//fprintf(stderr, "SUBIMAGE: %d\n", nc->details.image->current_subimage());
  auto pixels = spec.width * spec.height;// * spec.nchannels;
  if(spec.nchannels < 3 || spec.nchannels > 4){
    return NCERR_DECODE; // FIXME get some to test with
  }
  nc->details.frame = std::make_unique<uint32_t[]>(pixels);
  if(spec.nchannels == 3){ // FIXME replace with channel shuffle
    std::fill(nc->details.frame.get(), nc->details.frame.get() + pixels, 0xfffffffful);
  }
//fprintf(stderr, "READING: %d %ju\n", nc->details.image->current_subimage(), nc->details.framenum);
  if(!nc->details.image->read_image(nc->details.framenum++, 0, 0, spec.nchannels, OIIO::TypeDesc(OIIO::TypeDesc::UINT8, 4), nc->details.frame.get(), 4)){
    return NCERR_DECODE;
  }
//fprintf(stderr, "READ: %d %ju\n", nc->details.image->current_subimage(), nc->details.framenum);
/*for(int i = 0 ; i < pixels ; ++i){
  //fprintf(stderr, "%06d %02x %02x %02x %02x\n", i,
  fprintf(stderr, "%06d %d %d %d %d\n", i,
      (nc->details.frame[i]) & 0xff,
      (nc->details.frame[i] >> 8) & 0xff,
      (nc->details.frame[i] >> 16) & 0xff,
      nc->details.frame[i] >> 24
      );
}*/
  nc->cols = spec.width;
  nc->rows = spec.height;
  nc->rowstride = nc->cols * 4;
  OIIO::ImageSpec rgbaspec = spec;
  rgbaspec.nchannels = 4;
  nc->details.ibuf = std::make_unique<OIIO::ImageBuf>(rgbaspec, nc->details.frame.get());
//fprintf(stderr, "SUBS: %d\n", nc->details.ibuf->nsubimages());
  ncvisual_set_data(nc, static_cast<uint32_t*>(nc->details.ibuf->localpixels()), false);
//fprintf(stderr, "POST-DECODE DATA: %d %d %p %p\n", nc->rows, nc->cols, nc->data, nc->details.ibuf->localpixels());
  return NCERR_SUCCESS;
}

// resize, converting to RGBA (if necessary) along the way
nc_err_e ncvisual_resize(ncvisual* nc, int rows, int cols) {
//fprintf(stderr, "%d/%d -> %d/%d on the resize\n", ncv->rows, ncv->cols, rows, cols);
  auto ibuf = std::make_unique<OIIO::ImageBuf>();
  if(nc->details.ibuf && (nc->cols != cols || nc->rows != rows)){ // scale it
    OIIO::ImageSpec sp{};
    sp.width = cols;
    sp.height = rows;
    ibuf->reset(sp, OIIO::InitializePixels::Yes);
    OIIO::ROI roi(0, cols, 0, rows, 0, 1, 0, 4);
    if(!OIIO::ImageBufAlgo::resize(*ibuf, *nc->details.ibuf, "", 0, roi)){
      return NCERR_DECODE;
    }
    nc->cols = cols;
    nc->rows = rows;
    nc->rowstride = cols * 4;
    ncvisual_set_data(nc, static_cast<uint32_t*>(ibuf->localpixels()), false);
//fprintf(stderr, "HAVE SOME NEW DATA: %p\n", ibuf->localpixels());
  }
  return NCERR_SUCCESS;
}

nc_err_e ncvisual_blit(struct ncvisual* ncv, int rows, int cols,
                       ncplane* n, const struct blitset* bset,
                       int placey, int placex, int begy, int begx,
                       int leny, int lenx, bool blendcolors) {
//fprintf(stderr, "%d/%d -> %d/%d on the resize\n", ncv->rows, ncv->cols, rows, cols);
  void* data = nullptr;
  int stride = 0;
  auto ibuf = std::make_unique<OIIO::ImageBuf>();
  if(ncv->details.ibuf && (ncv->cols != cols || ncv->rows != rows)){ // scale it
    OIIO::ImageSpec sp{};
    sp.width = cols;
    sp.height = rows;
    ibuf->reset(sp, OIIO::InitializePixels::Yes);
    OIIO::ROI roi(0, cols, 0, rows, 0, 1, 0, 4);
    if(!OIIO::ImageBufAlgo::resize(*ibuf, *ncv->details.ibuf, "", 0, roi)){
      return NCERR_DECODE;
    }
    stride = cols * 4;
    data = ibuf->localpixels();
//fprintf(stderr, "HAVE SOME NEW DATA: %p\n", ibuf->localpixels());
  }else{
    data = ncv->data;
    stride = ncv->rowstride;
  }
  if(rgba_blit_dispatch(n, bset, placey, placex, stride, data, begy, begx,
                        leny, lenx, blendcolors) <= 0){
    return NCERR_DECODE;
  }
  return NCERR_SUCCESS;
}

auto ncvisual_stream(notcurses* nc, ncvisual* ncv, nc_err_e* ncerr, float timescale,
                     streamcb streamer, const struct ncvisual_options* vopts, void* curry) -> int {
  (void)timescale; // FIXME
  *ncerr = NCERR_SUCCESS;
  int frame = 1;
  struct timespec begin; // time we started
  clock_gettime(CLOCK_MONOTONIC, &begin);
  ncplane* newn = nullptr;
  ncvisual_options activevopts;
  memcpy(&activevopts, vopts, sizeof(*vopts));
  while((*ncerr = ncvisual_decode(ncv)) == NCERR_SUCCESS){
    if((newn = ncvisual_render(nc, ncv, &activevopts)) == NULL){
      if(activevopts.n != vopts->n){
        ncplane_destroy(activevopts.n);
      }
      return -1;
    }
    if(activevopts.n != newn){
      activevopts.n = newn;
    }
    // currently OIIO is so slow for videos that there's no real point in
    // any kind of delay FIXME
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    int r;
    if(streamer){
      r = streamer(newn, ncv, &now, curry);
    }
    if(r){
      if(activevopts.n != vopts->n){
        ncplane_destroy(activevopts.n);
      }
      return r;
    }
    ++frame;
  }
  if(activevopts.n != vopts->n){
    ncplane_destroy(activevopts.n);
  }
  if(*ncerr == NCERR_EOF){
    return 0;
  }
  return -1;
}

char* ncvisual_subtitle(const ncvisual* ncv) { // no support in OIIO
  (void)ncv;
  return nullptr;
}

// FIXME before we can enable this, we need build an OIIO::APPBUFFER-style
// ImageBuf in ncvisual in ncvisual_from_rgba().
/*
auto ncvisual_rotate(ncvisual* ncv, double rads) -> int {
  OIIO::ROI roi(0, ncv->cols, 0, ncv->rows, 0, 1, 0, 4);
  auto tmpibuf = std::move(*ncv->details.ibuf);
  ncv->details.ibuf = std::make_unique<OIIO::ImageBuf>();
  OIIO::ImageSpec sp{};
  sp.set_format(OIIO::TypeDesc(OIIO::TypeDesc::UINT8, 4));
  sp.nchannels = 4;
  ncv->details.ibuf->reset();
  if(!OIIO::ImageBufAlgo::rotate(*ncv->details.ibuf, tmpibuf, rads, "", 0, true, roi)){
    return NCERR_DECODE; // FIXME need we do anything further?
  }
  ncv->rowstride = ncv->cols * 4;
  ncvisual_set_data(ncv, static_cast<uint32_t*>(ncv->details.ibuf->localpixels()), false);
  return NCERR_SUCCESS;
}
*/

auto ncvisual_details_seed(ncvisual* ncv) -> void {
  (void)ncv;
  // FIXME?
}

int ncvisual_init(int loglevel) {
  // FIXME set OIIO global attribute "debug" based on loglevel
  (void)loglevel;
  // FIXME check OIIO_VERSION_STRING components against linked openimageio_version()
  return 0; // allow success here
}

extern "C" {
// FIXME would be nice to have OIIO::attributes("libraries") in here
const char* oiio_version(void){
  return OIIO_VERSION_STRING;
}
}
#endif
