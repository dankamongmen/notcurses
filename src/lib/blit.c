#include "internal.h"

static const unsigned char zeroes[] = "\x00\x00\x00\x00";

// linearly interpolate a 24-bit RGB value along each 8-bit channel
static inline uint32_t
lerp(uint32_t c0, uint32_t c1){
  uint32_t ret = 0;
  unsigned r0, g0, b0, r1, g1, b1;
  channel_rgb(c0, &r0, &g0, &b0);
  channel_rgb(c1, &r1, &g1, &b1);
  channel_set_rgb(&ret, (r0 + r1 + 1) / 2, (g0 + g1 + 1) / 2, (b0 + b1 + 1) / 2);
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
      c->attrword = 0;
      if(blendcolors){
        cell_set_bg_alpha(c, CELL_ALPHA_BLEND);
        cell_set_fg_alpha(c, CELL_ALPHA_BLEND);
      }
      if(ffmpeg_trans_p(bgr, rgbbase_up[3])){
        cell_set_bg_alpha(c, CELL_ALPHA_TRANSPARENT);
        cell_set_fg_alpha(c, CELL_ALPHA_TRANSPARENT);
      }else{
        cell_set_fg_rgb(c, rgbbase_up[rpos], rgbbase_up[1], rgbbase_up[bpos]);
        cell_set_bg_rgb(c, rgbbase_up[rpos], rgbbase_up[1], rgbbase_up[bpos]);
        if(cell_load(nc, c, " ") <= 0){
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
      c->attrword = 0;
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
          cell_set_fg_rgb(c, rgbbase_down[rpos], rgbbase_down[1], rgbbase_down[bpos]);
        }else{ // up has the color
          if(cell_load(nc, c, "\u2580") <= 0){ // upper half block
            return -1;
          }
          cell_set_fg_rgb(c, rgbbase_up[rpos], rgbbase_up[1], rgbbase_up[bpos]);
        }
      }else{
        if(memcmp(rgbbase_up, rgbbase_down, 3) == 0){
          cell_set_fg_rgb(c, rgbbase_down[rpos], rgbbase_down[1], rgbbase_down[bpos]);
          cell_set_bg_rgb(c, rgbbase_down[rpos], rgbbase_down[1], rgbbase_down[bpos]);
          if(cell_load(nc, c, " ") <= 0){ // only need the background
            return -1;
          }
        }else{
          cell_set_fg_rgb(c, rgbbase_up[rpos], rgbbase_up[1], rgbbase_up[bpos]);
          cell_set_bg_rgb(c, rgbbase_down[rpos], rgbbase_down[1], rgbbase_down[bpos]);
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
// colors, and determine whether
static const struct qdriver {
  int pair[2];      // indices of contributing pair
  int others[2];    // indices of excluded pair
  const char* egc;  // EGC corresponding to contributing pair
  const char* oth0egc; // EGC upon absorbing others[0]
  const char* oth1egc; // EGC upon absorbing others[1]
} quadrant_drivers[6] = {
  { .pair = { 0, 1 }, .others = { 2, 3 }, .egc = "▀", .oth0egc = "▛", .oth1egc = "▜", },
  { .pair = { 0, 2 }, .others = { 1, 3 }, .egc = "▋", .oth0egc = "▛", .oth1egc = "▙", },
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
  channel_rgb(tl, &colors[0].r, &colors[0].g, &colors[0].b);
  channel_rgb(tr, &colors[1].r, &colors[1].g, &colors[1].b);
  channel_rgb(bl, &colors[2].r, &colors[2].g, &colors[2].b);
  channel_rgb(br, &colors[3].r, &colors[3].g, &colors[3].b);
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
  uint32_t diffs[sizeof(quadrant_drivers) / sizeof(*quadrant_drivers)];
  rgb_4diff(diffs, tl, tr, bl, br);
  // compiler can't verify that we'll always be less than 769 somewhere,
  // so fuck it, just go ahead and initialize to 0 / diffs[0]
  size_t mindiffidx = 0;
  unsigned mindiff = diffs[0]; // 3 * 256 + 1; // max distance is 256 * 3
  for(size_t idx = 1 ; idx < sizeof(diffs) / sizeof(*diffs) ; ++idx){
    if(diffs[idx] < mindiff){
      mindiffidx = idx;
      mindiff = diffs[idx];
    }
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
  const char* egc = qd->egc;
  // break down the excluded pair and lerp
  unsigned r0, r1, g0, g1, b0, b1;
  unsigned roth, goth, both, rlerp, glerp, blerp;
  channel_rgb(qd->others[0], &r0, &g0, &b0);
  channel_rgb(qd->others[1], &r1, &g1, &b1);
  channel_rgb(*fore, &rlerp, &glerp, &blerp);
  channel_rgb(*back, &roth, &goth, &both);
  diffs[0] = rgb_diff(r0, g0, b0, rlerp, glerp, blerp);
  diffs[1] = rgb_diff(r0, g0, b0, roth, goth, both);
  diffs[2] = rgb_diff(r1, g1, b1, rlerp, glerp, blerp);
  diffs[3] = rgb_diff(r1, g1, b1, roth, goth, both);
  if(diffs[0] < diffs[1]){
    egc = qd->oth0egc;
    *back = colors[qd->others[1]];
    // FIXME relerp *fore?
  }else if(diffs[2] < diffs[3]){
    egc = qd->oth1egc;
    *back = colors[qd->others[0]];
    // FIXME relerp *fore?
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
//fprintf(stderr, "[%04d/%04d] bpp: %d lsize: %d %02x %02x %02x %02x\n", y, x, bpp, linesize, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2], rgbbase_up[3]);
      cell* c = ncplane_cell_ref_yx(nc, y, x);
      c->channels = 0;
      c->attrword = 0;
      // FIXME for now, we're only transparent if all four are transparent. we ought
      // match transparent like anything else...
      const char* egc = NULL;
      if(ffmpeg_trans_p(bgr, rgbbase_tl[3]) && ffmpeg_trans_p(bgr, rgbbase_tr[3])
          && ffmpeg_trans_p(bgr, rgbbase_bl[3]) && ffmpeg_trans_p(bgr, rgbbase_br[3])){
          cell_set_bg_alpha(c, CELL_ALPHA_TRANSPARENT);
          cell_set_fg_alpha(c, CELL_ALPHA_TRANSPARENT);
          egc = " ";
          // FIXME else look for pairs of transparency!
      }else{
        uint32_t tl = 0, tr = 0, bl = 0, br = 0;
        channel_set_rgb(&tl, rgbbase_tl[rpos], rgbbase_tl[1], rgbbase_tl[bpos]);
        channel_set_rgb(&tr, rgbbase_tr[rpos], rgbbase_tr[1], rgbbase_tr[bpos]);
        channel_set_rgb(&bl, rgbbase_bl[rpos], rgbbase_bl[1], rgbbase_bl[bpos]);
        channel_set_rgb(&br, rgbbase_br[rpos], rgbbase_br[1], rgbbase_br[bpos]);
        uint32_t bg, fg;
        egc = quadrant_solver(tl, tr, bl, br, &fg, &bg);
        cell_set_fchannel(c, fg);
        cell_set_bchannel(c, bg);
        if(blendcolors){
          cell_set_bg_alpha(c, CELL_ALPHA_BLEND);
          cell_set_fg_alpha(c, CELL_ALPHA_BLEND);
        }
      }
      assert(egc);
      if(cell_load(nc, c, egc) <= 0){
        return -1;
      }
      ++total;
    }
  }
  return total;
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
  const int rpos = bgr ? 2 : 0;
  const int bpos = bgr ? 0 : 2;
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
//fprintf(stderr, "[%04d/%04d] bpp: %d lsize: %d %02x %02x %02x %02x\n", y, x, bpp, linesize, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2], rgbbase_up[3]);
      cell* c = ncplane_cell_ref_yx(nc, y, x);
      // use the default for the background, as that's the only way it's
      // effective in that case anyway
      c->channels = 0;
      c->attrword = 0;
      if(blendcolors){
        cell_set_fg_alpha(c, CELL_ALPHA_BLEND);
      }
      // FIXME for now, we just sample, color-wise, and always draw crap.
      // more complicated to do optimally than quadrants, for sure. ideally,
      // we only get one color in an area.
      cell_set_bg_alpha(c, CELL_ALPHA_TRANSPARENT);
      const char* egc = NULL;
      if(ffmpeg_trans_p(bgr, rgbbase_l0[3]) && ffmpeg_trans_p(bgr, rgbbase_r0[3])
          && ffmpeg_trans_p(bgr, rgbbase_l1[3]) && ffmpeg_trans_p(bgr, rgbbase_r1[3])
          && ffmpeg_trans_p(bgr, rgbbase_l2[3]) && ffmpeg_trans_p(bgr, rgbbase_r2[3])
          && ffmpeg_trans_p(bgr, rgbbase_l3[3]) && ffmpeg_trans_p(bgr, rgbbase_r3[3])){
          cell_set_fg_alpha(c, CELL_ALPHA_TRANSPARENT);
          egc = " ";
          // FIXME else look for pairs of transparency!
      }else{
        // FIXME interpolate into 1
        cell_set_fg_rgb(c, rgbbase_l0[rpos], rgbbase_l0[1], rgbbase_l0[bpos]);
        egc = "⡜";
      }
      assert(egc);
      if(cell_load(nc, c, egc) <= 0){
        return -1;
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
     .blit = NULL,           .fill = false, },
   { .geom = NCBLIT_1x1,     .width = 1, .height = 1, .egcs = L" █",
     .blit = tria_blit_ascii,.fill = false, },
   { .geom = NCBLIT_2x1,     .width = 1, .height = 2, .egcs = L" ▄█",
     .blit = tria_blit,      .fill = false, },
   { .geom = NCBLIT_1x1x4,   .width = 1, .height = 4, .egcs = L" ▒░▓█",
     .blit = NULL,           .fill = false, },
   { .geom = NCBLIT_2x2,     .width = 2, .height = 2, .egcs = L" ▗▐▖▄▟▌▙█",
     .blit = quadrant_blit,  .fill = false, },
   { .geom = NCBLIT_4x1,     .width = 1, .height = 4, .egcs = L" ▂▄▆█",
     .blit = NULL,           .fill = false, },
   { .geom = NCBLIT_BRAILLE, .width = 2, .height = 4, .egcs = L"⠀⡀⡄⡆⡇⢀⣀⣄⣆⣇⢠⣠⣤⣦⣧⢰⣰⣴⣶⣷⢸⣸⣼⣾⣿",
     .blit = braille_blit,   .fill = true,  },
   { .geom = NCBLIT_SIXEL,   .width = 1, .height = 6, .egcs = L"",
     .blit = NULL,           .fill = true,  },
   { .geom = 0,              .width = 0, .height = 0, .egcs = NULL,
     .blit = NULL,           .fill = false,  },
};

// Blit a flat array 'data' of BGRx 32-bit values to the ncplane 'nc', offset
// from the upper left by 'placey' and 'placex'. Each row ought occupy
// 'linesize' bytes (this might be greater than lenx * 4 due to padding). A
// subregion of the input can be specified with 'begy'x'begx' and 'leny'x'lenx'.
int ncblit_bgrx(ncplane* nc, int placey, int placex,
                int linesize, const void* data, int begy, int begx, int leny,
                int lenx){
  if(!nc->nc->utf8){
    return tria_blit_ascii(nc, placey, placex, linesize, data, begy, begx,
                           leny, lenx, true, false);
  }
  return tria_blit(nc, placey, placex, linesize, data, begy, begx,
                   leny, lenx, true, false);
}

int ncblit_rgba(ncplane* nc, int placey, int placex,
                int linesize, const void* data, int begy, int begx, int leny,
                int lenx){
  if(!nc->nc->utf8){
    return tria_blit_ascii(nc, placey, placex, linesize, data, begy, begx,
                           leny, lenx, false, false);
  }
  return tria_blit(nc, placey, placex, linesize, data, begy, begx,
                   leny, lenx, false, false);
}

int rgba_blit_dispatch(ncplane* nc, const struct blitset* bset, int placey,
                       int placex, int linesize, const void* data, int begy,
                       int begx, int leny, int lenx, bool blendcolors){
  return bset->blit(nc, placey, placex, linesize, data, begy, begx,
                    leny, lenx, false, blendcolors);
}
