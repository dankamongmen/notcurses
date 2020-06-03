#include <cmath>
#include <cstring>
#include "version.h"
#include "visual-details.h"
#include "internal.h"

// Resize the provided ncviusal to the specified 'rows' x 'cols', but do not
// change the internals of the ncvisual. Uses oframe.
nc_err_e ncvisual_blit(struct ncvisual* ncv, int rows, int cols,
                       ncplane* n, const struct blitset* bset,
                       int placey, int placex, int begy, int begx,
                       int leny, int lenx, bool blendcolors);

// ncv constructors other than ncvisual_from_file() need to set up the
// AVFrame* 'frame' according to their own data, which is assumed to
// have been prepared already in 'ncv'.
auto ncvisual_details_seed(struct ncvisual* ncv) -> void;

// number of pixels that map to a single cell, height-wise
static inline auto
encoding_y_scale(const struct blitset* bset) -> int {
  return bset->height;
}

// number of pixels that map to a single cell, width-wise
static inline auto
encoding_x_scale(const struct blitset* bset) -> int {
  return bset->width;
}

static inline auto
ncvisual_default_blitter(const notcurses* nc) -> ncblitter_e {
  if(notcurses_canutf8(nc)){
    return NCBLIT_2x1;
  }
  return NCBLIT_1x1;
}

auto ncvisual_geom(const notcurses* nc, const ncvisual* n, ncblitter_e blitter,
                   int* y, int* x, int* toy, int* tox) -> int {
  if(blitter == NCBLIT_DEFAULT){
    blitter = ncvisual_default_blitter(nc);
  }
  const struct blitset* bset = lookup_blitset(nc, blitter, false);
  if(!bset){
    return -1;
  }
  if(y){
    *y = n->rows;
  }
  if(x){
    *x = n->cols;
  }
  if(toy){
    *toy = encoding_y_scale(bset);
  }
  if(tox){
    *tox = encoding_x_scale(bset);
  }
  return 0;
}

// RGBA visuals all use NCBLIT_2x1 by default (or NCBLIT_1x1 if not in
// UTF-8 mode), but an alternative can be specified.
static const struct blitset*
rgba_blitter(const notcurses* nc, const struct ncvisual_options* opts){
  const struct blitset* bset;
  const bool maydegrade = !opts || (opts->flags & NCVISUAL_OPTION_MAYDEGRADE);
  if(opts && opts->blitter != NCBLIT_DEFAULT){
    bset = lookup_blitset(nc, opts->blitter, maydegrade);
  }else{
    bset = lookup_blitset(nc, ncvisual_default_blitter(nc), maydegrade);
  }
  if(bset && !bset->blit){ // FIXME remove this once all blitters are enabled
    bset = nullptr;
  }
  return bset;
}

auto bgra_to_rgba(const void* data, int rows, int rowstride, int cols) -> void* {
  if(rowstride % 4){ // must be a multiple of 4 bytes
    return nullptr;
  }
//fprintf(stderr, "ROWS: %d\n", rows);
  auto ret = static_cast<uint32_t*>(malloc(rowstride * rows));
  if(ret){
    for(int y = 0 ; y < rows ; ++y){
      for(int x = 0 ; x < cols ; ++x){
        const uint32_t* src = (const uint32_t*)data + (rowstride / 4) * y + x;
        uint32_t* dst = ret + (rowstride / 4) * y + x;
//fprintf(stderr, "SRC: %x DST: %x\n", *src, *dst);
        *dst = ((*src & 0xff00u) << 16u) | (*src & 0xff00ffu) | ((*src & 0xff000000) >> 16u);
//fprintf(stderr, "y: %d x: %d SRC: %x DST: %x\n", y, x, *src, *dst);
      }
    }
  }
  return ret;
}

// Inspects the visual to find the minimum rectangle that can contain all
// "real" pixels, where "real" pixels are, by convention, all zeroes.
// Placing this box at offyXoffx relative to the visual will encompass all
// pixels. Returns the area of the box (0 if there are no pixels).
auto ncvisual_bounding_box(const ncvisual* ncv, int* leny, int* lenx,
                           int* offy, int* offx) -> int {
  int trow, lcol = -1, rcol = INT_MAX; // FIXME shouldn't need lcol init
  // first, find the topmost row with a real pixel. if there is no such row,
  // there are no such pixels. if we find one, we needn't look in this region
  // for other extrema, so long as we keep the leftmost and rightmost through
  // this row (from the top). said leftmost and rightmost will be the leftmost
  // and rightmost pixel of whichever row has the topmost valid pixel. unlike
  // the topmost, they'll need be further verified.
  for(trow = 0 ; trow < ncv->rows ; ++trow){
    int x;
    for(x = 0 ; x < ncv->cols ; ++x){
      uint32_t rgba = ncv->data[trow * ncv->rowstride / 4 + x];
      if(rgba){
        lcol = x; // leftmost pixel of topmost row
        // now find rightmost pixel of topmost row
        int xr;
        for(xr = ncv->cols - 1 ; xr > x ; --xr){
          rgba = ncv->data[trow * ncv->rowstride / 4 + xr];
          if(rgba){ // rightmost pixel of topmost row
            break;
          }
        }
        rcol = xr;
        break;
      }
    }
    if(rcol < INT_MAX){
      break;
    }
  }
  if(trow == ncv->rows){ // no real pixels
    *leny = 0;
    *lenx = 0;
    *offy = 0;
    *offx = 0;
  }else{
    assert(lcol >= 0);
    assert(rcol < ncv->cols);
    // we now know topmost row, and left/rightmost through said row. now we must
    // find the bottommost row, checking left/rightmost throughout.
    int brow;
    for(brow = ncv->rows - 1 ; brow > trow ; --brow){
      int x;
      for(x = 0 ; x < ncv->cols ; ++x){
        uint32_t rgba = ncv->data[brow * ncv->rowstride / 4 + x];
        if(rgba){
          if(x < lcol){
            lcol = x;
          }
          int xr;
          for(xr = ncv->cols - 1 ; xr > x && xr > rcol ; --xr){
            rgba = ncv->data[brow * ncv->rowstride / 4 + xr];
            if(rgba){ // rightmost pixel of bottommost row
              if(xr > rcol){
                rcol = xr;
              }
              break;
            }
          }
          break;
        }
      }
      if(x < ncv->cols){
        break;
      }
    }
    // we now know topmost and bottommost row, and left/rightmost within those
    // two sections. now check the rest for left and rightmost.
    for(int y = trow + 1 ; y < brow ; ++y){
      for(int x = 0 ; x < lcol ; ++x){
        uint32_t rgba = ncv->data[y * ncv->rowstride / 4 + x];
        if(rgba){
          lcol = x;
          break;
        }
      }
      for(int x = ncv->cols - 1 ; x > rcol ; --x){
        uint32_t rgba = ncv->data[y * ncv->rowstride / 4 + x];
        if(rgba){
          rcol = x;
          break;
        }
      }
    }
    *offy = trow;
    *leny = brow - trow + 1;
    *offx = lcol;
    *lenx = rcol - lcol + 1;
  }
  return *leny * *lenx;
}

// find the "center" cell of a visual. in the case of even rows/columns, we
// place the center on the top/left. in such a case there will be one more
// cell to the bottom/right of the center.
static inline void
ncvisual_center(const ncvisual* n, int* RESTRICT y, int* RESTRICT x){
  *y = n->rows;
  *x = n->cols;
  center_box(y, x);
}

// rotate the 0-indexed (origin-indexed) ['y', 'x'] through 'ctheta' and
// 'stheta' around the centerpoint at ['centy', 'centx']. write the results
// back to 'y' and 'x'.
static auto
rotate_point(int* y, int* x, double stheta, double ctheta, int centy, int centx) -> void {
  // convert coordinates from origin to left-handed cartesian
  const int convx = *x - centx;
  const int convy = *y - centy;
//fprintf(stderr, "%d, %d -> conv %d, %d\n", *y, *x, convy, convx);
  *x = round(convx * ctheta - convy * stheta);
  *y = round(convx * stheta + convy * ctheta);
}

// rotate the specified bounding box by the specified sine and cosine of some
// theta radians, enlarging or shrinking it as necessary. returns the area.
// 'leny', 'lenx', 'offy', and 'offx' describe the bounding box to be rotated,
// and might all be updated (in either direction).
static auto
rotate_bounding_box(double stheta, double ctheta, int* leny, int* lenx,
                    int* offy, int* offx) -> int {
//fprintf(stderr, "Incoming bounding box: %dx%d @ %dx%d rotate s(%f) c(%f)\n", *leny, *lenx, *offy, *offx, stheta, ctheta);
  int xs[4], ys[4]; // x and y locations of rotated coordinates
  int centy = *leny;
  int centx = *lenx;
  center_box(&centy, &centx);
  ys[0] = 0;
  xs[0] = 0;
  rotate_point(ys, xs, stheta, ctheta, centy, centx);
//fprintf(stderr, "rotated %d, %d -> %d %d\n", 0, 0, ys[0], xs[0]);
  ys[1] = 0;
  xs[1] = *lenx - 1;
  rotate_point(ys + 1, xs + 1, stheta, ctheta, centy, centx);
//fprintf(stderr, "rotated %d, %d -> %d %d\n", 0, *lenx - 1, ys[1], xs[1]);
  ys[2] = *leny - 1;
  xs[2] = *lenx - 1;
  rotate_point(ys + 2, xs + 2, stheta, ctheta, centy, centx);
//fprintf(stderr, "rotated %d, %d -> %d %d\n", *leny - 1, *lenx - 1, ys[2], xs[2]);
  ys[3] = *leny - 1;
  xs[3] = 0;
  rotate_point(ys + 3, xs + 3, stheta, ctheta, centy, centx);
//fprintf(stderr, "rotated %d, %d -> %d %d\n", *leny - 1, 0, ys[3], xs[3]);
  int trow = ys[0];
  int brow = ys[0];
  int lcol = xs[0];
  int rcol = xs[0];
  for(size_t i = 1 ; i < sizeof(xs) / sizeof(*xs) ; ++i){
    if(xs[i] < lcol){
      lcol = xs[i];
    }
    if(xs[i] > rcol){
      rcol = xs[i];
    }
    if(ys[i] < trow){
      trow = ys[i];
    }
    if(ys[i] > brow){
      brow = ys[i];
    }
  }
  *offy = trow;
  *leny = brow - trow + 1;
  *offx = lcol;
  *lenx = rcol - lcol + 1;
//fprintf(stderr, "Rotated bounding box: %dx%d @ %dx%d\n", *leny, *lenx, *offy, *offx);
  return *leny * *lenx;
}

auto ncvisual_rotate(ncvisual* ncv, double rads) -> nc_err_e {
  nc_err_e err = ncvisual_resize(ncv, ncv->rows, ncv->cols);
  if(err != NCERR_SUCCESS){
    return err;
  }
  assert(ncv->rowstride / 4 >= ncv->cols);
  rads = -rads; // we're a left-handed Cartesian
  if(ncv->data == nullptr){
    return NCERR_DECODE;
  }
  int centy, centx;
  ncvisual_center(ncv, &centy, &centx); // pixel center (center of 'data')
  double stheta, ctheta; // sine, cosine
  stheta = sin(rads);
  ctheta = cos(rads);
  // bounding box for real data within the ncvisual. we must only resize to
  // accommodate real data, lest we grow without band as we rotate.
  // see https://github.com/dankamongmen/notcurses/issues/599.
  int bby = ncv->rows;
  int bbx = ncv->cols;
  int bboffy = 0;
  int bboffx = 0;
  if(ncvisual_bounding_box(ncv, &bby, &bbx, &bboffy, &bboffx) <= 0){
    return NCERR_DECODE;
  }
  int bbarea;
  bbarea = rotate_bounding_box(stheta, ctheta, &bby, &bbx, &bboffy, &bboffx);
  if(bbarea <= 0){
    return NCERR_DECODE;
  }
  int bbcentx = bbx, bbcenty = bby;
  center_box(&bbcenty, &bbcentx);
//fprintf(stderr, "stride: %d height: %d width: %d\n", ncv->rowstride, ncv->rows, ncv->cols);
  assert(ncv->rowstride / 4 >= ncv->cols);
  auto data = static_cast<uint32_t*>(malloc(bbarea * 4));
  if(data == nullptr){
    return NCERR_NOMEM;
  }
  memset(data, 0, bbarea * 4);
//fprintf(stderr, "bbarea: %d bby: %d bbx: %d centy: %d centx: %d bbcenty: %d bbcentx: %d\n", bbarea, bby, bbx, centy, centx, bbcenty, bbcentx);
  for(int y = 0 ; y < ncv->rows ; ++y){
      for(int x = 0 ; x < ncv->cols ; ++x){
      int targx = x, targy = y;
      rotate_point(&targy, &targx, stheta, ctheta, centy, centx);
      const int deconvx = targx - bboffx;
      const int deconvy = targy - bboffy;
      if(deconvy >= 0 && deconvx >= 0 && deconvy < bby && deconvx < bbx){
        data[deconvy * bbx + deconvx] = ncv->data[y * (ncv->rowstride / 4) + x];
      }
 //     data[deconvy * (ncv->cols) + deconvx] = ncv->data[y * (ncv->rowstride / 4) + x];
//fprintf(stderr, "CW: %d/%d (%08x) -> %d/%d (stride: %d)\n", y, x, ncv->data[y * (ncv->rowstride / 4) + x], targy, targx, ncv->rowstride);
//fprintf(stderr, "wrote %08x to %d (%d)\n", data[targy * ncv->rows + targx], targy * ncv->rows + targx, (targy * ncv->rows + targx) * 4);
    }
  }
  ncvisual_set_data(ncv, data, true);
  ncv->cols = bbx;
  ncv->rows = bby;
  ncv->rowstride = bbx * 4;
  //ncplane_erase(ncv->ncp);
  return NCERR_SUCCESS;
}

auto ncvisual_from_rgba(const void* rgba, int rows, int rowstride,
                        int cols) -> ncvisual* {
  if(rowstride % 4){
    return nullptr;
  }
  ncvisual* ncv = ncvisual_create();
  if(ncv){
    ncv->rowstride = rowstride;
    ncv->cols = cols;
    ncv->rows = rows;
//fprintf(stderr, "MADE INITIAL ONE %d/%d\n", disprows, ncv->cols);
    auto data = static_cast<uint32_t*>(memdup(rgba, rowstride * ncv->rows));
    if(data == nullptr){
      ncvisual_destroy(ncv);
      return nullptr;
    }
//fprintf(stderr, "ROWS: %d STRIDE: %d (%d) COLS: %d\n", rows, rowstride, rowstride / 4, cols);
    ncvisual_set_data(ncv, data, true);
    ncvisual_details_seed(ncv);
  }
  return ncv;
}

auto ncvisual_from_bgra(const void* bgra, int rows, int rowstride,
                        int cols) -> ncvisual* {
  if(rowstride % 4){
    return nullptr;
  }
  ncvisual* ncv = ncvisual_create();
  if(ncv){
    ncv->rowstride = rowstride;
    ncv->cols = cols;
    ncv->rows = rows;
    auto data = static_cast<uint32_t*>(memdup(bgra, rowstride * ncv->rows));
    if(data == nullptr){
      ncvisual_destroy(ncv);
      return nullptr;
    }
    ncvisual_set_data(ncv, data, true);
    ncvisual_details_seed(ncv);
  }
  return ncv;
}

auto ncvisual_render(notcurses* nc, ncvisual* ncv,
                     const struct ncvisual_options* vopts) -> ncplane* {
  if(vopts && vopts->flags > NCVISUAL_OPTION_BLEND){
    return nullptr;
  }
  int lenx = vopts ? vopts->lenx : 0;
  int leny = vopts ? vopts->leny : 0;
  int begy = vopts ? vopts->begy : 0;
  int begx = vopts ? vopts->begx : 0;
//fprintf(stderr, "render %dx%d+%dx%d %p\n", begy, begx, leny, lenx, ncv->data);
  if(begy < 0 || begx < 0 || lenx < -1 || leny < -1){
    return nullptr;
  }
//fprintf(stderr, "OUR DATA: %p cols/rows: %d/%d\n", ncv->data, ncv->cols, ncv->rows);
  if(ncv->data == nullptr){
    return nullptr;
  }
//fprintf(stderr, "render %d/%d to %dx%d+%dx%d scaling: %d\n", ncv->rows, ncv->cols, begy, begx, leny, lenx, vopts ? vopts->scaling : 0);
  if(begx >= ncv->cols || begy >= ncv->rows){
    return nullptr;
  }
  if(lenx == 0){ // 0 means "to the end"; use all space available
    lenx = ncv->cols - begx;
  }
  if(leny == 0){
    leny = ncv->rows - begy;
  }
//fprintf(stderr, "render %d/%d to %dx%d+%dx%d scaling: %d\n", ncv->rows, ncv->cols, begy, begx, leny, lenx, vopts ? vopts->scaling : 0);
  if(lenx <= 0 || leny <= 0){ // no need to draw zero-size object, exit
    return nullptr;
  }
  if(begx + lenx > ncv->cols || begy + leny > ncv->rows){
    return nullptr;
  }
  auto bset = rgba_blitter(nc, vopts);
  if(!bset){
    return nullptr;
  }
//fprintf(stderr, "beg/len: %d %d %d %d scale: %d/%d\n", begy, leny, begx, lenx, encoding_y_scale(bset), encoding_x_scale(bset));
  int placey = vopts ? vopts->y : 0;
  int placex = vopts ? vopts->x : 0;
  int disprows, dispcols;
  ncplane* n = nullptr;
//fprintf(stderr, "INPUT N: %p\n", vopts ? vopts->n : nullptr);
  if((n = (vopts ? vopts->n : nullptr)) == nullptr){ // create plane
    if(!vopts || vopts->scaling == NCSCALE_NONE){
      dispcols = ncv->cols / encoding_x_scale(bset) + ncv->cols % encoding_x_scale(bset);
      disprows = ncv->rows / encoding_y_scale(bset) + ncv->rows % encoding_y_scale(bset);
    }else if(vopts->scaling == NCSCALE_SCALE){
      notcurses_term_dim_yx(nc, &disprows, &dispcols);
      // FIXME kill FP?
      double tmpratio = dispcols * encoding_x_scale(bset) / (double)ncv->cols;
      if(tmpratio * ncv->rows > disprows * encoding_y_scale(bset)){
        tmpratio = disprows * encoding_y_scale(bset) / (double)ncv->rows;
        assert(tmpratio <= 1);
        dispcols *= tmpratio;
      }else{
        assert(tmpratio <= 1);
        disprows *= tmpratio;
      }
    }else if(vopts->scaling == NCSCALE_STRETCH){
      notcurses_term_dim_yx(nc, &disprows, &dispcols);
    }else{
      return nullptr;
    }
    n = ncplane_new(nc, disprows, dispcols, placey, placex, nullptr);
    if(n == nullptr){
      return nullptr;
    }
    placey = 0;
    placex = 0;
  }else{
    if(!vopts || vopts->scaling == NCSCALE_NONE){
      dispcols = ncv->cols / encoding_x_scale(bset) + ncv->cols % encoding_x_scale(bset);
      disprows = ncv->rows / encoding_y_scale(bset) + ncv->rows % encoding_y_scale(bset);
    }else{ // FIXME handle SCALE
      ncplane_dim_yx(n, &disprows, &dispcols);
      disprows -= placey;
      dispcols -= placex;
    }
  }
  leny = (leny / (double)ncv->rows) * ((double)disprows * encoding_y_scale(bset));
  lenx = (lenx / (double)ncv->cols) * ((double)dispcols * encoding_x_scale(bset));
//fprintf(stderr, "render: %dx%d:%d+%d of %d/%d stride %u %p\n", begy, begx, leny, lenx, ncv->rows, ncv->cols, ncv->rowstride, ncv->data);
  if(ncvisual_blit(ncv, disprows * encoding_y_scale(bset),
                   dispcols * encoding_x_scale(bset), n, bset,
                   placey, placex, begy, begx, leny, lenx,
                   vopts && (vopts->flags & NCVISUAL_OPTION_BLEND))){
    ncplane_destroy(n);
    return nullptr;
  }
  return n;
}

auto ncvisual_from_plane(const ncplane* n, int begy, int begx,
                         int leny, int lenx) -> ncvisual* {
  uint32_t* rgba = ncplane_rgba(n, begx, begy, leny, lenx);
  if(rgba == nullptr){
    return nullptr;
  }
  int dimy, dimx;
  ncplane_dim_yx(n, &dimy, &dimx);
  if(lenx == -1){
    lenx = n->lenx - begx;
  }
  if(leny == -1){
    leny = n->leny - begy;
  }
  auto* ncv = ncvisual_from_rgba(rgba, leny, lenx * 4, lenx);
  free(rgba);
  if(ncv == nullptr){
    return nullptr;
  }
  return ncv;
}

auto ncvisual_destroy(ncvisual* ncv) -> void {
  if(ncv){
    ncvisual_details_destroy(&ncv->details);
    if(ncv->owndata){
      free(ncv->data);
    }
    delete ncv;
  }
}

auto ncvisual_simple_streamer(ncplane* n, ncvisual* ncv, const timespec* tspec,
                              void* curry) -> int {
  if(notcurses_render(ncplane_notcurses(n))){
    return -1;
  }
  int ret = 0;
  if(curry){
    // need a cast for C++ callers
    ncplane* subncp = static_cast<ncplane*>(curry);
    char* subtitle = ncvisual_subtitle(ncv);
    if(subtitle){
      if(ncplane_putstr_yx(subncp, 0, 0, subtitle) < 0){
        ret = -1;
      }
      free(subtitle);
    }
  }
  clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, tspec, NULL);
  return ret;
}

#ifndef USE_OIIO // built without ffmpeg or oiio
#ifndef USE_FFMPEG
auto ncvisual_from_file(const char* filename, nc_err_e* err) -> ncvisual* {
  (void)filename;
  *err = NCERR_UNIMPLEMENTED;
  return nullptr;
}

auto notcurses_canopen_images(const notcurses* nc __attribute__ ((unused))) -> bool {
  return false;
}

auto notcurses_canopen_videos(const notcurses* nc __attribute__ ((unused))) -> bool {
  return false;
}

auto ncvisual_decode(ncvisual* nc) -> nc_err_e {
  (void)nc;
  return NCERR_UNIMPLEMENTED;
}

auto ncvisual_stream(notcurses* nc, ncvisual* ncv, nc_err_e* ncerr,
                    float timescale, streamcb streamer,
                    const ncvisual_options* vopts, void* curry) -> int {
  (void)nc;
  (void)ncv;
  (void)timescale;
  (void)streamer;
  (void)vopts;
  (void)curry;
  *ncerr = NCERR_UNIMPLEMENTED;
  return -1;
}

auto ncvisual_subtitle(const ncvisual* ncv) -> char* {
  (void)ncv;
  return nullptr;
}

auto ncvisual_init(int loglevel) -> int {
  (void)loglevel;
  return 0; // allow success here
}

auto ncvisual_blit(ncvisual* ncv, int rows, int cols, ncplane* n,
                   const struct blitset* bset, int placey, int placex,
                   int begy, int begx, int leny, int lenx,
                   bool blendcolors) -> nc_err_e {
  (void)rows;
  (void)cols;
  if(rgba_blit_dispatch(n, bset, placey, placex, ncv->rowstride, ncv->data,
                        begy, begx, leny, lenx, blendcolors) <= 0){
    return NCERR_DECODE;
  }
  return NCERR_SUCCESS;
}

auto ncvisual_details_seed(struct ncvisual* ncv) -> void {
  (void)ncv;
}

auto ncvisual_resize(ncvisual* nc, int rows, int cols) -> nc_err_e {
  // we'd need to verify that it's RGBA as well, except that if we've got no
  // multimedia engine, we've only got memory-assembled ncvisuals, which are
  // RGBA-native. so we ought be good, but this is undeniably sloppy...
  if(nc->rows == rows && nc->cols == cols){
    return NCERR_SUCCESS;
  }
  return NCERR_UNIMPLEMENTED;
}

#endif
#endif
