#include "internal.h"

static inline void
break_sixel_comps(unsigned char comps[static 3], uint32_t rgba, unsigned char mask){
  comps[0] = (ncpixel_r(rgba) & mask) * 100 / 255;
  comps[1] = (ncpixel_g(rgba) & mask) * 100 / 255;
  comps[2] = (ncpixel_b(rgba) & mask) * 100 / 255;
//fprintf(stderr, "%u %u %u\n", comps[0], comps[1], comps[2]);
}

// FIXME you can have more (or fewer) than 256 registers...detect?
#define MAXCOLORS 256

// first pass: extract up to 256 sixelspace colors over arbitrarily many sixels
// sixelspace is 0..100 corresponding to 0..255, lame =[
typedef struct colortable {
  int colors;
  int sixelcount;
  unsigned char table[5 * MAXCOLORS]; // components + dtable index
} colortable;

// second pass: construct data for extracted colors over the sixels
typedef struct sixeltable {
  colortable* ctab;
  unsigned char* data;  // |colors|x|sixelcount|-byte arrays
} sixeltable;

static inline int
ctable_to_dtable(const unsigned char* ctable){
  return ctable[3] * 256 + ctable[4];
}

static inline void
dtable_to_ctable(int dtable, unsigned char* ctable){
  ctable[3] = dtable / 256;
  ctable[4] = dtable % 256;
}

// returns the index at which the provided color can be found *in the
// dtable*, possibly inserting it into the ctable. returns -1 if the
// color is not in the table and the table is full.
// FIXME replace all these 3s and 5s
static int
find_color(colortable* ctab, unsigned char comps[static 3]){
  int i;
  if(ctab->colors){
    int l, r;
    l = 0;
    r = ctab->colors - 1;
    do{
      i = l + (r - l) / 2;
//fprintf(stderr, "%02x%02x%02x L %d R %d m %d\n", comps[0], comps[1], comps[2], l, r, i);
      int cmp = memcmp(ctab->table + i * 5, comps, 3);
      if(cmp == 0){
        return ctable_to_dtable(ctab->table + i * 5);
      }
      if(cmp < 0){
        l = i + 1;
      }else{ // key is smaller
        r = i - 1;
      }
//fprintf(stderr, "BCMP: %d L %d R %d m: %d\n", cmp, l, r, i);
    }while(l <= r);
    if(r < 0){
      i = 0;
    }else if(l == ctab->colors){
      i = ctab->colors;
    }else{
      i = l;
    }
    if(ctab->colors == MAXCOLORS){
      return -1;
    }
    if(i < ctab->colors){
      memmove(ctab->table + (i + 1) * 5, ctab->table + i * 5, (ctab->colors - i) * 5);
    }
  }else{
    i = 0;
  }
  memcpy(ctab->table + i * 5, comps, 3);
  dtable_to_ctable(ctab->colors, ctab->table + i * 5);
  ++ctab->colors;
  return ctab->colors - 1;
  //return ctable_to_dtable(ctab->table + i * 5);
}

// rather inelegant preprocess of the entire image. colors are converted to the
// 100x100x100 sixel colorspace, and built into a table.
static int
extract_ctable_inner(const uint32_t* data, int linesize, int begy, int begx,
                     int leny, int lenx, sixeltable* stab, unsigned char mask){
  int pos = 0;
  for(int visy = begy ; visy < (begy + leny) ; visy += 6){
    for(int visx = begx ; visx < (begx + lenx) ; visx += 1){
      for(int sy = visy ; sy < (begy + leny) && sy < visy + 6 ; ++sy){
        const uint32_t* rgb = (const uint32_t*)(data + (linesize / 4 * sy) + visx);
        if(rgba_trans_p(ncpixel_a(*rgb))){
          continue;
        }
        unsigned char comps[3];
        break_sixel_comps(comps, *rgb, mask);
        int c = find_color(stab->ctab, comps);
        if(c < 0){
//fprintf(stderr, "FAILED FINDING COLOR AUGH 0x%02x\n", mask);
          return -1;
        }
        stab->data[c * stab->ctab->sixelcount + pos] |= (1u << (sy - visy));
//fprintf(stderr, "color %d pos %d: 0x%x\n", c, pos, stab->data[c * stab->ctab->sixelcount + pos]);
      }
      ++pos;
    }
  }
  return 0;
}

static inline void
initialize_stable(sixeltable* stab){
  stab->ctab->colors = 0;
  memset(stab->data, 0, stab->ctab->sixelcount * MAXCOLORS);
}

// Use as many of the original colors as we can, but not more than will fit
// into the set of color registers. We're already losing some precision by the
// RGB -> sixelspace conversion (256->100); try with the complete colors, and
// progressively mask more out until they all fit.
static int
extract_color_table(const uint32_t* data, int linesize, int begy, int begx,
                    int leny, int lenx, sixeltable* stab, unsigned char* mask){
  *mask = 0xff;
  while(mask){
    initialize_stable(stab);
    if(extract_ctable_inner(data, linesize, begy, begx, leny, lenx, stab, *mask) == 0){
      return 0;
    }
    *mask <<= 1;
    *mask &= 0xff;
  }
  return -1;
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
write_sixel_data(FILE* fp, int leny, int lenx, sixeltable* stab){
  fprintf(fp, "\e[?80h\ePq"); // FIXME pixelon

  // Set Raster Attributes - pan/pad=1 (pixel aspect ratio), Ph=lenx, Pv=leny
  fprintf(fp, "\"1;1;%d;%d", lenx, leny);

  for(int i = 0 ; i < stab->ctab->colors ; ++i){
    const unsigned char* rgb = stab->ctab->table + i * 5;
    fprintf(fp, "#%d;2;%u;%u;%u", i, rgb[0], rgb[1], rgb[2]);
  }
  int p = 0;
  while(p < stab->ctab->sixelcount){
    for(int i = 0 ; i < stab->ctab->colors ; ++i){
      fprintf(fp, "#%d", i);
      int seenrle = 0;
      unsigned char crle = 0;
      int idx = ctable_to_dtable(stab->ctab->table + i * 5);
      for(int m = p ; m < stab->ctab->sixelcount && m < p + lenx ; ++m){
//fprintf(stderr, "%d ", idx * stab->ctab->sixelcount + m);
//fputc(stab->data[idx * stab->ctab->sixelcount + m] + 63, stderr);
        if(seenrle){
          if(stab->data[idx * stab->ctab->sixelcount + m] == crle){
            ++seenrle;
          }else{
            write_rle(fp, seenrle, crle);
            seenrle = 1;
            crle = stab->data[idx * stab->ctab->sixelcount + m];
          }
        }else{
          seenrle = 1;
          crle = stab->data[idx * stab->ctab->sixelcount + m];
        }
      }
      write_rle(fp, seenrle, crle);
      if(i + 1 < stab->ctab->colors){
        fputc('$', fp);
      }else{
        if(p + lenx < stab->ctab->sixelcount){
          fputc('-', fp);
        }
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
int sixel_blit_inner(ncplane* nc, int placey, int placex, int leny, int lenx,
                     sixeltable* stab, unsigned cellpixx){
  char* buf = NULL;
  size_t size = 0;
  FILE* fp = open_memstream(&buf, &size);
  if(fp == NULL){
    return -1;
  }
  if(write_sixel_data(fp, leny, lenx, stab)){
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
  ctab->sixelcount = (lenx - begx) * ((leny - begy + 5) / 6);
  sixeltable stable = {
    .ctab = ctab,
    .data = malloc(MAXCOLORS * ctab->sixelcount),
  };
  if(stable.data == NULL){
    free(ctab);
    return -1;
  }
  unsigned char mask;
  if(extract_color_table(data, linesize, begy, begx, leny, lenx, &stable, &mask)){
    free(ctab);
    free(stable.data);
    return -1;
  }
  int r = sixel_blit_inner(nc, placey, placex, leny, lenx, &stable, cellpixx);
  free(stable.data);
  free(ctab);
  return r;
}
