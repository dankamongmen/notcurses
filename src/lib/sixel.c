#include "internal.h"

#define RGBSIZE 3
#define CENTSIZE (RGBSIZE + 1) // size of a color table entry

// we set P2 based on whether there is any transparency in the sixel. if not,
// use SIXEL_P2_ALLOPAQUE (0), for faster drawing in certain terminals.
typedef enum {
  SIXEL_P2_ALLOPAQUE = 0,
  SIXEL_P2_TRANS = 1,
} sixel_p2_e;

// the P2 parameter on a sixel specifies how unspecified pixels are drawn.
// if P2 is 1, unspecified pixels are transparent. otherwise, they're drawn
// as something else. some terminals (e.g. foot) can draw more quickly if
// P2 is 0, so we set that when we have no transparent pixels -- i.e. when
// all TAM entries are 0. P2 is at a fixed location in the sixel header.
// obviously, the sixel must already exist.
static inline void
change_p2(char* sixel, sixel_p2_e value){
  sixel[4] = value + '0';
}

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
  sixel_p2_e p2;        // set to SIXEL_P2_TRANS if we have transparent pixels
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
extract_color_table(const uint32_t* data, int linesize, int cols,
                    int leny, int lenx, sixeltable* stab,
                    sprixcell_e* tacache, const blitterargs* bargs){
  const int begx = bargs->begx;
  const int begy = bargs->begy;
  const int cdimy = bargs->u.pixel.celldimy;
  const int cdimx = bargs->u.pixel.celldimx;
  unsigned char mask = 0xc0;
  int pos = 0; // pixel position
  for(int visy = begy ; visy < (begy + leny) ; visy += 6){ // pixel row
    for(int visx = begx ; visx < (begx + lenx) ; visx += 1){ // pixel column
      for(int sy = visy ; sy < (begy + leny) && sy < visy + 6 ; ++sy){ // offset within sprixel
        const uint32_t* rgb = (data + (linesize / 4 * sy) + visx);
        int txyidx = (sy / cdimy) * cols + (visx / cdimx);
        if(tacache[txyidx] == SPRIXCELL_ANNIHILATED){
//fprintf(stderr, "TRANS SKIP %d %d %d %d (cell: %d %d)\n", visy, visx, sy, txyidx, sy / cdimy, visx / cdimx);
          continue;
        }
        if(rgba_trans_p(*rgb, bargs->transcolor)){
          if(sy % cdimy == 0 && visx % cdimx == 0){
            tacache[txyidx] = SPRIXCELL_TRANSPARENT;
          }else if(tacache[txyidx] == SPRIXCELL_OPAQUE_SIXEL){
            tacache[txyidx] = SPRIXCELL_MIXED_SIXEL;
          }
          stab->p2 = SIXEL_P2_TRANS; // even one forces P2=1
          continue;
        }else{
          if(sy % cdimy == 0 && visx % cdimx == 0){
            tacache[txyidx] = SPRIXCELL_OPAQUE_SIXEL;
          }else if(tacache[txyidx] == SPRIXCELL_TRANSPARENT){
            tacache[txyidx] = SPRIXCELL_MIXED_SIXEL;
          }
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
write_rle(int* printed, int color, FILE* fp, int seenrle, unsigned char crle,
          int* needclosure){
  if(!*printed){
    fprintf(fp, "%s#%d", *needclosure ? "$" : "", color);
    *printed = 1;
    *needclosure = 0;
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
write_sixel_data(FILE* fp, int leny, int lenx, const sixeltable* stab, int* parse_start,
                 const char* cursor_hack, sixel_p2_e p2){
  // Set Raster Attributes - pan/pad=1 (pixel aspect ratio), Ph=lenx, Pv=leny
  *parse_start = fprintf(fp, "\eP0;%d;0q\"1;1;%d;%d", p2, lenx, leny);
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
    int needclosure = 0;
    for(int i = 0 ; i < stab->colors ; ++i){
      int seenrle = 0; // number of repetitions
      unsigned char crle = 0; // character being repeated
      int idx = ctable_to_dtable(stab->table + i * CENTSIZE);
      int printed = 0;
      for(int m = p ; m < stab->sixelcount && m < p + lenx ; ++m){
//fprintf(stderr, "%d ", idx * stab->sixelcount + m);
//fputc(stab->data[idx * stab->sixelcount + m] + 63, stderr);
        if(seenrle){
          if(stab->data[idx * stab->sixelcount + m] == crle){
            ++seenrle;
          }else{
            write_rle(&printed, i, fp, seenrle, crle, &needclosure);
            seenrle = 1;
            crle = stab->data[idx * stab->sixelcount + m];
          }
        }else{
          seenrle = 1;
          crle = stab->data[idx * stab->sixelcount + m];
        }
      }
      if(crle){
        write_rle(&printed, i, fp, seenrle, crle, &needclosure);
      }
      needclosure = needclosure | printed;
    }
    if(p + lenx < stab->sixelcount){
      fputc('-', fp);
    }
    p += lenx;
  }
  // \x9c: 8-bit "string terminator" (end sixel) doesn't work on at
  // least xterm; we instead use '\e\\'
  fprintf(fp, "\e\\");
  if(cursor_hack){
    fprintf(fp, "%s", cursor_hack);
  }
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
  if(write_sixel_data(fp, leny, lenx, stab, &parse_start,
                      bargs->u.pixel.cursor_hack, stab->p2)){
    free(buf);
    return -1;
  }
  scrub_tam_boundaries(tacache, leny, lenx, bargs->u.pixel.celldimy,
                       bargs->u.pixel.celldimx);
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
  if((leny - bargs->begy) % 6){
    return -1;
  }
  int sixelcount = (lenx - bargs->begx) * (leny - bargs->begy) / 6;
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
    .p2 = SIXEL_P2_ALLOPAQUE,
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
    if(n->leny == rows && n->lenx == cols){
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
  if(extract_color_table(data, linesize, cols, leny, lenx,
                         &stable, tacache, bargs)){
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

int sixel_destroy(const notcurses* nc, const ncpile* p, FILE* out, sprixel* s){
//fprintf(stderr, "%d] %d %p\n", s->id, s->invalidated, s->n);
  (void)out;
  (void)nc;
  int starty = s->movedfromy;
  int startx = s->movedfromx;
  for(int yy = starty ; yy < starty + s->dimy && yy < p->dimy ; ++yy){
    for(int xx = startx ; xx < startx + s->dimx && xx < p->dimx ; ++xx){
      struct crender *r = &p->crender[yy * p->dimx + xx];
      if(!r->sprixel){
        r->s.damaged = 1;
      }
    }
  }
  return 0;
}

// offered 'rle' instances of 'c', up through 'y'x'x' (pixel coordinates)
// within 's', determine if any of these bytes need be elided due to recent
// annihilation. note that annihilation could have taken place anywhere along
// the continuous 'rle' instances. see deepclean_stream() regarding trusting
// our input, and keep your CVEs to yourselves. remember that this covers six
// rows at a time, [y..y + 5].
static int
deepclean_output(FILE* fp, const sprixel* s, int y, int *x, int rle,
                 int* printed, int color, int* needclosure, char c){
  c -= 63;
  int rlei = 0;
  // xi loops over the section we cover, a minimum of 1 pixel and a maximum
  // of one line. FIXME can skip (celldimx - 1) / celldimx checks, do so!
  for(int xi = *x ; xi < *x + rle ; ++xi){
    unsigned char mask = 0;
    for(int yi = y ; yi < y + 6 ; ++yi){
      const int tidx = (yi / s->cellpxy) * s->dimx + (xi / s->cellpxx);
      const bool nihil = (s->n->tacache[tidx] == SPRIXCELL_ANNIHILATED);
      if(!nihil){
        mask |= (1u << (yi - y));
      }
    }
    if((c & mask) != c){
      if(rlei){
//fprintf(stderr, "writing %d:%d..%d (%c)\n", y, xi, xi + rlei - 1, c + 63);
        if(write_rle(printed, color, fp, rlei, c, needclosure)){
          return -1;
        }
        rlei = 0;
      }
      // FIXME can rle on this
      if(write_rle(printed, color, fp, 1, c & mask, needclosure)){
        return -1;
      }
    }else{
      ++rlei;
    }
  }
  if(rlei){
//fprintf(stderr, "writing %d:%d..%d (%c)\n", y, *x - (rle - rlei), *x + rlei - 1, c + 63);
    if(write_rle(printed, color, fp, rlei, c, needclosure)){
      return -1;
    }
    rlei = 0;
  }
  *x += rle;
  return 0;
}

// we should have already copied everything up through parse_start. we now
// read from the old sixel, copying through whatever we find, unless it's been
// obliterated by a SPRIXCELL_ANNIHILATED. this is *not* suitable as a general
// sixel lexer, but it works for all sixels we generate. we explicitly do not
// protect against e.g. overflow of the color/RLE specs, or an RLE that takes
// us past data boundaries, because we're not taking random external data. i'm
// sure this will one day bite me in my ass and lead to president celine dion.
static int
deepclean_stream(sprixel* s, FILE* fp){
  int idx = s->parse_start;
  enum {
    SIXEL_WANT_HASH, // we ought get a '#' or '-'
    SIXEL_EAT_COLOR, // we're reading the color until we hit data
    SIXEL_EAT_COLOR_EPSILON, // actually process color
    SIXEL_EAT_RLE,   // we're reading the repetition count
    SIXEL_EAT_RLE_EPSILON, // actually process rle
    SIXEL_EAT_DATA   // we're reading data until we hit EOL
  } state = SIXEL_WANT_HASH;
  int color = 0;
  int rle = 1;
  int y = 0;
  int x = 0;
  int printed;
  int needclosure = 0;
  while(idx + 2 < s->glyphlen){
    const char c = s->glyph[idx];
//fprintf(stderr, "%d] %c (%d) (%d/%d)\n", state, c, c, idx, s->glyphlen);
    if(state == SIXEL_WANT_HASH){
      if(c == '#'){
        state = SIXEL_EAT_COLOR;
        color = 0;
        printed = 0;
      }else if(c == '-'){
        y += 6;
        x = 0;
      }else{
        return -1;
      }
    }
    // we require an actual digit where a color or an RLE is expected,
    // so verify that the first char in the state is indeed a digit,
    // and fall through to epsilon states below to actually process it.
    else if(state == SIXEL_EAT_COLOR){
      if(isdigit(c)){
        state = SIXEL_EAT_COLOR_EPSILON;
      }else{
        return -1;
      }
    }else if(state == SIXEL_EAT_RLE){
      if(isdigit(c)){
        state = SIXEL_EAT_RLE_EPSILON;
      }else{
        return -1;
      }
    }

    // handle the color/rle digits, with implicit fallthrough from
    // the EAT_COLOR/EAT_RLE states above.
    if(state == SIXEL_EAT_COLOR_EPSILON){
      if(isdigit(c)){
        color *= 10;
        color += c - '0';
      }else{
        state = SIXEL_EAT_DATA;
        rle = 1;
      }
    }else if(state == SIXEL_EAT_RLE_EPSILON){
      if(isdigit(c)){
        rle *= 10;
        rle += c - '0';
      }else{
        state = SIXEL_EAT_DATA;
      }
    }

    if(state == SIXEL_EAT_DATA){
      if(c == '!'){
        state = SIXEL_EAT_RLE;
        rle = 0;
      }else if(c == '-'){
        y += 6;
        x = 0;
        state = SIXEL_WANT_HASH;
        needclosure = 0;
        fputc('-', fp);
      }else if(c == '$'){
        x = 0;
        state = SIXEL_WANT_HASH;
        needclosure = needclosure | printed;
      }else if(c < 63 || c > 126){
        return -1;
      }else{ // data byte
        if(deepclean_output(fp, s, y, &x, rle, &printed, color,
                            &needclosure, c)){
          return -1;
        }
        rle = 1;
      }
    }
    ++idx;
  }
  fprintf(fp, "\e\\");
  return 0;
}

static int
sixel_deepclean(sprixel* s){
  char* buf = NULL;
  size_t size = 0;
  FILE* fp = open_memstream(&buf, &size);
  if(fwrite(s->glyph, 1, s->parse_start, fp) != (size_t)s->parse_start){
    goto err;
  }
  if(deepclean_stream(s, fp)){
    goto err;
  }
  if(fclose(fp) == EOF){
    free(buf);
    return -1;
  }
  free(s->glyph);
  s->glyph = buf;
//fprintf(stderr, "Deepclean! %d -> %zu\n", s->glyphlen, size);
  s->glyphlen = size;
  return 0;

err:
  fclose(fp);
  free(buf);
  return -1;
}

int sixel_draw(const notcurses* n, const ncpile* p, sprixel* s, FILE* out){
  (void)n;
  // if we've wiped any cells, we need actually wipe them out now, or else
  // we'll get flicker when we move to the new location
  if(s->wipes_outstanding){
    if(sixel_deepclean(s)){
      return -1;
    }
    s->wipes_outstanding = false;
  }
  if(s->invalidated == SPRIXEL_MOVED){
    for(int yy = s->movedfromy ; yy < s->movedfromy + s->dimy && yy < p->dimy ; ++yy){
      for(int xx = s->movedfromx ; xx < s->movedfromx + s->dimx && xx < p->dimx ; ++xx){
        struct crender *r = &p->crender[yy * p->dimx + xx];
        if(!r->sprixel || sprixel_state(r->sprixel, yy, xx) != SPRIXCELL_OPAQUE_SIXEL){
          r->s.damaged = 1;
        }
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

int sixel_init(int fd){
  // \e[?8452: DECSDM private "sixel scrolling" mode keeps the sixel from
  // scrolling, but puts it at the current cursor location (as opposed to
  // the upper left corner of the screen).
  return tty_emit("\e[?80;8452h", fd);
}

// we return -1 because we're not doing a proper wipe -- that's not possible
// using sixel. we just mark it as partially transparent, so that if it's
// redrawn, it's redrawn using P2=1.
int sixel_wipe(const notcurses* nc, sprixel* s, int ycell, int xcell){
  (void)nc;
  if(s->n->tacache[s->dimx * ycell + xcell] == SPRIXCELL_ANNIHILATED){
//fprintf(stderr, "CACHED WIPE %d %d/%d\n", s->id, ycell, xcell);
    return 1; // already annihilated FIXME but 0 breaks things
  }
  s->wipes_outstanding = true;
  change_p2(s->glyph, SIXEL_P2_TRANS);
  return -1;
}

// 80 (sixel scrolling) is enabled by default. 8452 is not. XTSAVE/XTRESTORE
// would be better, where they're supported.
int sixel_shutdown(int fd){
  return tty_emit("\e[?8452l", fd);
}
