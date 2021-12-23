#include "builddef.h"
#ifdef USE_OIIO
#include <OpenImageIO/filter.h>
#include <OpenImageIO/version.h>
#include <OpenImageIO/imageio.h>
#include <OpenImageIO/imagebuf.h>
#include <OpenImageIO/imagebufalgo.h>
#include "lib/visual-details.h"
#include "oiio.h"

typedef struct ncvisual_details {
  std::unique_ptr<OIIO::ImageInput> image;  // must be close()d
  std::unique_ptr<uint32_t[]> frame;
  std::unique_ptr<OIIO::ImageBuf> ibuf;
  uint64_t framenum;
} ncvisual_details;

auto oiio_details_init(void) -> ncvisual_details* {
  return new ncvisual_details{};
}

auto oiio_details_destroy(ncvisual_details* deets) -> void {
  if(deets->image){
    deets->image->close();
  }
  delete deets;
}

auto oiio_details_seed(ncvisual* ncv) -> void {
  int pixels = ncv->pixy * ncv->pixx;
  ncv->details->frame = std::make_unique<uint32_t[]>(pixels);
  OIIO::ImageSpec rgbaspec{static_cast<int>(ncv->pixx), static_cast<int>(ncv->pixy), 4};
  ncv->details->ibuf = std::make_unique<OIIO::ImageBuf>(rgbaspec, ncv->data);
//fprintf(stderr, "got pixel_stride: %ld %ld\n", ncv->details->ibuf->pixel_stride(), ncv->details->ibuf->scanline_stride());
}

auto oiio_create() -> ncvisual* {
  auto nc = new ncvisual{};
  if((nc->details = oiio_details_init()) == nullptr){
    delete nc;
    return nullptr;
  }
  return nc;
}

int oiio_decode(ncvisual* nc) {
//fprintf(stderr, "current subimage: %d frame: %p\n", nc->details->image->current_subimage(), nc->details->frame.get());
  const auto &spec = nc->details->image->spec_dimensions(nc->details->framenum);
  if(nc->details->frame){
//fprintf(stderr, "seeking subimage: %d\n", nc->details->image->current_subimage() + 1);
    OIIO::ImageSpec newspec;
    if(!nc->details->image->seek_subimage(nc->details->image->current_subimage() + 1, 0, newspec)){
       return 1;
    }
    // FIXME check newspec vis-a-vis image->spec()?
  }
//fprintf(stderr, "SUBIMAGE: %d\n", nc->details->image->current_subimage());
  auto pixels = spec.width * spec.height;// * spec.nchannels;
  if(spec.nchannels < 3 || spec.nchannels > 4){
    return -1; // FIXME get some to test with
  }
  nc->details->frame = std::make_unique<uint32_t[]>(pixels);
  if(spec.nchannels == 3){ // FIXME replace with channel shuffle
    std::fill(nc->details->frame.get(), nc->details->frame.get() + pixels, 0xfffffffful);
  }
//fprintf(stderr, "READING: %d %ju\n", nc->details->image->current_subimage(), nc->details->framenum);
  if(!nc->details->image->read_image(nc->details->framenum++, 0, 0, spec.nchannels, OIIO::TypeDesc(OIIO::TypeDesc::UINT8), nc->details->frame.get(), 4)){
    return -1;
  }
//fprintf(stderr, "READ: %d %ju\n", nc->details->image->current_subimage(), nc->details->framenum);
/*for(int i = 0 ; i < pixels ; ++i){
  //fprintf(stderr, "%06d %02x %02x %02x %02x\n", i,
  fprintf(stderr, "%06d %d %d %d %d\n", i,
      (nc->details->frame[i]) & 0xff,
      (nc->details->frame[i] >> 8) & 0xff,
      (nc->details->frame[i] >> 16) & 0xff,
      nc->details->frame[i] >> 24
      );
}*/
  nc->pixx = spec.width;
  nc->pixy = spec.height;
  nc->rowstride = nc->pixx * 4;
  OIIO::ImageSpec rgbaspec = spec;
  rgbaspec.nchannels = 4;
  nc->details->ibuf = std::make_unique<OIIO::ImageBuf>(rgbaspec, nc->details->frame.get());
//fprintf(stderr, "SUBS: %d\n", nc->details->ibuf->nsubimages());
  ncvisual_set_data(nc, static_cast<uint32_t*>(nc->details->ibuf->localpixels()), false);
//fprintf(stderr, "POST-DECODE DATA: %d %d %p %p\n", nc->pixy, nc->pixx, nc->data, nc->details->ibuf->localpixels());
  return 0;
}

ncvisual* oiio_from_file(const char* filename) {
  ncvisual* ncv = oiio_create();
  if(ncv == nullptr){
    return nullptr;
  }
  ncv->details->image = OIIO::ImageInput::open(filename);
  if(!ncv->details->image){
    // fprintf(stderr, "Couldn't create %s (%s)\n", filename, strerror(errno));
    ncvisual_destroy(ncv);
    return nullptr;
  }
/*const auto &spec = ncv->details->image->spec_dimensions(0);
std::cout << "Opened " << filename << ": " << spec.height << "x" <<
spec.width << "@" << spec.nchannels << " (" << spec.format << ")" << std::endl;*/
  if(oiio_decode(ncv)){
    ncvisual_destroy(ncv);
    return nullptr;
  }
  return ncv;
}

int oiio_decode_loop(ncvisual* ncv){
  int r = oiio_decode(ncv);
  if(r == 1){
    OIIO::ImageSpec newspec;
    if(!ncv->details->image->seek_subimage(0, 0, newspec)){
      return -1;
    }
    ncv->details->framenum = 0;
    if(oiio_decode(ncv) < 0){
      return -1;
    }
  }
  return r;
}

// resize, converting to RGBA (if necessary) along the way
int oiio_resize(ncvisual* nc, unsigned rows, unsigned cols) {
//fprintf(stderr, "%d/%d -> %d/%d on the resize\n", nc->pixy, nc->pixx, rows, cols);
  auto ibuf = std::make_unique<OIIO::ImageBuf>();
  if(nc->details->ibuf && (nc->pixx != cols || nc->pixy != rows)){ // scale it
    OIIO::ROI roi(0, cols, 0, rows, 0, 1, 0, 4);
    if(!OIIO::ImageBufAlgo::resize(*ibuf, *nc->details->ibuf, "", 0, roi)){
      return -1;
    }
    nc->pixx = cols;
    nc->pixy = rows;
    nc->rowstride = cols * 4;
    ncvisual_set_data(nc, static_cast<uint32_t*>(ibuf->localpixels()), false);
//fprintf(stderr, "HAVE SOME NEW DATA: %p\n", ibuf->localpixels());
    nc->details->ibuf = std::move(ibuf);
  }
  return 0;
}

int oiio_blit(const ncvisual* ncv, unsigned rows, unsigned cols,
              ncplane* n, const struct blitset* bset,
              const blitterargs* bargs) {
//fprintf(stderr, "%d/%d -> %d/%d on the resize\n", ncv->pixy, ncv->pixx, rows, cols);
  void* data = nullptr;
  int stride;
  auto ibuf = std::make_unique<OIIO::ImageBuf>();
  if(ncv->details->ibuf && (ncv->pixx != cols || ncv->pixy != rows)){ // scale it
    // FIXME need to honor leny/lenx and begy/begx
    OIIO::ROI roi(0, cols, 0, rows, 0, 1, 0, 4);
    if(!OIIO::ImageBufAlgo::resize(*ibuf, *ncv->details->ibuf, "", 0, roi)){
      return -1;
    }
    stride = ibuf->scanline_stride();
    data = ibuf->localpixels();
//fprintf(stderr, "HAVE SOME NEW DATA: %p\n", ibuf->localpixels());
  }else{
    data = ncv->data;
    stride = ncv->rowstride;
  }
//std::cerr << "output: " << ibuf->roi() << " stride: " << stride << " pstride: " << pstride << std::endl;
  return oiio_blit_dispatch(n, bset, stride, data, rows, cols, bargs);
}

// FIXME before we can enable this, we need build an OIIO::APPBUFFER-style
// ImageBuf in ncvisual in ncvisual_from_rgba().
/*
auto ncvisual_rotate(ncvisual* ncv, double rads) -> int {
  OIIO::ROI roi(0, ncv->pixx, 0, ncv->pixy, 0, 1, 0, 4);
  auto tmpibuf = std::move(*ncv->details->ibuf);
  ncv->details->ibuf = std::make_unique<OIIO::ImageBuf>();
  OIIO::ImageSpec sp{};
  sp.set_format(OIIO::TypeDesc(OIIO::TypeDesc::UINT8));
  sp.nchannels = 4;
  ncv->details->ibuf->reset();
  if(!OIIO::ImageBufAlgo::rotate(*ncv->details->ibuf, tmpibuf, rads, "", 0, true, roi)){
    return NCERR_DECODE; // FIXME need we do anything further?
  }
  ncv->rowstride = ncv->pixx * 4;
  ncvisual_set_data(ncv, static_cast<uint32_t*>(ncv->details->ibuf->localpixels()), false);
  return NCERR_SUCCESS;
}
*/

auto oiio_destroy(ncvisual* ncv) -> void {
  if(ncv){
    oiio_details_destroy(ncv->details);
    if(ncv->owndata){
      free(ncv->data);
    }
    delete ncv;
  }
}

// FIXME would be nice to have OIIO::attributes("libraries") in here
void oiio_printbanner(fbuf* f){
  fbuf_puts(f, "openimageio ");
  fbuf_puts(f, OIIO_VERSION_STRING);
  std::string s = OIIO::get_string_attribute("oiio:simd");
  fbuf_printf(f, " %s (with%s video)" NL, s.c_str(),
              local_visual_implementation.canopen_videos ? "" : "out");
}

int oiio_init(int logl __attribute__ ((unused))) {
  // FIXME set OIIO global attribute "debug" based on loglevel
  std::string s = OIIO::get_string_attribute("library_list");
  if(s.find("ffmpeg") != std::string::npos){
    local_visual_implementation.canopen_videos = true;
  }
  return 0; // allow success here
}

#endif
