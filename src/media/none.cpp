#include "builddef.h"
#ifndef USE_OIIO
#ifndef USE_FFMPEG
#include "internal.h"
#include "visual-details.h"

auto none_create() -> ncvisual* {
  return new ncvisual{};
}

int none_decode(ncvisual* nc) {
  (void)nc;
  return -1;
}

ncvisual* none_from_file(const char* filename) {
  (void)filename;
  return nullptr;
}

int none_decode_loop(ncvisual* ncv){
  (void)ncv;
  return -1;
}

// resize, converting to RGBA (if necessary) along the way
int none_resize(ncvisual* nc, int rows, int cols) {
  // we'd need to verify that it's RGBA as well, except that if we've got no
  // multimedia engine, we've only got memory-assembled ncvisuals, which are
  // RGBA-native. so we ought be good, but this is undeniably sloppy...
  if(nc->rows == rows && nc->cols == cols){
    return 0;
  }
  return -1;
}

int none_blit(struct ncvisual* ncv, int rows, int cols,
                  ncplane* n, const struct blitset* bset,
                  int placey, int placex, int begy, int begx,
                  int leny, int lenx, const blitterargs* bargs) {
  (void)rows;
  (void)cols;
  if(rgba_blit_dispatch(n, bset, placey, placex, ncv->rowstride, ncv->data,
                        begy, begx, leny, lenx, bargs) >= 0){
    return 0;
  }
  return -1;
}

auto none_stream(notcurses* nc, ncvisual* ncv, float timescale,
                 streamcb streamer, const struct ncvisual_options* vopts, void* curry) -> int {
  (void)nc;
  (void)ncv;
  (void)timescale;
  (void)streamer;
  (void)vopts;
  (void)curry;
  return -1;
}

char* none_subtitle(const ncvisual* ncv) { // no support in none
  (void)ncv;
  return nullptr;
}

// FIXME before we can enable this, we need build an none::APPBUFFER-style
// ImageBuf in ncvisual in ncvisual_from_rgba().
/*
auto ncvisual_rotate(ncvisual* ncv, double rads) -> int {
  none::ROI roi(0, ncv->cols, 0, ncv->rows, 0, 1, 0, 4);
  auto tmpibuf = std::move(*ncv->details->ibuf);
  ncv->details->ibuf = std::make_unique<none::ImageBuf>();
  none::ImageSpec sp{};
  sp.set_format(none::TypeDesc(none::TypeDesc::UINT8, 4));
  sp.nchannels = 4;
  ncv->details->ibuf->reset();
  if(!none::ImageBufAlgo::rotate(*ncv->details->ibuf, tmpibuf, rads, "", 0, true, roi)){
    return NCERR_DECODE; // FIXME need we do anything further?
  }
  ncv->rowstride = ncv->cols * 4;
  ncvisual_set_data(ncv, static_cast<uint32_t*>(ncv->details->ibuf->localpixels()), false);
  return NCERR_SUCCESS;
}
*/

auto none_details_seed(ncvisual* ncv) -> void {
  (void)ncv;
}

auto none_details_destroy(struct ncvisual_details* ncv) -> void {
  (void)ncv;
}

int none_init(int loglevel __attribute__ ((unused))) {
  return 0; // allow success here
}

void none_printbanner(const notcurses* nc){
  term_fg_palindex(nc, stderr, nc->tcache.colors <= 88 ? 1 % nc->tcache.colors : 0xcb);
  fprintf(stderr, "\n Warning! Notcurses was built without multimedia support.\n");
}

const static ncvisual_implementation none_impl = {
  .visual_init = none_init,
  .visual_printbanner = none_printbanner,
  .visual_blit = none_blit,
  .visual_create = none_create,
  .visual_from_file = none_from_file,
  .visual_details_seed = none_details_seed,
  .visual_details_destroy = none_details_destroy,
  .visual_decode = none_decode,
  .visual_decode_loop = none_decode_loop,
  .visual_stream = none_stream,
  .visual_subtitle = none_subtitle,
  .visual_resize = none_resize,
  .canopen_images = false,
  .canopen_videos = false,
};

const ncvisual_implementation* local_visual_implementation = &none_impl;

#endif
#endif
