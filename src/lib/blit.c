#include <stddef.h>
#include <inttypes.h>
#include "internal.h"

static const uint32_t zeroes32;
static const unsigned char zeroes[] = "\x00\x00\x00\x00";

// linearly interpolate a 24-bit RGB value along each 8-bit channel
static inline uint32_t
lerp(uint32_t c0, uint32_t c1, unsigned nointerpolate){
  unsigned r0, g0, b0, r1, g1, b1;
  uint32_t ret = 0;
  ncchannel_rgb8(c0, &r0, &g0, &b0);
  if(!nointerpolate){
    ncchannel_rgb8(c1, &r1, &g1, &b1);
    ncchannel_set_rgb8(&ret, (r0 + r1 + 1) / 2,
                          (g0 + g1 + 1) / 2,
                          (b0 + b1 + 1) / 2);
  }else{
    ncchannel_set_rgb8(&ret, r0, g0, b0);
  }
  return ret;
}

// trilinearly interpolate a 24-bit RGB value along each 8-bit channel
static inline uint32_t
trilerp(uint32_t c0, uint32_t c1, uint32_t c2, unsigned nointerpolate){
  uint32_t ret = 0;
  unsigned r0, g0, b0, r1, g1, b1, r2, g2, b2;
  ncchannel_rgb8(c0, &r0, &g0, &b0);
  if(!nointerpolate){
    ncchannel_rgb8(c1, &r1, &g1, &b1);
    ncchannel_rgb8(c2, &r2, &g2, &b2);
    ncchannel_set_rgb8(&ret, (r0 + r1 + r2 + 2) / 3,
                          (g0 + g1 + g2 + 2) / 3,
                          (b0 + b1 + b2 + 2) / 3);
  }else{
    ncchannel_set_rgb8(&ret, r0, g0, b0);
  }
  return ret;
}

// take a sum over channels, and the sample count, write back lerped channel
static inline uint32_t
generalerp(unsigned rsum, unsigned gsum, unsigned bsum, int count){
  if(count == 0){
    assert(0 == rsum);
    assert(0 == gsum);
    assert(0 == bsum);
    return 0;
  }
  return NCCHANNEL_INITIALIZER((rsum + (count - 1)) / count,
                               (gsum + (count - 1)) / count,
                               (bsum + (count - 1)) / count);
}

static inline unsigned
rgba_trans_q(const unsigned char* p, uint32_t transcolor){
  uint32_t q;
  memcpy(&q, p, sizeof(q));
  return rgba_trans_p(q, transcolor);
}

// Retarded RGBA blitter (ASCII only).
static inline int
tria_blit_ascii(ncplane* nc, int linesize, const void* data,
                int leny, int lenx, const blitterargs* bargs){
//fprintf(stderr, "ASCII %d X %d @ %d X %d (%p) place: %d X %d\n", leny, lenx, bargs->begy, bargs->begx, data, bargs->u.cell.placey, bargs->u.cell.placex);
  const bool blendcolors = bargs->flags & NCVISUAL_OPTION_BLEND;
  unsigned dimy, dimx, x, y;
  int total = 0; // number of cells written
  ncplane_dim_yx(nc, &dimy, &dimx);
  // FIXME not going to necessarily be safe on all architectures hrmmm
  const unsigned char* dat = data;
  int visy = bargs->begy;
  for(y = bargs->u.cell.placey ; visy < (bargs->begy + leny) && y < dimy ; ++y, ++visy){
    if(ncplane_cursor_move_yx(nc, y, bargs->u.cell.placex < 0 ? 0 : bargs->u.cell.placex)){
      return -1;
    }
    int visx = bargs->begx;
    for(x = bargs->u.cell.placex ; visx < (bargs->begx + lenx) && x < dimx ; ++x, ++visx){
      const unsigned char* rgbbase_up = dat + (linesize * visy) + (visx * 4);
//fprintf(stderr, "[%04d/%04d] lsize: %d %02x %02x %02x %02x\n", y, x, linesize, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2], rgbbase_up[3]);
      nccell* c = ncplane_cell_ref_yx(nc, y, x);
      // use the default for the background, as that's the only way it's
      // effective in that case anyway
      c->channels = 0;
      c->stylemask = 0;
      if(blendcolors){
        nccell_set_bg_alpha(c, NCALPHA_BLEND);
        nccell_set_fg_alpha(c, NCALPHA_BLEND);
      }
      if(rgba_trans_q(rgbbase_up, bargs->transcolor)){
        nccell_set_bg_alpha(c, NCALPHA_TRANSPARENT);
        nccell_set_fg_alpha(c, NCALPHA_TRANSPARENT);
        cell_set_blitquadrants(c, 0, 0, 0, 0);
        nccell_release(nc, c);
      }else{
        nccell_set_fg_rgb8(c, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2]);
        nccell_set_bg_rgb8(c, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2]);
        cell_set_blitquadrants(c, 1, 1, 1, 1);
        if(pool_blit_direct(&nc->pool, c, " ", 1, 1) <= 0){
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
tria_blit(ncplane* nc, int linesize, const void* data, int leny, int lenx,
          const blitterargs* bargs){
  const bool blendcolors = bargs->flags & NCVISUAL_OPTION_BLEND;
//fprintf(stderr, "HALF %d X %d @ %d X %d (%p) place: %d X %d\n", leny, lenx, bargs->begy, bargs->begx, data, bargs->u.cell.placey, bargs->u.cell.placex);
  uint32_t transcolor = bargs->transcolor;
  unsigned dimy, dimx, x, y;
  int total = 0; // number of cells written
  ncplane_dim_yx(nc, &dimy, &dimx);
  // FIXME not going to necessarily be safe on all architectures hrmmm
  const unsigned char* dat = data;
  int visy = bargs->begy;
  for(y = bargs->u.cell.placey ; visy < (bargs->begy + leny) && y < dimy ; ++y, visy += 2){
    if(ncplane_cursor_move_yx(nc, y, bargs->u.cell.placex < 0 ? 0 : bargs->u.cell.placex)){
      return -1;
    }
    int visx = bargs->begx;
    for(x = bargs->u.cell.placex ; visx < (bargs->begx + lenx) && x < dimx ; ++x, ++visx){
      const unsigned char* rgbbase_up = dat + (linesize * visy) + (visx * 4);
      const unsigned char* rgbbase_down = zeroes;
      if(visy < bargs->begy + leny - 1){
        rgbbase_down = dat + (linesize * (visy + 1)) + (visx * 4);
      }
//fprintf(stderr, "[%04d/%04d] lsize: %d %02x %02x %02x %02x\n", y, x, linesize, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2], rgbbase_up[3]);
      nccell* c = ncplane_cell_ref_yx(nc, y, x);
      // use the default for the background, as that's the only way it's
      // effective in that case anyway
      c->channels = 0;
      c->stylemask = 0;
      if(blendcolors){
        nccell_set_bg_alpha(c, NCALPHA_BLEND);
        nccell_set_fg_alpha(c, NCALPHA_BLEND);
      }
      if(rgba_trans_q(rgbbase_up, transcolor) || rgba_trans_q(rgbbase_down, transcolor)){
        nccell_set_bg_alpha(c, NCALPHA_TRANSPARENT);
        if(rgba_trans_q(rgbbase_up, transcolor) && rgba_trans_q(rgbbase_down, transcolor)){
          nccell_set_fg_alpha(c, NCALPHA_TRANSPARENT);
          nccell_release(nc, c);
        }else if(rgba_trans_q(rgbbase_up, transcolor)){ // down has the color
          if(pool_blit_direct(&nc->pool, c, "\u2584", strlen("\u2584"), 1) <= 0){
            return -1;
          }
          nccell_set_fg_rgb8(c, rgbbase_down[0], rgbbase_down[1], rgbbase_down[2]);
          cell_set_blitquadrants(c, 0, 0, 1, 1);
          ++total;
        }else{ // up has the color
          // upper half block
          if(pool_blit_direct(&nc->pool, c, "\u2580", strlen("\u2580"), 1) <= 0){
            return -1;
          }
          nccell_set_fg_rgb8(c, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2]);
          cell_set_blitquadrants(c, 1, 1, 0, 0);
          ++total;
        }
      }else{
        if(memcmp(rgbbase_up, rgbbase_down, 3) == 0){
          nccell_set_fg_rgb8(c, rgbbase_down[0], rgbbase_down[1], rgbbase_down[2]);
          nccell_set_bg_rgb8(c, rgbbase_down[0], rgbbase_down[1], rgbbase_down[2]);
          cell_set_blitquadrants(c, 0, 0, 0, 0);
          if(pool_blit_direct(&nc->pool, c, " ", 1, 1) <= 0){
            return -1;
          }
        }else{
          nccell_set_fg_rgb8(c, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2]);
          nccell_set_bg_rgb8(c, rgbbase_down[0], rgbbase_down[1], rgbbase_down[2]);
          cell_set_blitquadrants(c, 1, 1, 1, 1);
          if(pool_blit_direct(&nc->pool, c, "\u2580", strlen("\u2580"), 1) <= 0){
            return -1;
          }
        }
        ++total;
      }
    }
  }
  return total;
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
  ncchannel_rgb8(tl, &colors[0].r, &colors[0].g, &colors[0].b);
  ncchannel_rgb8(tr, &colors[1].r, &colors[1].g, &colors[1].b);
  ncchannel_rgb8(bl, &colors[2].r, &colors[2].g, &colors[2].b);
  ncchannel_rgb8(br, &colors[3].r, &colors[3].g, &colors[3].b);
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
                uint32_t* fore, uint32_t* back, unsigned nointerpolate){
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
  *fore = lerp(colors[qd->pair[0]], colors[qd->pair[1]], nointerpolate);
  *back = lerp(colors[qd->others[0]], colors[qd->others[1]], nointerpolate);
//fprintf(stderr, "mindiff: %u[%zu] fore: %08x back: %08x %d+%d/%d+%d\n", mindiff, mindiffidx, *fore, *back, qd->pair[0], qd->pair[1], qd->others[0], qd->others[1]);
  const char* egc = qd->egc;
  // break down the excluded pair and lerp
  unsigned r0, r1, r2, g0, g1, g2, b0, b1, b2;
  unsigned roth, goth, both, rlerp, glerp, blerp;
  ncchannel_rgb8(*back, &roth, &goth, &both);
  ncchannel_rgb8(*fore, &rlerp, &glerp, &blerp);
//fprintf(stderr, "rgbs: %02x %02x %02x / %02x %02x %02x\n", r0, g0, b0, r1, g1, b1);
  // get diffs of the excluded two from both lerps
  ncchannel_rgb8(colors[qd->others[0]], &r0, &g0, &b0);
  ncchannel_rgb8(colors[qd->others[1]], &r1, &g1, &b1);
  diffs[0] = rgb_diff(r0, g0, b0, roth, goth, both);
  diffs[1] = rgb_diff(r1, g1, b1, roth, goth, both);
  diffs[2] = rgb_diff(r0, g0, b0, rlerp, glerp, blerp);
  diffs[3] = rgb_diff(r1, g1, b1, rlerp, glerp, blerp);
  // get diffs of the included two from their lerp
  ncchannel_rgb8(colors[qd->pair[0]], &r0, &g0, &b0);
  ncchannel_rgb8(colors[qd->pair[1]], &r1, &g1, &b1);
  diffs[4] = rgb_diff(r0, g0, b0, rlerp, glerp, blerp);
  diffs[5] = rgb_diff(r1, g1, b1, rlerp, glerp, blerp);
  unsigned curdiff = diffs[0] + diffs[1] + diffs[4] + diffs[5];
  // it might be better to combine three, and leave one totally unchanged.
  // propose a trilerps; we only need consider the member of the excluded pair
  // closer to the primary lerp. recalculate total diff; merge if lower.
  if(diffs[2] < diffs[3]){
    unsigned tri = trilerp(colors[qd->pair[0]], colors[qd->pair[1]], colors[qd->others[0]],
                           nointerpolate);
    ncchannel_rgb8(colors[qd->others[0]], &r2, &g2, &b2);
    ncchannel_rgb8(tri, &roth, &goth, &both);
    if(rgb_diff(r0, g0, b0, roth, goth, both) +
       rgb_diff(r1, g1, b1, roth, goth, both) +
       rgb_diff(r2, g2, b2, roth, goth, both) < curdiff){
      egc = qd->oth0egc;
      *back = colors[qd->others[1]];
      *fore = tri;
    }
//fprintf(stderr, "quadblitter swap type 1\n");
  }else{
    unsigned tri = trilerp(colors[qd->pair[0]], colors[qd->pair[1]], colors[qd->others[1]],
                           nointerpolate);
    ncchannel_rgb8(colors[qd->others[1]], &r2, &g2, &b2);
    ncchannel_rgb8(tri, &roth, &goth, &both);
    if(rgb_diff(r0, g0, b0, roth, goth, both) +
       rgb_diff(r1, g1, b1, roth, goth, both) +
       rgb_diff(r2, g2, b2, roth, goth, both) < curdiff){
      egc = qd->oth1egc;
      *back = colors[qd->others[0]];
      *fore = tri;
    }
//fprintf(stderr, "quadblitter swap type 2\n");
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
// FIXME pass in rgbas as array of uint32_t ala sexblitter
static inline const char*
qtrans_check(nccell* c, unsigned blendcolors,
             const unsigned char* rgbbase_tl, const unsigned char* rgbbase_tr,
             const unsigned char* rgbbase_bl, const unsigned char* rgbbase_br,
             uint32_t transcolor, unsigned nointerpolate){
  uint32_t tl = 0, tr = 0, bl = 0, br = 0;
  ncchannel_set_rgb8(&tl, rgbbase_tl[0], rgbbase_tl[1], rgbbase_tl[2]);
  ncchannel_set_rgb8(&tr, rgbbase_tr[0], rgbbase_tr[1], rgbbase_tr[2]);
  ncchannel_set_rgb8(&bl, rgbbase_bl[0], rgbbase_bl[1], rgbbase_bl[2]);
  ncchannel_set_rgb8(&br, rgbbase_br[0], rgbbase_br[1], rgbbase_br[2]);
  const char* egc = NULL;
  if(rgba_trans_q(rgbbase_tl, transcolor)){
    // top left is transparent
    if(rgba_trans_q(rgbbase_tr, transcolor)){
      // all of top is transparent
      if(rgba_trans_q(rgbbase_bl, transcolor)){
        // top and left are transparent
        if(rgba_trans_q(rgbbase_br, transcolor)){
          // entirety is transparent, load with nul (but not NULL)
          nccell_set_fg_default(c);
          cell_set_blitquadrants(c, 0, 0, 0, 0);
          egc = "";
        }else{
          nccell_set_fg_rgb8(c, rgbbase_br[0], rgbbase_br[1], rgbbase_br[2]);
          cell_set_blitquadrants(c, 0, 0, 0, 1);
          egc = "▗";
        }
      }else{
        if(rgba_trans_q(rgbbase_br, transcolor)){
          nccell_set_fg_rgb8(c, rgbbase_bl[0], rgbbase_bl[1], rgbbase_bl[2]);
          cell_set_blitquadrants(c, 0, 0, 1, 0);
          egc = "▖";
        }else{
          cell_set_fchannel(c, lerp(bl, br, nointerpolate));
          cell_set_blitquadrants(c, 0, 0, 1, 1);
          egc = "▄";
        }
      }
    }else{ // top right is foreground, top left is transparent
      if(rgba_trans_q(rgbbase_bl, transcolor)){
        if(rgba_trans_q(rgbbase_br, transcolor)){ // entire bottom is transparent
          nccell_set_fg_rgb8(c, rgbbase_tr[0], rgbbase_tr[1], rgbbase_tr[2]);
          cell_set_blitquadrants(c, 0, 1, 0, 0);
          egc = "▝";
        }else{
          cell_set_fchannel(c, lerp(tr, br, nointerpolate));
          cell_set_blitquadrants(c, 0, 1, 0, 1);
          egc = "▐";
        }
      }else if(rgba_trans_q(rgbbase_br, transcolor)){ // only br is transparent
        cell_set_fchannel(c, lerp(tr, bl, nointerpolate));
        cell_set_blitquadrants(c, 0, 1, 1, 0);
        egc = "▞";
      }else{
        cell_set_fchannel(c, trilerp(tr, bl, br, nointerpolate));
        cell_set_blitquadrants(c, 0, 1, 1, 1);
        egc = "▟";
      }
    }
  }else{ // topleft is foreground for all here
    if(rgba_trans_q(rgbbase_tr, transcolor)){
      if(rgba_trans_q(rgbbase_bl, transcolor)){
        if(rgba_trans_q(rgbbase_br, transcolor)){
          nccell_set_fg_rgb8(c, rgbbase_tl[0], rgbbase_tl[1], rgbbase_tl[2]);
          cell_set_blitquadrants(c, 1, 0, 0, 0);
          egc = "▘";
        }else{
          cell_set_fchannel(c, lerp(tl, br, nointerpolate));
          cell_set_blitquadrants(c, 1, 0, 0, 1);
          egc = "▚";
        }
      }else if(rgba_trans_q(rgbbase_br, transcolor)){
        cell_set_fchannel(c, lerp(tl, bl, nointerpolate));
        cell_set_blitquadrants(c, 1, 0, 1, 0);
        egc = "▌";
      }else{
        cell_set_fchannel(c, trilerp(tl, bl, br, nointerpolate));
        cell_set_blitquadrants(c, 1, 0, 1, 1);
        egc = "▙";
      }
    }else if(rgba_trans_q(rgbbase_bl, transcolor)){
      if(rgba_trans_q(rgbbase_br, transcolor)){ // entire bottom is transparent
        cell_set_fchannel(c, lerp(tl, tr, nointerpolate));
        cell_set_blitquadrants(c, 1, 1, 0, 0);
        egc = "▀";
      }else{ // only bl is transparent
        cell_set_fchannel(c, trilerp(tl, tr, br, nointerpolate));
        cell_set_blitquadrants(c, 1, 1, 0, 1);
        egc = "▜";
      }
    }else if(rgba_trans_q(rgbbase_br, transcolor)){ // only br is transparent
      cell_set_fchannel(c, trilerp(tl, tr, bl, nointerpolate));
      cell_set_blitquadrants(c, 1, 1, 1, 0);
      egc = "▛";
    }else{
      return NULL; // no transparency
    }
  }
  assert(egc);
  nccell_set_bg_alpha(c, NCALPHA_TRANSPARENT);
  if(*egc == '\0'){
    nccell_set_fg_alpha(c, NCALPHA_TRANSPARENT);
  }else if(blendcolors){
    nccell_set_fg_alpha(c, NCALPHA_BLEND);
  }
//fprintf(stderr, "QBQ: 0x%x\n", cell_blittedquadrants(c));
  return egc;
}

// quadrant blitter. maps 2x2 to each cell. since we only have two colors at
// our disposal (foreground and background), we lose some fidelity.
static inline int
quadrant_blit(ncplane* nc, int linesize, const void* data, int leny, int lenx,
              const blitterargs* bargs){
  const unsigned nointerpolate = bargs->flags & NCVISUAL_OPTION_NOINTERPOLATE;
  const bool blendcolors = bargs->flags & NCVISUAL_OPTION_BLEND;
  unsigned dimy, dimx, x, y;
  int total = 0; // number of cells written
  ncplane_dim_yx(nc, &dimy, &dimx);
//fprintf(stderr, "quadblitter %dx%d -> %d/%d+%d/%d\n", leny, lenx, dimy, dimx, bargs->u.cell.placey, bargs->u.cell.placex);
  // FIXME not going to necessarily be safe on all architectures hrmmm
  const unsigned char* dat = data;
  int visy = bargs->begy;
  for(y = bargs->u.cell.placey ; visy < (bargs->begy + leny) && y < dimy ; ++y, visy += 2){
    if(ncplane_cursor_move_yx(nc, y, bargs->u.cell.placex < 0 ? 0 : bargs->u.cell.placex)){
      return -1;
    }
    int visx = bargs->begx;
    for(x = bargs->u.cell.placex ; visx < (bargs->begx + lenx) && x < dimx ; ++x, visx += 2){
      const unsigned char* rgbbase_tl = dat + (linesize * visy) + (visx * 4);
      const unsigned char* rgbbase_tr = zeroes;
      const unsigned char* rgbbase_bl = zeroes;
      const unsigned char* rgbbase_br = zeroes;
      if(visx < bargs->begx + lenx - 1){
        rgbbase_tr = dat + (linesize * visy) + ((visx + 1) * 4);
        if(visy < bargs->begy + leny - 1){
          rgbbase_br = dat + (linesize * (visy + 1)) + ((visx + 1) * 4);
        }
      }
      if(visy < bargs->begy + leny - 1){
        rgbbase_bl = dat + (linesize * (visy + 1)) + (visx * 4);
      }
//fprintf(stderr, "[%04d/%04d] lsize: %d %02x %02x %02x %02x\n", y, x, linesize, rgbbase_tl[0], rgbbase_tr[1], rgbbase_bl[2], rgbbase_br[3]);
      nccell* c = ncplane_cell_ref_yx(nc, y, x);
      c->channels = 0;
      c->stylemask = 0;
      const char* egc = qtrans_check(c, blendcolors, rgbbase_tl, rgbbase_tr,
                                     rgbbase_bl, rgbbase_br, bargs->transcolor,
                                     nointerpolate);
      if(egc == NULL){
        uint32_t tl = 0, tr = 0, bl = 0, br = 0;
        ncchannel_set_rgb8(&tl, rgbbase_tl[0], rgbbase_tl[1], rgbbase_tl[2]);
        ncchannel_set_rgb8(&tr, rgbbase_tr[0], rgbbase_tr[1], rgbbase_tr[2]);
        ncchannel_set_rgb8(&bl, rgbbase_bl[0], rgbbase_bl[1], rgbbase_bl[2]);
        ncchannel_set_rgb8(&br, rgbbase_br[0], rgbbase_br[1], rgbbase_br[2]);
        uint32_t bg, fg;
//fprintf(stderr, "qtrans check: %d/%d\n%08x %08x\n%08x %08x\n", y, x, *(const uint32_t*)rgbbase_tl, *(const uint32_t*)rgbbase_tr, *(const uint32_t*)rgbbase_bl, *(const uint32_t*)rgbbase_br);
        egc = quadrant_solver(tl, tr, bl, br, &fg, &bg, nointerpolate);
        assert(egc);
//fprintf(stderr, "%d/%d %08x/%08x\n", y, x, fg, bg);
        cell_set_fchannel(c, fg);
        cell_set_bchannel(c, bg);
        if(blendcolors){
          nccell_set_bg_alpha(c, NCALPHA_BLEND);
          nccell_set_fg_alpha(c, NCALPHA_BLEND);
        }
        cell_set_blitquadrants(c, 1, 1, 1, 1);
      }
      if(*egc){
        if(pool_blit_direct(&nc->pool, c, egc, strlen(egc), 1) <= 0){
          return -1;
        }
        ++total;
      }else{
        nccell_release(nc, c);
      }
    }
  }
  return total;
}

// Solve for the cell rendered by this cellheightX2 sample. None of the input
// pixels may be transparent (that ought already have been handled). We use
// exhaustive search, which might be quite computationally intensive for the
// worst case (all pixels are different colors). We want to solve for the
// 2-partition of pixels that minimizes total source distance from the
// resulting lerps.
static const char*
hires_solver(const uint32_t rgbas[6], uint64_t* channels, unsigned blendcolors,
             unsigned nointerpolate, unsigned cellheight){
  // FIXME need genericize to hires
  // each element within the set of 64 has an inverse element within the set,
  // for which we would calculate the same total differences, so just handle
  // the first 32. the partition[] bit masks represent combinations of
  // sextants, and their indices correspond to sex[].
  static const char* sex[32] = {
    " ", "🬀", "🬁", "🬃", "🬇", "🬏", "🬞", "🬂", // 0..7
    "🬄", "🬈", "🬐", "🬟", "🬅", "🬉", "🬑", "🬠", // 8..15
    "🬋", "🬓", "🬢", "🬖", "🬦", "🬭", "🬆", "🬊", // 16..23
    "🬒", "🬡", "🬌", "▌", "🬣", "🬗", "🬧", "🬮", // 24..31
  };
  static const unsigned partitions[32] = {
    0, // 1 way to arrange 0
    1, 2, 4, 8, 16, 32, // 6 ways to arrange 1
    3, 5, 9, 17, 33, 6, 10, 18, 34, 12, 20, 36, 24, 40, 48, // 15 ways for 2
    //  16 ways to arrange 3, *but* six of them are inverses, so 10
    7, 11, 19, 35, 13, 21, 37, 25, 41, 49 //  10 + 15 + 6 + 1 == 32
  };
  // we loop over the bitstrings, dividing the pixels into two sets, and then
  // taking a general lerp over each set. we then compute the sum of absolute
  // differences, and see if it's the new minimum.
  int best = -1;
  uint32_t mindiff = UINT_MAX;
//fprintf(stderr, "%06x %06x\n%06x %06x\n%06x %06x\n", rgbas[0], rgbas[1], rgbas[2], rgbas[3], rgbas[4], rgbas[5]);
  for(size_t glyph = 0 ; glyph < sizeof(partitions) / sizeof(*partitions) ; ++glyph){
    unsigned rsum0 = 0, rsum1 = 0;
    unsigned gsum0 = 0, gsum1 = 0;
    unsigned bsum0 = 0, bsum1 = 0;
    int insum = 0;
    int outsum = 0;
    for(unsigned mask = 0 ; mask < cellheight * 2 ; ++mask){
      if(partitions[glyph] & (1u << mask)){
        if(!nointerpolate || !insum){
          rsum0 += ncpixel_r(rgbas[mask]);
          gsum0 += ncpixel_g(rgbas[mask]);
          bsum0 += ncpixel_b(rgbas[mask]);
          ++insum;
        }
      }else{
        if(!nointerpolate || !outsum){
          rsum1 += ncpixel_r(rgbas[mask]);
          gsum1 += ncpixel_g(rgbas[mask]);
          bsum1 += ncpixel_b(rgbas[mask]);
          ++outsum;
        }
      }
    }
    uint32_t l0 = generalerp(rsum0, gsum0, bsum0, insum);
    uint32_t l1 = generalerp(rsum1, gsum1, bsum1, outsum);
//fprintf(stderr, "sum0: %06x sum1: %06x insum: %d\n", l0 & 0xffffffu, l1 & 0xffffffu, insum);
    uint32_t totaldiff = 0;
    for(unsigned mask = 0 ; mask < cellheight * 2 ; ++mask){
      unsigned r, g, b;
      if(partitions[glyph] & (1u << mask)){
        ncchannel_rgb8(l0, &r, &g, &b);
      }else{
        ncchannel_rgb8(l1, &r, &g, &b);
      }
      uint32_t rdiff = rgb_diff(ncpixel_r(rgbas[mask]), ncpixel_g(rgbas[mask]),
                                ncpixel_b(rgbas[mask]), r, g, b);
      totaldiff += rdiff;
//fprintf(stderr, "mask: %u totaldiff: %u insum: %d (%08x / %08x)\n", mask, totaldiff, insum, l0, l1);
    }
//fprintf(stderr, "bits: %u %zu totaldiff: %f best: %f (%d)\n", partitions[glyph], glyph, totaldiff, mindiff, best);
    if(totaldiff < mindiff){
      mindiff = totaldiff;
      best = glyph;
      ncchannels_set_fchannel(channels, l0);
      ncchannels_set_bchannel(channels, l1);
    }
    if(totaldiff == 0){ // can't beat that!
      break;
    }
  }
//fprintf(stderr, "solved for best: %d (%u)\n", best, mindiff);
  assert(best >= 0 && best < 32); // FIXME adapt to oct
  if(blendcolors){
    ncchannels_set_fg_alpha(channels, NCALPHA_BLEND);
    ncchannels_set_bg_alpha(channels, NCALPHA_BLEND);
  }
  return sex[best];
}

// FIXME replace both of these arrays of pointers with fixed-width matrices
// bit of index is *set* where sextant *is not*
// 32: bottom right 16: bottom left
//  8: middle right  4: middle left
//  2: upper right   1: upper left
static const char* sextrans[64] = {
  "█", "🬻", "🬺", "🬹", "🬸", "🬷", "🬶", "🬵",
  "🬴", "🬳", "🬲", "🬱", "🬰", "🬯", "🬮", "🬭",
  "🬬", "🬫", "🬪", "🬩", "🬨", "▐", "🬧", "🬦",
  "🬥", "🬤", "🬣", "🬢", "🬡", "🬠", "🬟", "🬞",
  "🬝", "🬜", "🬛", "🬚", "🬙", "🬘", "🬗", "🬖",
  "🬕", "🬔", "▌", "🬓", "🬒", "🬑", "🬐", "🬏",
  "🬎", "🬍", "🬌", "🬋", "🬊", "🬉", "🬈", "🬇",
  "🬆", "🬅", "🬄", "🬃", "🬂", "🬁", "🬀", " ",
};

// bit of index is *set* where octant *is not*
//   1: row 0 left   2: row 0 right
//   4: row 1 left   8: row 1 right
//  16: row 2 left  32: row 2 right
//  64: row 3 left 128: row 3 right
static const char* octtrans[256] = {
  "\U00002588", // █ 255 all eight set          (full)
  "\U0001cde5", // 𜷥 254 missing upper left     (o2345678)
  "\U0001cde4", // 𜷤 253 missing upper right    (o1345678)
  "\U00002586", // ▆ 252 missing row 0          (lower three quarters)
  "\U0001cde3", // 𜷣 251 missing row 1 left     (o1245678)
  "\U0000259f", // ▟ 250                        (q upper right and lower left and lower right)
  "\U0001cde2", // 𜷢 249                        (o1245678)
  "\U0001cde1", // 𜷡 248                        (o45678)
  "\U0001cde0", // 𜷠 247 missing row 1 right    (o1235678)
  "\U0001cddf", // 𜷟 246 missing 0 left 1 right (o235678)
  "\U00002599", // ▙ 245 missing 0/1 right      (q upper left and lower left and lower right)
  "\U0001cdde", // 𜷞 244
  "\U0001cddd", // 𜷝 243
  "\U0001cddc", // 𜷜 242
  "\U0001cddb", // 𜷛 241                        (o15678)
  "\U00002584", // ▄ 240 2/3 full               (lower half)
  "\U0001cdda", // 𜷚 239                        (o1234678)
  "\U0001cdd9", // 𜷙 238                        (o234678)
  "\U0001cdd8", // 𜷘 237                        (o134678)
  "\U0001cdd7", // 𜷗 236                        (o34678)
  "\U0001cdd6", // 𜷖 235                        (o124678)
  "\U0001cdd5", // 𜷕 234                        (o24678)
  "\U0001cdd4", // 𜷔 233                        (o14678)
  "\U0001cdd3", // 𜷓 232                        (o4678)
  "\U0001cdd2", // 𜷒 231
  "\U0001cdd1", // 𜷑 230
  "\U0001cdd0", // 𜷐 229
  "\U0001cdcf", // 𜷏 228
  "\U0001cdce", // 𜷎 227
  "\U0001cdcd", // 𜷍 226
  "\U0001cdcc", // 𜷌 225
  "\U0001cdcb", // 𜷋 224
  "\U0001cdca",
  "\U0001cdc9",
  "\U0001cdc8",
  "\U0001cdc7",
  "\U0001cdc6",
  "\U0001cdc5",
  "\U0001cdc4",
  "\U0001cdc3",
  "\U0001cdc2",
  "\U0001cdc1",
  "\U0001cdc0",
  "\U0001cdbf",
  "\U0001cdbe",
  "\U0001cdbd",
  "\U0001cdbc",
  "\U0001cdbb",
  "\U0001cdba",
  "\U0001cdb9",
  "\U0001cdb8",
  "\U0001cdb7",
  "\U0001cdb6",
  "\U0001cdb5",
  "\U0001cdb4",
  "\U0001cdb3",
  "\U0001cdb2",
  "\U0001cdb1",
  "\U0001cdb0",
  "\U0001cdaf",
  "\U0001cdae",
  "\U0001cdad",
  "\U0001cdac",
  "\U00002582", // ▂ 196                           (lower one quarter)
  "\U0001cdab",
  "\U0001cdaa",
  "\U0001cda9",
  "\U0001cda8",
  "\U0001cda7",
  "\U0001cda6",
  "\U0001cda5",
  "\U0001cda4",
  "\U0001cda3",
  "\U0001cda2",
  "\U0001cda1",
  "\U0001cda0",
  "\U0001cd9f",
  "\U0001cd9e",
  "\U0001cd9d",
  "\U0001cd9c",
  "\U0000259c", // ▜ 175                            (q upper left and upper right and lower right)
  "\U0001cd9b",
  "\U0001cd9a",
  "\U0001cd99",
  "\U0001cd98",
  "\U00002590", // ▐ 170                            (right half)
  "\U0001cd97",
  "\U0001cd96",
  "\U0001cd95",
  "\U0001cd94",
  "\U0000259a", // ▚                                (q upper left and lower right)
  "\U0001cd93",
  "\U0001cd92",
  "\U0001cd91",
  "\U0001cd90",
  "\U00002597", // ▗                                (q lower right)
  "\U0001cd8f",
  "\U0001cd8e",
  "\U0001cd8d",
  "\U0001cd8c",
  "\U0001cd8b",
  "\U0001cd8a",
  "\U0001cd89",
  "\U0001cd88",
  "\U0001cd87",
  "\U0001cd86",
  "\U0001cd85",
  "\U0001cd84",
  "\U0001cd83",
  "\U0001cd82",
  "\U0001cd81",
  "\U0001cd80",
  "\U0001cd7f",
  "\U0001cd7e",
  "\U0001cd7d",
  "\U0001cd7c",
  "\U0001cd7b",
  "\U0001cd7a",
  "\U0001cd79",
  "\U0001cd78",
  "\U0001cd77",
  "\U0001cd76",
  "\U0001cd75",
  "\U0001cd74",
  "\U0001cd73",
  "\U0001cd72",
  "\U0001cd71",
  "\U0001cea0", // 𜺠 128 lower right only       (right half lower one quarter)
  "\U0001cd70", // 𜵰 127 missing lower right    (u1234567)
  "\U0001cd6f",
  "\U0001cd6e",
  "\U0001cd6d",
  "\U0001cd6c",
  "\U0001cd6b",
  "\U0001cd6a",
  "\U0001cd69",
  "\U0001cd68",
  "\U0001cd67",
  "\U0001cd66",
  "\U0001cd65",
  "\U0001cd64",
  "\U0001cd63",
  "\U0001cd62",
  "\U0001cd61",
  "\U0001cd60",
  "\U0001cd5f",
  "\U0001cd5e",
  "\U0001cd5d",
  "\U0001cd5c",
  "\U0001cd5b",
  "\U0001cd5a",
  "\U0001cd59",
  "\U0001cd58",
  "\U0001cd57",
  "\U0001cd56",
  "\U0001cd55",
  "\U0001cd54",
  "\U0001cd53",
  "\U0001cd52",
  "\U0001cd51",
  "\U0000259b", // ▛  95 0/1 full 2/3 left      (q upper left and upper right and lower left)
  "\U0001cd50",
  "\U0001cd4f",
  "\U0001cd4e",
  "\U0001cd4d",
  "\U0000259e", // ▞  92 0/1 right 2/3 left     (q upper right and lower left)
  "\U0001cd4c",
  "\U0001cd4b",
  "\U0001cd4a",
  "\U0001cd49",
  "\U0000258c", // ▌  85 0/1/2/3 left           (left block)
  "\U0001cd48",
  "\U0001cd47",
  "\U0001cd46",
  "\U0001cd45",
  "\U00002596", // ▖  80 2/3 left               (q lower left)
  "\U0001cd44",
  "\U0001cd43",
  "\U0001cd42",
  "\U0001cd41",
  "\U0001cd40",
  "\U0001cd3f",
  "\U0001cd3e",
  "\U0001cd3d",
  "\U0001cd3c",
  "\U0001cd3b",
  "\U0001cd3a",
  "\U0001cd39",
  "\U0001cd38",
  "\U0001cd37", // 𜴷  66 0 right 3 left         (o27)
  "\U0001cd36", // 𜴶  65 0 left 3 left          (o17)
  "\U0001cea3", // 𜺣  64 lower left only        (left half lower one quarter)
  "\U0001fb85", // 🮅  63 row 0/1/2 full         (upper three quarters)
  "\U0001cd35", // 𜴵  62                        (o23456)
  "\U0001cd34", // 𜴴  61                        (o13456)
  "\U0001cd33", // 𜴳  60                        (o3456)
  "\U0001cd32", // 𜴲  59                        (o12456)
  "\U0001cd31", // 𜴱  58                        (o2456)
  "\U0001cd30", // 𜴰  57 0 left 1 right 2 full  (o1456)
  "\U0001cd2f", // 𜴯  56                        (o456)
  "\U0001cd2e", // 𜴮  55
  "\U0001cd2d", // 𜴭  54
  "\U0001cd2c", // 𜴬  53
  "\U0001cd2b", // 𜴫  52
  "\U0001cd2a", // 𜴪  51
  "\U0001cd29", // 𜴩  50
  "\U0001cd28", // 𜴨  49
  "\U0001cd27", // 𜴧  48
  "\U0001cd26", // 𜴦  47                        (o12346)
  "\U0001cd25", // 𜴥  46                        (o2346)
  "\U0001cd24", // 𜴤  45                        (o1346)
  "\U0001cd23", // 𜴣  44                        (o346)
  "\U0001cd22", // 𜴢  43                        (o1246)
  "\U0001cd21", // 𜴡  42                        (o246)
  "\U0001cd20", // 𜴠  41                        (o146)
  "\U0001fbe7", // 🯧  40                        (middle right one quarter)
  "\U0001cd1f", // 𜴟  39                        (o1236)
  "\U0001cd1e", // 𜴞  38                        (o236)
  "\U0001cd1d", // 𜴝  37                        (o136)
  "\U0001cd1c", // 𜴜  36                        (o36)
  "\U0001cd1b", // 𜴛  35                        (o126)
  "\U0001cd1a", // 𜴚  34                        (o26)
  "\U0001cd19", // 𜴙  33                        (o16)
  "\U0001cd18", // 𜴘  32 row 2 right only       (o6)
  "\U0001cd17", // 𜴗  31                        (o12345)
  "\U0001cd16", // 𜴖  30                        (o2345)
  "\U0001cd15", // 𜴕  29                        (o1345)
  "\U0001cd14", // 𜴔  28                        (o345)
  "\U0001cd13", // 𜴓  27                        (o1245)
  "\U0001cd12", // 𜴒  26 row 0/1 right row 2 l  (o245)
  "\U0001cd11", // 𜴑  25 row 1/2 left row 1 r   (o145)
  "\U0001cd10", // 𜴐  24 row 1 right row 2 left (o45)
  "\U0001cd0f", // 𜴏  23 row 0 full row 1/2 l   (o1235)
  "\U0001cd0e", // 𜴎  22 row 1 right row 2/3 l  (o235)
  "\U0001cd0d", // 𜴍  21 row 0/1/2 left         (o135)
  "\U0001fbe6", // 🯦  20 row 1/2 left           (middle left one quarter)
  "\U0001cd0c", // 𜴌  19 row 0 full row 2 left  (o125)
  "\U0001cd0b", // 𜴋  18 row 0 right row 2 left (o25)
  "\U0001cd0a", // 𜴊  17 row 0 left row 2 left  (o15)
  "\U0001cd09", // 𜴉  16 row 2 left only        (o5)
  "\U00002580", // ▀  15 row 0/1 full           (upper half)
  "\U0001cd08", // 𜴈  14 row 0 right row 1 full (o234)
  "\U0001cd07", // 𜴇  13 row 0 left row 1 full  (o134)
  "\U0001cd06", // 𜴆  12 row 1 full             (o34)
  "\U0001cd05", // 𜴅  11 row 0 full row 1 right (o124)
  "\U0000259d", // ▝  10 row 0/1 right only     (upper right quadrant)
  "\U0001cd04", // 𜴄   9 row 0 left row 1 right (o14)
  "\U0001cd03", // 𜴃   8 row 1 right only       (o4)
  "\U0001cd02", // 𜴂   7 row 0 full row 1 left  (o123)
  "\U0001cd01", // 𜴁   6 row 0 right row 1 left (o23)
  "\U00002598", // ▘   5 row 0/1 left only      (upper left quadrant)
  "\U0001cd00", // 𜴀   4 row 1 left only        (o3)
  "\U0001f8b2", // 🮂   3 row 0                  (upper one quarter)
  "\U0001ceab", // 𜺫   2 upper right only       (right half upper one quarter)
  "\U0001cea8", // 𜺨   1 upper left only        (left half upper one quarter)
  " "  //     0 none set               (space)
};

static const char*
hires_trans_check(nccell* c, const uint32_t* rgbas, unsigned blendcolors,
                  uint32_t transcolor, unsigned nointerpolate, int cellheight,
                  const char** transegcs){
  unsigned transstring = 0;
  unsigned r = 0, g = 0, b = 0;
  unsigned div = 0;
  // check each pixel for transparency
  for(int mask = 0 ; mask < cellheight * 2 ; ++mask){
    if(rgba_trans_p(rgbas[mask], transcolor)){
      transstring |= (1u << mask);
    }else if(!nointerpolate || !div){
      r += ncpixel_r(rgbas[mask]);
      g += ncpixel_g(rgbas[mask]);
      b += ncpixel_b(rgbas[mask]);
      ++div;
    }
  }
  // transstring can only have 0x80 and/or 0x40 set if cellheight was 4
  if(transstring == 0){ // there was no transparency
    return NULL;
  }
  nccell_set_bg_alpha(c, NCALPHA_TRANSPARENT);
  // there were some transparent pixels. since they get priority, the foreground
  // is just a general lerp across non-transparent pixels.
  const char* egc = transegcs[transstring];
  nccell_set_bg_alpha(c, NCALPHA_TRANSPARENT);
//fprintf(stderr, "transtring: %u egc: %s\n", transtring, egc);
  if(*egc == ' '){ // entirely transparent
    nccell_set_fg_alpha(c, NCALPHA_TRANSPARENT);
    return "";
  }else{ // partially transparent, thus div >= 1
//fprintf(stderr, "div: %u r: %u g: %u b: %u\n", div, r, g, b);
    cell_set_fchannel(c, generalerp(r, g, b, div));
    if(blendcolors){
      nccell_set_fg_alpha(c, NCALPHA_BLEND);
    }
    // FIXME genericize for hires
    cell_set_blitquadrants(c, !(transstring & 5u), !(transstring & 10u),
                              !(transstring & 20u), !(transstring & 40u));
  }
//fprintf(stderr, "SEX-BQ: 0x%x\n", cell_blittedquadrants(c));
  return egc;
}

// sextant/octant blitter. maps 3x2 or 4x2 to each cell. since we only have two
// colors at our disposal (foreground and background), we generally lose some
// color fidelity.
static inline int
hires_blit(ncplane* nc, int linesize, const void* data, int leny, int lenx,
           const blitterargs* bargs, int cellheight,
           const char** transegcs){
  const unsigned nointerpolate = bargs->flags & NCVISUAL_OPTION_NOINTERPOLATE;
  const bool blendcolors = bargs->flags & NCVISUAL_OPTION_BLEND;
  unsigned dimy, dimx, x, y;
  int total = 0; // number of cells written
  ncplane_dim_yx(nc, &dimy, &dimx);
//fprintf(stderr, "hiresblitter %dx%d -> %d/%d+%d/%d\n", leny, lenx, dimy, dimx, bargs->u.cell.placey, bargs->u.cell.placex);
  const unsigned char* dat = data;
  int visy = bargs->begy;
  for(y = bargs->u.cell.placey ; visy < (bargs->begy + leny) && y < dimy ; ++y, visy += cellheight){
    if(ncplane_cursor_move_yx(nc, y, bargs->u.cell.placex < 0 ? 0 : bargs->u.cell.placex)){
      return -1;
    }
    int visx = bargs->begx;
    for(x = bargs->u.cell.placex ; visx < (bargs->begx + lenx) && x < dimx ; ++x, visx += 2){
      uint32_t rgbas[cellheight * 2]; // row-major
      memset(rgbas, 0, sizeof(rgbas));
      memcpy(&rgbas[0], (dat + (linesize * visy) + (visx * 4)), sizeof(*rgbas));
      // conditional looks at first column, begininng at the second row
      for(int yoff = 1 ; yoff < cellheight ; ++yoff){
        if(visy < bargs->begy + leny - yoff){
          memcpy(&rgbas[yoff * 2], (dat + (linesize * (visy + yoff)) + (visx * 4)), sizeof(*rgbas));
        }
      }
      // conditional looks at second column, beginning at second row
      if(visx < bargs->begx + lenx - 1){
        memcpy(&rgbas[1], (dat + (linesize * visy) + ((visx + 1) * 4)), sizeof(*rgbas));
        for(int yoff = 1 ; yoff < cellheight ; ++yoff){
          if(visy < bargs->begy + leny - yoff){
            memcpy(&rgbas[1 + yoff * 2], (dat + (linesize * (visy + yoff)) + ((visx + 1) * 4)), sizeof(*rgbas));
          }
        }
      }
      nccell* c = ncplane_cell_ref_yx(nc, y, x);
      c->channels = 0;
      c->stylemask = 0;
      const char* egc = hires_trans_check(c, rgbas, blendcolors, bargs->transcolor,
                                          nointerpolate, cellheight, transegcs);
      if(egc == NULL){ // no transparency; run a full solver
        egc = hires_solver(rgbas, &c->channels, blendcolors, nointerpolate, cellheight);
        cell_set_blitquadrants(c, 1, 1, 1, 1);
      }
//fprintf(stderr, "hires EGC: %s channels: %016lx\n", egc, c->channels);
      if(*egc){
        if(pool_blit_direct(&nc->pool, c, egc, strlen(egc), 1) <= 0){
          return -1;
        }
        ++total;
      }else{
        nccell_release(nc, c);
      }
    }
  }
  return total;
}

static inline int
sextant_blit(ncplane* nc, int linesize, const void* data, int leny, int lenx,
             const blitterargs* bargs){
  return hires_blit(nc, linesize, data, leny, lenx, bargs, 3, sextrans);
}

static inline int
octant_blit(ncplane* nc, int linesize, const void* data, int leny, int lenx,
           const blitterargs* bargs){
  return hires_blit(nc, linesize, data, leny, lenx, bargs, 4, octtrans);
}

// Bit is set where Braille dot is present:
//   0  1
//   2  3
//   4  5
//   6  7
// Similar to NCBRAILLEEGCS, but in a different order since we number the bits differently
static const char braille_egcs[256][5] = {
  "\u2800", "\u2801", "\u2808", "\u2809", "\u2802", "\u2803", "\u280a", "\u280b",
  "\u2810", "\u2811", "\u2818", "\u2819", "\u2812", "\u2813", "\u281a", "\u281b",
  "\u2804", "\u2805", "\u280c", "\u280d", "\u2806", "\u2807", "\u280e", "\u280f",
  "\u2814", "\u2815", "\u281c", "\u281d", "\u2816", "\u2817", "\u281e", "\u281f",
  "\u2820", "\u2821", "\u2828", "\u2829", "\u2822", "\u2823", "\u282a", "\u282b",
  "\u2830", "\u2831", "\u2838", "\u2839", "\u2832", "\u2833", "\u283a", "\u283b",
  "\u2824", "\u2825", "\u282c", "\u282d", "\u2826", "\u2827", "\u282e", "\u282f",
  "\u2834", "\u2835", "\u283c", "\u283d", "\u2836", "\u2837", "\u283e", "\u283f",
  "\u2840", "\u2841", "\u2848", "\u2849", "\u2842", "\u2843", "\u284a", "\u284b",
  "\u2850", "\u2851", "\u2858", "\u2859", "\u2852", "\u2853", "\u285a", "\u285b",
  "\u2844", "\u2845", "\u284c", "\u284d", "\u2846", "\u2847", "\u284e", "\u284f",
  "\u2854", "\u2855", "\u285c", "\u285d", "\u2856", "\u2857", "\u285e", "\u285f",
  "\u2860", "\u2861", "\u2868", "\u2869", "\u2862", "\u2863", "\u286a", "\u286b",
  "\u2870", "\u2871", "\u2878", "\u2879", "\u2872", "\u2873", "\u287a", "\u287b",
  "\u2864", "\u2865", "\u286c", "\u286d", "\u2866", "\u2867", "\u286e", "\u286f",
  "\u2874", "\u2875", "\u287c", "\u287d", "\u2876", "\u2877", "\u287e", "\u287f",
  "\u2880", "\u2881", "\u2888", "\u2889", "\u2882", "\u2883", "\u288a", "\u288b",
  "\u2890", "\u2891", "\u2898", "\u2899", "\u2892", "\u2893", "\u289a", "\u289b",
  "\u2884", "\u2885", "\u288c", "\u288d", "\u2886", "\u2887", "\u288e", "\u288f",
  "\u2894", "\u2895", "\u289c", "\u289d", "\u2896", "\u2897", "\u289e", "\u289f",
  "\u28a0", "\u28a1", "\u28a8", "\u28a9", "\u28a2", "\u28a3", "\u28aa", "\u28ab",
  "\u28b0", "\u28b1", "\u28b8", "\u28b9", "\u28b2", "\u28b3", "\u28ba", "\u28bb",
  "\u28a4", "\u28a5", "\u28ac", "\u28ad", "\u28a6", "\u28a7", "\u28ae", "\u28af",
  "\u28b4", "\u28b5", "\u28bc", "\u28bd", "\u28b6", "\u28b7", "\u28be", "\u28bf",
  "\u28c0", "\u28c1", "\u28c8", "\u28c9", "\u28c2", "\u28c3", "\u28ca", "\u28cb",
  "\u28d0", "\u28d1", "\u28d8", "\u28d9", "\u28d2", "\u28d3", "\u28da", "\u28db",
  "\u28c4", "\u28c5", "\u28cc", "\u28cd", "\u28c6", "\u28c7", "\u28ce", "\u28cf",
  "\u28d4", "\u28d5", "\u28dc", "\u28dd", "\u28d6", "\u28d7", "\u28de", "\u28df",
  "\u28e0", "\u28e1", "\u28e8", "\u28e9", "\u28e2", "\u28e3", "\u28ea", "\u28eb",
  "\u28f0", "\u28f1", "\u28f8", "\u28f9", "\u28f2", "\u28f3", "\u28fa", "\u28fb",
  "\u28e4", "\u28e5", "\u28ec", "\u28ed", "\u28e6", "\u28e7", "\u28ee", "\u28ef",
  "\u28f4", "\u28f5", "\u28fc", "\u28fd", "\u28f6", "\u28f7", "\u28fe", "\u28ff",
};

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

// generic 4x2 blitter, used for octant and Braille. maps 4x2 to each
// cell. since we only have one color at our disposal (foreground), we
// lose some fidelity. this is optimal for visuals with only two
// colors in a given area, as it packs lots of resolution. always
// transparent background.
static inline int
blit_4x2(ncplane* nc, int linesize, const void* data, int leny, int lenx,
         const blitterargs* bargs, const char egcs[256][5]){
  const bool blendcolors = bargs->flags & NCVISUAL_OPTION_BLEND;
  unsigned dimy, dimx, x, y;
  int total = 0; // number of cells written
  ncplane_dim_yx(nc, &dimy, &dimx);
  // FIXME not going to necessarily be safe on all architectures hrmmm
  const unsigned char* dat = data;
  int visy = bargs->begy;
  for(y = bargs->u.cell.placey ; visy < (bargs->begy + leny) && y < dimy ; ++y, visy += 4){
    if(ncplane_cursor_move_yx(nc, y, bargs->u.cell.placex < 0 ? 0 : bargs->u.cell.placex)){
      return -1;
    }
    int visx = bargs->begx;
    for(x = bargs->u.cell.placex ; visx < (bargs->begx + lenx) && x < dimx ; ++x, visx += 2){
      const uint32_t* rgbbase_l0 = (const uint32_t*)(dat + (linesize * visy) + (visx * 4));
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
      if(visx < bargs->begx + lenx - 1){
        rgbbase_r0 = (const uint32_t*)(dat + (linesize * visy) + ((visx + 1) * 4));
        if(visy < bargs->begy + leny - 1){
          rgbbase_r1 = (const uint32_t*)(dat + (linesize * (visy + 1)) + ((visx + 1) * 4));
          if(visy < bargs->begy + leny - 2){
            rgbbase_r2 = (const uint32_t*)(dat + (linesize * (visy + 2)) + ((visx + 1) * 4));
            if(visy < bargs->begy + leny - 3){
              rgbbase_r3 = (const uint32_t*)(dat + (linesize * (visy + 3)) + ((visx + 1) * 4));
            }
          }
        }
      }
      if(visy < bargs->begy + leny - 1){
        rgbbase_l1 = (const uint32_t*)(dat + (linesize * (visy + 1)) + (visx * 4));
        if(visy < bargs->begy + leny - 2){
          rgbbase_l2 = (const uint32_t*)(dat + (linesize * (visy + 2)) + (visx * 4));
          if(visy < bargs->begy + leny - 3){
            rgbbase_l3 = (const uint32_t*)(dat + (linesize * (visy + 3)) + (visx * 4));
          }
        }
      }
      // 4x2 block is ordered (where 0 is the LSB)
      //  0 1
      //  2 3
      //  4 5
      //  6 7
      // FIXME fold this into the above?
      if(!rgba_trans_p(*rgbbase_l0, bargs->transcolor)){
        egcidx |= 1u;
        fold_rgb8(&r, &g, &b, rgbbase_l0, &blends);
      }
      if(!rgba_trans_p(*rgbbase_r0, bargs->transcolor)){
        egcidx |= 2u;
        fold_rgb8(&r, &g, &b, rgbbase_r0, &blends);
      }
      if(!rgba_trans_p(*rgbbase_l1, bargs->transcolor)){
        egcidx |= 4u;
        fold_rgb8(&r, &g, &b, rgbbase_l1, &blends);
      }
      if(!rgba_trans_p(*rgbbase_r1, bargs->transcolor)){
        egcidx |= 8u;
        fold_rgb8(&r, &g, &b, rgbbase_r1, &blends);
      }
      if(!rgba_trans_p(*rgbbase_l2, bargs->transcolor)){
        egcidx |= 16u;
        fold_rgb8(&r, &g, &b, rgbbase_l2, &blends);
      }
      if(!rgba_trans_p(*rgbbase_r2, bargs->transcolor)){
        egcidx |= 32u;
        fold_rgb8(&r, &g, &b, rgbbase_r2, &blends);
      }
      if(!rgba_trans_p(*rgbbase_l3, bargs->transcolor)){
        egcidx |= 64u;
        fold_rgb8(&r, &g, &b, rgbbase_l3, &blends);
      }
      if(!rgba_trans_p(*rgbbase_r3, bargs->transcolor)){
        egcidx |= 128u;
        fold_rgb8(&r, &g, &b, rgbbase_r3, &blends);
      }
//fprintf(stderr, "[%04d/%04d] lsize: %d %02x %02x %02x %02x\n", y, x, linesize, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2], rgbbase_up[3]);
      nccell* c = ncplane_cell_ref_yx(nc, y, x);
      // use the default for the background, as that's the only way it's
      // effective in that case anyway
      c->channels = 0;
      c->stylemask = 0;
      if(blendcolors){
        nccell_set_fg_alpha(c, NCALPHA_BLEND);
      }
      // FIXME for now, we just sample, color-wise, and always draw crap.
      // more complicated to do optimally than quadrants, for sure. ideally,
      // we only get one color in an area.
      nccell_set_bg_alpha(c, NCALPHA_TRANSPARENT);
      if(!egcidx){
          nccell_set_fg_alpha(c, NCALPHA_TRANSPARENT);
          // FIXME else look for pairs of transparency!
      }else{
        if(blends){
          nccell_set_fg_rgb8(c, r / blends, g / blends, b / blends);
        }
        const char* egc = egcs[egcidx];
        if(pool_blit_direct(&nc->pool, c, egc, strlen(egc), 1) <= 0){
          return -1;
        }
      }
      ++total;
    }
  }
  return total;
}

static inline int
braille_blit(ncplane* nc, int linesize, const void* data, int leny, int lenx,
             const blitterargs* bargs){
  return blit_4x2(nc, linesize, data, leny, lenx, bargs, braille_egcs);
}

// NCBLIT_DEFAULT is not included, as it has no defined properties. It ought
// be replaced with some real blitter implementation by the calling widget.
// The order of contents is critical for 'egcs': ncplane_as_rgba() uses these
// arrays to map cells to source pixels. Map the upper-left logical bit to
// 1, and increase to the right, followed by down. The first egc ought thus
// always be space, to indicate an empty cell (all zeroes). These need be
// kept in the same order as the enums!
static struct blitset notcurses_blitters[] = {
   { .geom = NCBLIT_1x1,     .width = 1, .height = 1,
     .egcs = L" █", .plotegcs = L" █",
     .blit = tria_blit_ascii,.name = "ascii",         .fill = false, },
   { .geom = NCBLIT_2x1,     .width = 1, .height = 2,
     .egcs = NCHALFBLOCKS,   .plotegcs = L" ▄█",
     .blit = tria_blit,      .name = "half",          .fill = false, },
   { .geom = NCBLIT_2x2,     .width = 2, .height = 2,
     .egcs = NCQUADBLOCKS,   .plotegcs = L" ▗▐▖▄▟▌▙█",
     .blit = quadrant_blit,  .name = "quad",          .fill = false, },
   { .geom = NCBLIT_3x2,     .width = 2, .height = 3,
     .egcs = NCSEXBLOCKS,    .plotegcs = L" 🬞🬦▐🬏🬭🬵🬷🬓🬱🬹🬻▌🬲🬺█",
     .blit = sextant_blit,   .name = "sex",           .fill = false, },
   { .geom = NCBLIT_4x2,     .width = 2, .height = 4,
     .egcs = NCOCTBLOCKS,
     .plotegcs = (L"\0x20"
                  L"\U0001cea0"
                  L"\U00002597"
                  L"\U0001CD96"
                  L"\U0001CD91"
                  L"\U0001CEA3"
                  L"\U00002582"
                  L"\U0001CDCB"
                  L"\U0001CDD3"
                  L"\U0001CDCD"
                  L"\U00002596"
                  L"\U0001CDBB"
                  L"\U00002584"
                  L"\U0001CDE1"
                  L"\U0001CDDC"
                  L"\U0001CD48"
                  L"\U0001CDBF"
                  L"\U0001CDDE"
                  L"\U00002586"
                  L"\U0001CDDF"
                  L"\U0000258C"
                  L"\U0001CDC0"
                  L"\U00002599"
                  L"\U0001CDE4"
                  L"\U0001CDE0"),
     .blit = octant_blit,    .name = "oct",           .fill = false, },
   { .geom = NCBLIT_BRAILLE, .width = 2, .height = 4,
     .egcs = NCBRAILLEEGCS,
     .plotegcs = L"⠀⢀⢠⢰⢸⡀⣀⣠⣰⣸⡄⣄⣤⣴⣼⡆⣆⣦⣶⣾⡇⣇⣧⣷⣿",
     .blit = braille_blit,   .name = "braille",       .fill = true,  },
   { .geom = NCBLIT_PIXEL,   .width = 1, .height = 1,
     .egcs = L"", .plotegcs = NULL,
     .blit = NULL,           .name = "pixel",         .fill = true,  },
   { .geom = NCBLIT_4x1,     .width = 1, .height = 4,
     .egcs = NULL, .plotegcs = L" ▂▄▆█",
     .blit = tria_blit,      .name = "fourstep",      .fill = false, },
   { .geom = NCBLIT_8x1,     .width = 1, .height = 8,
     .egcs = NULL, .plotegcs = NCEIGHTHSB,
     .blit = tria_blit,      .name = "eightstep",     .fill = false, },
   { .geom = 0,              .width = 0, .height = 0,
     .egcs = NULL, .plotegcs = NULL,
     .blit = NULL,           .name = NULL,            .fill = false,  },
};

void set_pixel_blitter(ncblitter blitfxn){
  struct blitset* b = notcurses_blitters;
  while(b->geom != NCBLIT_PIXEL){
    ++b;
  }
  b->blit = blitfxn;
}

const struct blitset* lookup_blitset(const tinfo* tcache, ncblitter_e setid,
                                     bool may_degrade){
  if(setid == NCBLIT_DEFAULT){ // ought have resolved NCBLIT_DEFAULT before now
    return NULL;
  }
  // without braille support, NCBLIT_BRAILLE decays to NCBLIT_4x2
  if(setid == NCBLIT_BRAILLE){
    if(tcache->caps.braille){
      return &notcurses_blitters[setid - 1];
    }else if(!may_degrade){
      return NULL;
    }
    setid = NCBLIT_4x2;
  }
  // without octant support, NCBLIT_4x2 decays to NCBLIT_3x2
  if(setid == NCBLIT_4x2){
    if(tcache->caps.octants){
       return &notcurses_blitters[setid - 1];
    }else if(!may_degrade){
      return NULL;
    }
    setid = NCBLIT_3x2;
  }
  // without bitmap support, NCBLIT_PIXEL decays to NCBLIT_3x2
  if(setid == NCBLIT_PIXEL){
    if(tcache->pixel_draw || tcache->pixel_draw_late){
      return &notcurses_blitters[setid - 1];
    }else if(!may_degrade){
      return NULL;
    }
    setid = NCBLIT_3x2;
  }
  // without eighths support, NCBLIT_8x1 decays to NCBLIT_4x1
  if(setid == NCBLIT_8x1){ // plotter only
    if(tcache->caps.quadrants){
       return &notcurses_blitters[setid - 1];
    }else if(!may_degrade){
      return NULL;
    }
    setid = NCBLIT_4x1;
  }
  // without quarters support, NCBLIT_4x1 decays to NCBLIT_2x1
  if(setid == NCBLIT_4x1){ // plotter only
    if(tcache->caps.quadrants){
       return &notcurses_blitters[setid - 1];
    }else if(!may_degrade){
      return NULL;
    }
    setid = NCBLIT_2x1;
  }
  // without sextant support, NCBLIT_3x2 decays to NCBLIT_2x2
  if(setid == NCBLIT_3x2){
    if(tcache->caps.sextants){
       return &notcurses_blitters[setid - 1];
    }else if(!may_degrade){
      return NULL;
    }
    setid = NCBLIT_2x2;
  }
  // without quadrant support, NCBLIT_2x2 decays to NCBLIT_2x1
  if(setid == NCBLIT_2x2){
    if(tcache->caps.quadrants){
       return &notcurses_blitters[setid - 1];
    }else if(!may_degrade){
      return NULL;
    }
    setid = NCBLIT_2x1;
  }
  // without halfblock support, NCBLIT_2x1 decays to NCBLIT_1x1
  if(setid == NCBLIT_2x1){
    if(tcache->caps.halfblocks){
       return &notcurses_blitters[setid - 1];
    }else if(!may_degrade){
      return NULL;
    }
    setid = NCBLIT_1x1;
  }
  assert(NCBLIT_1x1 == setid);
  return &notcurses_blitters[setid - 1];
}

int notcurses_lex_blitter(const char* op, ncblitter_e* blitfxn){
  const struct blitset* bset = notcurses_blitters;
  while(bset->name){
    if(strcasecmp(bset->name, op) == 0){
      *blitfxn = bset->geom;
      return 0;
    }
    ++bset;
  }
  if(strcasecmp("default", op) == 0){
    *blitfxn = NCBLIT_DEFAULT;
    return 0;
  }
  return -1;
}

const char* notcurses_str_blitter(ncblitter_e blitfxn){
  if(blitfxn == NCBLIT_DEFAULT){
    return "default";
  }
  const struct blitset* bset = notcurses_blitters;
  while(bset->name){
    if(bset->geom == blitfxn){
      return bset->name;
    }
    ++bset;
  }
  return NULL;
}

int ncblit_bgrx(const void* data, int linesize, const struct ncvisual_options* vopts){
  if(vopts->leny <= 0 || vopts->lenx <= 0){
    logerror("invalid lengths %u %u", vopts->leny, vopts->lenx);
    return -1;
  }
  if(vopts->n == NULL){
    logerror("prohibited null plane");
    return -1;
  }
  void* rdata = bgra_to_rgba(data, vopts->leny, &linesize, vopts->lenx, 0xff);
  if(rdata == NULL){
    return -1;
  }
  int r = ncblit_rgba(rdata, linesize, vopts);
  free(rdata);
  return r;
}

int ncblit_rgb_loose(const void* data, int linesize,
                     const struct ncvisual_options* vopts, int alpha){
  if(vopts->leny <= 0 || vopts->lenx <= 0){
    return -1;
  }
  void* rdata = rgb_loose_to_rgba(data, vopts->leny, &linesize, vopts->lenx, alpha);
  if(rdata == NULL){
    return -1;
  }
  int r = ncblit_rgba(rdata, linesize, vopts);
  free(rdata);
  return r;
}

int ncblit_rgb_packed(const void* data, int linesize,
                      const struct ncvisual_options* vopts, int alpha){
  if(vopts->leny <= 0 || vopts->lenx <= 0){
    return -1;
  }
  void* rdata = rgb_packed_to_rgba(data, vopts->leny, &linesize, vopts->lenx, alpha);
  if(rdata == NULL){
    return -1;
  }
  int r = ncblit_rgba(rdata, linesize, vopts);
  free(rdata);
  return r;
}

int ncblit_rgba(const void* data, int linesize, const struct ncvisual_options* vopts){
  if(vopts->leny <= 0 || vopts->lenx <= 0){
    logerror("invalid lengths %u %u", vopts->leny, vopts->lenx);
    return -1;
  }
  if(vopts->n == NULL){
    logerror("prohibited null plane");
    return -1;
  }
  struct ncvisual* ncv = ncvisual_from_rgba(data, vopts->leny, linesize, vopts->lenx);
  if(ncv == NULL){
    return -1;
  }
  if(ncvisual_blit(ncplane_notcurses(vopts->n), ncv, vopts) == NULL){
    ncvisual_destroy(ncv);
    return -1;
  }
  ncvisual_destroy(ncv);
  return 0;
}

ncblitter_e ncvisual_media_defblitter(const notcurses* nc, ncscale_e scale){
  return rgba_blitter_default(&nc->tcache, scale);
}
