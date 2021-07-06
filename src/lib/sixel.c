#include "internal.h"

#define RGBSIZE 3
#define CENTSIZE (RGBSIZE + 1) // size of a color table entry

// we set P2 based on whether there is any transparency in the sixel. if not,
// use SIXEL_P2_ALLOPAQUE (0), for faster drawing in certain terminals.
typedef enum {
  SIXEL_P2_ALLOPAQUE = 0,
  SIXEL_P2_TRANS = 1,
} sixel_p2_e;

// returns the number of individual sixels necessary to represent the specified
// pixel geometry. these might encompass more pixel rows than |dimy| would
// suggest, up to the next multiple of 6 (i.e. a single row becomes a 6-row
// bitmap; as do two, three, four, five, or six rows). input is scaled geometry.
static inline int
sixelcount(int dimy, int dimx){
  return (dimy + 5) / 6 * dimx;
}

// we keep a color-indexed set of sixels (a single-row column of six pixels,
// encoded as a byte) across the life of the sprixel. This provides a good
// combination of easy-to-edit (for wipes and restores) -- you can index by
// color, and then by position, in O(1) -- and a form which can easily be
// converted to the actual Sixel encoding. wipes and restores come in and edit
// these sixels in O(1), and then at display time we recreate the encoded
// bitmap in one go if necessary. we could just wipe and restore directly using
// the encoded form, but it's a tremendous pain in the ass. this sixelmap will
// be kept in the sprixel. when first encoding, data and table each have an
// entry for every color register; call sixelmap_trim() when done to cut them
// down to the actual number of colors used.
typedef struct sixelmap {
  int colors;
  int sixelcount;
  unsigned char* data;  // |colors| x |sixelcount|-byte arrays
  unsigned char* table; // |colors| x CENTSIZE: components + dtable index
} sixelmap;

// whip up an all-zero sixelmap for the specified pixel geometry and color
// register count. we might not use all the available color registers; call
// sixelmap_trim() to release any unused memory once done encoding.
static sixelmap*
sixelmap_create(int cregs, int dimy, int dimx){
  sixelmap* ret = malloc(sizeof(*ret));
  if(ret){
    ret->sixelcount = sixelcount(dimy, dimx);
    if(ret->sixelcount){
      size_t dsize = sizeof(*ret->data) * cregs * ret->sixelcount;
      ret->data = malloc(dsize);
      if(ret->data){
        size_t tsize = CENTSIZE * cregs;
        ret->table = malloc(tsize);
        if(ret->table){
          memset(ret->table, 0, tsize);
          memset(ret->data, 0, dsize);
          ret->colors = 0;
          return ret;
        }
        free(ret->data);
      }
    }
    free(ret);
  }
  return NULL;
}

// trims s->data down to the number of colors actually used (as opposed to the
// number of color registers available).
static int
sixelmap_trim(sixelmap* s){
  if(s->colors == 0){
    free(s->table);
    s->table = NULL;
    free(s->data);
    s->data = NULL;
    return 0;
  }
  size_t dsize = sizeof(*s->data) * s->colors * s->sixelcount;
  unsigned char* tmp = realloc(s->data, dsize);
  if(tmp == NULL){
    return -1;
  }
  s->data = tmp;
  size_t tsize = CENTSIZE * s->colors;
  if((tmp = realloc(s->table, tsize)) == NULL){
    return -1;
  }
  s->table = tmp;
  return 0;
}

void sixelmap_free(sixelmap *s){
  if(s){
    free(s->table);
    free(s->data);
    free(s);
  }
}

typedef struct cdetails {
  int64_t sums[3];   // sum of components of all matching original colors
  int32_t count;     // count of pixels matching
  char hi[RGBSIZE];  // highest sixelspace components we've seen
  char lo[RGBSIZE];  // lowest sixelspace color we've seen
} cdetails;

// second pass: construct data for extracted colors over the sixels
typedef struct sixeltable {
  sixelmap* map;        // copy of palette indices / transparency bits
  // FIXME keep these internal to palette extraction; finalize there
  cdetails* deets;      // |colorregs| cdetails structures
  int colorregs;
  sixel_p2_e p2;        // set to SIXEL_P2_TRANS if we have transparent pixels
} sixeltable;

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

// wipe the color from startx to endx, from starty to endy. returns 1 if any
// pixels were actually wiped.
static inline int
wipe_color(sixelmap* smap, int color, int sband, int eband,
           int startx, int endx, int starty, int endy, int dimx,
           int cellpixy, int cellpixx, uint8_t* auxvec){
  int wiped = 0;
  int didx = ctable_to_dtable(smap->table + color * CENTSIZE);
  // offset into map->data where our color starts
  int coff = smap->sixelcount * didx;
//fprintf(stderr, "didx: %d sixels: %d color: %d B: %d-%d Y: %d-%d X: %d-%d coff: %d\n", didx, smap->sixelcount, color, sband, eband, starty, endy, startx, endx, coff);
  // we're going to repurpose starty as "starting row of this band", so keep it
  // around as originy for auxvecidx computations
  int originy = starty;
  for(int b = sband ; b <= eband && b * 6 <= endy ; ++b){
    const int boff = coff + b * dimx; // offset in data where band starts
    unsigned char mask = 63;
    for(int i = 0 ; i < 6 ; ++i){
      if(b * 6 + i >= starty && b * 6 + i <= endy){
        mask &= ~(1u << i);
      }
//fprintf(stderr, "s/e: %d/%d mask: %02x\n", starty, endy, mask);
    }
    for(int x = startx ; x <= endx ; ++x){
      const int xoff = boff + x;
      assert(xoff < (smap->colors + 1) * smap->sixelcount);
//fprintf(stderr, "band: %d color: %d idx: %d mask: %02x\n", b, color, color * smap->sixelcount + xoff, mask);
//fprintf(stderr, "color: %d idx: %d data: %02x\n", color, color * smap->sixelcount + xoff, smap->data[color * smap->sixelcount + xoff]);
      // this is the auxvec position of the upperleftmost pixel of the sixel
      // there will be up to five more, each cellpxx away, for the five pixels
      // below it. there will be cellpxx - 1 after it, each with their own five.
//fprintf(stderr, "smap->data[%d] = %02x boff: %d x: %d color: %d\n", xoff, smap->data[xoff], boff, x, color);
      for(int i = 0 ; i < 6 && b * 6 + i <= endy ; ++i){
        int auxvecidx = (x - startx) + ((b * 6 + i - originy) * cellpixx);
        unsigned bit = 1u << i;
//fprintf(stderr, "xoff: %d i: %d b: %d endy: %d mask: 0x%02x\n", xoff, i, b, endy, mask);
        if(!(mask & bit) && (smap->data[xoff] & bit)){
//fprintf(stderr, "band %d %d/%d writing %d to auxvec[%d] %p xoff: %d boff: %d\n", b, b * 6 + i, x, color, auxvecidx, auxvec, xoff, boff);
          auxvec[auxvecidx] = color;
          auxvec[cellpixx * cellpixy + auxvecidx] = 0;
        }
      }
      if((smap->data[xoff] & mask) != smap->data[xoff]){
        smap->data[xoff] &= mask;
        wiped = 1;
      }
//fprintf(stderr, "post: %02x\n", smap->data[color * smap->sixelcount + xoff]);
    }
    starty = (starty + 6) / 6 * 6;
  }
  return wiped;
}

// we return -1 because we're not doing a proper wipe -- that's not possible
// using sixel. we just mark it as partially transparent, so that if it's
// redrawn, it's redrawn using P2=1.
int sixel_wipe(sprixel* s, int ycell, int xcell){
//fprintf(stderr, "WIPING %d/%d\n", ycell, xcell);
  uint8_t* auxvec = sprixel_auxiliary_vector(s);
  memset(auxvec + s->cellpxx * s->cellpxy, 0xff, s->cellpxx * s->cellpxy);
  sixelmap* smap = s->smap;
  const int startx = xcell * s->cellpxx;
  const int starty = ycell * s->cellpxy;
  int endx = ((xcell + 1) * s->cellpxx) - 1;
  if(endx >= s->pixx){
    endx = s->pixx - 1;
  }
  int endy = ((ycell + 1) * s->cellpxy) - 1;
  if(endy >= s->pixy){
    endy = s->pixy - 1;
  }
  const int startband = starty / 6;
  const int endband = endy / 6;
//fprintf(stderr, "y/x: %d/%d start: %d/%d end: %d/%d\n", ycell, xcell, starty, startx, endy, endx);
  // walk through each color, and wipe the necessary sixels from each band
  int w = 0;
  for(int c = 0 ; c < smap->colors ; ++c){
    w |= wipe_color(smap, c, startband, endband, startx, endx, starty, endy,
                    s->pixx, s->cellpxy, s->cellpxx, auxvec);
  }
  if(w){
    s->wipes_outstanding = true;
  }
  change_p2(s->glyph, SIXEL_P2_TRANS);
  s->n->tam[s->dimx * ycell + xcell].auxvector = auxvec;
  return 0;
}

// rebuilds the auxiliary vectors, and scrubs the actual pixels, following
// extraction of the palette. doing so allows the new frame's pixels to
// contribute to the solved palette, even if they were wiped in the previous
// frame. pixels ought thus have been set up in sixel_blit(), despite TAM
// entries in the ANNIHILATED state.
static int
scrub_color_table(sprixel* s){
  if(s->n && s->n->tam){
    // we use the sprixel cell geometry rather than the plane's because this
    // is called during our initial blit, before we've resized the plane.
    for(int y = 0 ; y < s->dimy ; ++y){
      for(int x = 0 ; x < s->dimx ; ++x){
        int txyidx = y * s->dimx + x;
        sprixcell_e state = s->n->tam[txyidx].state;
        if(state == SPRIXCELL_ANNIHILATED || state == SPRIXCELL_ANNIHILATED_TRANS){
//fprintf(stderr, "POSTEXRACT WIPE %d/%d\n", y, x);
          sixel_wipe(s, y, x);
        }
      }
    }
  }
  return 0;
}

// returns the index at which the provided color can be found *in the
// dtable*, possibly inserting it into the ctable. returns -1 if the
// color is not in the table and the table is full.
static int
find_color(sixeltable* stab, unsigned char comps[static RGBSIZE]){
  int i;
  if(stab->map->colors){
    int l, r;
    l = 0;
    r = stab->map->colors - 1;
    do{
      i = l + (r - l) / 2;
//fprintf(stderr, "%02x%02x%02x L %d R %d m %d\n", comps[0], comps[1], comps[2], l, r, i);
      int cmp = memcmp(stab->map->table + i * CENTSIZE, comps, RGBSIZE);
      if(cmp == 0){
        return ctable_to_dtable(stab->map->table + i * CENTSIZE);
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
    }else if(l == stab->map->colors){
      i = stab->map->colors;
    }else{
      i = l;
    }
    if(stab->map->colors == stab->colorregs){
      return -1;
    }
    if(i < stab->map->colors){
//fprintf(stderr, "INSERTING COLOR %u %u %u AT %d\n", comps[0], comps[1], comps[2], i);
      memmove(stab->map->table + (i + 1) * CENTSIZE,
              stab->map->table + i * CENTSIZE,
              (stab->map->colors - i) * CENTSIZE);
    }
  }else{
    i = 0;
  }
//fprintf(stderr, "NEW COLOR CONCAT %u %u %u AT %d\n", comps[0], comps[1], comps[2], i);
  memcpy(stab->map->table + i * CENTSIZE, comps, RGBSIZE);
  dtable_to_ctable(stab->map->colors, stab->map->table + i * CENTSIZE);
  ++stab->map->colors;
  return stab->map->colors - 1;
  //return ctable_to_dtable(stab->map->table + i * CENTSIZE);
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
                    tament* tam, const blitterargs* bargs){
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
        // we do *not* exempt already-wiped pixels from palette creation. once
        // we're done, we'll call sixel_wipe() on these cells. so they remain
        // one of SPRIXCELL_ANNIHILATED or SPRIXCELL_ANNIHILATED_TRANS.
        if(tam[txyidx].state != SPRIXCELL_ANNIHILATED && tam[txyidx].state != SPRIXCELL_ANNIHILATED_TRANS){
          if(rgba_trans_p(*rgb, bargs->transcolor)){
            if(sy % cdimy == 0 && visx % cdimx == 0){
              tam[txyidx].state = SPRIXCELL_TRANSPARENT;
            }else if(tam[txyidx].state == SPRIXCELL_OPAQUE_SIXEL){
              tam[txyidx].state = SPRIXCELL_MIXED_SIXEL;
            }
            stab->p2 = SIXEL_P2_TRANS; // even one forces P2=1
            continue;
          }else{
            if(sy % cdimy == 0 && visx % cdimx == 0){
              tam[txyidx].state = SPRIXCELL_OPAQUE_SIXEL;
            }else if(tam[txyidx].state == SPRIXCELL_TRANSPARENT){
              tam[txyidx].state = SPRIXCELL_MIXED_SIXEL;
            }
          }
        }else{
//fprintf(stderr, "TRANS SKIP %d %d %d %d (cell: %d %d)\n", visy, visx, sy, txyidx, sy / cdimy, visx / cdimx);
          if(rgba_trans_p(*rgb, bargs->transcolor)){
            if(sy % cdimy == 0 && visx % cdimx == 0){
              tam[txyidx].state = SPRIXCELL_ANNIHILATED_TRANS;
            }
          }else{
            tam[txyidx].state = SPRIXCELL_ANNIHILATED;
          }
        }
        unsigned char comps[RGBSIZE];
        break_sixel_comps(comps, *rgb, mask);
        int c = find_color(stab, comps);
        if(c < 0){
//fprintf(stderr, "FAILED FINDING COLOR AUGH 0x%02x\n", mask);
          return -1;
        }
        stab->map->data[c * stab->map->sixelcount + pos] |= (1u << (sy - visy));
        update_deets(*rgb, &stab->deets[c]);
//fprintf(stderr, "color %d pos %d: 0x%x\n", c, pos, stab->data[c * stab->map->sixelcount + pos]);
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
  unsigned char* tcrec = stab->map->table + CENTSIZE * stab->map->colors;
  dtable_to_ctable(stab->map->colors, tcrec);
  cdetails* targdeets = stab->deets + stab->map->colors;
  unsigned char* crec = stab->map->table + CENTSIZE * src;
  int didx = ctable_to_dtable(crec);
  cdetails* deets = stab->deets + didx;
  unsigned char* srcsixels = stab->map->data + stab->map->sixelcount * didx;
  unsigned char* dstsixels = stab->map->data + stab->map->sixelcount * stab->map->colors;
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
  unsigned char* crec = stab->map->table + CENTSIZE * color;
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
//fprintf(stderr, "[%d->%d] SPLIT ON GREEN %d %d (pop: %d)\n", color, stab->map->colors, deets->hi[1], deets->lo[1], deets->count);
    rgbmax[1] = deets->lo[1] + (deets->hi[1] - deets->lo[1]) / 2;
  }else if(rdelt >= gdelt && rdelt >= bdelt){ // split on red
    if(rdelt < 3){
      return 0;
    }
//fprintf(stderr, "[%d->%d] SPLIT ON RED %d %d (pop: %d)\n", color, stab->map->colors, deets->hi[0], deets->lo[0], deets->count);
    rgbmax[0] = deets->lo[0] + (deets->hi[0] - deets->lo[0]) / 2;
  }else{ // split on blue
    if(bdelt < 3){
      return 0;
    }
//fprintf(stderr, "[%d->%d] SPLIT ON BLUE %d %d (pop: %d)\n", color, stab->map->colors, deets->hi[2], deets->lo[2], deets->count);
    rgbmax[2] = deets->lo[2] + (deets->hi[2] - deets->lo[2]) / 2;
  }
  unzip_color(data, linesize, begy, begx, leny, lenx, stab, color, rgbmax);
  ++stab->map->colors;
  return 1;
}

// relax the details down into free color registers
static void
refine_color_table(const uint32_t* data, int linesize, int begy, int begx,
                   int leny, int lenx, sixeltable* stab){
  while(stab->map->colors < stab->colorregs){
    bool refined = false;
    int tmpcolors = stab->map->colors; // force us to come back through
    for(int i = 0 ; i < tmpcolors ; ++i){
      unsigned char* crec = stab->map->table + CENTSIZE * i;
      int didx = ctable_to_dtable(crec);
      cdetails* deets = stab->deets + didx;
//fprintf(stderr, "[%d->%d] hi: %d %d %d lo: %d %d %d\n", i, didx, deets->hi[0], deets->hi[1], deets->hi[2], deets->lo[0], deets->lo[1], deets->lo[2]);
      if(deets->count > leny * lenx / stab->colorregs){
        if(refine_color(data, linesize, begy, begx, leny, lenx, stab, i)){
          if(stab->map->colors == stab->colorregs){
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

// write the escape which opens a Sixel, plus the palette table. returns the
// number of bytes written, so that this header can be directly copied in
// future reencodings. |leny| and |lenx| are output pixel geometry.
static int
write_sixel_header(FILE* fp, int leny, int lenx, const sixeltable* stab, sixel_p2_e p2){
  if(leny % 6){
    return -1;
  }
  // Set Raster Attributes - pan/pad=1 (pixel aspect ratio), Ph=lenx, Pv=leny
  int r = fprintf(fp, "\eP0;%d;0q\"1;1;%d;%d", p2, lenx, leny);
  if(r < 0){
    return -1;
  }
  for(int i = 0 ; i < stab->map->colors ; ++i){
    const unsigned char* rgb = stab->map->table + i * CENTSIZE;
    int idx = ctable_to_dtable(rgb);
    int count = stab->deets[idx].count;
//fprintf(stderr, "RGB: %3u(%d) %3u(%d) %3u(%d) DT: %d SUMS: %3ld %3ld %3ld COUNT: %d\n", rgb[0], ss(rgb[0], 0xff), rgb[1], ss(rgb[1], 0xff), rgb[2], ss(rgb[2], 0xff), idx, stab->deets[idx].sums[0] / count * 100 / 255, stab->deets[idx].sums[1] / count * 100 / 255, stab->deets[idx].sums[2] / count * 100 / 255, count);
    //fprintf(fp, "#%d;2;%u;%u;%u", i, rgb[0], rgb[1], rgb[2]);
    // we emit the average of the actual sums rather than the RGB clustering
    // point, as it can be (and usually is) much more accurate.
    int f = fprintf(fp, "#%d;2;%jd;%jd;%jd", i,
                    (intmax_t)(stab->deets[idx].sums[0] * 100 / count / 255),
                    (intmax_t)(stab->deets[idx].sums[1] * 100 / count / 255),
                    (intmax_t)(stab->deets[idx].sums[2] * 100 / count / 255));
    if(f < 0){
      return -1;
    }
    r += f;
  }
  return r;
}

static int
write_sixel_payload(FILE* fp, int lenx, const sixelmap* map){
  int p = 0;
  while(p < map->sixelcount){
    int needclosure = 0;
    for(int i = 0 ; i < map->colors ; ++i){
      int seenrle = 0; // number of repetitions
      unsigned char crle = 0; // character being repeated
      int idx = ctable_to_dtable(map->table + i * CENTSIZE);
      int printed = 0;
      for(int m = p ; m < map->sixelcount && m < p + lenx ; ++m){
//fprintf(stderr, "%d ", idx * map->sixelcount + m);
//fputc(map->data[idx * map->sixelcount + m] + 63, stderr);
        if(seenrle){
          if(map->data[idx * map->sixelcount + m] == crle){
            ++seenrle;
          }else{
            write_rle(&printed, i, fp, seenrle, crle, &needclosure);
            seenrle = 1;
            crle = map->data[idx * map->sixelcount + m];
          }
        }else{
          seenrle = 1;
          crle = map->data[idx * map->sixelcount + m];
        }
      }
      if(crle){
        write_rle(&printed, i, fp, seenrle, crle, &needclosure);
      }
      needclosure = needclosure | printed;
    }
    if(p + lenx < map->sixelcount){
      fputc('-', fp);
    }
    p += lenx;
  }
  fprintf(fp, "\e\\");
  return 0;
}

// emit the sixel in its entirety, plus escapes to start and end pixel mode.
// only called the first time we encode; after that, the palette remains
// constant, and is simply copied. fclose()s |fp| on success. |outx| and |outy|
// are output geometry.
static int
write_sixel(FILE* fp, int outy, int outx, const sixeltable* stab,
            int* parse_start, sixel_p2_e p2){
  *parse_start = write_sixel_header(fp, outy, outx, stab, p2);
  if(*parse_start < 0){
    return -1;
  }
  if(write_sixel_payload(fp, outx, stab->map) < 0){
    return -1;
  }
  if(fclose(fp) == EOF){
    return -1;
  }
  return 0;
}

// once per render cycle (if needed), make the actual payload match the TAM. we
// don't do these one at a time due to the complex (expensive) process involved
// in regenerating a sixel (we can't easily do it in-place). anything newly
// ANNIHILATED (state is ANNIHILATED, but no auxvec present) is dropped from
// the payload, and an auxvec is generated. anything newly restored (state is
// OPAQUE_SIXEL or MIXED_SIXEL, but an auxvec is present) is restored to the
// payload, and the auxvec is freed. none of this takes effect until the sixel
// is redrawn, and annihilated sprixcells still require a glyph to be emitted.
static inline int
sixel_reblit(sprixel* s){
  char* buf = NULL;
  size_t size = 0;
  FILE* fp = open_memstream(&buf, &size);
  if(fp == NULL){
    return -1;
  }
  if(fwrite(s->glyph, s->parse_start, 1, fp) != 1){
    fclose(fp);
    free(buf);
    return -1;
  }
  if(write_sixel_payload(fp, s->pixx, s->smap) < 0){
    fclose(fp);
    free(buf);
    return -1;
  }
  if(fclose(fp) == EOF){
    free(buf);
    return -1;
  }
  free(s->glyph);
  // FIXME update P2 if necessary
  s->glyph = buf;
  s->glyphlen = size;
  return 0;
}

// Sixel blitter. Sixels are stacks 6 pixels high, and 1 pixel wide. RGB colors
// are programmed as a set of registers, which are then referenced by the
// stacks. There is also a RLE component, handled in rasterization.
// A pixel block is indicated by setting cell_pixels_p(). |leny| and |lenx| are
// scaled geometry in pixels. We calculate output geometry herein, and supply
// transparent filler input for any missing rows.
static inline int
sixel_blit_inner(int leny, int lenx, sixeltable* stab,
                 const blitterargs* bargs, tament* tam){
  char* buf = NULL;
  size_t size = 0;
  FILE* fp = open_memstream(&buf, &size);
  if(fp == NULL){
    return -1;
  }
  int parse_start = 0;
  int outy = leny;
  if(leny % 6){
    outy += 6 - (leny % 6);
    stab->p2 = SIXEL_P2_TRANS;
  }
  // calls fclose() on success
  if(write_sixel(fp, outy, lenx, stab, &parse_start, stab->p2)){
    fclose(fp);
    free(buf);
    return -1;
  }
  scrub_tam_boundaries(tam, outy, lenx, bargs->u.pixel.celldimy,
                       bargs->u.pixel.celldimx);
  // take ownership of buf on success
  if(plane_blit_sixel(bargs->u.pixel.spx, buf, size,
                      outy, lenx, parse_start, tam) < 0){
    free(buf);
    return -1;
  }
  sixelmap_trim(stab->map);
  bargs->u.pixel.spx->smap = stab->map;
  return 1;
}

// |leny| and |lenx| are the scaled output geometry. we take |leny| up to the
// nearest multiple of six greater than or equal to |leny|.
int sixel_blit(ncplane* n, int linesize, const void* data, int leny, int lenx,
               const blitterargs* bargs, int bpp __attribute__ ((unused))){
  int colorregs = bargs->u.pixel.colorregs;
  if(colorregs > 256){
    colorregs = 256;
  }
  assert(colorregs >= 64);
  sixeltable stable = {
    .map = sixelmap_create(colorregs, leny - bargs->begy, lenx - bargs->begx),
    .deets = malloc(colorregs * sizeof(cdetails)),
    .colorregs = colorregs,
    .p2 = SIXEL_P2_ALLOPAQUE,
  };
  if(stable.deets == NULL || stable.map == NULL){
    sixelmap_free(stable.map);
    free(stable.deets);
    return -1;
  }
  // stable.table doesn't need initializing; we start from the bottom
  memset(stable.deets, 0, sizeof(*stable.deets) * colorregs);
  int cols = bargs->u.pixel.spx->dimx;
  int rows = bargs->u.pixel.spx->dimy;
  tament* tam = NULL;
  bool reuse = false;
  // if we have a sprixel attached to this plane, see if we can reuse it
  // (we need the same dimensions) and thus immediately apply its T-A table.
  if(n->tam){
//fprintf(stderr, "IT'S A REUSE %d %d\n", rows, cols);
    if(n->leny == rows && n->lenx == cols){
      tam = n->tam;
      reuse = true;
    }else{
      // FIXME free up old TAM (well, shrink it anyway)
    }
  }
  if(!reuse){
    tam = malloc(sizeof(*tam) * rows * cols);
    if(tam == NULL){
      sixelmap_free(stable.map);
      free(stable.deets);
      return -1;
    }
    memset(tam, 0, sizeof(*tam) * rows * cols);
  }
  if(extract_color_table(data, linesize, cols, leny, lenx, &stable, tam, bargs)){
    if(!reuse){
      free(tam);
    }
    sixelmap_free(stable.map);
    free(stable.deets);
    return -1;
  }
  refine_color_table(data, linesize, bargs->begy, bargs->begx, leny, lenx, &stable);
  // takes ownership of sixelmap on success
  int r = sixel_blit_inner(leny, lenx, &stable, bargs, tam);
  if(r < 0){
    sixelmap_free(stable.map);
  }
  free(stable.deets);
  scrub_color_table(bargs->u.pixel.spx);
  return r;
}

// to destroy a sixel, we damage all cells underneath it. we might not have
// to, though, if we've got a new sixel ready to go where the old sixel was
// (though we'll still need to if the new sprixcell not opaque, and the
// old and new sprixcell are different in any transparent pixel).
int sixel_scrub(const ncpile* p, sprixel* s){
//fprintf(stderr, "DESTROYING %d %d %p at %d/%d (%d/%d)\n", s->id, s->invalidated, s->n, s->movedfromy, s->movedfromx, s->dimy, s->dimx);
  int starty = s->movedfromy;
  int startx = s->movedfromx;
  for(int yy = starty ; yy < starty + s->dimy && yy < p->dimy ; ++yy){
    for(int xx = startx ; xx < startx + s->dimx && xx < p->dimx ; ++xx){
      int ridx = yy * p->dimx + xx;
      struct crender *r = &p->crender[ridx];
      if(r->sprixel){
        s = r->sprixel;
      }
      if(s->n){
        sprixcell_e state = sprixel_state(s, yy - s->movedfromy,
                                          xx - s->movedfromx);
//fprintf(stderr, "CHECKING %d/%d state: %d %d/%d\n", yy - s->movedfromy - s->n->absy, xx - s->movedfromx - s->n->absx, state, yy, xx);
        if(state == SPRIXCELL_TRANSPARENT || state == SPRIXCELL_MIXED_SIXEL){
          r->s.damaged = 1;
        }else if(s->invalidated == SPRIXEL_MOVED){
          // ideally, we wouldn't damage our annihilated sprixcells, but if
          // we're being annihilated only during this cycle, we need to go
          // ahead and damage it.
          r->s.damaged = 1;
        }
      }else{
        // need this to damage cells underneath a sprixel we're removing
        r->s.damaged = 1;
      }
    }
  }
  return 0;
}

// returns the number of bytes written
int sixel_draw(const ncpile* p, sprixel* s, FILE* out){
  // if we've wiped or rebuilt any cells, effect those changes now, or else
  // we'll get flicker when we move to the new location.
  if(s->wipes_outstanding){
    if(sixel_reblit(s)){
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
  }
  if(fwrite(s->glyph, s->glyphlen, 1, out) != 1){
    return -1;
  }
  s->invalidated = SPRIXEL_QUIESCENT;
fprintf(stderr, "POSTDRAW: %d %d/%d\n", s->invalidated, s->movedfromy, s->movedfromx);
  return s->glyphlen;
}

int sixel_init(const tinfo* ti, int fd){
  (void)ti;
  // \e[?8452: DECSDM private "sixel scrolling" mode keeps the sixel from
  // scrolling, but puts it at the current cursor location (as opposed to
  // the upper left corner of the screen).
  return tty_emit("\e[?80;8452h", fd);
}

int sixel_init_inverted(const tinfo* ti, int fd){
  (void)ti;
  // except MLterm (and a few others, possibly including the physical VT340),
  // which inverts the usual sense of DECSDM.
  return tty_emit("\e[?80l\e[?8452h", fd);
}

// only called for cells in SPRIXCELL_ANNIHILATED[_TRANS]. just post to
// wipes_outstanding, so the Sixel gets regenerated the next render cycle,
// just like wiping. this is necessary due to the complex nature of
// modifying a Sixel -- we want to do them all in one batch.
int sixel_rebuild(sprixel* s, int ycell, int xcell, uint8_t* auxvec){
  s->wipes_outstanding = true;
  sixelmap* smap = s->smap;
  const int startx = xcell * s->cellpxx;
  const int starty = ycell * s->cellpxy;
  int endx = ((xcell + 1) * s->cellpxx) - 1;
  if(endx > s->pixx){
    endx = s->pixx;
  }
  int endy = ((ycell + 1) * s->cellpxy) - 1;
  if(endy > s->pixy){
    endy = s->pixy;
  }
  int transparent = 0;
//fprintf(stderr, "%d/%d start: %d/%d end: %d/%d bands: %d-%d\n", ycell, xcell, starty, startx, endy, endx, starty / 6, endy / 6);
  for(int x = startx ; x <= endx ; ++x){
    for(int y = starty ; y <= endy ; ++y){
      int auxvecidx = (y - starty) * s->cellpxx + (x - startx);
      int trans = auxvec[s->cellpxx * s->cellpxy + auxvecidx];
      if(!trans){
        int color = auxvec[auxvecidx];
        int didx = ctable_to_dtable(smap->table + color * CENTSIZE);
        int coff = smap->sixelcount * didx;
        int band = y / 6;
        int boff = coff + band * s->pixx;
        int xoff = boff + x;
//fprintf(stderr, "DIDX: %d %d/%d band: %d coff: %d boff: %d rebuild %d/%d with color %d from %d %p xoff: %d\n", didx, ycell, xcell, band, coff, boff, y, x, color, auxvecidx, auxvec, xoff);
        s->smap->data[xoff] |= (1u << (y % 6));
      }else{
        ++transparent;
      }
    }
  }
  sprixcell_e newstate;
  if(transparent == s->cellpxx * s->cellpxy){
    newstate = SPRIXCELL_TRANSPARENT;
  }else if(transparent){
    newstate = SPRIXCELL_MIXED_SIXEL;
  }else{
    newstate = SPRIXCELL_OPAQUE_SIXEL;
  }
  s->n->tam[s->dimx * ycell + xcell].state = newstate;
  return 0;
}

// 80 (sixel scrolling) is enabled by default. 8452 is not. XTSAVE/XTRESTORE
// would be better, where they're supported.
int sixel_shutdown(FILE* fp){
  return term_emit("\e[?8452l", fp, false);
}

uint8_t* sixel_trans_auxvec(const tinfo* ti){
  const size_t slen = 2 * ti->cellpixy * ti->cellpixx;
  uint8_t* a = malloc(slen);
  if(a){
    memset(a, 0, slen);
  }
  return a;
}
