#include "builddef.h"
#ifdef USE_OIIO
#include "lib/visual-details.h"
#include "lib/internal.h"
#include "oiio.h"

int oiio_blit_dispatch(struct ncplane* nc, const struct blitset* bset,
                       int linesize, const void* data,
                       int leny, int lenx, const blitterargs* bargs){
  if(rgba_blit_dispatch(nc, bset, linesize, data, leny, lenx, bargs) < 0){
    return -1;
  }
  return 0;
}

int oiio_stream(struct notcurses* nc, ncvisual* ncv, float timescale,
                ncstreamcb streamer, const struct ncvisual_options* vopts,
                void* curry){
  (void)timescale; // FIXME
  int frame = 1;
  struct timespec begin; // time we started
  clock_gettime(CLOCK_MONOTONIC, &begin);
  ncplane* newn = NULL;
  struct ncvisual_options activevopts;
  memcpy(&activevopts, vopts, sizeof(*vopts));
  int ncerr;
  do{
    // decay the blitter explicitly, so that the callback knows the blitter it
    // was actually rendered with
    ncvisual_blitter_geom(nc, ncv, &activevopts, NULL, NULL, NULL, NULL, &activevopts.blitter);
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
      r = streamer(ncv, &activevopts, &now, curry);
    }else{
      r = ncvisual_simple_streamer(ncv, &activevopts, &now, curry);
    }
    if(r){
      if(activevopts.n != vopts->n){
        ncplane_destroy(activevopts.n);
      }
      return r;
    }
    ++frame;
  }while((ncerr = oiio_decode(ncv)) == 0);
  if(activevopts.n != vopts->n){
    ncplane_destroy(activevopts.n);
  }
  if(ncerr == 1){
    return 0;
  }
  return -1;
}

int oiio_init(int logl __attribute__ ((unused))) {
  // FIXME set OIIO global attribute "debug" based on loglevel
  // FIXME check OIIO_VERSION_STRING components against linked openimageio_version()
  return 0; // allow success here
}

const ncvisual_implementation local_visual_implementation = {
  .visual_init = oiio_init,
  .visual_printbanner = oiio_printbanner,
  .visual_blit = oiio_blit,
  .visual_create = oiio_create,
  .visual_from_file = oiio_from_file,
  .visual_details_seed = oiio_details_seed,
  .visual_decode = oiio_decode,
  .visual_decode_loop = oiio_decode_loop,
  .visual_stream = oiio_stream,
  .visual_resize = oiio_resize,
  .visual_destroy = oiio_destroy,
  .canopen_images = true,
  .canopen_videos = false,
};

#endif
