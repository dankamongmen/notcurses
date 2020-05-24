#include "internal.h"

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
                int leny, int lenx, bool bgr){
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
          int leny, int lenx, bool bgr){
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
      const unsigned char* rgbbase_down = (const unsigned char*)"\x00\x00\x00\x00";
      if(visy < begy + leny - 1){
        rgbbase_down = dat + (linesize * (visy + 1)) + (visx * bpp / CHAR_BIT);
      }
//fprintf(stderr, "[%04d/%04d] bpp: %d lsize: %d %02x %02x %02x %02x\n", y, x, bpp, linesize, rgbbase_up[0], rgbbase_up[1], rgbbase_up[2], rgbbase_up[3]);
      cell* c = ncplane_cell_ref_yx(nc, y, x);
      // use the default for the background, as that's the only way it's
      // effective in that case anyway
      c->channels = 0;
      c->attrword = 0;
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

// quadrant blitter. maps 2x2 to each cell. since we only have two colors at
// our disposal (foreground and background), we lose some fidelity.
static inline int
quadrant_blit(ncplane* nc, int placey, int placex, int linesize,
              const void* data, int begy, int begx,
              int leny, int lenx, bool bgr){
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
      const unsigned char* rgbbase_tr = (const unsigned char*)"\x00\x00\x00\x00";
      const unsigned char* rgbbase_bl = (const unsigned char*)"\x00\x00\x00\x00";
      const unsigned char* rgbbase_br = (const unsigned char*)"\x00\x00\x00\x00";
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
      // use the default for the background, as that's the only way it's
      // effective in that case anyway
      c->channels = 0;
      c->attrword = 0;
      // FIXME for now, we just sample, color-wise, and always draw a Panamanian.
      // we ought look for pixels with the same color, and combine them glyph-wise.
      // even a pair helps tremendously. we then ought interpolate the rest. we
      // can't have three pairs, or even a triplet plus a pair, so the first pair we
      // find gets locked in, and then lerp the rest. assign an index into the egcs.
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
        cell_set_fg_rgb(c, rgbbase_tl[rpos], rgbbase_tl[1], rgbbase_tl[bpos]);
        cell_set_bg_rgb(c, rgbbase_br[rpos], rgbbase_br[1], rgbbase_br[bpos]);
        egc = "▚";
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
// be replaced with some real blitter implementation.
const struct blitset geomdata[] = {
   { .geom = NCBLIT_8x1,     .width = 1, .height = 9, .egcs = L" ▁▂▃▄▅▆▇█",
     .blit = NULL,           .fill = false, },
   { .geom = NCBLIT_1x1,     .width = 1, .height = 2, .egcs = L" █",
     .blit = tria_blit_ascii,.fill = false, },
   { .geom = NCBLIT_2x1,     .width = 1, .height = 3, .egcs = L" ▄█",
     .blit = tria_blit,      .fill = false, },
   { .geom = NCBLIT_1x1x4,   .width = 1, .height = 5, .egcs = L" ▒░▓█",
     .blit = NULL,           .fill = false, },
   { .geom = NCBLIT_2x2,     .width = 2, .height = 3, .egcs = L" ▗▐▖▄▟▌▙█",
     .blit = quadrant_blit,  .fill = false, },
   { .geom = NCBLIT_4x1,     .width = 1, .height = 5, .egcs = L" ▂▄▆█",
     .blit = NULL,           .fill = false, },
   { .geom = NCBLIT_BRAILLE, .width = 2, .height = 5, .egcs = L"⠀⡀⡄⡆⡇⢀⣀⣄⣆⣇⢠⣠⣤⣦⣧⢰⣰⣴⣶⣷⢸⣸⣼⣾⣿",
     .blit = NULL,           .fill = true,  },
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
    return tria_blit_ascii(nc, placey, placex, linesize, data, begy, begx, leny, lenx, true);
  }
  return tria_blit(nc, placey, placex, linesize, data, begy, begx, leny, lenx, true);
}

int ncblit_rgba(ncplane* nc, int placey, int placex,
                int linesize, const void* data, int begy, int begx, int leny,
                int lenx){
  if(!nc->nc->utf8){
    return tria_blit_ascii(nc, placey, placex, linesize, data, begy, begx, leny, lenx, false);
  }
  return tria_blit(nc, placey, placex, linesize, data, begy, begx, leny, lenx, false);
}

int rgba_blit_dispatch(ncplane* nc, const struct blitset* bset, int placey,
                       int placex, int linesize, const void* data, int begy,
                       int begx, int leny, int lenx){
  return bset->blit(nc, placey, placex, linesize, data, begy, begx, leny, lenx, false);
}
