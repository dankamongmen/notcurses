#include "internal.h"
#include "neuquant.h"

// FIXME you can have more (or fewer) than 256 registers...detect?
#define MAXCOLORS 256

// second pass: construct data for extracted colors over the sixels
typedef struct sixeltable {
  unsigned char* data;  // |colors|x|sixelcount|-byte arrays
  kohonenctx* kctx;
  int sixelcount;
} sixeltable;

// rather inelegant preprocess of the entire image. colors are converted to the
// 100x100x100 sixel colorspace, and built into a table.
static int
extract_ctable_inner(const uint32_t* data, int linesize, int begy, int begx,
                     int leny, int lenx, sixeltable* stab){
  int pos = 0;
  for(int visy = begy ; visy < (begy + leny) ; visy += 6){
    for(int visx = begx ; visx < (begx + lenx) ; visx += 1){
      for(int sy = visy ; sy < (begy + leny) && sy < visy + 6 ; ++sy){
        const uint32_t* rgb = (const uint32_t*)(data + (linesize / 4 * sy) + visx);
        if(rgba_trans_p(ncpixel_a(*rgb))){
          continue;
        }
        int c = inxsearch(stab->kctx, ncpixel_r(*rgb), ncpixel_g(*rgb), ncpixel_b(*rgb));
        if(c < 0){
//fprintf(stderr, "FAILED FINDING COLOR AUGH\n");
          return -1;
        }
        stab->data[c * stab->sixelcount + pos] |= (1u << (sy - visy));
//fprintf(stderr, "color %d pos %d: 0x%x\n", c, pos, stab->data[c * stab->sixelcount + pos]);
      }
      ++pos;
    }
  }
  return 0;
}

static inline void
initialize_stable(sixeltable* stab){
  memset(stab->data, 0, stab->sixelcount * MAXCOLORS);
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

  for(int i = 0 ; i < MAXCOLORS ; ++i){
    unsigned char rgb[3];
    netcolor(stab->kctx, i, rgb);
fprintf(stderr, "RGB[%03d]: %3d %3d %3d\n", i, rgb[0], rgb[1], rgb[2]);
    fprintf(fp, "#%d;2;%u;%u;%u", i, rgb[0] * 100 / 255, rgb[1] * 100 / 255, rgb[2] * 100 / 255);
  }
  int p = 0;
  while(p < stab->sixelcount){
    for(int i = 0 ; i < MAXCOLORS ; ++i){
      int printed = 0;
      int seenrle = 0; // number of repetitions
      unsigned char crle = 0; // character being repeated
      for(int m = p ; m < stab->sixelcount && m < p + lenx ; ++m){
//fprintf(stderr, "%d ", i * stab->sixelcount + m);
//fputc(stab->data[i * stab->sixelcount + m] + 63, stderr);
        if(seenrle){
          if(stab->data[i * stab->sixelcount + m] == crle){
            ++seenrle;
          }else{
            write_rle(&printed, i, fp, seenrle, crle);
            seenrle = 1;
            crle = stab->data[i * stab->sixelcount + m];
          }
        }else{
          seenrle = 1;
          crle = stab->data[i * stab->sixelcount + m];
        }
      }
      if(crle){
        write_rle(&printed, i, fp, seenrle, crle);
      }
      if(i + 1 < MAXCOLORS){
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
               int leny, int lenx, unsigned cellpixx){
  int sixelcount = (lenx - begx) * ((leny - begy + 5) / 6);
  sixeltable stable = {
    .data = malloc(MAXCOLORS * sixelcount),
    .sixelcount = sixelcount,
  };
  if(stable.data == NULL){
    return -1;
  }
  if((stable.kctx = initnet(data, leny, linesize, lenx, 1)) == NULL){
    free(stable.data);
  }
  learn(stable.kctx);
  unbiasnet(stable.kctx);
  inxbuild(stable.kctx);
  if(extract_color_table(data, linesize, begy, begx, leny, lenx, &stable)){
    free(stable.data);
    freenet(stable.kctx);
    return -1;
  }
  int r = sixel_blit_inner(nc, placey, placex, lenx, &stable, cellpixx);
  free(stable.data);
  freenet(stable.kctx);
  return r;
}
