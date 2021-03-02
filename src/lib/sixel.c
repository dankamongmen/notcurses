#include "internal.h"

static inline void
break_sixel_comps(unsigned char comps[static 3], uint32_t rgba, unsigned char mask){
  comps[0] = (ncpixel_r(rgba) & mask) * 100 / 255;
  comps[1] = (ncpixel_g(rgba) & mask) * 100 / 255;
  comps[2] = (ncpixel_b(rgba) & mask) * 100 / 255;
//fprintf(stderr, "%u %u %u\n", comps[0], comps[1], comps[2]);
}

// first pass: extract up to 256 sixelspace colors over arbitrarily many sixels
// sixelspace is 0..100 corresponding to 0..255, lame =[
// FIXME you can have more (or fewer) than 256 registers...detect?
typedef struct colortable {
  int colors;
  int sixelcount;
  unsigned char table[3 * 256];
} colortable;

// second pass: construct data for extracted colors over the sixels
typedef struct sixeltable {
  const colortable* ctab;
  unsigned char* data;  // |colors|x|sixelcount|-byte arrays
} sixeltable;

// returns the index at which the provided color can be found, possibly
// inserting it into the table. returns -1 if the color is not in the
// table and the table is full.
// FIXME switch to binary search, duh
// FIXME replace all these 3s with sizeof(comps)
static int
find_color(colortable* ctab, unsigned char comps[static 3]){
  int i;
  for(i = 0 ; i < ctab->colors ; ++i){
    int cmp = memcmp(ctab->table + i * 3, comps, 3);
    if(cmp == 0){
      return i;
    }else if(cmp > 0){
      break;
    }
  }
  if(ctab->colors == sizeof(ctab->table) / 3){
    return -1;
  }
  if(i < ctab->colors){
    memmove(ctab->table + (i + 1) * 3, ctab->table + i * 3, (ctab->colors - i) * 3);
  }
  memcpy(ctab->table + i * 3, comps, 3);
  ++ctab->colors;
  return i;
}

// rather inelegant preprocess of the entire image. colors are converted to the
// 100x100x100 sixel colorspace, and built into a table. if there are more than
// 256 converted colors, we (currently) reject it FIXME. ideally we'd do the
// image piecemeal, allowing us to get complete color fidelity; barring that,
// we'd have something sensibly quantize us down first FIXME. we ought do
// everything in a single pass FIXME.
// what do we do if every pixel is transparent (0 colors)? FIXME
static int
extract_ctable_inner(const uint32_t* data, int linesize, int begy, int begx,
                     int leny, int lenx, colortable* ctab, unsigned char mask){
  for(int visy = begy ; visy < (begy + leny) ; visy += 6){
    for(int visx = begx ; visx < (begx + lenx) ; visx += 1){
      for(int sy = visy ; sy < (begy + leny) && sy < visy + 6 ; ++sy){
        const uint32_t* rgb = (const uint32_t*)(data + (linesize / 4 * sy) + (visx));
        if(rgba_trans_p(ncpixel_a(*rgb))){
          continue;
        }
        unsigned char comps[3];
        break_sixel_comps(comps, *rgb, mask);
        if(find_color(ctab, comps) < 0){
          return -1;
        }
      }
      ++ctab->sixelcount;
    }
  }
  return 0;
}

static inline void
initialize_ctable(colortable* ctab){
  ctab->colors = 0;
  ctab->sixelcount = 0;
}

// Use as many of the original colors as we can, but not more than will fit
// into the set of color registers. We're already losing some precision by the
// RGB -> sixelspace conversion (256->100); try with the complete colors, and
// progressively mask more out until they all fit.
static int
extract_color_table(const uint32_t* data, int linesize, int begy, int begx,
                    int leny, int lenx, colortable* ctab, unsigned char* mask){
  *mask = 0xff;
  while(mask){
    initialize_ctable(ctab);
    if(extract_ctable_inner(data, linesize, begy, begx, leny, lenx, ctab, *mask) == 0){
      return 0;
    }
    *mask <<= 1;
    *mask &= 0xff;
  }
  return -1;
}

// Having assembled the color table, build the raw sixels.
static int
extract_data_table(const uint32_t* data, int linesize, int begy, int begx,
                   int leny, int lenx, sixeltable* stab, unsigned char mask){
//fprintf(stderr, "colors: %d sixelcount: %d\n", stab->ctab->colors, stab->ctab->sixelcount);
  for(int c = 0 ; c < stab->ctab->colors ; ++c){
    int pos = 0;
//fprintf(stderr, "dimy/x: %d/%d placey/x: %d/%d begyx: %d/%d lenyx: %d/%d\n", dimy, dimx, placey, placex, begy, begx, leny, lenx);
    for(int visy = begy ; visy < (begy + leny) ; visy += 6){
      for(int visx = begx ; visx < (begx + lenx) ; visx += 1){
//fprintf(stderr, "handling sixel %d for color %d visy: %d\n", pos, c, visy);
        for(int sy = visy ; sy < (begy + leny) && sy < visy + 6 ; ++sy){
          const uint32_t* rgb = (const uint32_t*)(data + (linesize / 4 * sy) + visx);
//fprintf(stderr, "%p: %08x\n", rgb, *rgb);
          if(rgba_trans_p(ncpixel_a(*rgb))){
//fprintf(stderr, "transparent\n");
            continue;
          }
          unsigned char comps[3];
          break_sixel_comps(comps, *rgb, mask);
//fprintf(stderr, "%d/%d/%d\n", comps[0], comps[1], comps[2]);
          if(memcmp(comps, stab->ctab->table + c * 3, 3) == 0){
            stab->data[c * stab->ctab->sixelcount + pos] |= (1u << (sy - visy));
//fprintf(stderr, "%d ", c * stab->ctab->sixelcount + pos);
//fputc(stab->data[c * stab->ctab->sixelcount + pos] + 63, stderr);
          }
        }
//fprintf(stderr, "color %d pos %d: %u\n", c, pos, stab->data[c * stab->ctab->sixelcount + pos]);
        ++pos;
      }
    }
  }
  return 0;
}

// Emit some number of equivalent, subsequent sixels, using sixel RLE. We've
// seen the sixel |crle| for |seenrle| columns in a row. |seenrle| must > 0.
static int
write_rle(FILE* fp, int seenrle, unsigned char crle){
  crle += 63;
  if(seenrle == 1){
    if(fputc(crle, fp) == EOF){
      return -1;
    }
  }else if(seenrle == 2){
    if(fprintf(fp, "%c%c", crle, crle) <= 0){
      return -1;
    }
  }else{
    if(fprintf(fp, "!%d%c", seenrle, crle) <= 0){
      return -1;
    }
  }
  return 0;
}

// Emit the sprixel in its entirety, plus enable and disable pixel mode.
static int
write_sixel_data(FILE* fp, int lenx, sixeltable* stab){
  fprintf(fp, "\e[?80h\ePq"); // FIXME pixelon
  for(int i = 0 ; i < stab->ctab->colors ; ++i){
    const unsigned char* rgb = stab->ctab->table + i * 3;
    fprintf(fp, "#%d;2;%u;%u;%u", i, rgb[0], rgb[1], rgb[2]);
  }
  int p = 0;
  while(p < stab->ctab->sixelcount){
    for(int i = 0 ; i < stab->ctab->colors ; ++i){
      fprintf(fp, "#%d", i);
      int seenrle = 0;
      unsigned char crle = 0;
      for(int m = p ; m < stab->ctab->sixelcount && m < p + lenx ; ++m){
//fprintf(stderr, "%d ", i * stab->ctab->sixelcount + m);
//fputc(stab->data[i * stab->ctab->sixelcount + m] + 63, stderr);
        if(seenrle){
          if(stab->data[i * stab->ctab->sixelcount + m] == crle){
            ++seenrle;
          }else{
            write_rle(fp, seenrle, crle);
            seenrle = 1;
            crle = stab->data[i * stab->ctab->sixelcount + m];
          }
        }else{
          seenrle = 1;
          crle = stab->data[i * stab->ctab->sixelcount + m];
        }
      }
      write_rle(fp, seenrle, crle);
      if(i + 1 < stab->ctab->colors){
        fputc('$', fp);
      }else{
        fputc('-', fp);
      }
    }
    p += lenx;
  }
  fprintf(fp, "\e\\"); // FIXME pixeloff
  if(fclose(fp) == EOF){
    return -1;
  }
  return 0;
}

// Sixel blitter. Sixels are stacks 6 pixels high, and 1 pixel wide. RGB colors
// are programmed as a set of registers, which are then referenced by the
// stacks. There is also a RLE component, handled in rasterization.
// A pixel block is indicated by setting cell_pixels_p().
int sixel_blit_inner(ncplane* nc, int placey, int placex, int lenx,
                     sixeltable* stab, unsigned cellpixx){
  char* buf = NULL;
  size_t size = 0;
  FILE* fp = open_memstream(&buf, &size);
  if(fp == NULL){
    return -1;
  }
  if(write_sixel_data(fp, lenx, stab)){
    fclose(fp);
    free(buf);
    return -1;
  }
  nccell* c = ncplane_cell_ref_yx(nc, placey, placex);
  unsigned width = lenx / cellpixx + !!(lenx % cellpixx);
  if(pool_blit_direct(&nc->pool, c, buf, size, width) < 0){
    free(buf);
    return -1;
  }
  free(buf);
  return 1;
}

int sixel_blit(ncplane* nc, int placey, int placex, int linesize,
               const void* data, int begy, int begx,
               int leny, int lenx, unsigned cellpixx){
  colortable* ctab = malloc(sizeof(*ctab));
  if(ctab == NULL){
    return -1;
  }
  unsigned char mask;
  if(extract_color_table(data, linesize, begy, begx, leny, lenx, ctab, &mask)){
    free(ctab);
    return -1;
  }
  sixeltable stable = {
    .ctab = ctab,
    .data = malloc(ctab->colors * ctab->sixelcount),
  };
  if(stable.data == NULL){
    free(ctab);
    return -1;
  }
  memset(stable.data, 0, ctab->colors * ctab->sixelcount);
  if(extract_data_table(data, linesize, begy, begx, leny, lenx, &stable, mask)){
    free(stable.data);
    free(ctab);
    return -1;
  }
  int r = sixel_blit_inner(nc, placey, placex, lenx, &stable, cellpixx);
  free(stable.data);
  free(ctab);
  return r;
}
