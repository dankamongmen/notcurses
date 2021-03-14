#include "internal.h"

#define RGBSIZE 3
// FIXME you can have more (or fewer) than 256 registers...detect?
#define MAXCOLORS 256
#define CENTSIZE (RGBSIZE + 2) // size of a color table entry

static inline void
break_sixel_comps(unsigned char comps[static RGBSIZE], uint32_t rgba, unsigned char mask){
  comps[0] = (ncpixel_r(rgba) & mask) * 100 / 255;
  comps[1] = (ncpixel_g(rgba) & mask) * 100 / 255;
  comps[2] = (ncpixel_b(rgba) & mask) * 100 / 255;
//fprintf(stderr, "%u %u %u\n", comps[0], comps[1], comps[2]);
}

typedef struct cdetails {
  int64_t sums[3];         // sum of components of all matching original colors
  int32_t count;           // count of pixels matching
  unsigned char two_colors;// have we seen at least two colors?
  unsigned char r, g, b;   // until two_colors is true, track one r/g/b
} cdetails;

// second pass: construct data for extracted colors over the sixels
typedef struct sixeltable {
  // FIXME keep these internal to palette extraction; finalize there
  int colors;
  cdetails* deets;      // |maxcolors| cdetails structures
  unsigned char* data;  // |maxcolors| x |sixelcount|-byte arrays
  unsigned char* table; // |maxcolors| x CENTSIZE: components + dtable index
  int sixelcount;
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
static int
find_color(sixeltable* stab, unsigned char comps[static RGBSIZE]){
  int i;
  if(stab->colors){
    int l, r;
    l = 0;
    r = stab->colors - 1;
    do{
      i = l + (r - l) / 2;
//fprintf(stderr, "%02x%02x%02x L %d R %d m %d\n", comps[0], comps[1], comps[2], l, r, i);
      int cmp = memcmp(stab->table + i * CENTSIZE, comps, RGBSIZE);
      if(cmp == 0){
        return ctable_to_dtable(stab->table + i * CENTSIZE);
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
    }else if(l == stab->colors){
      i = stab->colors;
    }else{
      i = l;
    }
    if(stab->colors == MAXCOLORS){
      return -1;
    }
    if(i < stab->colors){
//fprintf(stderr, "INSERTING COLOR %u %u %u AT %d\n", comps[0], comps[1], comps[2], i);
      memmove(stab->table + (i + 1) * CENTSIZE, stab->table + i * CENTSIZE,
              (stab->colors - i) * CENTSIZE);
    }
  }else{
    i = 0;
  }
//fprintf(stderr, "NEW COLOR CONCAT %u %u %u AT %d\n", comps[0], comps[1], comps[2], i);
  memcpy(stab->table + i * CENTSIZE, comps, RGBSIZE);
  dtable_to_ctable(stab->colors, stab->table + i * CENTSIZE);
  ++stab->colors;
  return stab->colors - 1;
  //return ctable_to_dtable(stab->table + i * CENTSIZE);
}

// no matter the input palette, we can always get a maximum of 64 colors if we
// mask at 0xc0 on each component (this partitions each component into 4 chunks,
// and 4 * 4 * 4 -> 64). so this will never overflow our color register table
// (assumed to have at least 256 registers). at each color, we store a pixel
// count, and a sum of all three channels. in addition, we track whether we've
// seen at least two colors in the chunk.
static int
extract_color_table(const uint32_t* data, int linesize, int begy, int begx,
                    int leny, int lenx, sixeltable* stab){
  unsigned char mask = 0xc0;
  int pos = 0;
  for(int visy = begy ; visy < (begy + leny) ; visy += 6){
    for(int visx = begx ; visx < (begx + lenx) ; visx += 1){
      for(int sy = visy ; sy < (begy + leny) && sy < visy + 6 ; ++sy){
        const uint32_t* rgb = (const uint32_t*)(data + (linesize / 4 * sy) + visx);
        if(rgba_trans_p(ncpixel_a(*rgb))){
          continue;
        }
        unsigned char comps[RGBSIZE];
        break_sixel_comps(comps, *rgb, mask);
        int c = find_color(stab, comps);
        if(c < 0){
//fprintf(stderr, "FAILED FINDING COLOR AUGH 0x%02x\n", mask);
          return -1;
        }
        stab->data[c * stab->sixelcount + pos] |= (1u << (sy - visy));
        if(stab->deets[c].count == 0){
          stab->deets[c].two_colors = false;
          stab->deets[c].r = ncpixel_r(*rgb);
          stab->deets[c].g = ncpixel_g(*rgb);
          stab->deets[c].b = ncpixel_b(*rgb);
        }else if(stab->deets[c].two_colors == false){
          if(stab->deets[c].r != ncpixel_r(*rgb) ||
             stab->deets[c].g != ncpixel_g(*rgb) ||
             stab->deets[c].b != ncpixel_b(*rgb)){
            stab->deets[c].two_colors = true;
          }
        }
        stab->deets[c].sums[0] += ncpixel_r(*rgb);
        stab->deets[c].sums[1] += ncpixel_g(*rgb);
        stab->deets[c].sums[2] += ncpixel_b(*rgb);
        ++stab->deets[c].count;
//fprintf(stderr, "color %d pos %d: 0x%x\n", c, pos, stab->data[c * stab->sixelcount + pos]);
//fprintf(stderr, " sums: %u %u %u count: %d r/g/b: %u %u %u\n", stab->deets[c].sums[0], stab->deets[c].sums[1], stab->deets[c].sums[2], stab->deets[c].count, ncpixel_r(*rgb), ncpixel_g(*rgb), ncpixel_b(*rgb));
      }
      ++pos;
    }
  }
  return 0;
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

  for(int i = 0 ; i < stab->colors ; ++i){
    const unsigned char* rgb = stab->table + i * CENTSIZE;
    int idx = ctable_to_dtable(rgb);
    int count = stab->deets[idx].count;
//fprintf(stderr, "RGB: %3u %3u %3u DT: %d SUMS: %3d %3d %3d COUNT: %d\n", rgb[0], rgb[1], rgb[2], idx, stab->deets[idx].sums[0] / count * 100 / 255, stab->deets[idx].sums[1] / count * 100 / 255, stab->deets[idx].sums[2] / count * 100 / 255, count);
    //fprintf(fp, "#%d;2;%u;%u;%u", i, rgb[0], rgb[1], rgb[2]);
    fprintf(fp, "#%d;2;%jd;%jd;%jd", i,
            (intmax_t)(stab->deets[idx].sums[0] * 100 / count / 255),
            (intmax_t)(stab->deets[idx].sums[1] * 100 / count / 255),
            (intmax_t)(stab->deets[idx].sums[2] * 100 / count / 255));
  }
  int p = 0;
  while(p < stab->sixelcount){
    for(int i = 0 ; i < stab->colors ; ++i){
      int printed = 0;
      int seenrle = 0; // number of repetitions
      unsigned char crle = 0; // character being repeated
      int idx = ctable_to_dtable(stab->table + i * CENTSIZE);
      for(int m = p ; m < stab->sixelcount && m < p + lenx ; ++m){
//fprintf(stderr, "%d ", idx * stab->sixelcount + m);
//fputc(stab->data[idx * stab->sixelcount + m] + 63, stderr);
        if(seenrle){
          if(stab->data[idx * stab->sixelcount + m] == crle){
            ++seenrle;
          }else{
            write_rle(&printed, i, fp, seenrle, crle);
            seenrle = 1;
            crle = stab->data[idx * stab->sixelcount + m];
          }
        }else{
          seenrle = 1;
          crle = stab->data[idx * stab->sixelcount + m];
        }
      }
      if(crle){
        write_rle(&printed, i, fp, seenrle, crle);
      }
      if(i + 1 < stab->colors){
        if(printed){
          fputc('$', fp);
        }
      }else{
        if(p + lenx < stab->sixelcount){
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
               int leny, int lenx, const blitterargs* bargs){
  int sixelcount = (lenx - begx) * ((leny - begy + 5) / 6);
  sixeltable stable = {
    .data = malloc(MAXCOLORS * sixelcount),
    .deets = malloc(MAXCOLORS * sizeof(cdetails)),
    .table = malloc(MAXCOLORS * CENTSIZE),
    .sixelcount = sixelcount,
    .colors = 0,
  };
  if(stable.data == NULL || stable.deets == NULL || stable.table == NULL){
    free(stable.table);
    free(stable.deets);
    free(stable.data);
    return -1;
  }
  // stable.table doesn't need initializing; we start from the bottom
  memset(stable.data, 0, sixelcount * MAXCOLORS);
  memset(stable.deets, 0, sizeof(*stable.deets) * MAXCOLORS);
  if(extract_color_table(data, linesize, begy, begx, leny, lenx, &stable)){
    free(stable.table);
    free(stable.data);
    free(stable.deets);
    return -1;
  }
  int r = sixel_blit_inner(nc, placey, placex, lenx, &stable, bargs->pixel.celldimx);
  free(stable.data);
  free(stable.deets);
  free(stable.table);
  return r;
}
