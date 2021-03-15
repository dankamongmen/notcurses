#include <cmath>
#include <cstring>
#include "builddef.h"
#include "visual-details.h"
#include "internal.h"

const ncvisual_implementation* visual_implementation = nullptr;

auto ncvisual_decode(ncvisual* nc) -> int {
  if(!visual_implementation){
    return -1;
  }
  return visual_implementation->visual_decode(nc);
}

auto ncvisual_blit(ncvisual* ncv, int rows, int cols, ncplane* n,
                   const struct blitset* bset, int begy, int begx,
                   int leny, int lenx, const blitterargs* barg) -> int {
  int ret = -1;
  if(visual_implementation){
    if(visual_implementation->visual_blit(ncv, rows, cols, n, bset,
                                          begy, begx, leny, lenx, barg) >= 0){
      ret = 0;
    }
  }else{
    if(rgba_blit_dispatch(n, bset, ncv->rowstride, ncv->data,
                          begy, begx, leny, lenx, barg) >= 0){
      ret = 0;
    }
  }
  return ret;
}

// ncv constructors other than ncvisual_from_file() need to set up the
// AVFrame* 'frame' according to their own data, which is assumed to
// have been prepared already in 'ncv'.
auto ncvisual_details_seed(struct ncvisual* ncv) -> void {
  if(visual_implementation){
    visual_implementation->visual_details_seed(ncv);
  }
}

auto ncvisual_init(int loglevel) -> int {
  if(visual_implementation){
    return visual_implementation->visual_init(loglevel);
  }
  return 0;
}

auto ncvisual_from_file(const char* filename) -> ncvisual* {
  if(!visual_implementation){
    return nullptr;
  }
  return visual_implementation->visual_from_file(filename);
}

auto ncvisual_create(void) -> ncvisual* {
  if(visual_implementation){
    return visual_implementation->visual_create();
  }
  return new ncvisual{};
}

auto ncvisual_printbanner(const notcurses* nc) -> void {
  if(visual_implementation){
    visual_implementation->visual_printbanner(nc);
  }
}

auto ncvisual_geom(const notcurses* nc, const ncvisual* n,
                   const struct ncvisual_options* vopts,
                   int* y, int* x, int* toy, int* tox) -> int {
  const ncscale_e scale = vopts ? vopts->scaling : NCSCALE_NONE;
  ncblitter_e blitfxn;
  if(!vopts || vopts->blitter == NCBLIT_DEFAULT){
    blitfxn = ncvisual_media_defblitter(nc, scale);
  }else{
    blitfxn = vopts->blitter;
  }
  const bool maydegrade = !(vopts && (vopts->flags & NCVISUAL_OPTION_NODEGRADE));
  const struct blitset* bset = lookup_blitset(&nc->tcache, blitfxn, maydegrade);
  if(!bset){
    return -1;
  }
  int fauxy, fauxx;
  if(!y){
    y = &fauxy;
  }
  if(!x){
    x = &fauxx;
  }
  if(n){
    if(scale == NCSCALE_NONE || scale == NCSCALE_NONE_HIRES){
      *y = n->rows;
      *x = n->cols;
    }else{
      int rows = vopts->n ? ncplane_dim_y(vopts->n) : ncplane_dim_y(nc->stdplane);
      int cols = vopts->n ? ncplane_dim_x(vopts->n) : ncplane_dim_x(nc->stdplane);
      *y = rows * encoding_y_scale(&nc->tcache, bset);
      *x = cols * encoding_x_scale(&nc->tcache, bset);
    }
    if(scale == NCSCALE_SCALE || scale == NCSCALE_SCALE_HIRES){
      scale_visual(n, y, x);
    }
  }
  if(toy){
    *toy = encoding_y_scale(&nc->tcache, bset);
  }
  if(tox){
    *tox = encoding_x_scale(&nc->tcache, bset);
  }
  return 0;
}

auto bgra_to_rgba(const void* data, int rows, int rowstride, int cols) -> void* {
  if(rowstride % 4){ // must be a multiple of 4 bytes
    return nullptr;
  }
  auto ret = static_cast<uint32_t*>(malloc(rowstride * rows));
  if(ret){
    for(int y = 0 ; y < rows ; ++y){
      for(int x = 0 ; x < cols ; ++x){
        const uint32_t* src = (const uint32_t*)data + (rowstride / 4) * y + x;
        uint32_t* dst = ret + (rowstride / 4) * y + x;
        ncpixel_set_a(dst, 0xff);
        ncpixel_set_r(dst, ncpixel_b(*src));
        ncpixel_set_g(dst, ncpixel_g(*src));
        ncpixel_set_b(dst, ncpixel_r(*src));
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

auto ncvisual_rotate(ncvisual* ncv, double rads) -> int {
  // done to force conversion into RGBA
  int err = ncvisual_resize(ncv, ncv->rows, ncv->cols);
  if(err){
    return err;
  }
  assert(ncv->rowstride / 4 >= ncv->cols);
  rads = -rads; // we're a left-handed Cartesian
  if(ncv->data == nullptr){
    return -1;
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
    return -1;
  }
  int bbarea;
  bbarea = rotate_bounding_box(stheta, ctheta, &bby, &bbx, &bboffy, &bboffx);
  if(bbarea <= 0){
    return -1;
  }
  int bbcentx = bbx, bbcenty = bby;
  center_box(&bbcenty, &bbcentx);
//fprintf(stderr, "stride: %d height: %d width: %d\n", ncv->rowstride, ncv->rows, ncv->cols);
  assert(ncv->rowstride / 4 >= ncv->cols);
  auto data = static_cast<uint32_t*>(malloc(bbarea * 4));
  if(data == nullptr){
    return -1;
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
  ncvisual_details_seed(ncv);
  return 0;
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
    auto data = static_cast<uint32_t*>(memdup(rgba, rowstride * ncv->rows));
//fprintf(stderr, "COPY US %zu (%d)\n", rowstride * ncv->rows, ncv->rows);
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
    for(int p = 0 ; p < rowstride / 4 * ncv->rows ; ++p){
      const unsigned r = (data[p] & 0xffllu) << 16u;
      const unsigned b = (data[p] & 0xff0000llu) >> 16u;
      data[p] = (data[p] & 0xff00ff00llu) | r | b;
    }
    if(data == nullptr){
      ncvisual_destroy(ncv);
      return nullptr;
    }
    ncvisual_set_data(ncv, data, true);
    ncvisual_details_seed(ncv);
  }
  return ncv;
}

auto ncvisual_render_cells(notcurses* nc, ncvisual* ncv, const blitset* bset,
                           int placey, int placex, int begy, int begx,
                           int leny, int lenx, ncplane* n, ncscale_e scaling,
                           bool blendcolors) -> ncplane* {
  int disprows, dispcols;
//fprintf(stderr, "INPUT N: %p\n", vopts ? vopts->n : nullptr);
  if(n == nullptr){ // create plane
    if(scaling == NCSCALE_NONE || scaling == NCSCALE_NONE_HIRES){
      dispcols = ncv->cols;
      disprows = ncv->rows;
    }else{
      notcurses_term_dim_yx(nc, &disprows, &dispcols);
      dispcols *= encoding_x_scale(&nc->tcache, bset);
      disprows *= encoding_y_scale(&nc->tcache, bset);
      if(scaling == NCSCALE_SCALE || scaling == NCSCALE_SCALE_HIRES){
        scale_visual(ncv, &disprows, &dispcols);
      } // else stretch
    }
//fprintf(stderr, "PLACING NEW PLANE: %d/%d @ %d/%d\n", disprows, dispcols, placey, placex);
    struct ncplane_options nopts = {
      .y = placey,
      .x = placex,
      .rows = disprows / encoding_y_scale(&nc->tcache, bset),
      .cols = dispcols / encoding_x_scale(&nc->tcache, bset),
      .userptr = nullptr,
      .name = "rgba",
      .resizecb = nullptr,
      .flags = 0,
    };
    if((n = ncplane_create(notcurses_stdplane(nc), &nopts)) == nullptr){
      return nullptr;
    }
    placey = 0;
    placex = 0;
  }else{
    if(scaling == NCSCALE_NONE || scaling == NCSCALE_NONE_HIRES){
      dispcols = ncv->cols;
      disprows = ncv->rows;
    }else{
      ncplane_dim_yx(n, &disprows, &dispcols);
      dispcols *= encoding_x_scale(&nc->tcache, bset);
      disprows *= encoding_y_scale(&nc->tcache, bset);
      disprows -= placey;
      dispcols -= placex;
      if(scaling == NCSCALE_SCALE || scaling == NCSCALE_SCALE_HIRES){
        scale_visual(ncv, &disprows, &dispcols);
      } // else stretch
    }
  }
  leny = (leny / (double)ncv->rows) * ((double)disprows);
  lenx = (lenx / (double)ncv->cols) * ((double)dispcols);
//fprintf(stderr, "blit: %dx%d:%d+%d of %d/%d stride %u %p\n", begy, begx, leny, lenx, ncv->rows, ncv->cols, ncv->rowstride, ncv->data);
  blitterargs bargs;
  bargs.cell.placey = placey;
  bargs.cell.placex = placex;
  bargs.cell.blendcolors = blendcolors;
  if(ncvisual_blit(ncv, disprows, dispcols, n, bset, begy, begx, leny, lenx, &bargs)){
    ncplane_destroy(n);
    return nullptr;
  }
  return n;
}

auto ncvisual_render_pixels(tinfo* tcache, ncvisual* ncv, const blitset* bset,
                            int placey, int placex, int begy, int begx,
                            ncplane* n, ncscale_e scaling, ncplane* stdn) -> ncplane* {
  int disprows, dispcols;
  if(scaling == NCSCALE_NONE || scaling == NCSCALE_NONE_HIRES){
    dispcols = ncv->cols;
    disprows = ncv->rows;
  }
//fprintf(stderr, "INPUT N: %p\n", vopts ? vopts->n : nullptr);
  if(n == nullptr){ // create plane
    if(scaling != NCSCALE_NONE && scaling != NCSCALE_NONE_HIRES){
      ncplane_dim_yx(stdn, &disprows, &dispcols);
      dispcols *= tcache->cellpixx;
      disprows *= tcache->cellpixy;
    }
//fprintf(stderr, "PLACING NEW PLANE: %d/%d @ %d/%d\n", disprows, dispcols, placey, placex);
    struct ncplane_options nopts = {
      .y = placey,
      .x = placex,
      .rows = disprows / tcache->cellpixy,
      .cols = dispcols / tcache->cellpixx,
      .userptr = nullptr,
      .name = "rgba",
      .resizecb = nullptr,
      .flags = 0,
    };
    if((n = ncplane_create(stdn, &nopts)) == nullptr){
      return nullptr;
    }
    placey = 0;
    placex = 0;
  }else{
    if(scaling != NCSCALE_NONE && scaling != NCSCALE_NONE_HIRES){
      ncplane_dim_yx(n, &disprows, &dispcols);
      dispcols *= tcache->cellpixx;
      disprows *= tcache->cellpixy;
      dispcols -= (placex * tcache->cellpixx + 1);
      disprows -= (placey * tcache->cellpixy + 1);
    }
  }
  if(scaling == NCSCALE_SCALE || scaling == NCSCALE_SCALE_HIRES){
    scale_visual(ncv, &disprows, &dispcols);
  }
//fprintf(stderr, "pblit: %dx%d <- %dx%d of %d/%d stride %u @%dx%d %p %u\n", disprows, dispcols, begy, begx, ncv->rows, ncv->cols, ncv->rowstride, placey, placex, ncv->data, ncplane_notcurses(stdn)->tcache.cellpixx);
  blitterargs bargs;
  bargs.pixel.celldimx = ncplane_notcurses(stdn)->tcache.cellpixx;
  bargs.pixel.celldimy = ncplane_notcurses(stdn)->tcache.cellpixy;
  bargs.pixel.colorregs = ncplane_notcurses(stdn)->tcache.color_registers;
  if(ncvisual_blit(ncv, disprows, dispcols, n, bset,
                   begy, begx, disprows, dispcols, &bargs)){
    ncplane_destroy(n);
    return nullptr;
  }
  return n;
}

auto ncvisual_render(notcurses* nc, ncvisual* ncv,
                     const struct ncvisual_options* vopts) -> ncplane* {
  if(vopts && vopts->flags > NCVISUAL_OPTION_BLEND){
    logwarn(nc, "Warning: unknown ncvisual options %016jx\n", (uintmax_t)vopts->flags);
  }
  int lenx = vopts ? vopts->lenx : 0;
  int leny = vopts ? vopts->leny : 0;
  int begy = vopts ? vopts->begy : 0;
  int begx = vopts ? vopts->begx : 0;
//fprintf(stderr, "blit %dx%d+%dx%d %p\n", begy, begx, leny, lenx, ncv->data);
  if(begy < 0 || begx < 0 || lenx < -1 || leny < -1){
    return nullptr;
  }
//fprintf(stderr, "OUR DATA: %p rows/cols: %d/%d\n", ncv->data, ncv->rows, ncv->cols);
  if(ncv->data == nullptr){
    return nullptr;
  }
//fprintf(stderr, "blit %d/%d to %dx%d+%dx%d scaling: %d\n", ncv->rows, ncv->cols, begy, begx, leny, lenx, vopts ? vopts->scaling : 0);
  if(begx >= ncv->cols || begy >= ncv->rows){
    return nullptr;
  }
  if(lenx == 0){ // 0 means "to the end"; use all available source material
    lenx = ncv->cols - begx;
  }
  if(leny == 0){
    leny = ncv->rows - begy;
  }
//fprintf(stderr, "blit %d/%d to %dx%d+%dx%d scaling: %d\n", ncv->rows, ncv->cols, begy, begx, leny, lenx, vopts ? vopts->scaling : 0);
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
  ncplane* n = (vopts ? vopts->n : nullptr);
  ncscale_e scaling = vopts ? vopts->scaling : NCSCALE_NONE;
  if(bset->geom != NCBLIT_PIXEL){
    n = ncvisual_render_cells(nc, ncv, bset, placey, placex, begy, begx, leny, lenx,
                              n, scaling,
                              vopts && (vopts->flags & NCVISUAL_OPTION_BLEND));
  }else{
    n = ncvisual_render_pixels(&nc->tcache, ncv, bset, placey, placex, begy, begx,
                               n, scaling, notcurses_stdplane(nc));
  }
  return n;
}

auto ncvisual_from_plane(const ncplane* n, ncblitter_e blit, int begy, int begx,
                         int leny, int lenx) -> ncvisual* {
  uint32_t* rgba = ncplane_rgba(n, blit, begy, begx, leny, lenx);
//fprintf(stderr, "snarg: %d/%d @ %d/%d (%p)\n", leny, lenx, begy, begx, rgba);
//fprintf(stderr, "RGBA %p\n", rgba);
  if(rgba == nullptr){
    return nullptr;
  }
  int dimy, dimx;
  ncplane_dim_yx(n, &dimy, &dimx);
  if(lenx == -1){
    lenx = n->lenx - begx;
  }
  if(leny == -1){
    leny = (n->leny - begy);
  }
  auto* ncv = ncvisual_from_rgba(rgba, leny * 2, lenx * 4, lenx);
  free(rgba);
//fprintf(stderr, "RETURNING %p\n", ncv);
  return ncv;
}

auto ncvisual_details_destroy(ncvisual_details* deets){
  if(visual_implementation){
    visual_implementation->visual_details_destroy(deets);
  }
}

auto ncvisual_destroy(ncvisual* ncv) -> void {
  if(ncv){
    ncvisual_details_destroy(ncv->details);
    if(ncv->owndata){
      free(ncv->data);
    }
    delete ncv;
  }
}

auto ncvisual_simple_streamer(ncvisual* ncv, struct ncvisual_options* vopts,
                              const timespec* tspec, void* curry) -> int {
  if(notcurses_render(ncplane_notcurses(vopts->n))){
    return -1;
  }
  int ret = 0;
  if(curry){
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

auto ncvisual_set_yx(const struct ncvisual* n, int y, int x, uint32_t pixel) -> int {
  if(y >= n->rows || y < 0){
    return -1;
  }
  if(x >= n->cols || x < 0){
    return -1;
  }
  n->data[y * (n->rowstride / 4) + x] = pixel;
  return 0;
}

auto ncvisual_at_yx(const ncvisual* n, int y, int x, uint32_t* pixel) -> int {
  if(y >= n->rows || y < 0){
    return -1;
  }
  if(x >= n->cols || x < 0){
    return -1;
  }
  *pixel = n->data[y * (n->rowstride / 4) + x];
  return 0;
}

auto ncvisual_polyfill_recurse(ncvisual* n, int y, int x,
                               uint32_t rgba, uint32_t match) -> int {
  if(y < 0 || y >= n->rows){
    return 0;
  }
  if(x < 0 || x >= n->cols){
    return 0;
  }
  uint32_t* pixel = &n->data[y * (n->rowstride / 4) + x];
  if(*pixel != match || *pixel == rgba){
    return 0;
  }
// fprintf(stderr, "%d/%d: setting %08x to %08x\n", y, x, *pixel, rgba);
  *pixel = rgba;
  int ret = 1;
  ret += ncvisual_polyfill_recurse(n, y - 1, x, rgba, match);
  ret += ncvisual_polyfill_recurse(n, y + 1, x, rgba, match);
  ret += ncvisual_polyfill_recurse(n, y, x - 1, rgba, match);
  ret += ncvisual_polyfill_recurse(n, y, x + 1, rgba, match);
  return ret;
}

auto ncvisual_polyfill_yx(ncvisual* n, int y, int x, uint32_t rgba) -> int {
  if(y >= n->rows || y < 0){
    return -1;
  }
  if(x >= n->cols || x < 0){
    return -1;
  }
  uint32_t* pixel = &n->data[y * (n->rowstride / 4) + x];
  return ncvisual_polyfill_recurse(n, y, x, rgba, *pixel);
}

auto notcurses_canopen_images(const notcurses* nc __attribute__ ((unused))) -> bool {
  if(!visual_implementation){
    return false;
  }
  return visual_implementation->canopen_images;
}

auto notcurses_canopen_videos(const notcurses* nc __attribute__ ((unused))) -> bool {
  if(!visual_implementation){
    return false;
  }
  return visual_implementation->canopen_videos;
}

auto ncvisual_decode_loop(ncvisual* nc) -> int {
  if(!visual_implementation){
    return -1;
  }
  return visual_implementation->visual_decode_loop(nc);
}

auto ncvisual_stream(notcurses* nc, ncvisual* ncv, float timescale,
                     streamcb streamer, const ncvisual_options* vopts,
                     void* curry) -> int {
  if(!visual_implementation){
    return -1;
  }
  return visual_implementation->visual_stream(nc, ncv, timescale, streamer, vopts, curry);
}

auto ncvisual_subtitle(const ncvisual* ncv) -> char* {
  if(!visual_implementation){
    return nullptr;
  }
  return visual_implementation->visual_subtitle(ncv);
}

auto ncvisual_resize(ncvisual* nc, int rows, int cols) -> int {
  if(!visual_implementation){
    return -1;
  }
  return visual_implementation->visual_resize(nc, rows, cols);
}
