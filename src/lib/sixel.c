#include "internal.h"

#define RGBSIZE 3
#define CENTSIZE (RGBSIZE + 1) // size of a color table entry

// take (8-bit rgb value & mask) to sixelspace [0..100]
static inline char
ss(unsigned rgb, unsigned char mask){
  return (rgb & mask) * 100 / 255;
}

static inline void
break_sixel_comps(unsigned char comps[static RGBSIZE], uint32_t rgba, unsigned char mask){
  comps[0] = ss(ncpixel_r(rgba), mask);
  comps[1] = ss(ncpixel_g(rgba), mask);
  comps[2] = ss(ncpixel_b(rgba), mask);
//fprintf(stderr, "%u %u %u\n", comps[0], comps[1], comps[2]);
}

typedef struct cdetails {
  int64_t sums[3];   // sum of components of all matching original colors
  int32_t count;     // count of pixels matching
  char hi[RGBSIZE];  // highest sixelspace components we've seen
  char lo[RGBSIZE];  // lowest sixelspace color we've seen
} cdetails;

// second pass: construct data for extracted colors over the sixels
typedef struct sixeltable {
  // FIXME keep these internal to palette extraction; finalize there
  int colors;
  cdetails* deets;      // |colorregs| cdetails structures
  unsigned char* data;  // |colorregs| x |sixelcount|-byte arrays
  unsigned char* table; // |colorregs| x CENTSIZE: components + dtable index
  int sixelcount;
  int colorregs;
} sixeltable;

static inline int
ctable_to_dtable(const unsigned char* ctable){
  return ctable[3]; // * 256 + ctable[4];
}

static inline void
dtable_to_ctable(int dtable, unsigned char* ctable){
  ctable[3] = dtable;
  /*ctable[3] = dtable / 256;
  ctable[4] = dtable % 256;*/
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
    if(stab->colors == stab->colorregs){
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

static void
update_deets(uint32_t rgb, cdetails* deets){
  unsigned char comps[RGBSIZE];
  deets->sums[0] += ncpixel_r(rgb);
  deets->sums[1] += ncpixel_g(rgb);
  deets->sums[2] += ncpixel_b(rgb);
  comps[0] = ss(ncpixel_r(rgb), 0xff);
  comps[1] = ss(ncpixel_g(rgb), 0xff);
  comps[2] = ss(ncpixel_b(rgb), 0xff);
  if(deets->count == 0){
    deets->lo[0] = deets->hi[0] = comps[0];
    deets->lo[1] = deets->hi[1] = comps[1];
    deets->lo[2] = deets->hi[2] = comps[2];
  }else{
    if(deets->hi[0] < comps[0]){
      deets->hi[0] = comps[0];
    }else if(deets->lo[0] > comps[0]){
      deets->lo[0] = comps[0];
    }
    if(deets->hi[1] < comps[1]){
      deets->hi[1] = comps[1];
    }else if(deets->lo[1] > comps[1]){
      deets->lo[1] = comps[1];
    }
    if(deets->hi[2] < comps[2]){
      deets->hi[2] = comps[2];
    }else if(deets->lo[2] > comps[2]){
      deets->lo[2] = comps[2];
    }
  }
  ++deets->count;
}

// no matter the input palette, we can always get a maximum of 64 colors if we
// mask at 0xc0 on each component (this partitions each component into 4 chunks,
// and 4 * 4 * 4 -> 64). so this will never overflow our color register table
// (assumed to have at least 256 registers). at each color, we store a pixel
// count, and a sum of all three channels. in addition, we track whether we've
// seen at least two colors in the chunk.
static inline int
extract_color_table(const uint32_t* data, int linesize, int begy, int begx, int cols,
                    int leny, int lenx, int cdimy, int cdimx, sixeltable* stab,
                    sprixcell_e* tacache){
  unsigned char mask = 0xc0;
  int pos = 0; // pixel position
  for(int visy = begy ; visy < (begy + leny) ; visy += 6){ // pixel row
    for(int visx = begx ; visx < (begx + lenx) ; visx += 1){ // pixel column
      for(int sy = visy ; sy < (begy + leny) && sy < visy + 6 ; ++sy){ // offset within sprixel
        const uint32_t* rgb = (data + (linesize / 4 * sy) + visx);
        int txyidx = (sy / cdimy) * cols + (visx / cdimx);
        if(rgba_trans_p(ncpixel_a(*rgb))){
          if(tacache[txyidx] == SPRIXCELL_NORMAL){
            tacache[txyidx] = SPRIXCELL_CONTAINS_TRANS;
          }
          continue;
        }
        if(tacache[txyidx] == SPRIXCELL_ANNIHILATED){
//fprintf(stderr, "TRANS SKIP %d %d %d %d (cell: %d %d)\n", visy, visx, sy, txyidx, sy / cdimy, visx / cdimx);
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
        update_deets(*rgb, &stab->deets[c]);
//fprintf(stderr, "color %d pos %d: 0x%x\n", c, pos, stab->data[c * stab->sixelcount + pos]);
//fprintf(stderr, " sums: %u %u %u count: %d r/g/b: %u %u %u\n", stab->deets[c].sums[0], stab->deets[c].sums[1], stab->deets[c].sums[2], stab->deets[c].count, ncpixel_r(*rgb), ncpixel_g(*rgb), ncpixel_b(*rgb));
      }
      ++pos;
    }
  }
  return 0;
}

// run through the sixels matching color |src|, going to color |stab->colors|,
// keeping those under |r||g||b|, and putting those above it into the new
// color. rebuilds both sixel groups and color details.
static void
unzip_color(const uint32_t* data, int linesize, int begy, int begx,
            int leny, int lenx, sixeltable* stab, int src,
            unsigned char rgb[static 3]){
  unsigned char* tcrec = stab->table + CENTSIZE * stab->colors;
  dtable_to_ctable(stab->colors, tcrec);
  cdetails* targdeets = stab->deets + stab->colors;
  unsigned char* crec = stab->table + CENTSIZE * src;
  int didx = ctable_to_dtable(crec);
  cdetails* deets = stab->deets + didx;
  unsigned char* srcsixels = stab->data + stab->sixelcount * didx;
  unsigned char* dstsixels = stab->data + stab->sixelcount * stab->colors;
//fprintf(stderr, "counts: src: %d dst: %d src: %p dst: %p\n", deets->count, targdeets->count, srcsixels, dstsixels);
  int sixel = 0;
  memset(deets, 0, sizeof(*deets));
  for(int visy = begy ; visy < (begy + leny) ; visy += 6){
    for(int visx = begx ; visx < (begx + lenx) ; visx += 1, ++sixel){
      if(srcsixels[sixel]){
        for(int sy = visy ; sy < (begy + leny) && sy < visy + 6 ; ++sy){
          if(srcsixels[sixel] & (1u << (sy - visy))){
            const uint32_t* pixel = (const uint32_t*)(data + (linesize / 4 * sy) + visx);
            unsigned char comps[RGBSIZE];
            break_sixel_comps(comps, *pixel, 0xff);
            if(comps[0] > rgb[0] || comps[1] > rgb[1] || comps[2] > rgb[2]){
              dstsixels[sixel] |= (1u << (sy - visy));
              srcsixels[sixel] &= ~(1u << (sy - visy));
              update_deets(*pixel, targdeets);
//fprintf(stderr, "%u/%u/%u comps: [%u/%u/%u]\n", r, g, b, comps[0], comps[1], comps[2]);
//fprintf(stderr, "match sixel %d %u %u\n", sixel, srcsixels[sixel], 1u << (sy - visy));
            }else{
              update_deets(*pixel, deets);
            }
          }
        }
      }
    }
  }
}

// relax segment |coloridx|. we must have room for a new color. we find the
// biggest component gap, and split our color entry in half there. we know
// the elements can't go into any preexisting color entry, so the only
// choices are staying where they are, or going to the new one. "unzip" the
// sixels from the data table by looking back to the sources and classifying
// them in one or the other centry. rebuild our sums, sixels, hi/lo, and
// counts as we do so. anaphase, baybee! target always gets the upper range.
// returns 1 if we did a refinement, 0 otherwise.
static int
refine_color(const uint32_t* data, int linesize, int begy, int begx,
             int leny, int lenx, sixeltable* stab, int color){
  unsigned char* crec = stab->table + CENTSIZE * color;
  int didx = ctable_to_dtable(crec);
  cdetails* deets = stab->deets + didx;
  int rdelt = deets->hi[0] - deets->lo[0];
  int gdelt = deets->hi[1] - deets->lo[1];
  int bdelt = deets->hi[2] - deets->lo[2];
  unsigned char rgbmax[3] = { deets->hi[0], deets->hi[1], deets->hi[2] };
  if(gdelt >= rdelt && gdelt >= bdelt){ // split on green
    if(gdelt < 3){
      return 0;
    }
//fprintf(stderr, "[%d->%d] SPLIT ON GREEN %d %d (pop: %d)\n", color, stab->colors, deets->hi[1], deets->lo[1], deets->count);
    rgbmax[1] = deets->lo[1] + (deets->hi[1] - deets->lo[1]) / 2;
  }else if(rdelt >= gdelt && rdelt >= bdelt){ // split on red
    if(rdelt < 3){
      return 0;
    }
//fprintf(stderr, "[%d->%d] SPLIT ON RED %d %d (pop: %d)\n", color, stab->colors, deets->hi[0], deets->lo[0], deets->count);
    rgbmax[0] = deets->lo[0] + (deets->hi[0] - deets->lo[0]) / 2;
  }else{ // split on blue
    if(bdelt < 3){
      return 0;
    }
//fprintf(stderr, "[%d->%d] SPLIT ON BLUE %d %d (pop: %d)\n", color, stab->colors, deets->hi[2], deets->lo[2], deets->count);
    rgbmax[2] = deets->lo[2] + (deets->hi[2] - deets->lo[2]) / 2;
  }
  unzip_color(data, linesize, begy, begx, leny, lenx, stab, color, rgbmax);
  ++stab->colors;
  return 1;
}

// relax the details down into free color registers
static void
refine_color_table(const uint32_t* data, int linesize, int begy, int begx,
                   int leny, int lenx, sixeltable* stab){
  while(stab->colors < stab->colorregs){
    bool refined = false;
    int tmpcolors = stab->colors; // force us to come back through
    for(int i = 0 ; i < tmpcolors ; ++i){
      unsigned char* crec = stab->table + CENTSIZE * i;
      int didx = ctable_to_dtable(crec);
      cdetails* deets = stab->deets + didx;
//fprintf(stderr, "[%d->%d] hi: %d %d %d lo: %d %d %d\n", i, didx, deets->hi[0], deets->hi[1], deets->hi[2], deets->lo[0], deets->lo[1], deets->lo[2]);
      if(deets->count > leny * lenx / stab->colorregs){
        if(refine_color(data, linesize, begy, begx, leny, lenx, stab, i)){
          if(stab->colors == stab->colorregs){
  //fprintf(stderr, "filled table!\n");
            break;
          }
          refined = true;
        }
      }
    }
    if(!refined){ // no more possible work
      break;
    }
  }
  // we're full!
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
// Closes |fp| on all paths.
static int
write_sixel_data(FILE* fp, int lenx, const sixeltable* stab, int* parse_start){
  *parse_start = fprintf(fp, "\ePq");
  // Set Raster Attributes - pan/pad=1 (pixel aspect ratio), Ph=lenx, Pv=leny
  // using Ph/Pv causes a background to be drawn using color register 0 for all
  // unspecified pixels, which we do not want.
//  fprintf(fp, "\"1;1;%d;%d", lenx, leny);

  for(int i = 0 ; i < stab->colors ; ++i){
    const unsigned char* rgb = stab->table + i * CENTSIZE;
    int idx = ctable_to_dtable(rgb);
    int count = stab->deets[idx].count;
//fprintf(stderr, "RGB: %3u %3u %3u DT: %d SUMS: %3d %3d %3d COUNT: %d\n", rgb[0], rgb[1], rgb[2], idx, stab->deets[idx].sums[0] / count * 100 / 255, stab->deets[idx].sums[1] / count * 100 / 255, stab->deets[idx].sums[2] / count * 100 / 255, count);
    //fprintf(fp, "#%d;2;%u;%u;%u", i, rgb[0], rgb[1], rgb[2]);
    *parse_start += fprintf(fp, "#%d;2;%jd;%jd;%jd", i,
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
static inline int
sixel_blit_inner(int leny, int lenx, const sixeltable* stab, int rows, int cols,
                 const blitterargs* bargs, sprixcell_e* tacache){
  char* buf = NULL;
  size_t size = 0;
  FILE* fp = open_memstream(&buf, &size);
  if(fp == NULL){
    return -1;
  }
  int parse_start = 0;
  // calls fclose() on success
  if(write_sixel_data(fp, lenx, stab, &parse_start)){
    free(buf);
    return -1;
  }
  // take ownership of buf on success
  if(plane_blit_sixel(bargs->u.pixel.spx, buf, size, rows, cols,
                      bargs->placey, bargs->placex,
                      leny, lenx, parse_start, tacache) < 0){
    free(buf);
    return -1;
  }
  return 1;
}

int sixel_blit(ncplane* n, int linesize, const void* data,
               int leny, int lenx, const blitterargs* bargs){
  int sixelcount = (lenx - bargs->begx) * ((leny - bargs->begy + 5) / 6);
  int colorregs = bargs->u.pixel.colorregs;
  if(colorregs <= 0){
    return -1;
  }
  if(colorregs > 256){
    colorregs = 256;
  }
  sixeltable stable = {
    .data = malloc(colorregs * sixelcount),
    .deets = malloc(colorregs * sizeof(cdetails)),
    .table = malloc(colorregs * CENTSIZE),
    .sixelcount = sixelcount,
    .colorregs = colorregs,
    .colors = 0,
  };
  if(stable.data == NULL || stable.deets == NULL || stable.table == NULL){
    free(stable.table);
    free(stable.deets);
    free(stable.data);
    return -1;
  }
  // stable.table doesn't need initializing; we start from the bottom
  memset(stable.data, 0, sixelcount * colorregs);
  memset(stable.deets, 0, sizeof(*stable.deets) * colorregs);
  int cols = bargs->u.pixel.spx->dimx;
  int rows = bargs->u.pixel.spx->dimy;
  sprixcell_e* tacache = NULL;
  bool reuse = false;
  // if we have a sprixel attached to this plane, see if we can reuse it
  // (we need the same dimensions) and thus immediately apply its T-A table.
  if(n->tacache){
//fprintf(stderr, "IT'S A REUSE %d %d %d %d\n", n->tacachey, rows, n->tacachex, cols);
    if(n->tacachey == rows && n->tacachex == cols){
      tacache = n->tacache;
      reuse = true;
    }
  }
  if(!reuse){
    tacache = malloc(sizeof(*tacache) * rows * cols);
    if(tacache == NULL){
      return -1;
    }
    memset(tacache, 0, sizeof(*tacache) * rows * cols);
  }
  if(extract_color_table(data, linesize, bargs->begy, bargs->begx, cols, leny, lenx,
                         bargs->u.pixel.celldimy, bargs->u.pixel.celldimx,
                         &stable, tacache)){
    if(!reuse){
      free(tacache);
    }
    free(stable.table);
    free(stable.data);
    free(stable.deets);
    return -1;
  }
  refine_color_table(data, linesize, bargs->begy, bargs->begx, leny, lenx, &stable);
  int r = sixel_blit_inner(leny, lenx, &stable, rows, cols, bargs, tacache);
  free(stable.data);
  free(stable.deets);
  free(stable.table);
  return r;
}

int sixel_delete(const notcurses* nc, const ncpile* p, FILE* out, sprixel* s){
//fprintf(stderr, "%d] %d %p\n", s->id, s->invalidated, s->n);
  (void)nc;
  (void)out;
  for(int yy = s->movedfromy ; yy < s->movedfromy + s->dimy ; ++yy){
    for(int xx = s->movedfromx ; xx < s->movedfromx + s->dimx ; ++xx){
      struct crender *r = &p->crender[yy * p->dimx + xx];
      if(!r->sprixel){
        r->s.damaged = 1;
      }
    }
  }
  return 0;
}

int sixel_draw(const notcurses* n, const ncpile* p, sprixel* s, FILE* out){
  (void)n;
  if(s->invalidated == SPRIXEL_MOVED){
    for(int yy = s->movedfromy ; yy < s->movedfromy + s->dimy ; ++yy){
      for(int xx = s->movedfromx ; xx < s->movedfromx + s->dimx ; ++xx){
        p->crender[yy * p->dimx + xx].s.damaged = 1;
      }
    }
    s->invalidated = SPRIXEL_INVALIDATED;
  }else{
    if(fwrite(s->glyph, s->glyphlen, 1, out) != 1){
      return -1;
    }
    s->invalidated = SPRIXEL_QUIESCENT;
  }
  return 0;
}

int sprite_sixel_init(int fd){
  // \e[?8452: DECSDM private "sixel scrolling" mode keeps the sixel from
  // scrolling, but puts it at the current cursor location (as opposed to
  // the upper left corner of the screen).
  return tty_emit("\e[?80;8452h", fd);
}
