#include "internal.h"
#ifdef USE_LIBSIXEL
#include <sixel/sixel.h>

typedef struct libsixel_closure {
  void* buf;
  int size;
} libsixel_closure;

static int
libsixel_writer(char* data, int size, void* priv){
  libsixel_closure* closure = priv;
  void* tmp = realloc(closure->buf, closure->size + size);
  if(tmp == NULL){
    return -1;
  }
  closure->buf = tmp;
  memcpy(closure->buf + closure->size, data, size);
  closure->size += size;
  return 0;
}

// libsixel wants to mutate its input, and can't accept arbitrary linesizes.
// create a space of 4 * leny * lenx, and copy in the source.
static void*
dup_for_libsixel(const void* data, int linesize, int leny, int lenx){
  char* cpy = malloc(leny * lenx * 4);
  if(cpy){
    for(int y = 0 ; y < leny ; ++y){
      memcpy(cpy + y * lenx * 4, (const char*)data + linesize * y, lenx * 4);
    }
  }
  return cpy;
}

static int
libsixel_blit_inner(ncplane* nc, int linesize, const void* data,
                        int begy, int begx, int leny, int lenx,
                        const blitterargs* bargs){
  int colors = bargs->pixel.colorregs > 256 ? 256 : bargs->pixel.colorregs;
  (void)begy;
  (void)begx;
  void* cpy = dup_for_libsixel(data, linesize, leny, lenx);
  if(cpy == NULL){
    return -1;
  }
  libsixel_closure closure = { .buf = NULL, .size = 0, };
  sixel_dither_t* dither = NULL;
  sixel_output_t* output;
  // FIXME provide bargs->pixels.colorregs
  SIXELSTATUS status = sixel_dither_new(&dither, colors, NULL);
  if(SIXEL_FAILED(status)){
    free(cpy);
    return -1;
  }
  status = sixel_dither_initialize(dither, cpy, lenx, leny, SIXEL_PIXELFORMAT_RGBA8888, SIXEL_LARGE_AUTO, SIXEL_REP_AUTO, SIXEL_QUALITY_AUTO);
  if(SIXEL_FAILED(status)){
    sixel_dither_destroy(dither);
    free(cpy);
    return -1;
  }
  if(SIXEL_OK != sixel_output_new(&output, libsixel_writer, &closure, NULL)){
    sixel_dither_destroy(dither);
    free(cpy);
    return -1;
  }
  if(SIXEL_OK != sixel_encode(cpy, lenx, leny, 0, dither, output)){
    sixel_output_destroy(output);
    sixel_dither_destroy(dither);
    free(closure.buf);
    free(cpy);
    return -1;
  }
  sixel_dither_destroy(dither);
  sixel_output_destroy(output);
  free(cpy);
  unsigned cols = lenx / bargs->pixel.celldimx + !!(lenx % bargs->pixel.celldimx);
  unsigned rows = leny / bargs->pixel.celldimy + !!(leny % bargs->pixel.celldimx);
  if(closure.buf == NULL){
    return -1;
  }
  if(plane_blit_sixel(nc, closure.buf, closure.size, rows, cols, bargs->pixel.sprixelid) < 0){
    free(closure.buf);
    return -1;
  }
  free(closure.buf);
  return 1;
}

// sixel blitting implemented via libsixel. this gives superior output than our
// sixel blitter, with less overhead. it's better to use when available. if it
// fails, we fall back to our own.
int libsixel_blit(ncplane* nc, int linesize, const void* data,
                  int begy, int begx, int leny, int lenx,
                  const blitterargs* bargs){
  int r = libsixel_blit_inner(nc, linesize, data, begy, begx, leny, lenx, bargs);
  if(r < 0){
    r = sixel_blit(nc, linesize, data, begy, begx, leny, lenx, bargs);
  }
  return r;
}
#endif
