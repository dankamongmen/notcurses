#include "internal.h"

#define RGBSIZE 3

static inline void
break_sixel_comps(unsigned char comps[static RGBSIZE], uint32_t rgba){
  comps[0] = ncpixel_r(rgba) * 100 / 255;
  comps[1] = ncpixel_g(rgba) * 100 / 255;
  comps[2] = ncpixel_b(rgba) * 100 / 255;
//fprintf(stderr, "%u %u %u\n", comps[0], comps[1], comps[2]);
}

// FIXME you can have more (or fewer) than 256 registers...detect?
#define MAXCOLORS 256
#define CENTSIZE (4 + (MAXCOLORS > 256))

// first pass: extract up to 256 sixelspace colors over arbitrarily many sixels
// sixelspace is 0..100 corresponding to 0..255, lame =[
typedef struct colortable {
  int colors;
  int sixelcount;
  unsigned char table[CENTSIZE * MAXCOLORS]; // components + dtable index
} colortable;

// second pass: construct data for extracted colors over the sixels
typedef struct sixeltable {
  colortable* ctab;
  unsigned char* data;  // |colors|x|sixelcount|-byte arrays
} sixeltable;

static inline int
ctable_to_dtable(const unsigned char* ctable){
  if(MAXCOLORS > 256){
    return ctable[RGBSIZE] * 256 + ctable[RGBSIZE + 1];
  }else{
    return ctable[RGBSIZE];
  }
}

static inline void
dtable_to_ctable(int dtable, unsigned char* ctable){
  if(MAXCOLORS > 256){
    ctable[RGBSIZE] = dtable / 256;
    ctable[RGBSIZE + 1] = dtable % 256;
  }else{
    ctable[RGBSIZE] = dtable;
  }
}

// returns the index at which the provided color can be found *in the
// dtable*, possibly inserting it into the ctable. returns -1 if the
// color is not in the table and the table is full.
static int
find_color(colortable* ctab, unsigned char comps[static RGBSIZE]){
  int i;
  if(ctab->colors){
    int l, r;
    l = 0;
    r = ctab->colors - 1;
    do{
      i = l + (r - l) / 2;
//fprintf(stderr, "%02x%02x%02x L %d R %d m %d\n", comps[0], comps[1], comps[2], l, r, i);
      int cmp = memcmp(ctab->table + i * CENTSIZE, comps, RGBSIZE);
      if(cmp == 0){
        return ctable_to_dtable(ctab->table + i * CENTSIZE);
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
      memmove(ctab->table + (i + 1) * CENTSIZE, ctab->table + i * CENTSIZE,
              (ctab->colors - i) * CENTSIZE);
    }
  }else{
    i = 0;
  }
  memcpy(ctab->table + i * CENTSIZE, comps, RGBSIZE);
  dtable_to_ctable(ctab->colors, ctab->table + i * CENTSIZE);
  ++ctab->colors;
  return ctab->colors - 1;
  //return ctable_to_dtable(ctab->table + i * CENTSIZE);
}

// rather inelegant preprocess of the entire image. colors are converted to the
// 100x100x100 sixel colorspace, and built into a table.
static int
extract_ctable_inner(const uint32_t* data, int linesize, int begy, int begx,
                     int leny, int lenx, sixeltable* stab){
  int pos = 0;
  for(int visy = begy ; visy < (begy + leny) ; visy += 6){
    for(int visx = begx ; visx < (begx + lenx) ; visx += 1){
      for(int sy = visy ; sy < (begy + leny) && sy < visy + 6 ; ++sy){
        const uint32_t* rgb = (const uint32_t*)(data + (linesize / CENTSIZE * sy) + visx);
        if(rgba_trans_p(ncpixel_a(*rgb))){
          continue;
        }
        unsigned char comps[RGBSIZE];
        break_sixel_comps(comps, *rgb);
        int c = find_color(stab->ctab, comps);
        if(c < 0){
//fprintf(stderr, "FAILED FINDING COLOR AUGH\n");
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
// RGB -> sixelspace conversion (256->100); try with the complete colors.
static int
extract_color_table(const uint32_t* data, int linesize, int begy, int begx,
                    int leny, int lenx, sixeltable* stab){
  initialize_stable(stab);
  if(extract_ctable_inner(data, linesize, begy, begx, leny, lenx, stab) == 0){
    return 0;
  }
  return -1;
}

// Emit some number of equivalent, subsequent sixels, using sixel RLE. We've
// seen the sixel |crle| for |seenrle| columns in a row. |seenrle| must > 0.
static int
write_rle(int* printed, int color, FILE* fp, int seenrle, unsigned char crle){
  if(!*printed){
    fprintf(fp, "#%d", color);
    *printed = 1;
  }
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
  // \e[?80: DECSDM "sixel scrolling" mode (put output at cursor location)
  // \x90: 8-bit "device control sequence", lowercase q (start sixel)
  // doesn't seem to work with at least xterm; we instead use '\ePq'
  // FIXME i think we can print DESDM on the first one, and never again
  fprintf(fp, "\e[?80h\ePq");

  // Set Raster Attributes - pan/pad=1 (pixel aspect ratio), Ph=lenx, Pv=leny
  // using Ph/Pv causes a background to be drawn using color register 0 for all
  // unspecified pixels, which we do not want.
  //fprintf(fp, "\"1;1;%d;%d", lenx, leny);

  for(int i = 0 ; i < stab->ctab->colors ; ++i){
    const unsigned char* rgb = stab->ctab->table + i * CENTSIZE;
    fprintf(fp, "#%d;2;%u;%u;%u", i, rgb[0], rgb[1], rgb[2]);
  }
  int p = 0;
  while(p < stab->ctab->sixelcount){
    for(int i = 0 ; i < stab->ctab->colors ; ++i){
      int printed = 0;
      int seenrle = 0; // number of repetitions
      unsigned char crle = 0; // character being repeated
      int idx = ctable_to_dtable(stab->ctab->table + i * CENTSIZE);
      for(int m = p ; m < stab->ctab->sixelcount && m < p + lenx ; ++m){
//fprintf(stderr, "%d ", idx * stab->ctab->sixelcount + m);
//fputc(stab->data[idx * stab->ctab->sixelcount + m] + 63, stderr);
        if(seenrle){
          if(stab->data[idx * stab->ctab->sixelcount + m] == crle){
            ++seenrle;
          }else{
            write_rle(&printed, i, fp, seenrle, crle);
            seenrle = 1;
            crle = stab->data[idx * stab->ctab->sixelcount + m];
          }
        }else{
          seenrle = 1;
          crle = stab->data[idx * stab->ctab->sixelcount + m];
        }
      }
      if(crle){
        write_rle(&printed, i, fp, seenrle, crle);
      }
      if(i + 1 < stab->ctab->colors){
        if(printed){
          fputc('$', fp);
        }
      }else{
        if(p + lenx < stab->ctab->sixelcount){
          fputc('-', fp);
        }
      }
    }
    p += lenx;
  }
  // \x9c: 8-bit "string terminator" (end sixel) doesn't work on at
  // least xterm; we instead use '\e\\'
  fprintf(fp, "\e\\");
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
  cell_set_pixels(c, 1);
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
  if(extract_color_table(data, linesize, begy, begx, leny, lenx, &stable)){
    free(ctab);
    free(stable.data);
    return -1;
  }
  int r = sixel_blit_inner(nc, placey, placex, lenx, &stable, cellpixx);
  free(stable.data);
  free(ctab);
  return r;
}
