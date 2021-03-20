#include "builddef.h"
#ifndef USE_OIIO
#ifndef USE_FFMPEG
#include "internal.h"
#include "visual-details.h"

ncvisual* none_create(){
  return new ncvisual{};
}

void none_destroy(ncvisual* ncv){
  delete ncv;
}

int none_decode(ncvisual* nc){
  (void)nc;
  return -1;
}

ncvisual* none_from_file(const char* filename){
  (void)filename;
  return NULL;
}

int none_decode_loop(ncvisual* ncv){
  (void)ncv;
  return -1;
}

// resize, converting to RGBA (if necessary) along the way
int none_resize(ncvisual* nc, int rows, int cols){
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
                  int begy, int begx, int leny, int lenx, const blitterargs* bargs){
  (void)rows;
  (void)cols;
  if(rgba_blit_dispatch(n, bset, ncv->rowstride, ncv->data,
                        begy, begx, leny, lenx, bargs) >= 0){
    return 0;
  }
  return -1;
}

int none_stream(notcurses* nc, ncvisual* ncv, float timescale,
                streamcb streamer, const struct ncvisual_options* vopts, void* curry){
  (void)nc;
  (void)ncv;
  (void)timescale;
  (void)streamer;
  (void)vopts;
  (void)curry;
  return -1;
}

char* none_subtitle(const ncvisual* ncv){ // no support in none
  (void)ncv;
  return NULL;
}

void none_details_seed(ncvisual* ncv){
  (void)ncv;
}

void none_details_destroy(struct ncvisual_details* ncv){
  (void)ncv;
}

int none_init(int loglevel __attribute__ ((unused))) {
  return 0; // allow success here
}

void none_printbanner(const notcurses* nc){
  term_fg_palindex(nc, stderr, nc->tcache.colors <= 88 ? 1 % nc->tcache.colors : 0xcb);
  fprintf(stderr, "\n Warning! Notcurses was built without multimedia support.\n");
}

static const ncvisual_implementation none_impl = {
  .visual_init = none_init,
  .visual_printbanner = none_printbanner,
  .visual_blit = none_blit,
  .visual_create = none_create,
  .visual_from_file = none_from_file,
  .visual_details_seed = none_details_seed,
  .visual_decode = none_decode,
  .visual_decode_loop = none_decode_loop,
  .visual_stream = none_stream,
  .visual_subtitle = none_subtitle,
  .visual_resize = none_resize,
  .visual_destroy = none_destroy,
  .canopen_images = false,
  .canopen_videos = false,
};

const ncvisual_implementation* local_visual_implementation = &none_impl;

#endif
#endif
