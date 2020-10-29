#include <stddef.h>
#include "internal.h"

static const uint32_t zeroes32;
static const unsigned char zeroes[] = "\x00\x00\x00\x00";

// linearly interpolate a 24-bit RGB value along each 8-bit channel
static inline uint32_t
lerp(uint32_t c0, uint32_t c1){
  uint32_t ret = 0;
  unsigned r0, g0, b0, r1, g1, b1;
  channel_rgb8(c0, &r0, &g0, &b0);
  channel_rgb8(c1, &r1, &g1, &b1);
  channel_set_rgb8(&ret, (r0 + r1 + 1) / 2,
                         (g0 + g1 + 1) / 2,
                         (b0 + b1 + 1) / 2);
  return ret;
}

// linearly interpolate a 24-bit RGB value along each 8-bit channel
static inline uint32_t
trilerp(uint32_t c0, uint32_t c1, uint32_t c2){
  uint32_t ret = 0;
  unsigned r0, g0, b0, r1, g1, b1, r2, g2, b2;
  channel_rgb8(c0, &r0, &g0, &b0);
  channel_rgb8(c1, &r1, &g1, &b1);
  channel_rgb8(c2, &r2, &g2, &b2);
  channel_set_rgb8(&ret, (r0 + r1 + r2 + 2) / 3,
                         (g0 + g1 + g2 + 2) / 3,
                         (b0 + b1 + b2 + 2) / 3);
  return ret;
}

// alpha comes to us 0--255, but we have only 3 alpha values to map them to.
// settled on experimentally.
static inline bool
ffmpeg_trans_p(unsigned char alpha){
  if(alpha < 192){
//fprintf(stderr, "TRANSPARENT!\n");
    return true;
  }
  return false;
}

// Retarded RGBA blitter (ASCII only).
static inline int
tria_blit_ascii(ncplane* nc, int placey, int placex, int linesize,
                const void* data, int begy, int begx,
                int leny, int lenx, bool blendcolors){
//fprintf(stderr, "ASCII %d X %d @ %d X %d (%p) place: %d X %d\n", leny, lenx, begy, begx, data, placey, placex);
  const int bpp = 32;
  int dimy, dimx, x, y;
  int total = 0; // number of cells written
  ncplane_dim_yx(nc, &dimy, &dimx);
  // FIXME not going to necessarily be safe on all architectures hrmmm
  const unsigned char* dat = data;
  int visy = begy;
  for(y = placey ; visy < (begy + leny) && y < dimy ; ++y, ++visy){
    if(ncplane_cursor_move_yx(nc, y, placex)){
      return -1;
    }
    int visx = begx;
    for(x = placex ; visx < (begx + lenx) && x < dimx ; ++x, ++visx){
      const unsigned char* rgbbase_up = dat + (linesize * visy) + (visx * bpp / CHAR_BIT);
//fprintf(stderr, "[%04d/%04d] bpp: %d lsize: %d %02x %02x %02x %02x\n", y, x, bpp, linesize, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2], rgbbase_up[3]);
      cell* c = ncplane_cell_ref_yx(nc, y, x);
      // use the default for the background, as that's the only way it's
      // effective in that case anyway
      c->channels = 0;
      c->stylemask = 0;
      if(blendcolors){
        cell_set_bg_alpha(c, CELL_ALPHA_BLEND);
        cell_set_fg_alpha(c, CELL_ALPHA_BLEND);
      }
      if(ffmpeg_trans_p(rgbbase_up[3])){
        cell_set_bg_alpha(c, CELL_ALPHA_TRANSPARENT);
        cell_set_fg_alpha(c, CELL_ALPHA_TRANSPARENT);
      }else{
        cell_set_fg_rgb8(c, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2]);
        cell_set_bg_rgb8(c, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2]);
        if(pool_load_direct(&nc->pool, c, " ", 1, 1) <= 0){
          return -1;
        }
        ++total;
      }
    }
  }
  return total;
}

// RGBA half-block blitter. Best for most images/videos. Full fidelity
// combined with 1:1 pixel aspect ratio.
static inline int
tria_blit(ncplane* nc, int placey, int placex, int linesize,
          const void* data, int begy, int begx,
          int leny, int lenx, bool blendcolors){
//fprintf(stderr, "HALF %d X %d @ %d X %d (%p) place: %d X %d\n", leny, lenx, begy, begx, data, placey, placex);
  const int bpp = 32;
  int dimy, dimx, x, y;
  int total = 0; // number of cells written
  ncplane_dim_yx(nc, &dimy, &dimx);
  // FIXME not going to necessarily be safe on all architectures hrmmm
  const unsigned char* dat = data;
  int visy = begy;
  for(y = placey ; visy < (begy + leny) && y < dimy ; ++y, visy += 2){
    if(ncplane_cursor_move_yx(nc, y, placex)){
      return -1;
    }
    int visx = begx;
    for(x = placex ; visx < (begx + lenx) && x < dimx ; ++x, ++visx){
      const unsigned char* rgbbase_up = dat + (linesize * visy) + (visx * bpp / CHAR_BIT);
      const unsigned char* rgbbase_down = zeroes;
      if(visy < begy + leny - 1){
        rgbbase_down = dat + (linesize * (visy + 1)) + (visx * bpp / CHAR_BIT);
      }
//fprintf(stderr, "[%04d/%04d] bpp: %d lsize: %d %02x %02x %02x %02x\n", y, x, bpp, linesize, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2], rgbbase_up[3]);
      cell* c = ncplane_cell_ref_yx(nc, y, x);
      // use the default for the background, as that's the only way it's
      // effective in that case anyway
      c->channels = 0;
      c->stylemask = 0;
      if(blendcolors){
        cell_set_bg_alpha(c, CELL_ALPHA_BLEND);
        cell_set_fg_alpha(c, CELL_ALPHA_BLEND);
      }
      if(ffmpeg_trans_p(rgbbase_up[3]) || ffmpeg_trans_p(rgbbase_down[3])){
        cell_set_bg_alpha(c, CELL_ALPHA_TRANSPARENT);
        if(ffmpeg_trans_p(rgbbase_up[3]) && ffmpeg_trans_p(rgbbase_down[3])){
          cell_set_fg_alpha(c, CELL_ALPHA_TRANSPARENT);
        }else if(ffmpeg_trans_p(rgbbase_up[3])){ // down has the color
          if(cell_load(nc, c, "\u2584") <= 0){ // lower half block
            return -1;
          }
          cell_set_fg_rgb8(c, rgbbase_down[0], rgbbase_down[1], rgbbase_down[2]);
          ++total;
        }else{ // up has the color
          if(cell_load(nc, c, "\u2580") <= 0){ // upper half block
            return -1;
          }
          cell_set_fg_rgb8(c, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2]);
          ++total;
        }
      }else{
        if(memcmp(rgbbase_up, rgbbase_down, 3) == 0){
          cell_set_fg_rgb8(c, rgbbase_down[0], rgbbase_down[1], rgbbase_down[2]);
          cell_set_bg_rgb8(c, rgbbase_down[0], rgbbase_down[1], rgbbase_down[2]);
          if(cell_load(nc, c, " ") <= 0){ // only need the background
            return -1;
          }
        }else{
          cell_set_fg_rgb8(c, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2]);
          cell_set_bg_rgb8(c, rgbbase_down[0], rgbbase_down[1], rgbbase_down[2]);
          if(cell_load(nc, c, "\u2580") <= 0){ // upper half block
            return -1;
          }
        }
        ++total;
      }
    }
  }
  return total;
}

// get a non-negative "distance" between two rgb values
static inline uint32_t
rgb_diff(unsigned r1, unsigned g1, unsigned b1, unsigned r2, unsigned g2, unsigned b2){
  uint32_t distance = 0;
  distance += r1 > r2 ? r1 - r2 : r2 - r1;
  distance += g1 > g2 ? g1 - g2 : g2 - g1;
  distance += b1 > b2 ? b1 - b2 : b2 - b1;
  return distance;
}

// once we find the closest pair of colors, we need look at the other two
// colors, and determine whether either belongs with us rather with them.
// if so, take the closer, and trilerp it in with us. otherwise, lerp the
// two excluded pixels (and retain our original lerp).
static const struct qdriver {
  int pair[2];      // indices of contributing pair
  int others[2];    // indices of excluded pair
  const char* egc;  // EGC corresponding to contributing pair
  const char* oth0egc; // EGC upon absorbing others[0]
  const char* oth1egc; // EGC upon absorbing others[1]
} quadrant_drivers[6] = {
  { .pair = { 0, 1 }, .others = { 2, 3 }, .egc = "▀", .oth0egc = "▛", .oth1egc = "▜", },
  { .pair = { 0, 2 }, .others = { 1, 3 }, .egc = "▌", .oth0egc = "▛", .oth1egc = "▙", },
  { .pair = { 0, 3 }, .others = { 1, 2 }, .egc = "▚", .oth0egc = "▜", .oth1egc = "▙", },
  { .pair = { 1, 2 }, .others = { 0, 3 }, .egc = "▞", .oth0egc = "▛", .oth1egc = "▟", },
  { .pair = { 1, 3 }, .others = { 0, 2 }, .egc = "▐", .oth0egc = "▜", .oth1egc = "▟", },
  { .pair = { 2, 3 }, .others = { 0, 1 }, .egc = "▄", .oth0egc = "▙", .oth1egc = "▟", },
};

// get the six distances between four colors. diffs must be an array of
// at least 6 uint32_t values.
static void
rgb_4diff(uint32_t* diffs, uint32_t tl, uint32_t tr, uint32_t bl, uint32_t br){
  struct rgb {
    unsigned r, g, b;
  } colors[4];
  channel_rgb8(tl, &colors[0].r, &colors[0].g, &colors[0].b);
  channel_rgb8(tr, &colors[1].r, &colors[1].g, &colors[1].b);
  channel_rgb8(bl, &colors[2].r, &colors[2].g, &colors[2].b);
  channel_rgb8(br, &colors[3].r, &colors[3].g, &colors[3].b);
  for(size_t idx = 0 ; idx < sizeof(quadrant_drivers) / sizeof(*quadrant_drivers) ; ++idx){
    const struct qdriver* qd = quadrant_drivers + idx;
    const struct rgb* rgb0 = colors + qd->pair[0];
    const struct rgb* rgb1 = colors + qd->pair[1];
    diffs[idx] = rgb_diff(rgb0->r, rgb0->g, rgb0->b,
                          rgb1->r, rgb1->g, rgb1->b);
  }
}

// solve for the EGC and two colors to best represent four colors at top
// left, top right, bot left, bot right
static inline const char*
quadrant_solver(uint32_t tl, uint32_t tr, uint32_t bl, uint32_t br,
                uint32_t* fore, uint32_t* back){
  const uint32_t colors[4] = { tl, tr, bl, br };
//fprintf(stderr, "%08x/%08x/%08x/%08x\n", tl, tr, bl, br);
  uint32_t diffs[sizeof(quadrant_drivers) / sizeof(*quadrant_drivers)];
  rgb_4diff(diffs, tl, tr, bl, br);
  // compiler can't verify that we'll always be less than 769 somewhere,
  // so fuck it, just go ahead and initialize to 0 / diffs[0]
  size_t mindiffidx = 0;
  unsigned mindiff = diffs[0]; // 3 * 256 + 1; // max distance is 256 * 3
  // if all diffs are 0, emit a space
  bool allzerodiffs = (mindiff == 0);
  for(size_t idx = 1 ; idx < sizeof(diffs) / sizeof(*diffs) ; ++idx){
    if(diffs[idx] < mindiff){
      mindiffidx = idx;
      mindiff = diffs[idx];
    }
    if(diffs[idx]){
      allzerodiffs = false;
    }
  }
  if(allzerodiffs){
    *fore = *back = tl;
    return " ";
  }
  // at this point, 0 <= mindiffidx <= 5. foreground color will be the
  // lerp of this nearest pair. we then check the other two. if they are
  // closer to one another than either is to our lerp, lerp between them.
  // otherwise, bring the closer one into our lerped fold.
  const struct qdriver* qd = &quadrant_drivers[mindiffidx];
  // the diff of the excluded pair is conveniently located at the inverse
  // location within diffs[] viz mindiffidx.
  // const uint32_t otherdiff = diffs[5 - mindiffidx];
  *fore = lerp(colors[qd->pair[0]], colors[qd->pair[1]]);
  *back = lerp(colors[qd->others[0]], colors[qd->others[1]]);
//fprintf(stderr, "mindiff: %u[%zu] fore: %08x back: %08x %d+%d/%d+%d\n", mindiff, mindiffidx, *fore, *back, qd->pair[0], qd->pair[1], qd->others[0], qd->others[1]);
  const char* egc = qd->egc;
  // break down the excluded pair and lerp
  unsigned r0, r1, g0, g1, b0, b1;
  unsigned roth, goth, both, rlerp, glerp, blerp;
  channel_rgb8(colors[qd->others[0]], &r0, &g0, &b0);
  channel_rgb8(colors[qd->others[1]], &r1, &g1, &b1);
  channel_rgb8(*fore, &rlerp, &glerp, &blerp);
  channel_rgb8(*back, &roth, &goth, &both);
//fprintf(stderr, "rgbs: %02x %02x %02x / %02x %02x %02x\n", r0, g0, b0, r1, g1, b1);
  diffs[0] = rgb_diff(r0, g0, b0, rlerp, glerp, blerp);
  diffs[1] = rgb_diff(r0, g0, b0, roth, goth, both);
  diffs[2] = rgb_diff(r1, g1, b1, rlerp, glerp, blerp);
  diffs[3] = rgb_diff(r1, g1, b1, roth, goth, both);
//fprintf(stderr, "diffs: %08x %08x %08x %08x\n", diffs[0], diffs[1], diffs[2], diffs[3]);
  if(diffs[0] < diffs[1] && diffs[0] < diffs[2]){
    egc = qd->oth0egc;
    *back = colors[qd->others[1]];
    *fore = trilerp(colors[qd->pair[0]], colors[qd->pair[1]], colors[qd->others[0]]);
//fprintf(stderr, "swap 1 %08x %08x\n", *fore, *back);
  }else if(diffs[2] < diffs[3]){
    egc = qd->oth1egc;
    *back = colors[qd->others[0]];
    *fore = trilerp(colors[qd->pair[0]], colors[qd->pair[1]], colors[qd->others[1]]);
//fprintf(stderr, "swap 2 %08x %08x\n", *fore, *back);
  }
  return egc;
}

// quadrant check for transparency. returns an EGC if we found transparent
// quads and have solved for colors (this EGC ought then be loaded into the
// cell). returns NULL otherwise. transparency trumps everything else in terms
// of priority -- if even one quadrant is transparent, we will have a
// transparent background, and lerp the rest together for foreground. we thus
// have a 16-way conditional tree in which each EGC must show up exactly once.
// FIXME we ought be able to just build up a bitstring and use it as an index!
static inline const char*
qtrans_check(cell* c, bool blendcolors,
             const unsigned char* rgbbase_tl, const unsigned char* rgbbase_tr,
             const unsigned char* rgbbase_bl, const unsigned char* rgbbase_br){
  uint32_t tl = 0, tr = 0, bl = 0, br = 0;
  channel_set_rgb8(&tl, rgbbase_tl[0], rgbbase_tl[1], rgbbase_tl[2]);
  channel_set_rgb8(&tr, rgbbase_tr[0], rgbbase_tr[1], rgbbase_tr[2]);
  channel_set_rgb8(&bl, rgbbase_bl[0], rgbbase_bl[1], rgbbase_bl[2]);
  channel_set_rgb8(&br, rgbbase_br[0], rgbbase_br[1], rgbbase_br[2]);
  const char* egc = NULL;
  if(ffmpeg_trans_p(rgbbase_tl[3])){
    // top left is transparent
    if(ffmpeg_trans_p(rgbbase_tr[3])){
      // all of top is transparent
      if(ffmpeg_trans_p(rgbbase_bl[3])){
        // top and left are transparent
        if(ffmpeg_trans_p(rgbbase_br[3])){
          // entirety is transparent, load with nul (but not NULL)
          cell_set_fg_default(c);
          egc = "";
        }else{
          cell_set_fg_rgb8(c, rgbbase_br[0], rgbbase_br[1], rgbbase_br[2]);
          egc = "▗";
        }
      }else{
        if(ffmpeg_trans_p(rgbbase_br[3])){
          cell_set_fg_rgb8(c, rgbbase_bl[0], rgbbase_bl[1], rgbbase_bl[2]);
          egc = "▖";
        }else{
          cell_set_fchannel(c, lerp(bl, br));
          egc = "▄";
        }
      }
    }else{ // top right is foreground, top left is transparent
      if(ffmpeg_trans_p(rgbbase_bl[3])){
        if(ffmpeg_trans_p(rgbbase_br[3])){ // entire bottom is transparent
          cell_set_fg_rgb8(c, rgbbase_tr[0], rgbbase_tr[1], rgbbase_tr[2]);
          egc = "▝";
        }else{
          cell_set_fchannel(c, lerp(tr, br));
          egc = "▐";
        }
      }else if(ffmpeg_trans_p(rgbbase_br[3])){ // only br is transparent
        cell_set_fchannel(c, lerp(tr, bl));
        egc = "▞";
      }else{
        cell_set_fchannel(c, trilerp(tr, bl, br));
        egc = "▟";
      }
    }
  }else{ // topleft is foreground for all here
    if(ffmpeg_trans_p(rgbbase_tr[3])){
      if(ffmpeg_trans_p(rgbbase_bl[3])){
        if(ffmpeg_trans_p(rgbbase_br[3])){
          cell_set_fg_rgb8(c, rgbbase_tl[0], rgbbase_tl[1], rgbbase_tl[2]);
          egc = "▘";
        }else{
          cell_set_fchannel(c, lerp(tl, br));
          egc = "▚";
        }
      }else if(ffmpeg_trans_p(rgbbase_br[3])){
        cell_set_fchannel(c, lerp(tl, bl));
        egc = "▌";
      }else{
        cell_set_fchannel(c, trilerp(tl, bl, br));
        egc = "▙";
      }
    }else if(ffmpeg_trans_p(rgbbase_bl[3])){
      if(ffmpeg_trans_p(rgbbase_br[3])){ // entire bottom is transparent
        cell_set_fchannel(c, lerp(tl, tr));
        egc = "▀";
      }else{ // only bl is transparent
        cell_set_fchannel(c, trilerp(tl, tr, br));
        egc = "▜";
      }
    }else if(ffmpeg_trans_p(rgbbase_br[3])){ // only br is transparent
      cell_set_fchannel(c, trilerp(tl, tr, bl));
      egc = "▛";
    }else{
      return NULL; // no transparency
    }
  }
  assert(egc);
  cell_set_bg_alpha(c, CELL_ALPHA_TRANSPARENT);
  if(*egc == '\0'){
    cell_set_fg_alpha(c, CELL_ALPHA_TRANSPARENT);
  }else if(blendcolors){
    cell_set_fg_alpha(c, CELL_ALPHA_BLEND);
  }
  return egc;
}

// quadrant blitter. maps 2x2 to each cell. since we only have two colors at
// our disposal (foreground and background), we lose some fidelity.
static inline int
quadrant_blit(ncplane* nc, int placey, int placex, int linesize,
              const void* data, int begy, int begx,
              int leny, int lenx, bool blendcolors){
  const int bpp = 32;
  int dimy, dimx, x, y;
  int total = 0; // number of cells written
  ncplane_dim_yx(nc, &dimy, &dimx);
//fprintf(stderr, "quadblitter %dx%d -> %d/%d+%d/%d\n", leny, lenx, dimy, dimx, placey, placex);
  // FIXME not going to necessarily be safe on all architectures hrmmm
  const unsigned char* dat = data;
  int visy = begy;
  for(y = placey ; visy < (begy + leny) && y < dimy ; ++y, visy += 2){
    if(ncplane_cursor_move_yx(nc, y, placex)){
      return -1;
    }
    int visx = begx;
    for(x = placex ; visx < (begx + lenx) && x < dimx ; ++x, visx += 2){
      const unsigned char* rgbbase_tl = dat + (linesize * visy) + (visx * bpp / CHAR_BIT);
      const unsigned char* rgbbase_tr = zeroes;
      const unsigned char* rgbbase_bl = zeroes;
      const unsigned char* rgbbase_br = zeroes;
      if(visx < begx + lenx - 1){
        rgbbase_tr = dat + (linesize * visy) + ((visx + 1) * bpp / CHAR_BIT);
        if(visy < begy + leny - 1){
          rgbbase_br = dat + (linesize * (visy + 1)) + ((visx + 1) * bpp / CHAR_BIT);
        }
      }
      if(visy < begy + leny - 1){
        rgbbase_bl = dat + (linesize * (visy + 1)) + (visx * bpp / CHAR_BIT);
      }
//fprintf(stderr, "[%04d/%04d] bpp: %d lsize: %d %02x %02x %02x %02x\n", y, x, bpp, linesize, rgbbase_tl[0], rgbbase_tr[1], rgbbase_bl[2], rgbbase_br[3]);
      cell* c = ncplane_cell_ref_yx(nc, y, x);
      c->channels = 0;
      c->stylemask = 0;
      const char* egc = qtrans_check(c, blendcolors, rgbbase_tl, rgbbase_tr, rgbbase_bl, rgbbase_br);
      if(egc == NULL){
        uint32_t tl = 0, tr = 0, bl = 0, br = 0;
        channel_set_rgb8(&tl, rgbbase_tl[0], rgbbase_tl[1], rgbbase_tl[2]);
        channel_set_rgb8(&tr, rgbbase_tr[0], rgbbase_tr[1], rgbbase_tr[2]);
        channel_set_rgb8(&bl, rgbbase_bl[0], rgbbase_bl[1], rgbbase_bl[2]);
        channel_set_rgb8(&br, rgbbase_br[0], rgbbase_br[1], rgbbase_br[2]);
        uint32_t bg, fg;
//fprintf(stderr, "qtrans check: %d/%d\n%08x %08x\n%08x %08x\n", y, x, *(const uint32_t*)rgbbase_tl, *(const uint32_t*)rgbbase_tr, *(const uint32_t*)rgbbase_bl, *(const uint32_t*)rgbbase_br);
        egc = quadrant_solver(tl, tr, bl, br, &fg, &bg);
        assert(egc);
//fprintf(stderr, "%d/%d %08x/%08x\n", y, x, fg, bg);
        cell_set_fchannel(c, fg);
        cell_set_bchannel(c, bg);
        if(blendcolors){
          cell_set_bg_alpha(c, CELL_ALPHA_BLEND);
          cell_set_fg_alpha(c, CELL_ALPHA_BLEND);
        }
      }
      if(*egc){
        if(pool_blit_direct(&nc->pool, c, egc, strlen(egc), 1) <= 0){
          return -1;
        }
        ++total;
      }
    }
  }
  return total;
}

// returns true iff the pixel is transparent. otherwise, the r/g/b values are
// accumulated into rsum/gsum/bsum, and npopcnt is increased.
static inline bool
strans_fold(unsigned* bitstring, unsigned bit, const uint32_t* rgba,
            unsigned* rsum, unsigned* gsum, unsigned* bsum, unsigned* npopcnt){
  if(ffmpeg_trans_p(ncpixel_a(*rgba))){
    *bitstring |= bit;
    return true;
  }
  ++*npopcnt;
  *rsum += ncpixel_r(*rgba);
  *gsum += ncpixel_g(*rgba);
  *bsum += ncpixel_b(*rgba);
//fprintf(stderr, "adding %u %u %u\n", rgba[0], rgba[1], rgba[2]);
  return false;
}

// if neither bit is present in the bitstring, store the sum of the absolute
// component differences into *diff. otherwise, set *diff to UINT_MAX. returns
// true if neither pixel was transparent, and their components were equal.
static inline bool
collect_diffs(unsigned* diff, unsigned bitstring, const uint32_t* rgba1,
              unsigned bit1, const uint32_t* rgba2, unsigned bit2){
  if((bitstring & (bit1 | bit2)) == 0){
    *diff = rgb_diff(ncpixel_r(*rgba1), ncpixel_g(*rgba1), ncpixel_b(*rgba1),
                     ncpixel_r(*rgba2), ncpixel_g(*rgba2), ncpixel_b(*rgba2));
    if(!*diff){
      return true;
    }
  }else{
    *diff = UINT_MAX;
  }
  return false;
}

// if the candidate diff is less than the minimum diff, update |mindiffidx|,
// and reset |*mindiffbits|. if the candidate diff is equal to the minimum
// diff, add bits to |*mindiffbits|.
static inline void
collect_mindiff(unsigned* mindiffidx, const unsigned diffs[15],
                unsigned candidate, unsigned *mindiffbits,
                const uint32_t* rgbas[6]){
  // the two bits which are turned on by adding in a given difference pair
  static unsigned mindiffkeys[15] = {
    0x3, 0x5, 0x9, 0x11, 0x21,
    0x6, 0xa, 0x12, 0x22, 0xc,
    0x14, 0x24, 0x18, 0x28, 0x30,
  };
  if(diffs[candidate] <= diffs[*mindiffidx]){
    if(diffs[candidate] < diffs[*mindiffidx]){
      *mindiffidx = candidate;
      *mindiffbits = mindiffkeys[candidate];
    }else{
//fprintf(stderr, "mindiffbits: %08x candidate: %08x\n", *mindiffbits, mindiffkeys[candidate]);
      uint32_t lowestkey;
      // find an RGB value from the candidate mindiff set
      for(size_t i = 0 ; i < 6 ; ++i){
        if(mindiffkeys[candidate] & (0x1 << i)){
          lowestkey = *rgbas[i];
//fprintf(stderr, "found lowestkey %08x at bit %zu\n", lowestkey, i);
          break;
        }
      }
      const uint32_t* lowestcur = NULL;
      // find an RGB value from the current mindiff set
      for(size_t i = 0 ; i < 6 ; ++i){
        if((i != lowestkey) && (*mindiffbits & (0x1 << i))){
          lowestcur = rgbas[i];
//fprintf(stderr, "found lowestcur %08x at bit %zu\n", lowestkey, i);
          break;
        }
      }
      if(!lowestcur ||
         (ncpixel_r(*lowestcur) == ncpixel_r(lowestkey) &&
          ncpixel_g(*lowestcur) == ncpixel_g(lowestkey) &&
          ncpixel_b(*lowestcur) == ncpixel_b(lowestkey))){
        *mindiffbits |= mindiffkeys[candidate];
      }
      // FIXME if diff was equal, but values are different, need to track 2
      // (since we can find 3 or even 4 with diff of D0, val of V1 after
      // finding 2 with diff of D0, val of V0).
    }
  }
}

// pick the foreground color based off lerping mindiffbits
static void
get_sex_colors(uint32_t* fg, uint32_t* bg, unsigned mindiffbits,
               unsigned r, unsigned g, unsigned b, unsigned div,
               const uint32_t* rgbas[6]){
  // FIXME fg lerps mindiffbits
  uint32_t fgcs[2];
  size_t i = 0;
//fprintf(stderr, "start r: %u g: %u b: %u div: %u midbs: 0x%02x\n", r, g, b, div, mindiffbits);
  for(unsigned shift = 0 ; shift < 6 ; ++shift){
    if(mindiffbits & (1u << shift)){
      // FIXME verify this
      if(i < sizeof(fgcs) / sizeof(*fgcs)){
        fgcs[i] = 0;
        channel_set_rgb8(&fgcs[i], ncpixel_r(*rgbas[shift]), ncpixel_g(*rgbas[shift]), ncpixel_b(*rgbas[shift]));
        ++i;
      }
      r -= ncpixel_r(*rgbas[shift]);
      g -= ncpixel_g(*rgbas[shift]);
      b -= ncpixel_b(*rgbas[shift]);
      --div;
    }
  }
  *fg = lerp(fgcs[0], fgcs[1]);
  *bg = 0;
  channel_set_rgb8(bg, r / div, g / div, b / div);
//fprintf(stderr, "fg: 0x%08x bg: 0x%08x r: %u g: %u b: %u div: %u\n", *fg, *bg, r, g, b, div);
}

static const char* sex[64] = {
 "█", "🬻", "🬺", "🬹", "🬸", "🬷", "🬶", "🬵", "🬴", "🬳", "🬲", // 10
 "🬱", "🬰", "🬯", "🬮", "🬭", "🬬", "🬫", "🬪", "🬩", "🬨", "▐", // 21
 "🬧", "🬦", "🬥", "🬤", "🬣", "🬢", "🬡", "🬠", "🬟", // 30
 "🬞", "🬝", "🬜", "🬛", "🬚", "🬙", "🬘", "🬗", "🬖", "🬕", // 40
 "🬔", "▌", "🬓", "🬒", "🬑", "🬐", "🬏", "🬎", "🬍", "🬌", // 50
 "🬋", "🬊", "🬉", "🬈", "🬇", "🬆", "🬅", "🬄", "🬃", "🬂", // 60
 "🬁", "🬀", " ",
};

// sextant check for transparency. returns an EGC if we found transparent pixels
// and have solved for colors (this EGC ought then be loaded into the cell).
// returns NULL otherwise. transparency trumps everything else in terms of
// priority -- if even one pixel is transparent, we will have a transparent
// background, and lerp the rest together for foreground. we thus have a 32-way
// conditional tree in which each EGC must show up exactly once. this also
// handles the opaque case where all six pixels are the same, emitting a space
// with both foreground and background set to that color.
// there are 15 sums of absolute differences between pixels:
//  l1 - r1, l1 - l2, l1 - r2, l1 - l3, l1 - r3 (0, 1, 2, 3, 4)
//  r1 - l2, r1 - r2, r1 - l3, r1 - r3 (5, 6, 7, 8)
//  l2 - r2, l2 - l3, l2 - r3 (9, 10, 11)
//  r2 - l3, r2 - r3 (12, 13)
//  l3 - r3 (14)
// 6 bits for 6 pixels: l1 r1 l2 r2 l3 r3, same as rgbas[]
static inline const char*
strans_check(uint64_t* channels, bool blendcolors, unsigned diffs[15],
             const uint32_t* rgbas[6]){
  unsigned transtring = 0; // bits which are transparent
  unsigned mindiffbits = 0; // bits which are involved in the minimum diff
  // unsigned min2diffbits = 0; FIXME need to track two equivalency sets (but
  // never three), since the second equivalency set we find might have three
  // members and thus become fg.
  unsigned div = 0;
  unsigned r, g, b;
  r = g = b = 0;
  bool allzerodiffs = true;
  strans_fold(&transtring,  1u, rgbas[0], &r, &g, &b, &div);
  strans_fold(&transtring,  2u, rgbas[1], &r, &g, &b, &div);
  // if either pixel is transparent, the difference is UINT_MAX
  allzerodiffs &= collect_diffs(&diffs[0], transtring, rgbas[1], 2u, rgbas[0], 1u);
  unsigned mindiffidx = 0;
  strans_fold(&transtring,  4u, rgbas[2], &r, &g, &b, &div);
  allzerodiffs &= collect_diffs(&diffs[1], transtring, rgbas[2], 4u, rgbas[0], 1u);
  collect_mindiff(&mindiffidx, diffs, 1, &mindiffbits, rgbas);
  allzerodiffs &= collect_diffs(&diffs[5], transtring, rgbas[2], 4u, rgbas[1], 2u);
  collect_mindiff(&mindiffidx, diffs, 5, &mindiffbits, rgbas);
  strans_fold(&transtring,  8u, rgbas[3], &r, &g, &b, &div);
  allzerodiffs &= collect_diffs(&diffs[2], transtring, rgbas[3], 8u, rgbas[0], 1u);
  allzerodiffs &= collect_diffs(&diffs[6], transtring, rgbas[3], 8u, rgbas[1], 2u);
  allzerodiffs &= collect_diffs(&diffs[9], transtring, rgbas[3], 8u, rgbas[2], 4u);
  collect_mindiff(&mindiffidx, diffs, 2, &mindiffbits, rgbas);
  collect_mindiff(&mindiffidx, diffs, 6, &mindiffbits, rgbas);
  collect_mindiff(&mindiffidx, diffs, 9, &mindiffbits, rgbas);
  strans_fold(&transtring, 16u, rgbas[4], &r, &g, &b, &div);
  allzerodiffs &= collect_diffs(&diffs[3], transtring, rgbas[4],  16u, rgbas[0], 1u);
  allzerodiffs &= collect_diffs(&diffs[7], transtring, rgbas[4],  16u, rgbas[1], 2u);
  allzerodiffs &= collect_diffs(&diffs[10], transtring, rgbas[4],  16u, rgbas[2], 4u);
  allzerodiffs &= collect_diffs(&diffs[12], transtring, rgbas[4], 16u, rgbas[3], 8u);
  collect_mindiff(&mindiffidx, diffs, 3, &mindiffbits, rgbas);
  collect_mindiff(&mindiffidx, diffs, 7, &mindiffbits, rgbas);
  collect_mindiff(&mindiffidx, diffs, 10, &mindiffbits, rgbas);
  collect_mindiff(&mindiffidx, diffs, 12, &mindiffbits, rgbas);
  strans_fold(&transtring, 32u, rgbas[5], &r, &g, &b, &div);
  allzerodiffs &= collect_diffs(&diffs[4], transtring, rgbas[5],  32u, rgbas[0], 1u);
  allzerodiffs &= collect_diffs(&diffs[8], transtring, rgbas[5],  32u, rgbas[1], 2u);
  allzerodiffs &= collect_diffs(&diffs[11], transtring, rgbas[5], 32u, rgbas[2], 4u);
  allzerodiffs &= collect_diffs(&diffs[13], transtring, rgbas[5], 32u, rgbas[3], 8u);
  allzerodiffs &= collect_diffs(&diffs[14], transtring, rgbas[5], 32u, rgbas[4], 16u);
  collect_mindiff(&mindiffidx, diffs, 4, &mindiffbits, rgbas);
  collect_mindiff(&mindiffidx, diffs, 8, &mindiffbits, rgbas);
  collect_mindiff(&mindiffidx, diffs, 11, &mindiffbits, rgbas);
  collect_mindiff(&mindiffidx, diffs, 13, &mindiffbits, rgbas);
  collect_mindiff(&mindiffidx, diffs, 14, &mindiffbits, rgbas);
  if(!transtring){ // no transparency. use complex solver unless all are equal.
    if(allzerodiffs){ // every diff was 0, so all must be the same rgb value.
      channels_set_bg_rgb8(channels, r / div, g / div, b / div);
      return " ";
    }
    uint32_t fg, bg;
    get_sex_colors(&fg, &bg, mindiffbits, r, g, b, div, rgbas);
    channels_set_fchannel(channels, fg);
    channels_set_bchannel(channels, bg);
    if(mindiffbits == 0){
      mindiffbits = 63;
      channels_set_fchannel(channels, bg);
      channels_set_bchannel(channels, fg);
    }
//fprintf(stderr, "solved channels: %016lx\n", *channels);
    return sex[mindiffbits];
  }
  // there were some transparent pixels. since they get priority, the foreground
  // is just a general lerp across non-transparent pixels.
  const char* egc = sex[transtring];
  channels_set_bg_alpha(channels, CELL_ALPHA_TRANSPARENT);
//fprintf(stderr, "transtring: %u egcp: %p egc: %s\n", transtring, egc, egc ? egc : "null");
  if(*egc == ' '){ // entirely transparent
    channels_set_fg_alpha(channels, CELL_ALPHA_TRANSPARENT);
    return "";
  }else{ // partially transparent, thus div >= 1
//fprintf(stderr, "div: %u r: %u g: %u b: %u\n", div, r, g, b);
    channels_set_fg_rgb8(channels, r / div, g / div, b / div);
    if(blendcolors){
      channels_set_fg_alpha(channels, CELL_ALPHA_BLEND);
    }
  }
  return egc;
}

// sextant blitter. maps 3x2 to each cell. since we only have two colors at
// our disposal (foreground and background), we lose some fidelity.
static inline int
sextant_blit(ncplane* nc, int placey, int placex, int linesize,
             const void* data, int begy, int begx,
             int leny, int lenx, bool blendcolors){
  const int bpp = 32;
  int dimy, dimx, x, y;
  int total = 0; // number of cells written
  ncplane_dim_yx(nc, &dimy, &dimx);
//fprintf(stderr, "sexblitter %dx%d -> %d/%d+%d/%d\n", leny, lenx, dimy, dimx, placey, placex);
  const unsigned char* dat = data;
  int visy = begy;
  for(y = placey ; visy < (begy + leny) && y < dimy ; ++y, visy += 3){
    if(ncplane_cursor_move_yx(nc, y, placex)){
      return -1;
    }
    int visx = begx;
    for(x = placex ; visx < (begx + lenx) && x < dimx ; ++x, visx += 2){
      const uint32_t* rgbbase_l1 = (const uint32_t*)(dat + (linesize * visy) + (visx * bpp / CHAR_BIT));
      const uint32_t* rgbbase_r1 = &zeroes32;
      const uint32_t* rgbbase_l2 = &zeroes32;
      const uint32_t* rgbbase_r2 = &zeroes32;
      const uint32_t* rgbbase_l3 = &zeroes32;
      const uint32_t* rgbbase_r3 = &zeroes32;
      if(visx < begx + lenx - 1){
        rgbbase_r1 = (const uint32_t*)(dat + (linesize * visy) + ((visx + 1) * bpp / CHAR_BIT));
        if(visy < begy + leny - 1){
          rgbbase_r2 = (const uint32_t*)(dat + (linesize * (visy + 1)) + ((visx + 1) * bpp / CHAR_BIT));
          if(visy < begy + leny - 2){
            rgbbase_r3 = (const uint32_t*)(dat + (linesize * (visy + 2)) + ((visx + 1) * bpp / CHAR_BIT));
          }
        }
      }
      if(visy < begy + leny - 1){
        rgbbase_l2 = (const uint32_t*)(dat + (linesize * (visy + 1)) + (visx * bpp / CHAR_BIT));
        if(visy < begy + leny - 2){
          rgbbase_l3 = (const uint32_t*)(dat + (linesize * (visy + 2)) + (visx  * bpp / CHAR_BIT));
        }
      }
//fprintf(stderr, "[%04d/%04d] bpp: %d lsize: %d %02x %02x %02x %02x\n", y, x, bpp, linesize, rgbbase_tl[0], rgbbase_tr[1], rgbbase_bl[2], rgbbase_br[3]);
      cell* c = ncplane_cell_ref_yx(nc, y, x);
      c->channels = 0;
      c->stylemask = 0;
      unsigned diffs[15];
      // FIXME just lead with these directly
      const uint32_t* rgbas[6] = {
        rgbbase_l1, rgbbase_r1, rgbbase_l2, rgbbase_r2, rgbbase_l3, rgbbase_r3,
      };
//fprintf(stderr, "strans check: %d/%d\n%08x %08x\n%08x %08x\n%08x %08x\n", y, x, *rgbas[0], *rgbas[1], *rgbas[2], *rgbas[3], *rgbas[4], *rgbas[5]);
      const char* egc = strans_check(&c->channels, blendcolors, diffs, rgbas);
//fprintf(stderr, "strans EGC: %s channels: %016lx\n", egc, c->channels);
      if(*egc){
        if(pool_blit_direct(&nc->pool, c, egc, strlen(egc), 1) <= 0){
          return -1;
        }
        ++total;
      }
    }
  }
  return total;
}

// fold the r, g, and b components of the pixel into *r, *g, and *b, and
// increment *foldcount
static inline void
fold_rgb8(unsigned* restrict r, unsigned* restrict g, unsigned* restrict b,
          const uint32_t* pixel, unsigned* foldcount){
  *r += ncpixel_r(*pixel);
  *g += ncpixel_g(*pixel);
  *b += ncpixel_b(*pixel);
  ++*foldcount;
}

// Braille blitter. maps 4x2 to each cell. since we only have one color at
// our disposal (foreground), we lose some fidelity. this is optimal for
// visuals with only two colors in a given area, as it packs lots of
// resolution. always transparent background.
static inline int
braille_blit(ncplane* nc, int placey, int placex, int linesize,
             const void* data, int begy, int begx,
             int leny, int lenx, bool blendcolors){
  const int bpp = 32;
  int dimy, dimx, x, y;
  int total = 0; // number of cells written
  ncplane_dim_yx(nc, &dimy, &dimx);
  // FIXME not going to necessarily be safe on all architectures hrmmm
  const unsigned char* dat = data;
  int visy = begy;
  for(y = placey ; visy < (begy + leny) && y < dimy ; ++y, visy += 4){
    if(ncplane_cursor_move_yx(nc, y, placex)){
      return -1;
    }
    int visx = begx;
    for(x = placex ; visx < (begx + lenx) && x < dimx ; ++x, visx += 2){
      const uint32_t* rgbbase_l0 = (const uint32_t*)(dat + (linesize * visy) + (visx * bpp / CHAR_BIT));
      const uint32_t* rgbbase_r0 = &zeroes32;
      const uint32_t* rgbbase_l1 = &zeroes32;
      const uint32_t* rgbbase_r1 = &zeroes32;
      const uint32_t* rgbbase_l2 = &zeroes32;
      const uint32_t* rgbbase_r2 = &zeroes32;
      const uint32_t* rgbbase_l3 = &zeroes32;
      const uint32_t* rgbbase_r3 = &zeroes32;
      unsigned r = 0, g = 0, b = 0;
      unsigned blends = 0;
      unsigned egcidx = 0;
      if(visx < begx + lenx - 1){
        rgbbase_r0 = (const uint32_t*)(dat + (linesize * visy) + ((visx + 1) * bpp / CHAR_BIT));
        if(visy < begy + leny - 1){
          rgbbase_r1 = (const uint32_t*)(dat + (linesize * (visy + 1)) + ((visx + 1) * bpp / CHAR_BIT));
          if(visy < begy + leny - 2){
            rgbbase_r2 = (const uint32_t*)(dat + (linesize * (visy + 2)) + ((visx + 1) * bpp / CHAR_BIT));
            if(visy < begy + leny - 3){
              rgbbase_r3 = (const uint32_t*)(dat + (linesize * (visy + 3)) + ((visx + 1) * bpp / CHAR_BIT));
            }
          }
        }
      }
      if(visy < begy + leny - 1){
        rgbbase_l1 = (const uint32_t*)(dat + (linesize * (visy + 1)) + (visx * bpp / CHAR_BIT));
        if(visy < begy + leny - 2){
          rgbbase_l2 = (const uint32_t*)(dat + (linesize * (visy + 2)) + (visx * bpp / CHAR_BIT));
          if(visy < begy + leny - 3){
            rgbbase_l3 = (const uint32_t*)(dat + (linesize * (visy + 3)) + (visx * bpp / CHAR_BIT));
          }
        }
      }
      // FIXME fold this into the above?
      if(!ffmpeg_trans_p(ncpixel_a(*rgbbase_l0))){
        egcidx |= 1u;
        fold_rgb8(&r, &g, &b, rgbbase_l0, &blends);
      }
      if(!ffmpeg_trans_p(ncpixel_a(*rgbbase_l1))){
        egcidx |= 2u;
        fold_rgb8(&r, &g, &b, rgbbase_l1, &blends);
      }
      if(!ffmpeg_trans_p(ncpixel_a(*rgbbase_l2))){
        egcidx |= 4u;
        fold_rgb8(&r, &g, &b, rgbbase_l2, &blends);
      }
      if(!ffmpeg_trans_p(ncpixel_a(*rgbbase_r0))){
        egcidx |= 8u;
        fold_rgb8(&r, &g, &b, rgbbase_r0, &blends);
      }
      if(!ffmpeg_trans_p(ncpixel_a(*rgbbase_r1))){
        egcidx |= 16u;
        fold_rgb8(&r, &g, &b, rgbbase_r1, &blends);
      }
      if(!ffmpeg_trans_p(ncpixel_a(*rgbbase_r2))){
        egcidx |= 32u;
        fold_rgb8(&r, &g, &b, rgbbase_r2, &blends);
      }
      if(!ffmpeg_trans_p(ncpixel_a(*rgbbase_l3))){
        egcidx |= 64u;
        fold_rgb8(&r, &g, &b, rgbbase_l3, &blends);
      }
      if(!ffmpeg_trans_p(ncpixel_a(*rgbbase_r3))){
        egcidx |= 128u;
        fold_rgb8(&r, &g, &b, rgbbase_r3, &blends);
      }
//fprintf(stderr, "[%04d/%04d] bpp: %d lsize: %d %02x %02x %02x %02x\n", y, x, bpp, linesize, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2], rgbbase_up[3]);
      cell* c = ncplane_cell_ref_yx(nc, y, x);
      // use the default for the background, as that's the only way it's
      // effective in that case anyway
      c->channels = 0;
      c->stylemask = 0;
      if(blendcolors){
        cell_set_fg_alpha(c, CELL_ALPHA_BLEND);
      }
      // FIXME for now, we just sample, color-wise, and always draw crap.
      // more complicated to do optimally than quadrants, for sure. ideally,
      // we only get one color in an area.
      cell_set_bg_alpha(c, CELL_ALPHA_TRANSPARENT);
      if(!egcidx){
          cell_set_fg_alpha(c, CELL_ALPHA_TRANSPARENT);
          // FIXME else look for pairs of transparency!
      }else{
        if(blends){
          cell_set_fg_rgb8(c, r / blends, g / blends, b / blends);
        }
        // UTF-8 encodings of the Braille Patterns are always 0xe2 0xaX 0xCC,
        // where 0 <= X <= 3 and 0x80 <= CC <= 0xbf (4 groups of 64).
        char egc[4] = { 0xe2, 0xa0, 0x80, 0x00 };
        egc[2] += egcidx % 64;
        egc[1] += egcidx / 64;
        if(pool_blit_direct(&nc->pool, c, egc, strlen(egc), 1) <= 0){
          return -1;
        }
      }
      ++total;
    }
  }
  return total;
}

// NCBLIT_DEFAULT is not included, as it has no defined properties. It ought
// be replaced with some real blitter implementation by the calling widget.
const struct blitset notcurses_blitters[] = {
   { .geom = NCBLIT_8x1,     .width = 1, .height = 8, .egcs = L" ▁▂▃▄▅▆▇█",
     .blit = tria_blit,      .name = "eightstep",     .fill = false, },
   { .geom = NCBLIT_1x1,     .width = 1, .height = 1, .egcs = L" █",
     .blit = tria_blit_ascii,.name = "ascii",         .fill = false, },
   { .geom = NCBLIT_2x1,     .width = 1, .height = 2, .egcs = L" ▄█",
     .blit = tria_blit,      .name = "halfblock",     .fill = false, },
   { .geom = NCBLIT_2x2,     .width = 2, .height = 2, .egcs = L" ▗▐▖▄▟▌▙█",
     .blit = quadrant_blit,  .name = "quadblitter",   .fill = false, },
   { .geom = NCBLIT_3x2,     .width = 2, .height = 3, .egcs = L" 🬀🬁🬂🬃🬄🬅🬆🬇🬈🬉🬊🬋🬌🬍🬎🬏🬐🬑🬒🬓🬔🬕🬖🬗🬘🬙🬚🬛🬜🬝🬞🬟🬠🬡🬢🬣🬤🬥🬦🬧🬨🬩🬪🬫🬬🬭🬮🬯🬰🬱🬲🬳🬴🬵🬶🬷🬸🬹🬺🬻█",
     .blit = sextant_blit,   .name = "sexblitter",   .fill = false, },
   { .geom = NCBLIT_4x1,     .width = 1, .height = 4, .egcs = L" ▂▄▆█",
     .blit = tria_blit,      .name = "fourstep",      .fill = false, },
   { .geom = NCBLIT_BRAILLE, .width = 2, .height = 4, .egcs = L"⠀⢀⢠⢰⢸⡀⣀⣠⣰⣸⡄⣄⣤⣴⣼⡆⣆⣦⣶⣾⡇⣇⣧⣷⣿",
     .blit = braille_blit,   .name = "braille",       .fill = true,  },
   { .geom = NCBLIT_SIXEL,   .width = 1, .height = 6, .egcs = L"",
     .blit = NULL,           .name = "sixel",         .fill = true,  }, // FIXME
   { .geom = 0,              .width = 0, .height = 0, .egcs = NULL,
     .blit = NULL,           .name = NULL,            .fill = false,  },
};

int notcurses_lex_blitter(const char* op, ncblitter_e* blitter){
  const struct blitset* bset = notcurses_blitters;
  while(bset->name){
    if(strcasecmp(bset->name, op) == 0){
      *blitter = bset->geom;
      return 0;
    }
    ++bset;
  }
  if(strcasecmp("default", op) == 0){
    *blitter = NCBLIT_DEFAULT;
    return 0;
  }
  return -1;
}

const char* notcurses_str_blitter(ncblitter_e blitter){
  if(blitter == NCBLIT_DEFAULT){
    return "default";
  }
  const struct blitset* bset = notcurses_blitters;
  while(bset->name){
    if(bset->geom == blitter){
      return bset->name;
    }
    ++bset;
  }
  return NULL;
}

int ncblit_bgrx(const void* data, int linesize, const struct ncvisual_options* vopts){
  if(vopts->flags > NCVISUAL_OPTION_BLEND){
    return -1;
  }
  struct ncplane* nc = vopts->n;
  if(nc == NULL){
    return -1;
  }
  int lenx = vopts->lenx;
  int leny = vopts->leny;
  int begy = vopts->begy;
  int begx = vopts->begx;
//fprintf(stderr, "render %dx%d+%dx%d %p\n", begy, begx, leny, lenx, ncv->data);
  if(begy < 0 || begx < 0 || lenx < -1 || leny < -1){
    return -1;
  }
  const bool degrade = !(vopts->flags & NCVISUAL_OPTION_NODEGRADE);
  const struct blitset* bset = lookup_blitset(notcurses_canutf8(nc->nc),
                                              vopts->blitter, degrade);
  if(bset == NULL){
    return -1;
  }
  const bool blend = (vopts->flags & NCVISUAL_OPTION_BLEND);
  // FIXME swap bytes in input
  return bset->blit(nc, vopts->y, vopts->x, linesize, data, begy, begx,
                    leny, lenx, blend);
}

int ncblit_rgba(const void* data, int linesize, const struct ncvisual_options* vopts){
  if(vopts->flags > NCVISUAL_OPTION_BLEND){
    return -1;
  }
  struct ncplane* nc = vopts->n;
  if(nc == NULL){
    return -1;
  }
  int lenx = vopts->lenx;
  int leny = vopts->leny;
  int begy = vopts->begy;
  int begx = vopts->begx;
//fprintf(stderr, "render %dx%d+%dx%d %p\n", begy, begx, leny, lenx, ncv->data);
  if(begy < 0 || begx < 0 || lenx < -1 || leny < -1){
    return -1;
  }
  const bool degrade = !(vopts->flags & NCVISUAL_OPTION_NODEGRADE);
  const struct blitset* bset = lookup_blitset(notcurses_canutf8(nc->nc),
                                              vopts->blitter, degrade);
  if(bset == NULL){
    return -1;
  }
  const bool blend = (vopts->flags & NCVISUAL_OPTION_BLEND);
  return bset->blit(nc, vopts->y, vopts->x, linesize, data, begy, begx,
                    leny, lenx, blend);
}

int rgba_blit_dispatch(ncplane* nc, const struct blitset* bset, int placey,
                       int placex, int linesize, const void* data, int begy,
                       int begx, int leny, int lenx, bool blendcolors){
  return bset->blit(nc, placey, placex, linesize, data, begy, begx,
                    leny, lenx, blendcolors);
}
