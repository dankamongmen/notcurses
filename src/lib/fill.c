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

// Given the four channels arguments, verify that:
//
// - if any is default foreground, all are default foreground
// - if any is default background, all are default background
// - all foregrounds must have the same alpha
// - all backgrounds must have the same alpha
// - palette-indexed color must not be used
static bool
check_gradient_args(uint64_t ul, uint64_t ur, uint64_t bl, uint64_t br){
  if(channels_fg_default_p(ul) || channels_fg_default_p(ur) ||
     channels_fg_default_p(bl) || channels_fg_default_p(br)){
    if(!(channels_fg_default_p(ul) && channels_fg_default_p(ur) &&
         channels_fg_default_p(bl) && channels_fg_default_p(br))){
      return true;
    }
  }
  if(channels_bg_default_p(ul) || channels_bg_default_p(ur) ||
     channels_bg_default_p(bl) || channels_bg_default_p(br)){
    if(!(channels_bg_default_p(ul) && channels_bg_default_p(ur) &&
         channels_bg_default_p(bl) && channels_bg_default_p(br))){
      return true;
    }
  }
  if(channels_fg_alpha(ul) != channels_fg_alpha(ur) ||
     channels_fg_alpha(ur) != channels_fg_alpha(bl) ||
     channels_fg_alpha(bl) != channels_fg_alpha(br)){
    return true;
  }
  if(channels_bg_alpha(ul) != channels_bg_alpha(ur) ||
     channels_bg_alpha(ur) != channels_bg_alpha(bl) ||
     channels_bg_alpha(bl) != channels_bg_alpha(br)){
    return true;
  }
  if(channels_fg_palindex_p(ul) || channels_fg_palindex_p(bl) ||
     channels_fg_palindex_p(br) || channels_fg_palindex_p(ur)){
    return true;
  }
  if(channels_bg_palindex_p(ul) || channels_bg_palindex_p(bl) ||
     channels_bg_palindex_p(br) || channels_bg_palindex_p(ur)){
    return true;
  }
  return false;
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
