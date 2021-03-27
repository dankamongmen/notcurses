#include "internal.h"

// sixel is in a sense simpler to edit in-place than kitty, as it has neither
// chunking nor base64 to worry about. in another sense, it's waaay suckier,
// because you effectively have to lex through a byte at a time (since the
// color bands have varying size). le sigh! we work geometrically here,
// blasting through each band and scrubbing the necessary cells therein.
// define a rectangle that will be scrubbed.
int sprite_sixel_cell_wipe(const notcurses* nc, sprixel* s, int ycell, int xcell){
  const int xpixels = nc->tcache.cellpixx;
  const int ypixels = nc->tcache.cellpixy;
  const int top = ypixels * ycell;          // start scrubbing on this row
  int bottom = ypixels * (ycell + 1); // do *not* scrub this row
  const int left = xpixels * xcell;         // start scrubbing on this column
  int right = xpixels * (xcell + 1);  // do *not* scrub this column
  // if the cell is on the right or bottom borders, it might only be partially
  // filled by actual graphic data, and we need to cap our target area.
  if(right > s->pixx){
    right = s->pixx;
  }
  if(bottom > s->pixy){
    bottom = s->pixy;
  }
//fprintf(stderr, "TARGET AREA: [ %dx%d -> %dx%d ] of %dx%d\n", top, left, bottom - 1, right - 1, s->pixy, s->pixx);
  char* c = s->glyph;
  // lines of sixels are broken by a hyphen. if we were guaranteed to already
  // be in the meat of the sixel, it would be sufficient to count hyphens, but
  // we must distinguish the introductory material from the sixmap, alas
  // (after that, simply count hyphens). FIXME store loc in sprixel metadata?
  // it seems sufficient to look for the first #d not followed by a semicolon.
  // remember, these are sixels *we've* created internally, not random ones.
  while(*c != '#'){
    ++c;
  }
  do{
    ++c;
    while(isdigit(*c)){
      ++c;
    }
    while(*c == ';'){
      ++c;
      while(isdigit(*c)){
        ++c;
      }
    }
  }while(*c == '#');
  --c;
  int row = 0;
  while(row + 6 <= top){
    while(*c != '-'){
      ++c;
    }
    row += 6;
    unsigned mask = 0;
    if(row < top){
      for(int i = 0 ; i < top - row ; ++i){
        mask |= (1 << i);
      }
    }
    // make masks containing only pixels which we will *not* be turning off
    // (on the top or bottom), if any. go through each entry and if it
    // occupies our target columns, scrub scrub scrub!
    while(*c == '#' || isdigit(*c)){
      while(*c == '#' || isdigit(*c)){
        ++c;
      }
      int column = 0;
      int rle = 0;
      // here begins the substance, concluded by '-', '$', or '\e'. '!' indicates rle.
      while(*c != '-' && *c != '$' && *c != '\e'){
        if(*c == '!'){
          rle = 0;
        }else if(isdigit(*c)){
          rle *= 10;
          rle += (*c - '0');
        }else{
          if(rle){
            // FIXME this can skip over the starting column
            column += (rle - 1);
            rle = 0;
          }
          if(column >= left && column < right){ // zorch it
//fprintf(stderr, "STARTED WITH %d %c\n", *c, *c);
            *c = ((*c - 63) & mask) + 63;
//fprintf(stderr, "CHANGED TO %d %c\n", *c, *c);
          }
          ++column;
        }
        ++c;
      }
      if(*c == '-'){
        row += 6;
        if(row >= bottom){
          return 0;
        }
        mask = 0;
        if(bottom - row < 6){
          for(int i = 0 ; i < bottom - row ; ++i){
            mask |= (1 << (6 - i));
          }
        }
      }else if(*c == '\e'){
        return 0;
      }
      column = 0;
      ++c;
    }
  }
  return 0;
}

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
  int pos = 0;
  for(int visy = begy ; visy < (begy + leny) ; visy += 6){ // pixel row
    for(int visx = begx ; visx < (begx + lenx) ; visx += 1){ // pixel column
      for(int sy = visy ; sy < (begy + leny) && sy < visy + 6 ; ++sy){ // offset within sprixel
        const uint32_t* rgb = (data + (linesize / 4 * sy) + visx);
        if(rgba_trans_p(ncpixel_a(*rgb))){
          continue;
        }
        int txyidx = (sy / cdimy) * cols + (visx / cdimx);
        if(tacache[txyidx] == SPRIXCELL_ANNIHILATED){
//fprintf(stderr, "TRANS SKIP %d %d %d %d\n", visy, visx, sy, txyidx);
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
write_sixel_data(FILE* fp, int lenx, sixeltable* stab, int* parse_start){
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
sixel_blit_inner(ncplane* n, int leny, int lenx, sixeltable* stab,
                 const blitterargs* bargs, unsigned reuse,
                 sprixcell_e* tacache){
  char* buf = NULL;
  size_t size = 0;
  FILE* fp = open_memstream(&buf, &size);
  if(fp == NULL){
    return -1;
  }
  int parse_start = 0;
  int cols = lenx / bargs->u.pixel.celldimx + !!(lenx % bargs->u.pixel.celldimx);
  int rows = leny / bargs->u.pixel.celldimy + !!(leny % bargs->u.pixel.celldimy);
  // calls fclose() on success
  if(write_sixel_data(fp, lenx, stab, &parse_start)){
    free(buf);
    return -1;
  }
  // both paths take ownership of buf on success
  if(reuse){
    sprixel_update(n->sprite, buf, size);
  }else{
    if(plane_blit_sixel(n, buf, size, bargs->placey, bargs->placex,
                        rows, cols, bargs->u.pixel.sprixelid, leny, lenx,
                        parse_start, tacache) < 0){
      free(buf);
      return -1;
    }
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
  int cols = lenx / bargs->u.pixel.celldimx + !!(lenx % bargs->u.pixel.celldimx);
  int rows = leny / bargs->u.pixel.celldimy + !!(leny % bargs->u.pixel.celldimy);
  sprixcell_e* tacache = NULL;
  bool reuse = false;
  // if we have a sprixel attached to this plane, see if we can reuse it
  // (we need the same dimensions) and thus immediately apply its T-A table.
  if(n->sprite){
    sprixel* s = n->sprite;
    if(s->dimy == rows && s->dimx == cols){
      tacache = s->tacache;
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
  int r = sixel_blit_inner(n, leny, lenx, &stable, bargs, reuse, tacache);
  free(stable.data);
  free(stable.deets);
  free(stable.table);
  return r;
}

int sprite_sixel_init(int fd){
  // \e[?8452: DECSDM private "sixel scrolling" mode keeps the sixel from
  // scrolling, but puts it at the current cursor location (as opposed to
  // the upper left corner of the screen).
  return tty_emit("\e[?80;8452h", fd);
}
