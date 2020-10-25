#include <stddef.h>
#include "internal.h"

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
ffmpeg_trans_p(bool bgr, unsigned char alpha){
  if(!bgr && alpha < 192){
//fprintf(stderr, "TRANSPARENT!\n");
    return true;
  }
  return false;
}

// Retarded RGBA/BGRx blitter (ASCII only).
// For incoming BGRx (no transparency), bgr == true.
static inline int
tria_blit_ascii(ncplane* nc, int placey, int placex, int linesize,
                const void* data, int begy, int begx,
                int leny, int lenx, bool bgr, bool blendcolors){
//fprintf(stderr, "ASCII %d X %d @ %d X %d (%p) place: %d X %d\n", leny, lenx, begy, begx, data, placey, placex);
  const int bpp = 32;
  const int rpos = bgr ? 2 : 0;
  const int bpos = bgr ? 0 : 2;
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
      if(ffmpeg_trans_p(bgr, rgbbase_up[3])){
        cell_set_bg_alpha(c, CELL_ALPHA_TRANSPARENT);
        cell_set_fg_alpha(c, CELL_ALPHA_TRANSPARENT);
      }else{
        cell_set_fg_rgb8(c, rgbbase_up[rpos], rgbbase_up[1], rgbbase_up[bpos]);
        cell_set_bg_rgb8(c, rgbbase_up[rpos], rgbbase_up[1], rgbbase_up[bpos]);
        if(pool_load_direct(&nc->pool, c, " ", 1, 1) <= 0){
          return -1;
        }
      }
      ++total;
    }
  }
  return total;
}

// RGBA/BGRx half-block blitter. Best for most images/videos. Full fidelity
// combined with 1:1 pixel aspect ratio.
// For incoming BGRx (no transparency), bgr == true.
static inline int
tria_blit(ncplane* nc, int placey, int placex, int linesize,
          const void* data, int begy, int begx,
          int leny, int lenx, bool bgr, bool blendcolors){
//fprintf(stderr, "HALF %d X %d @ %d X %d (%p) place: %d X %d\n", leny, lenx, begy, begx, data, placey, placex);
  const int bpp = 32;
  const int rpos = bgr ? 2 : 0;
  const int bpos = bgr ? 0 : 2;
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
      if(ffmpeg_trans_p(bgr, rgbbase_up[3]) || ffmpeg_trans_p(bgr, rgbbase_down[3])){
        cell_set_bg_alpha(c, CELL_ALPHA_TRANSPARENT);
        if(ffmpeg_trans_p(bgr, rgbbase_up[3]) && ffmpeg_trans_p(bgr, rgbbase_down[3])){
          cell_set_fg_alpha(c, CELL_ALPHA_TRANSPARENT);
        }else if(ffmpeg_trans_p(bgr, rgbbase_up[3])){ // down has the color
          if(cell_load(nc, c, "\u2584") <= 0){ // lower half block
            return -1;
          }
          cell_set_fg_rgb8(c, rgbbase_down[rpos], rgbbase_down[1], rgbbase_down[bpos]);
        }else{ // up has the color
          if(cell_load(nc, c, "\u2580") <= 0){ // upper half block
            return -1;
          }
          cell_set_fg_rgb8(c, rgbbase_up[rpos], rgbbase_up[1], rgbbase_up[bpos]);
        }
      }else{
        if(memcmp(rgbbase_up, rgbbase_down, 3) == 0){
          cell_set_fg_rgb8(c, rgbbase_down[rpos], rgbbase_down[1], rgbbase_down[bpos]);
          cell_set_bg_rgb8(c, rgbbase_down[rpos], rgbbase_down[1], rgbbase_down[bpos]);
          if(cell_load(nc, c, " ") <= 0){ // only need the background
            return -1;
          }
        }else{
          cell_set_fg_rgb8(c, rgbbase_up[rpos], rgbbase_up[1], rgbbase_up[bpos]);
          cell_set_bg_rgb8(c, rgbbase_down[rpos], rgbbase_down[1], rgbbase_down[bpos]);
          if(cell_load(nc, c, "\u2580") <= 0){ // upper half block
            return -1;
          }
        }
      }
      ++total;
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
static inline const char*
qtrans_check(cell* c, bool bgr, bool blendcolors,
             const unsigned char* rgbbase_tl, const unsigned char* rgbbase_tr,
             const unsigned char* rgbbase_bl, const unsigned char* rgbbase_br){
  const int rpos = bgr ? 2 : 0;
  const int bpos = bgr ? 0 : 2;
  uint32_t tl = 0, tr = 0, bl = 0, br = 0;
  channel_set_rgb8(&tl, rgbbase_tl[rpos], rgbbase_tl[1], rgbbase_tl[bpos]);
  channel_set_rgb8(&tr, rgbbase_tr[rpos], rgbbase_tr[1], rgbbase_tr[bpos]);
  channel_set_rgb8(&bl, rgbbase_bl[rpos], rgbbase_bl[1], rgbbase_bl[bpos]);
  channel_set_rgb8(&br, rgbbase_br[rpos], rgbbase_br[1], rgbbase_br[bpos]);
  const char* egc = NULL;
  if(ffmpeg_trans_p(bgr, rgbbase_tl[3])){
    // top left is transparent
    if(ffmpeg_trans_p(bgr, rgbbase_tr[3])){
      // all of top is transparent
      if(ffmpeg_trans_p(bgr, rgbbase_bl[3])){
        // top and left are transparent
        if(ffmpeg_trans_p(bgr, rgbbase_br[3])){
          // entirety is transparent, load with nul (but not NULL)
          cell_set_fg_default(c);
          egc = "";
        }else{
          cell_set_fg_rgb8(c, rgbbase_br[rpos], rgbbase_br[1], rgbbase_br[bpos]);
          egc = "▗";
        }
      }else{
        if(ffmpeg_trans_p(bgr, rgbbase_br[3])){
          cell_set_fg_rgb8(c, rgbbase_bl[rpos], rgbbase_bl[1], rgbbase_bl[bpos]);
          egc = "▖";
        }else{
          cell_set_fchannel(c, lerp(bl, br));
          egc = "▄";
        }
      }
    }else{ // top right is foreground, top left is transparent
      if(ffmpeg_trans_p(bgr, rgbbase_bl[3])){
        if(ffmpeg_trans_p(bgr, rgbbase_br[3])){ // entire bottom is transparent
          cell_set_fg_rgb8(c, rgbbase_tr[rpos], rgbbase_tr[1], rgbbase_tr[bpos]);
          egc = "▝";
        }else{
          cell_set_fchannel(c, lerp(tr, br));
          egc = "▐";
        }
      }else if(ffmpeg_trans_p(bgr, rgbbase_br[3])){ // only br is transparent
        cell_set_fchannel(c, lerp(tr, bl));
        egc = "▞";
      }else{
        cell_set_fchannel(c, trilerp(tr, bl, br));
        egc = "▟";
      }
    }
  }else{ // topleft is foreground for all here
    if(ffmpeg_trans_p(bgr, rgbbase_tr[3])){
      if(ffmpeg_trans_p(bgr, rgbbase_bl[3])){
        if(ffmpeg_trans_p(bgr, rgbbase_br[3])){
          cell_set_fg_rgb8(c, rgbbase_tl[rpos], rgbbase_tl[1], rgbbase_tl[bpos]);
          egc = "▘";
        }else{
          cell_set_fchannel(c, lerp(tl, br));
          egc = "▚";
        }
      }else if(ffmpeg_trans_p(bgr, rgbbase_br[3])){
        cell_set_fchannel(c, lerp(tl, bl));
        egc = "▌";
      }else{
        cell_set_fchannel(c, trilerp(tl, bl, br));
        egc = "▙";
      }
    }else if(ffmpeg_trans_p(bgr, rgbbase_bl[3])){
      if(ffmpeg_trans_p(bgr, rgbbase_br[3])){ // entire bottom is transparent
        cell_set_fchannel(c, lerp(tl, tr));
        egc = "▀";
      }else{ // only bl is transparent
        cell_set_fchannel(c, trilerp(tl, tr, br));
        egc = "▜";
      }
    }else if(ffmpeg_trans_p(bgr, rgbbase_br[3])){ // only br is transparent
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
              int leny, int lenx, bool bgr, bool blendcolors){
  const int bpp = 32;
  const int rpos = bgr ? 2 : 0;
  const int bpos = bgr ? 0 : 2;
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
      const char* egc = qtrans_check(c, bgr, blendcolors, rgbbase_tl, rgbbase_tr, rgbbase_bl, rgbbase_br);
      if(egc == NULL){
        uint32_t tl = 0, tr = 0, bl = 0, br = 0;
        channel_set_rgb8(&tl, rgbbase_tl[rpos], rgbbase_tl[1], rgbbase_tl[bpos]);
        channel_set_rgb8(&tr, rgbbase_tr[rpos], rgbbase_tr[1], rgbbase_tr[bpos]);
        channel_set_rgb8(&bl, rgbbase_bl[rpos], rgbbase_bl[1], rgbbase_bl[bpos]);
        channel_set_rgb8(&br, rgbbase_br[rpos], rgbbase_br[1], rgbbase_br[bpos]);
        uint32_t bg, fg;
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
      if(*egc && pool_blit_direct(&nc->pool, c, egc, strlen(egc), 1) <= 0){
        return -1;
      }
      ++total;
    }
  }
  return total;
}

// quadrant blitter. maps 2x2 to each cell. since we only have two colors at
// our disposal (foreground and background), we lose some fidelity.
static inline int
sextant_blit(ncplane* nc, int placey, int placex, int linesize,
             const void* data, int begy, int begx,
             int leny, int lenx, bool bgr, bool blendcolors){
  return 0;
}

// fold the r, g, and b components of the pixel into *r, *g, and *b, and
// increment *foldcount
static inline void
fold_rgb8(unsigned* restrict r, unsigned* restrict g, unsigned* restrict b,
         bool bgr, const uint8_t* pixel, unsigned* foldcount){
  *r += bgr ? pixel[2] : pixel[0];
  *g += pixel[1];
  *b += bgr ? pixel[0] : pixel[2];
  ++*foldcount;
}

// Braille blitter. maps 4x2 to each cell. since we only have one color at
// our disposal (foreground), we lose some fidelity. this is optimal for
// visuals with only two colors in a given area, as it packs lots of
// resolution. always transparent background.
static inline int
braille_blit(ncplane* nc, int placey, int placex, int linesize,
             const void* data, int begy, int begx,
             int leny, int lenx, bool bgr, bool blendcolors){
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
      const unsigned char* rgbbase_l0 = dat + (linesize * visy) + (visx * bpp / CHAR_BIT);
      const unsigned char* rgbbase_r0 = zeroes;
      const unsigned char* rgbbase_l1 = zeroes;
      const unsigned char* rgbbase_r1 = zeroes;
      const unsigned char* rgbbase_l2 = zeroes;
      const unsigned char* rgbbase_r2 = zeroes;
      const unsigned char* rgbbase_l3 = zeroes;
      const unsigned char* rgbbase_r3 = zeroes;
      unsigned r = 0, g = 0, b = 0;
      unsigned blends = 0;
      unsigned egcidx = 0;
      if(visx < begx + lenx - 1){
        rgbbase_r0 = dat + (linesize * visy) + ((visx + 1) * bpp / CHAR_BIT);
        if(visy < begy + leny - 1){
          rgbbase_r1 = dat + (linesize * (visy + 1)) + ((visx + 1) * bpp / CHAR_BIT);
          if(visy < begy + leny - 2){
            rgbbase_r2 = dat + (linesize * (visy + 2)) + ((visx + 1) * bpp / CHAR_BIT);
            if(visy < begy + leny - 3){
              rgbbase_r3 = dat + (linesize * (visy + 3)) + ((visx + 1) * bpp / CHAR_BIT);
            }
          }
        }
      }
      if(visy < begy + leny - 1){
        rgbbase_l1 = dat + (linesize * (visy + 1)) + (visx * bpp / CHAR_BIT);
        if(visy < begy + leny - 2){
          rgbbase_l2 = dat + (linesize * (visy + 2)) + (visx * bpp / CHAR_BIT);
          if(visy < begy + leny - 3){
            rgbbase_l3 = dat + (linesize * (visy + 3)) + (visx * bpp / CHAR_BIT);
          }
        }
      }
      // FIXME fold this into the above?
      if(!ffmpeg_trans_p(bgr, rgbbase_l0[3])){
        egcidx |= 1u;
        fold_rgb8(&r, &g, &b, bgr, rgbbase_l0, &blends);
      }
      if(!ffmpeg_trans_p(bgr, rgbbase_l1[3])){
        egcidx |= 2u;
        fold_rgb8(&r, &g, &b, bgr, rgbbase_l1, &blends);
      }
      if(!ffmpeg_trans_p(bgr, rgbbase_l2[3])){
        egcidx |= 4u;
        fold_rgb8(&r, &g, &b, bgr, rgbbase_l2, &blends);
      }
      if(!ffmpeg_trans_p(bgr, rgbbase_r0[3])){
        egcidx |= 8u;
        fold_rgb8(&r, &g, &b, bgr, rgbbase_r0, &blends);
      }
      if(!ffmpeg_trans_p(bgr, rgbbase_r1[3])){
        egcidx |= 16u;
        fold_rgb8(&r, &g, &b, bgr, rgbbase_r1, &blends);
      }
      if(!ffmpeg_trans_p(bgr, rgbbase_r2[3])){
        egcidx |= 32u;
        fold_rgb8(&r, &g, &b, bgr, rgbbase_r2, &blends);
      }
      if(!ffmpeg_trans_p(bgr, rgbbase_l3[3])){
        egcidx |= 64u;
        fold_rgb8(&r, &g, &b, bgr, rgbbase_l3, &blends);
      }
      if(!ffmpeg_trans_p(bgr, rgbbase_r3[3])){
        egcidx |= 128u;
        fold_rgb8(&r, &g, &b, bgr, rgbbase_r3, &blends);
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
      if(ffmpeg_trans_p(bgr, rgbbase_l0[3]) && ffmpeg_trans_p(bgr, rgbbase_r0[3])
          && ffmpeg_trans_p(bgr, rgbbase_l1[3]) && ffmpeg_trans_p(bgr, rgbbase_r1[3])
          && ffmpeg_trans_p(bgr, rgbbase_l2[3]) && ffmpeg_trans_p(bgr, rgbbase_r2[3])
          && ffmpeg_trans_p(bgr, rgbbase_l3[3]) && ffmpeg_trans_p(bgr, rgbbase_r3[3])){
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
     .blit = sextant_blit,   .name = "hexblitter",   .fill = false, },
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
  return bset->blit(nc, vopts->y, vopts->x, linesize, data, begy, begx,
                    leny, lenx, true, blend);
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
                    leny, lenx, false, blend);
}

int rgba_blit_dispatch(ncplane* nc, const struct blitset* bset, int placey,
                       int placex, int linesize, const void* data, int begy,
                       int begx, int leny, int lenx, bool blendcolors){
  return bset->blit(nc, placey, placex, linesize, data, begy, begx,
                    leny, lenx, false, blendcolors);
}
