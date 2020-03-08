#include "internal.h"

void ncplane_greyscale(ncplane *n){
  for(int y = 0 ; y < n->leny ; ++y){
    for(int x = 0 ; x < n->lenx ; ++x){
      cell* c = &n->fb[nfbcellidx(n, y, x)];
      unsigned r, g, b;
      cell_fg_rgb(c, &r, &g, &b);
      int gy = rgb_greyscale(r, g, b);
      cell_set_fg_rgb(c, gy, gy, gy);
      cell_bg_rgb(c, &r, &g, &b);
      gy = rgb_greyscale(r, g, b);
      cell_set_bg_rgb(c, gy, gy, gy);
    }
  }
}

// if this is not polyfillable cell, we return 0. if it is, we attempt to fill
// it, then recurse out. return -1 on error, or number of cells filled on
// success. so a return of 0 means there's no work to be done here, and N means
// we did some work here, filling everything we could reach. out-of-plane is 0.
static int
ncplane_polyfill_recurse(ncplane* n, int y, int x, const cell* c){
  if(y >= n->leny || x >= n->lenx){
    return 0; // not fillable
  }
  if(y < 0 || x < 0){
    return 0; // not fillable
  }
  cell* cur = &n->fb[nfbcellidx(n, y, x)];
  if(cur->gcluster){
    return 0; // glyph, not polyfillable
  }
  if(cell_duplicate(n, cur, c) < 0){
    return -1;
  }
  int r, ret = 1;
  if((r = ncplane_polyfill_recurse(n, y - 1, x, c)) < 0){
    return -1;
  }
  ret += r;
  if((r = ncplane_polyfill_recurse(n, y + 1, x, c)) < 0){
    return -1;
  }
  ret += r;
  if((r = ncplane_polyfill_recurse(n, y, x - 1, c)) < 0){
    return -1;
  }
  ret += r;
  if((r = ncplane_polyfill_recurse(n, y, x + 1, c)) < 0){
    return -1;
  }
  ret += r;
  return ret;
}

// at the initial step only, invalid y, x is an error, so explicitly check.
int ncplane_polyfill_yx(ncplane* n, int y, int x, const cell* c){
  int ret = -1;
  if(y < n->leny && x < n->lenx){
    if(y >= 0 && x >= 0){
      ret = ncplane_polyfill_recurse(n, y, x, c);
    }
  }
  return ret;
}

// Our gradient is a 2d lerp among the four corners of the region. We start
// with the observation that each corner ought be its exact specified corner,
// and the middle ought be the exact average of all four corners' components.
// Another observation is that if all four corners are the same, every cell
// ought be the exact same color. From this arises the observation that a
// perimeter element is not affected by the other three sides:
//
//  a corner element is defined by itself
//  a perimeter element is defined by the two points on its side
//  an internal element is defined by all four points
//
// 2D equation of state: solve for each quadrant's contribution (min 2x2):
//
//  X' = (xlen - 1) - X
//  Y' = (ylen - 1) - Y
//  TLC: X' * Y' * TL
//  TRC: X * Y' * TR
//  BLC: X' * Y * BL
//  BRC: X * Y * BR
//  steps: (xlen - 1) * (ylen - 1) [maximum steps away from origin]
//
// Then add TLC + TRC + BLC + BRC + steps / 2, and divide by steps (the
//  steps / 2 is to work around truncate-towards-zero).
static int
calc_gradient_component(unsigned tl, unsigned tr, unsigned bl, unsigned br,
                        int y, int x, int ylen, int xlen){
  assert(y >= 0);
  assert(y < ylen);
  assert(x >= 0);
  assert(x < xlen);
  const int avm = (ylen - 1) - y;
  const int ahm = (xlen - 1) - x;
  if(xlen < 2){
    if(ylen < 2){
      return tl;
    }
    return (tl * avm + bl * y) / (ylen - 1);
  }
  if(ylen < 2){
    return (tl * ahm + tr * x) / (xlen - 1);
  }
  const int tlc = ahm * avm * tl;
  const int blc = ahm * y * bl;
  const int trc = x * avm * tr;
  const int brc = y * x * br;
  const int divisor = (ylen - 1) * (xlen - 1);
  return ((tlc + blc + trc + brc) + divisor / 2) / divisor;
}

// calculate one of the channels of a gradient at a particular point.
static inline uint32_t
calc_gradient_channel(uint32_t ul, uint32_t ur, uint32_t ll, uint32_t lr,
                      int y, int x, int ylen, int xlen){
  uint32_t chan = 0;
  channel_set_rgb_clipped(&chan,
                         calc_gradient_component(channel_r(ul), channel_r(ur),
                                                 channel_r(ll), channel_r(lr),
                                                 y, x, ylen, xlen),
                         calc_gradient_component(channel_g(ul), channel_g(ur),
                                                 channel_g(ll), channel_g(lr),
                                                 y, x, ylen, xlen),
                         calc_gradient_component(channel_b(ul), channel_b(ur),
                                                 channel_b(ll), channel_b(lr),
                                                 y, x, ylen, xlen));
  channel_set_alpha(&chan, channel_alpha(ul)); // precondition: all αs are equal
  return chan;
}

// calculate both channels of a gradient at a particular point, storing them
// into `c`->channels. x and y ought be the location within the gradient.
static inline void
calc_gradient_channels(cell* c, uint64_t ul, uint64_t ur, uint64_t ll,
                       uint64_t lr, int y, int x, int ylen, int xlen){
  if(!channels_fg_default_p(ul)){
    cell_set_fchannel(c, calc_gradient_channel(channels_fchannel(ul),
                                               channels_fchannel(ur),
                                               channels_fchannel(ll),
                                               channels_fchannel(lr),
                                               y, x, ylen, xlen));
  }else{
    cell_set_fg_default(c);
  }
  if(!channels_bg_default_p(ul)){
    cell_set_bchannel(c, calc_gradient_channel(channels_bchannel(ul),
                                               channels_bchannel(ur),
                                               channels_bchannel(ll),
                                               channels_bchannel(lr),
                                               y, x, ylen, xlen));
  }else{
    cell_set_bg_default(c);
  }
}

static bool
check_gradient_channel_args(uint32_t ul, uint32_t ur, uint32_t bl, uint32_t br){
  if(channel_default_p(ul) || channel_default_p(ur) ||
     channel_default_p(bl) || channel_default_p(br)){
    if(!(channel_default_p(ul) && channel_default_p(ur) &&
         channel_default_p(bl) && channel_default_p(br))){
      return true;
    }
  }
  if(channel_alpha(ul) != channel_alpha(ur) ||
     channel_alpha(ur) != channel_alpha(bl) ||
     channel_alpha(bl) != channel_alpha(br)){
    return true;
  }
  if(channel_palindex_p(ul) || channel_palindex_p(bl) ||
     channel_palindex_p(br) || channel_palindex_p(ur)){
    return true;
  }
  return false;
}

// Given the four channels arguments, verify that:
//
// - if any is default foreground, all are default foreground
// - if any is default background, all are default background
// - all foregrounds must have the same alpha
// - all backgrounds must have the same alpha
// - palette-indexed color must not be used
static bool
check_gradient_args(uint64_t ul, uint64_t ur, uint64_t bl, uint64_t br){
  if(check_gradient_channel_args(channels_fchannel(ul), channels_fchannel(ur),
                                 channels_fchannel(bl), channels_fchannel(br))){
    return true;
  }
  if(check_gradient_channel_args(channels_bchannel(ul), channels_bchannel(ur),
                                 channels_bchannel(bl), channels_bchannel(br))){
    return true;
  }
  return false;
}

// calculate both channels of a gradient at a particular point, knowing that
// we're using double halfblocks, into `c`->channels.
static inline void
calc_highgradient(cell* c, uint64_t ul, uint64_t ur, uint64_t ll,
                  uint64_t lr, int y, int x, int ylen, int xlen){
  if(!channel_default_p(ul)){
    cell_set_fchannel(c, calc_gradient_channel(ul, ur, ll, lr,
                                               y * 2, x, ylen, xlen));
    cell_set_bchannel(c, calc_gradient_channel(ul, ur, ll, lr,
                                               y * 2 + 1, x, ylen, xlen));
  }else{
    cell_set_fg_default(c);
    cell_set_bg_default(c);
  }
}

int ncplane_highgradient(ncplane* n, uint32_t ul, uint32_t ur,
                         uint32_t ll, uint32_t lr, int ystop, int xstop){
  if(check_gradient_channel_args(ul, ur, ll, lr)){
    return -1;
  }
  int yoff, xoff, ymax, xmax;
  ncplane_cursor_yx(n, &yoff, &xoff);
  // must be at least 1x1, with its upper-left corner at the current cursor
  if(ystop < yoff){
    return -1;
  }
  if(xstop < xoff){
    return -1;
  }
  ncplane_dim_yx(n, &ymax, &xmax);
  // must be within the ncplane
  if(xstop >= xmax || ystop >= ymax){
    return -1;
  }
  const int xlen = xstop - xoff + 1;
  const int ylen = (ystop - yoff + 1) * 2;
  if(xlen == 1){
    if(ul != ur || ll != lr){
      return -1;
    }
  }
  for(int y = yoff ; y <= ystop ; ++y){
    for(int x = xoff ; x <= xstop ; ++x){
      cell* targc = ncplane_cell_ref_yx(n, y, x);
      targc->channels = 0;
      if(cell_load(n, targc, "▀") < 0){
        return -1;
      }
      // FIXME do the loop
      calc_highgradient(targc, ul, ur, ll, lr, y - yoff, x - xoff, ylen, xlen);
    }
  }
  return 0;
}

int ncplane_gradient(ncplane* n, const char* egc, uint32_t attrword,
                     uint64_t ul, uint64_t ur, uint64_t bl, uint64_t br,
                     int ystop, int xstop){
  if(check_gradient_args(ul, ur, bl, br)){
    return -1;
  }
  if(egc == NULL){
    return true;
  }
  int yoff, xoff, ymax, xmax;
  ncplane_cursor_yx(n, &yoff, &xoff);
  // must be at least 1x1, with its upper-left corner at the current cursor
  if(ystop < yoff){
    return -1;
  }
  if(xstop < xoff){
    return -1;
  }
  ncplane_dim_yx(n, &ymax, &xmax);
  // must be within the ncplane
  if(xstop >= xmax || ystop >= ymax){
    return -1;
  }
  const int xlen = xstop - xoff + 1;
  const int ylen = ystop - yoff + 1;
  if(ylen == 1){
    if(xlen == 1){
      if(ul != ur || ur != br || br != bl){
        return -1;
      }
    }else{
      if(ul != bl || ur != br){
        return -1;
      }
    }
  }else if(xlen == 1){
    if(ul != ur || bl != br){
      return -1;
    }
  }
  for(int y = yoff ; y <= ystop ; ++y){
    for(int x = xoff ; x <= xstop ; ++x){
      cell* targc = ncplane_cell_ref_yx(n, y, x);
      targc->channels = 0;
      if(cell_load(n, targc, egc) < 0){
        return -1;
      }
      targc->attrword = attrword;
      calc_gradient_channels(targc, ul, ur, bl, br, y - yoff, x - xoff, ylen, xlen);
    }
  }
  return 0;
}

int ncplane_stain(struct ncplane* n, int ystop, int xstop,
                  uint64_t tl, uint64_t tr, uint64_t bl, uint64_t br){
  // Can't use default or palette-indexed colors in a gradient
  if(check_gradient_args(tl, tr, bl, br)){
    return -1;
  }
  int yoff, xoff, ymax, xmax;
  ncplane_cursor_yx(n, &yoff, &xoff);
  // must be at least 1x1, with its upper-left corner at the current cursor
  if(ystop < yoff){
    return -1;
  }
  if(xstop < xoff){
    return -1;
  }
  ncplane_dim_yx(n, &ymax, &xmax);
  // must be within the ncplane
  if(xstop >= xmax || ystop >= ymax){
    return -1;
  }
  const int xlen = xstop - xoff + 1;
  const int ylen = ystop - yoff + 1;
  for(int y = yoff ; y <= ystop ; ++y){
    for(int x = xoff ; x <= xstop ; ++x){
      cell* targc = ncplane_cell_ref_yx(n, y, x);
      calc_gradient_channels(targc, tl, tr, bl, br, y - yoff, x - xoff, ylen, xlen);
    }
  }
  return 0;
}

int ncplane_format(struct ncplane* n, int ystop, int xstop, uint32_t attrword){
  int yoff, xoff, ymax, xmax;
  ncplane_cursor_yx(n, &yoff, &xoff);
  // must be at least 1x1, with its upper-left corner at the current cursor
  if(ystop < yoff){
    return -1;
  }
  if(xstop < xoff){
    return -1;
  }
  ncplane_dim_yx(n, &ymax, &xmax);
  // must be within the ncplane
  if(xstop >= xmax || ystop >= ymax){
    return -1;
  }
  for(int y = yoff ; y < ystop + 1 ; ++y){
    for(int x = xoff ; x < xstop + 1 ; ++x){
      cell* targc = ncplane_cell_ref_yx(n, y, x);
      targc->attrword = attrword;
    }
  }
  return 0;
}

// generate a temporary plane that can hold the contents of n, rotated 90°
static ncplane* rotate_plane(const ncplane* n){
  int absy, absx;
  ncplane_yx(n, &absy, &absx);
  int dimy, dimx;
  ncplane_dim_yx(n, &dimy, &dimx);
  if(dimx % 2 != 0){
    return NULL;
  }
  const int newy = dimx / 2;
  const int newx = dimy * 2;
  ncplane* newp = ncplane_new(n->nc, newy, newx, absy, absx, n->userptr);
  return newp;
}

// if we're a lower block, reverse the channels. if we're a space, set both to
// the background. if we're a full block, set both to the foreground.
static void
rotate_channels(ncplane* src, const cell* c, uint32_t* fchan, uint32_t* bchan){
  if(cell_simple_p(c)){
    if(!isgraph(c->gcluster)){
      *fchan = *bchan;
    }
    return;
  }
  const char* origc = cell_extended_gcluster(src, c);
  if(strcmp(origc, "▄") == 0){
    uint32_t tmp = *fchan;
    *fchan = *bchan;
    *bchan = tmp;
    return;
  }else if(strcmp(origc, "█") == 0){
    *bchan = *fchan;
    return;
  }
}

static int
rotate_output(ncplane* dst, uint32_t tchan, uint32_t bchan){
  dst->channels = channels_combine(tchan, bchan);
  if(tchan != bchan){
    return ncplane_putegc(dst, "▀", NULL);
  }
  if(channel_default_p(tchan) && channel_default_p(bchan)){
    return ncplane_putegc(dst, "", NULL);
  }else if(channel_default_p(tchan)){
    return ncplane_putegc(dst, " ", NULL);
  }
  return ncplane_putegc(dst, "█", NULL);
}

// rotation works at two levels:
//  1) each 1x2 block is rotated into a 1x2 block ala
//      ab   cw    ca   ccw   ab   ccw   bd  ccw   dc  ccw   ca  ccw  ab
//      cd   -->   db   -->   cd   -->   ac  -->   ba  -->   db  -->  cd
//  2) each 1x2 block is rotated into its new location
//
// Characters which can be rotated must be RGB, to differentiate full blocks,
// spaces, and nuls. For clockwise rotations:
//
//  nul: converts to two half defaults
//  space: converts to two half backgrounds
//  full: converts to two half foregrounds
//  upper: converts to half background + half foreground
//  lower: converts to half foreground + half background
//
// Fore/background carry full channel, including transparency.
//
// Ideally, rotation through 360 degrees will restore the original 2x1 squre.
// Unfortunately, the case where a half block occupies a cell having the same
// fore- and background will see it roated into a single full block. In
// addition, lower blocks eventually become upper blocks with their channels
// reversed. In general:
//
//  if a "row" (the bottom or top halves) are the same forechannel, merge to a
//    single full block of that color (what is its background?).
//  if a "row" is two different channels, they become a upper block (why not
//   lower?) having the two channels as fore- and background.
static int
rotate_2x1_cw(ncplane* src, ncplane* dst, int srcy, int srcx, int dsty, int dstx){
  cell c1 = CELL_TRIVIAL_INITIALIZER;
  cell c2 = CELL_TRIVIAL_INITIALIZER;
  if(ncplane_at_yx(src, srcy, srcx, &c1) < 0){
    return -1;
  }
  if(ncplane_at_yx(src, srcy, srcx + 1, &c2) < 0){
    cell_release(src, &c1);
    return -1;
  }
  // there can be at most 4 colors and 4 transparencies:
  //  - c1fg, c1bg, c2fg, c2bg, c1ftrans, c2ftrans, c1btrans, c2btrans
  // but not all are necessarily used:
  //  - topleft gets lowerleft. if lowerleft is foreground, c1fg c1ftrans.
  //     otherwise, c1bg c1btrans
  //  - topright gets upperleft. if upperleft is foreground, c1fg c1ftrans.
  //     otherwise, c1bg c1btrans
  //  - botleft get botright. if botright is foreground, c2fg c2ftrans.
  //     otherwise, c2bg c2btrans
  //  - botright gets topright. if topright is foreground, c2fg c2ftrans.
  //     otherwise, c2bg c2btrans
  uint32_t c1b = cell_bchannel(&c1);
  uint32_t c2b = cell_bchannel(&c2);
  uint32_t c1t = cell_fchannel(&c1);
  uint32_t c2t = cell_fchannel(&c2);
  rotate_channels(src, &c1, &c1t, &c1b);
  rotate_channels(src, &c2, &c2t, &c2b);
  // right char comes from two tops. left char comes from two bottoms. if
  // they're the same channel, they become a:
  //
  //  nul if the channel is default
  //  space if the fore is default
  //  full if the back is default
  ncplane_cursor_move_yx(dst, dsty, dstx);
  rotate_output(dst, c1b, c2b);
  rotate_output(dst, c1t, c2t);
  return 0;
}

int rotate_2x1_ccw(ncplane* src, ncplane* dst, int srcy, int srcx, int dsty, int dstx){
  cell c1 = CELL_TRIVIAL_INITIALIZER;
  cell c2 = CELL_TRIVIAL_INITIALIZER;
  if(ncplane_at_yx(src, srcy, srcx, &c1) < 0){
    return -1;
  }
  if(ncplane_at_yx(src, srcy, srcx + 1, &c2) < 0){
    cell_release(src, &c1);
    return -1;
  }
  uint32_t c1b = cell_bchannel(&c1);
  unsigned c2b = cell_bchannel(&c2);
  unsigned c1t = cell_fchannel(&c1);
  unsigned c2t = cell_fchannel(&c2);
  rotate_channels(src, &c1, &c1t, &c1b);
  rotate_channels(src, &c2, &c2t, &c2b);
  ncplane_cursor_move_yx(dst, dsty, dstx);
  rotate_output(dst, c1t, c2t);
  rotate_output(dst, c1b, c2b);
  return 0;
}

// copy 'newp' into 'n' after resizing 'n' to match 'newp'
static int
rotate_merge(ncplane* n, ncplane* newp){
  int dimy, dimx;
  ncplane_dim_yx(newp, &dimy, &dimx);
  int ret = ncplane_resize(n, 0, 0, 0, 0, 0, 0, dimy, dimx);
  if(ret == 0){
    for(int y = 0 ; y < dimy ; ++y){
      for(int x = 0 ; x < dimx ; ++x){
        const cell* src = &newp->fb[fbcellidx(y, dimx, x)];
        cell* targ = &n->fb[fbcellidx(y, dimx, x)];
        if(cell_duplicate_far(&n->pool, targ, newp, src) < 0){
          return -1;
        }
      }
    }
  }
  return ret;
}

int ncplane_rotate_cw(ncplane* n){
  ncplane* newp = rotate_plane(n);
  if(newp == NULL){
    return -1;
  }
  int dimy, dimx;
  ncplane_dim_yx(n, &dimy, &dimx);
  // the topmost row consists of the leftmost two columns. the rightmost column
  // of the topmost row consists of the top half of the top two leftmost cells.
  // the penultimate column of the topmost row consists of the bottom half of
  // the top two leftmost cells. work from the bottom up on the source, so we
  // can copy to the top row from the left to the right.
  int targx, targy = 0;
  for(int x = 0 ; x < dimx ; x += 2){
    targx = 0;
    for(int y = dimy - 1 ; y >= 0 ; --y){
      if(rotate_2x1_cw(n, newp, y, x, targy, targx)){
        ncplane_destroy(newp);
        return -1;
      }
      targx += 2;
    }
    ++targy;
  }
  int ret = rotate_merge(n, newp);
  ret |= ncplane_destroy(newp);
  return ret;
}

int ncplane_rotate_ccw(ncplane* n){
  ncplane* newp = rotate_plane(n);
  if(newp == NULL){
    return -1;
  }
  int dimy, dimx, targdimy, targdimx;
  ncplane_dim_yx(n, &dimy, &dimx);
  ncplane_dim_yx(newp, &targdimy, &targdimx);
  int x = dimx - 2, y;
  // Each row of the target plane is taken from a column of the source plane.
  // As the target row grows (down), the source column shrinks (moves left).
  for(int targy = 0 ; targy < targdimy ; ++targy){
    y = 0;
    for(int targx = 0 ; targx < targdimx ; targx += 2){
      if(rotate_2x1_ccw(n, newp, y, x, targy, targx)){
        ncplane_destroy(newp);
        return -1;
      }
      ++y;
    }
    x -= 2;
  }
  int ret = rotate_merge(n, newp);
  ret |= ncplane_destroy(newp);
  return ret;
}
