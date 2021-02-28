#include "internal.h"

/*
// monochromatic blitter for testing
static inline int
sixel_blit(ncplane* nc, int placey, int placex, int linesize,
           const void* data, int begy, int begx,
           int leny, int lenx, bool blendcolors){
  int dimy, dimx, x, y;
  int total = 0; // number of cells written
  ncplane_dim_yx(nc, &dimy, &dimx);
  int visy = begy;
  for(y = placey ; visy < (begy + leny) && y < dimy ; ++y, visy += 6){
    if(ncplane_cursor_move_yx(nc, y, placex)){
      return -1;
    }
    int visx = begx;
    for(x = placex ; visx < (begx + lenx) && x < dimx ; ++x, visx += 1){
      char sixel[128];
      unsigned bitsused = 0; // once 63, we're done
      for(int sy = visy ; sy < dimy && sy < visy + 6 ; ++sy){
        const uint32_t* rgb = (const uint32_t*)(data + (linesize * sy) + (visx * 4));
        if(rgba_trans_p(ncpixel_a(*rgb))){
          continue;
        }
        bitsused |= (1u << (sy - visy));
      }
      nccell* c = ncplane_cell_ref_yx(nc, y, x);
      int n = snprintf(sixel, sizeof(sixel), "#1;2;100;100;100#1%c", bitsused + 63);
      if(n){
        if(pool_blit_direct(&nc->pool, c, sixel, n, 1) <= 0){
          return -1;
        }
      } // FIXME otherwise, reset?
      cell_set_pixels(c, 1);
    }
  }
  (void)blendcolors; // FIXME
  return total;
}
*/

static inline void
break_sixel_comps(unsigned char comps[static 3], uint32_t rgba){
  comps[0] = ncpixel_r(rgba) * 100 / 255;
  comps[1] = ncpixel_g(rgba) * 100 / 255;
  comps[2] = ncpixel_b(rgba) * 100 / 255;
}

// Sixel blitter. Sixels are stacks 6 pixels high, and 1 pixel wide. RGB colors
// are programmed as a set of registers, which are then referenced by the
// stacks. There is also a RLE component, handled in rasterization.
// A pixel block is indicated by setting cell_pixels_p().
int sixel_blit(ncplane* nc, int placey, int placex, int linesize,
               const void* data, int begy, int begx,
               int leny, int lenx, bool blendcolors){
  int dimy, dimx, x, y;
  int total = 0; // number of cells written
  ncplane_dim_yx(nc, &dimy, &dimx);
  int visy = begy;
  for(y = placey ; visy < (begy + leny) && y < dimy ; ++y, visy += 6){
    if(ncplane_cursor_move_yx(nc, y, placex)){
      return -1;
    }
    int visx = begx;
    for(x = placex ; visx < (begx + lenx) && x < dimx ; ++x, visx += 1){
      size_t offset = 0;
#define GROWTHFACTOR 256
      size_t avail = GROWTHFACTOR;
      char* sixel = malloc(avail);
      // FIXME find sixels with common colors for single register program
      unsigned bitsused = 0; // once 63, we're done
      int colorreg = 1; // leave 0 as background
      bool printed = false;
      for(int sy = visy ; sy < dimy && sy < visy + 6 ; ++sy){
        const uint32_t* rgb = (const uint32_t*)(data + (linesize * sy) + (visx * 4));
        if(rgba_trans_p(ncpixel_a(*rgb))){
          continue;
        }
        if(bitsused & (1u << (sy - visy))){
          continue;
        }
        unsigned char comps[3];
        break_sixel_comps(comps, *rgb);
        unsigned thesebits = 1u << (sy - visy);
        for(int ty = sy + 1 ; ty < dimy && ty < visy + 6 ; ++ty){
          const uint32_t* trgb = (const uint32_t*)(data + (linesize * ty) + (visx * 4));
          if(!rgba_trans_p(ncpixel_a(*trgb))){
            unsigned char candcomps[3];
            break_sixel_comps(candcomps, *trgb);
            if(memcmp(comps, candcomps, sizeof(comps)) == 0){
              thesebits |= (1u << (ty - visy));
            }
          }
        }
        if(thesebits){
          bitsused |= thesebits;
          char c = 63 + thesebits;
          // FIXME use percentages(rgb)
          // bitstring is added to 63, resulting in [63, 126] aka '?'..'~'
          // FIXME grow if necessary
          int n = snprintf(sixel + offset, avail - offset,
                           "%s#%d;2;%u;%u;%u#%d%c", printed ? "$" : "",
                           colorreg, comps[0], comps[1], comps[2], colorreg, c);
          if(n < 0){
            return -1;
          }
          offset += n;
          ++colorreg;
          printed = true;
        }
        if(bitsused == 63){
          break;
        }
      }
      if(offset){
        nccell* c = ncplane_cell_ref_yx(nc, y, x);
        if(pool_blit_direct(&nc->pool, c, sixel, offset, 1) <= 0){
          free(sixel);
          return -1;
        }
        cell_set_pixels(c, 1);
      } // FIXME otherwise, reset?
      free(sixel);
    }
  }
  (void)blendcolors; // FIXME
  return total;
#undef GROWTHFACTOR
}

