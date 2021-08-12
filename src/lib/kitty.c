#include <zlib.h>
#include "internal.h"
#include "base64.h"

// Kitty has its own bitmap graphics protocol, rather superior to DEC Sixel.
// A header is written with various directives, followed by a number of
// chunks. Each chunk carries up to 4096B of base64-encoded pixels. Bitmaps
// can be ordered on a z-axis, with text at a logical z=0. A bitmap at a
// positive coordinate will be drawn above text; a negative coordinate will
// be drawn below text. It is not possible for a single bitmap to be under
// some text and above other text; since we need both, we draw at a positive
// coordinate (above all text), and cut out sections by setting their alpha
// values to 0. We thus require RGBA, meaning 768 pixels per 4096B chunk
// (768pix * 4Bpp * 4/3 base64 overhead == 4096B).
//
// 0.20.0 introduced an animation protocol which drastically reduces the
// bandwidth necessary for wipe-and-rebuild. 0.21.1 improved it further.
// we thus have three strategies:
//
// pre-0.20.0: keep an auxvec for each wiped cell, with a byte per pixel.
//  on wipe, copy the alphas into the auxvec, and set them to 0 in the
//  encoded graphic. on rebuild, rewrite the alphas from the auxvec. both
//  operations require delicate edits directly to the encoded form. the
//  graphic is updated by completely retransmitting it.
//
// 0.20.0: we make a copy of the RGBA data, populating all auxvecs upon
//  blit. to wipe, we generate a cell's woth of 0s, and merge them into
//  the existing image. to rebuild, we merge the original data into the
//  existing image. this cuts down on bandwidth--unchanged cells are not
//  retransmitted. it does require a fairly expensive copy of the source,
//  even though we might never use it.
//
// 0.21.1+: our auxvecs are now a single word -- the sprixcell state prior
//  to annihilation. we never need retransmit the original RGBA on
//  restore, as we can instead use composition with reflection.
//
// if a graphic needs be moved, we can move it with a control operation,
// rather than erasing it and redrawing it manually.
//
// It has some interesting features of which we do not yet take advantage:
//  * in-terminal scaling of image data (we prescale)
//  * subregion display of a transmitted bitmap
//
// https://sw.kovidgoyal.net/kitty/graphics-protocol.html

// convert a base64 character into its equivalent integer 0..63
static inline int
b64idx(char b64){
  if(b64 >= 'A' && b64 <= 'Z'){
    return b64 - 'A';
  }else if(b64 >= 'a' && b64 <= 'z'){
    return b64 - 'a' + 26;
  }else if(b64 >= '0' && b64 <= '9'){
    return b64 - '0' + 52;
  }else if(b64 == '+'){
    return 62;
  }else{
    return 63;
  }
}

// null out part of a triplet (a triplet is 3 pixels, which map to 12 bytes, which map to
// 16 bytes when base64 encoded). skip the initial |skip| pixels, and null out a maximum
// of |max| pixels after that. returns the number of pixels nulled out. |max| must be
// positive. |skip| must be non-negative, and less than 3. |pleft| is the number of pixels
// available in the chunk. the RGB is 24 bits, and thus 4 base64 bytes, but
// unfortunately don't always start on a byte boundary =[.
// 0: R1(0..5)
// 1: R1(6..7), G1(0..3)
// 2: G1(4..7), B1(0..1)
// 3: B1(2..7)
// 4: A1(0..5)
// 5: A1(6..7), R2(0..3)
// 6: R2(4..7), G2(0..1)
// 7: G2(2..7)
// 8: B2(0..5)
// 9: B2(6..7), A2(0..3)
// A: A2(4..7), R3(0..1)
// B: R3(2..7)
// C: G3(0..5)
// D: G3(6..7), B3(0..3)
// E: B3(4..7), A3(0..1)
// F: A3(2..7)
// so we will only ever zero out bytes 4, 5, 9, A, E, and F

// get the first alpha from the triplet
static inline uint8_t
triplet_alpha1(const char* triplet){
  uint8_t c1 = b64idx(triplet[0x4]);
  uint8_t c2 = b64idx(triplet[0x5]);
  return (c1 << 2u) | ((c2 & 0x30) >> 4);
}

static inline uint8_t
triplet_alpha2(const char* triplet){
  uint8_t c1 = b64idx(triplet[0x9]);
  uint8_t c2 = b64idx(triplet[0xA]);
  return ((c1 & 0xf) << 4u) | ((c2 & 0x3c) >> 2);
}

static inline uint8_t
triplet_alpha3(const char* triplet){
  uint8_t c1 = b64idx(triplet[0xE]);
  uint8_t c2 = b64idx(triplet[0xF]);
  return ((c1 & 0x3) << 6u) | c2;
}

static inline int
kitty_null(char* triplet, int skip, int max, int pleft, uint8_t* auxvec){
//fprintf(stderr, "SKIP/MAX/PLEFT %d/%d/%d\n", skip, max, pleft);
  if(pleft > 3){
    pleft = 3;
  }
  if(max + skip > pleft){
    max = pleft - skip;
  }
//fprintf(stderr, "alpha-nulling %d after %d\n", max, skip);
  if(skip == 0){
    auxvec[0] = triplet_alpha1(triplet);
    triplet[0x4] = b64subs[0];
    triplet[0x5] = b64subs[b64idx(triplet[0x5]) & 0xf];
    if(max > 1){
      auxvec[1] = triplet_alpha2(triplet);
      triplet[0x9] = b64subs[b64idx(triplet[0x9]) & 0x30];
      triplet[0xA] = b64subs[b64idx(triplet[0xA]) & 0x3];
    }
    if(max == 3){
      auxvec[2] = triplet_alpha3(triplet);
      triplet[0xE] = b64subs[b64idx(triplet[0xE]) & 0x3c];
      triplet[0xF] = b64subs[0];
    }
  }else if(skip == 1){
    auxvec[0] = triplet_alpha2(triplet);
    triplet[0x9] = b64subs[b64idx(triplet[0x9]) & 0x30];
    triplet[0xA] = b64subs[b64idx(triplet[0xA]) & 0x3];
    if(max == 2){
      auxvec[1] = triplet_alpha3(triplet);
      triplet[0xE] = b64subs[b64idx(triplet[0xE]) & 0x3c];
      triplet[0xF] = b64subs[0];
    }
  }else{ // skip == 2
    auxvec[0] = triplet_alpha3(triplet);
    triplet[0xE] = b64subs[b64idx(triplet[0xE]) & 0x3c];
    triplet[0xF] = b64subs[0];
  }
  return max;
}

// restore part of a triplet (a triplet is 3 pixels, which map to 12 bytes,
// which map to 16 bytes when base64 encoded). skip the initial |skip| pixels,
// and restore a maximum of |max| pixels after that. returns the number of
// pixels restored. |max| must be positive. |skip| must be non-negative, and
// less than 3. |pleft| is the number of pixels available in the chunk.
// |state| is set to MIXED if we find transparent pixels.
static inline int
kitty_restore(char* triplet, int skip, int max, int pleft,
              const uint8_t* auxvec, sprixcell_e* state){
//fprintf(stderr, "SKIP/MAX/PLEFT %d/%d/%d auxvec %p\n", skip, max, pleft, auxvec);
  if(pleft > 3){
    pleft = 3;
  }
  if(max + skip > pleft){
    max = pleft - skip;
  }
  if(skip == 0){
    int a = auxvec[0];
    if(a == 0){
      *state = SPRIXCELL_MIXED_KITTY;
    }
    triplet[0x4] = b64subs[(a & 0xfc) >> 2];
    triplet[0x5] = b64subs[((a & 0x3) << 4) | (b64idx(triplet[0x5]) & 0xf)];
    if(max > 1){
      a = auxvec[1];
      if(a == 0){
        *state = SPRIXCELL_MIXED_KITTY;
      }
      triplet[0x9] = b64subs[(b64idx(triplet[0x9]) & 0x30) | ((a & 0xf0) >> 4)];
      triplet[0xA] = b64subs[((a & 0xf) << 2) | (b64idx(triplet[0xA]) & 0x3)];
    }
    if(max == 3){
      a = auxvec[2];
      if(a == 0){
        *state = SPRIXCELL_MIXED_KITTY;
      }
      triplet[0xE] = b64subs[((a & 0xc0) >> 6) | (b64idx(triplet[0xE]) & 0x3c)];
      triplet[0xF] = b64subs[(a & 0x3f)];
    }
  }else if(skip == 1){
    int a = auxvec[0];
    if(a == 0){
      *state = SPRIXCELL_MIXED_KITTY;
    }
    triplet[0x9] = b64subs[(b64idx(triplet[0x9]) & 0x30) | ((a & 0xf0) >> 4)];
    triplet[0xA] = b64subs[((a & 0xf) << 2) | (b64idx(triplet[0xA]) & 0x3)];
    if(max == 2){
      a = auxvec[1];
      if(a == 0){
        *state = SPRIXCELL_MIXED_KITTY;
      }
      triplet[0xE] = b64subs[((a & 0xc0) >> 6) | (b64idx(triplet[0xE]) & 0x3c)];
      triplet[0xF] = b64subs[(a & 0x3f)];
    }
  }else{ // skip == 2
    int a = auxvec[0];
    if(a == 0){
      *state = SPRIXCELL_MIXED_KITTY;
    }
    triplet[0xE] = b64subs[((a & 0xc0) >> 6) | (b64idx(triplet[0xE]) & 0x3c)];
    triplet[0xF] = b64subs[(a & 0x3f)];
  }
  return max;
}

// if there is no mstreamfp open, create one, using glyph and glyphlen as the
// base. we're blowing away the glyph.
static int
init_sprixel_animation(sprixel* s){
  if(s->animating){
    return 0;
  }
  if(fbuf_init(&s->glyph)){
    return -1;
  }
  s->animating = true;
  return 0;
}

#define RGBA_MAXLEN 768 // 768 base64-encoded pixels in 4096 bytes
// restore an annihilated sprixcell by copying the alpha values from the
// auxiliary vector back into the actual data. we then free the auxvector.
int kitty_rebuild(sprixel* s, int ycell, int xcell, uint8_t* auxvec){
  const int totalpixels = s->pixy * s->pixx;
  const int xpixels = s->cellpxx;
  const int ypixels = s->cellpxy;
  int targx = xpixels;
  if((xcell + 1) * xpixels > s->pixx){
    targx = s->pixx - xcell * xpixels;
  }
  int targy = ypixels;
  if((ycell + 1) * ypixels > s->pixy){
    targy = s->pixy - ycell * ypixels;
  }
  char* c = (char*)s->glyph.buf + s->parse_start;
  int nextpixel = (s->pixx * ycell * ypixels) + (xpixels * xcell);
  int thisrow = targx;
  int chunkedhandled = 0;
  sprixcell_e state = SPRIXCELL_OPAQUE_KITTY;
  const int chunks = totalpixels / RGBA_MAXLEN + !!(totalpixels % RGBA_MAXLEN);
  int auxvecidx = 0;
  while(targy && chunkedhandled < chunks){ // need to null out |targy| rows of |targx| pixels, track with |thisrow|
    int inchunk = totalpixels - chunkedhandled * RGBA_MAXLEN;
    if(inchunk > RGBA_MAXLEN){
      inchunk = RGBA_MAXLEN;
    }
    const int curpixel = chunkedhandled * RGBA_MAXLEN;
    // a full chunk is 4096 + 2 + 7 (5005)
    while(nextpixel - curpixel < RGBA_MAXLEN && thisrow){
      // our next pixel is within this chunk. find the pixel offset of the
      // first pixel (within the chunk).
      int pixoffset = nextpixel - curpixel;
      int triples = pixoffset / 3;
      int tripbytes = triples * 16;
      int tripskip = pixoffset - triples * 3;
      // we start within a 16-byte chunk |tripbytes| into the chunk. determine
      // the number of bits.
//fprintf(stderr, "pixoffset: %d next: %d tripbytes: %d tripskip: %d thisrow: %d\n", pixoffset, nextpixel, tripbytes, tripskip, thisrow);
      // the maximum number of pixels we can convert is the minimum of the
      // pixels remaining in the target row, and the pixels left in the chunk.
//fprintf(stderr, "inchunk: %d total: %d triples: %d\n", inchunk, totalpixels, triples);
      int chomped = kitty_restore(c + tripbytes, tripskip, thisrow,
                                  inchunk - triples * 3, auxvec + auxvecidx,
                                  &state);
      assert(chomped >= 0);
      auxvecidx += chomped;
      thisrow -= chomped;
//fprintf(stderr, "POSTCHIMP CHOMP: %d pixoffset: %d next: %d tripbytes: %d tripskip: %d thisrow: %d\n", chomped, pixoffset, nextpixel, tripbytes, tripskip, thisrow);
      if(thisrow == 0){
//fprintf(stderr, "CLEARED ROW, TARGY: %d\n", targy - 1);
        if(--targy == 0){
          s->n->tam[s->dimx * ycell + xcell].state = state;
          s->invalidated = SPRIXEL_INVALIDATED;
          return 1;
        }
        thisrow = targx;
//fprintf(stderr, "BUMP IT: %d %d %d %d\n", nextpixel, s->pixx, targx, chomped);
        nextpixel += s->pixx - targx + chomped;
      }else{
        nextpixel += chomped;
      }
    }
    c += RGBA_MAXLEN * 4 * 4 / 3; // 4bpp * 4/3 for base64, 4096b per chunk
    c += 8; // new chunk header
    ++chunkedhandled;
//fprintf(stderr, "LOOKING NOW AT %u [%s]\n", c - s->glyph, c);
    while(*c != ';'){
      ++c;
    }
    ++c;
  }
  return -1;
}

// an animation auxvec requires storing all the pixel data for the cell,
// instead of just the alpha channel. pass the start of the RGBA to be
// copied, and the rowstride. dimy and dimx are the source image's total
// size in pixels. posy and posx are the origin of the cell to be copied,
// again in pixels. data is the image source. around the edges, we might
// get truncated regions.
static inline void*
kitty_anim_auxvec(int dimy, int dimx, int posy, int posx,
                  int cellpxy, int cellpxx, const uint32_t* data,
                  int rowstride, uint8_t* existing, uint32_t transcolor){
  const size_t slen = 4 * cellpxy * cellpxx;
  uint32_t* a = existing ? existing : malloc(slen);
  if(a){
    for(int y = posy ; y < posy + cellpxy && y < dimy ; ++y){
      int pixels = cellpxx;
      if(pixels + posx > dimx){
        pixels = dimx - posx;
      }
      /*logtrace("Copying %d (%d) from %p to %p %d/%d\n",
               pixels * 4, y,
               data + y * (rowstride / 4) + posx,
               a + (y - posy) * (pixels * 4),
               posy / cellpxy, posx / cellpxx);*/
      memcpy(a + (y - posy) * pixels, data + y * (rowstride / 4) + posx, pixels * 4);
      for(int x = posx ; x < posx + cellpxx && x < dimx ; ++x){
        uint32_t pixel = data[y * (rowstride / 4) + x];
        if(rgba_trans_p(pixel, transcolor)){
          uint32_t* ap = a + (y - posy) * pixels + (x - posx);
          ncpixel_set_a(ap, 0);
        }
      }
    }
  }
  return a;
}

uint8_t* kitty_trans_auxvec(const tinfo* ti){
  const size_t slen = ti->cellpixy * ti->cellpixx;
  uint8_t* a = malloc(slen);
  if(a){
    memset(a, 0, slen);
  }
  return a;
}

// we lay a cell-sixed animation block atop the graphic, giving it a
// cell id with which we can delete it in O(1) for a rebuild. this
// way, we needn't delete and redraw the entire sprixel.
int kitty_wipe_animation(sprixel* s, int ycell, int xcell){
  if(init_sprixel_animation(s)){
    return -1;
  }
  logdebug("wiping sprixel %u at %d/%d\n", s->id, ycell, xcell);
  fbuf* f = &s->glyph;
  if(fbuf_puts(f, "\x1b_Ga=f,x=") < 0){
    return -1;
  }
  if(fbuf_putint(f, xcell * s->cellpxx) < 0){
    return -1;
  }
  if(fbuf_puts(f, ",y=") < 0){
    return -1;
  }
  if(fbuf_putint(f, ycell * s->cellpxy)){
    return -1;
  }
  if(fbuf_puts(f, ",s=") < 0){
    return -1;
  }
  if(fbuf_putint(f, s->cellpxx)){
    return -1;
  }
  if(fbuf_puts(f, ",v=") < 0){
    return -1;
  }
  if(fbuf_putint(f, s->cellpxy)){
    return -1;
  }
  if(fbuf_puts(f, ",i=") < 0){
    return -1;
  }
  if(fbuf_putint(f, s->id)){
    return -1;
  }
  if(fbuf_puts(f, ",X=1,r=1,q=2;") < 0){
    return -1;
  }
  // FIXME ought be smaller around the fringes!
  int totalp = s->cellpxy * s->cellpxx;
  // FIXME preserve so long as cellpixel geom stays constant?
  for(int p = 0 ; p + 3 <= totalp ; p += 3){
    if(fbuf_putn(f, "AAAAAAAAAAAAAAAA", strlen("AAAAAAAAAAAAAAAA")) < 0){
      return -1;
    }
  }
  if(totalp % 3 == 1){
    if(fbuf_putn(f, "AAAAAA==", strlen("AAAAAA==")) < 0){
      return -1;
    }
  }else if(totalp % 3 == 2){
    if(fbuf_putn(f, "AAAAAAAAAAA=", strlen("AAAAAAAAAAA=")) < 0){
      return -1;
    }
  }
  // FIXME need chunking for cells of 768+ pixels
  if(fbuf_putn(f, "\x1b\\", 2) < 0){
    return -1;
  }
  s->invalidated = SPRIXEL_INVALIDATED;
  return 1;
}

// FIXME merge back with kitty_wipe_animation
int kitty_wipe_selfref(sprixel* s, int ycell, int xcell){
  if(init_sprixel_animation(s)){
    return -1;
  }
  logdebug("Wiping sprixel %u at %d/%d\n", s->id, ycell, xcell);
  fbuf* f = &s->glyph;
  if(fbuf_printf(f, "\e_Ga=f,x=%d,y=%d,s=%d,v=%d,i=%d,X=1,r=2,c=1,q=2;",
                 xcell * s->cellpxx, ycell * s->cellpxy,
                 s->cellpxx, s->cellpxy, s->id) < 0){
    return -1;
  }
  // FIXME ought be smaller around the fringes!
  int totalp = s->cellpxy * s->cellpxx;
  // FIXME preserve so long as cellpixel geom stays constant?
  #define TRINULLALPHA "AAAAAAAAAAAAAAAA"
  for(int p = 0 ; p + 3 <= totalp ; p += 3){
    if(fbuf_putn(f, TRINULLALPHA, strlen(TRINULLALPHA)) < 0){
      return -1;
    }
  }
  #undef TRINULLALPHA
  if(totalp % 3 == 1){
  #define UNUMNULLALPHA "AAAAAA=="
    if(fbuf_putn(f, UNUMNULLALPHA, strlen(UNUMNULLALPHA)) < 0){
      return -1;
    }
  #undef UNUMNULLALPHA
  }else if(totalp % 3 == 2){
  #define DUONULLALPHA "AAAAAAAAAAA="
    if(fbuf_putn(f, DUONULLALPHA, strlen(DUONULLALPHA)) < 0){
      return -1;
    }
  #undef DUONULLALPHA
  }
  // FIXME need chunking for cells of 768+ pixels
  if(fbuf_printf(f, "\e\\\e_Ga=a,i=%d,c=2,q=2;\e\\", s->id) < 0){
    return -1;
  }
  s->invalidated = SPRIXEL_INVALIDATED;
  return 1;
}

sprixel* kitty_recycle(ncplane* n){
  assert(n->sprite);
  sprixel* hides = n->sprite;
  int dimy = hides->dimy;
  int dimx = hides->dimx;
  sprixel_hide(hides);
  return sprixel_alloc(&ncplane_notcurses_const(n)->tcache, n, dimy, dimx);
}

int kitty_wipe(sprixel* s, int ycell, int xcell){
//fprintf(stderr, "NEW WIPE %d %d/%d\n", s->id, ycell, xcell);
  uint8_t* auxvec = sprixel_auxiliary_vector(s);
  if(auxvec == NULL){
    return -1;
  }
  const int totalpixels = s->pixy * s->pixx;
  const int xpixels = s->cellpxx;
  const int ypixels = s->cellpxy;
  // if the cell is on the right or bottom borders, it might only be partially
  // filled by actual graphic data, and we need to cap our target area.
  int targx = xpixels;
  if((xcell + 1) * xpixels > s->pixx){
    targx = s->pixx - xcell * xpixels;
  }
  int targy = ypixels;
  if((ycell + 1) * ypixels > s->pixy){
    targy = s->pixy - ycell * ypixels;
  }
  char* c = (char*)s->glyph.buf + s->parse_start;
//fprintf(stderr, "TARGET AREA: %d x %d @ %dx%d of %d/%d (%d/%d) len %zu\n", targy, targx, ycell, xcell, s->dimy, s->dimx, s->pixy, s->pixx, strlen(c));
  // every pixel was 4 source bytes, 32 bits, 6.33 base64 bytes. every 3 input pixels is
  // 12 bytes (96 bits), an even 16 base64 bytes. there is chunking to worry about. there
  // are up to 768 pixels in a chunk.
  int nextpixel = (s->pixx * ycell * ypixels) + (xpixels * xcell);
  int thisrow = targx;
  int chunkedhandled = 0;
  const int chunks = totalpixels / RGBA_MAXLEN + !!(totalpixels % RGBA_MAXLEN);
  int auxvecidx = 0;
  while(targy && chunkedhandled < chunks){ // need to null out |targy| rows of |targx| pixels, track with |thisrow|
//fprintf(stderr, "PLUCKING FROM [%s]\n", c);
    int inchunk = totalpixels - chunkedhandled * RGBA_MAXLEN;
    if(inchunk > RGBA_MAXLEN){
      inchunk = RGBA_MAXLEN;
    }
    const int curpixel = chunkedhandled * RGBA_MAXLEN;
    // a full chunk is 4096 + 2 + 7 (5005)
    while(nextpixel - curpixel < RGBA_MAXLEN && thisrow){
      // our next pixel is within this chunk. find the pixel offset of the
      // first pixel (within the chunk).
      int pixoffset = nextpixel - curpixel;
      int triples = pixoffset / 3;
      int tripbytes = triples * 16;
      // we start within a 16-byte chunk |tripbytes| into the chunk. determine
      // the number of bits.
      int tripskip = pixoffset - triples * 3;
//fprintf(stderr, "pixoffset: %d next: %d tripbytes: %d tripskip: %d thisrow: %d\n", pixoffset, nextpixel, tripbytes, tripskip, thisrow);
      // the maximum number of pixels we can convert is the minimum of the
      // pixels remaining in the target row, and the pixels left in the chunk.
//fprintf(stderr, "inchunk: %d total: %d triples: %d\n", inchunk, totalpixels, triples);
//fprintf(stderr, "PRECHOMP:  [%.16s]\n", c + tripbytes);
      int chomped = kitty_null(c + tripbytes, tripskip, thisrow,
                               inchunk - triples * 3, auxvec + auxvecidx);
//fprintf(stderr, "POSTCHOMP: [%.16s]\n", c + tripbytes);
      assert(chomped >= 0);
      auxvecidx += chomped;
      assert(auxvecidx <= s->cellpxy * s->cellpxx);
      thisrow -= chomped;
//fprintf(stderr, "POSTCHIMP CHOMP: %d pixoffset: %d next: %d tripbytes: %d tripskip: %d thisrow: %d\n", chomped, pixoffset, nextpixel, tripbytes, tripskip, thisrow);
      if(thisrow == 0){
//fprintf(stderr, "CLEARED ROW, TARGY: %d\n", targy - 1);
        if(--targy == 0){
          s->n->tam[s->dimx * ycell + xcell].auxvector = auxvec;
          s->invalidated = SPRIXEL_INVALIDATED;
          return 1;
        }
        thisrow = targx;
//fprintf(stderr, "BUMP IT: %d %d %d %d\n", nextpixel, s->pixx, targx, chomped);
        nextpixel += s->pixx - targx + chomped;
      }else{
        nextpixel += chomped;
      }
    }
    c += RGBA_MAXLEN * 4 * 4 / 3; // 4bpp * 4/3 for base64, 4096b per chunk
    c += 8; // new chunk header
    ++chunkedhandled;
//fprintf(stderr, "LOOKING NOW AT %u [%s]\n", c - s->glyph, c);
    while(*c != ';'){
      ++c;
    }
    ++c;
  }
  free(auxvec);
  return -1;
}

int kitty_commit(fbuf* f, sprixel* s, unsigned noscroll){
  loginfo("Committing Kitty graphic id %u\n", s->id);
  fbuf_printf(f, "\e_Ga=p,i=%u,p=1,q=2%s\e\\", s->id, noscroll ? ",C=1" : "");
  s->invalidated = SPRIXEL_QUIESCENT;
  return 0;
}

static void
cleanup_tam(tament* tam, int ydim, int xdim){
  for(int y = 0 ; y < ydim ; ++y){
    for(int x = 0 ; x < xdim ; ++x){
      free(tam[y * xdim + x].auxvector);
    }
  }
}

static inline void*
zctx_origbuf(z_stream* zctx, int pixy, int pixx){
  size_t blen = pixx * pixy * 4;
  unsigned long b = deflateBound(zctx, blen);
  return (unsigned char *)zctx->next_out - (b - zctx->avail_out);
}

static int
encode_and_chunkify(fbuf* f, z_stream* zctx, int pixy, int pixx){
  unsigned long totw = zctx->total_out;
  unsigned char* buf = zctx_origbuf(zctx, pixy, pixx);
  // need to terminate the header, requiring semicolon
  if(totw > 4096 * 3 / 4){
    if(fbuf_putn(f, ",m=1", 4) < 0){
      return -1;
    }
  }
  if(fbuf_putc(f, ';') < 0){
    return -1;
  }
  bool first = true;
  unsigned long i = 0;
  char b64d[4];
  while(totw - i > 4096 * 3 / 4){
    if(!first){
      if(fbuf_putn(f, "\x1b_Gm=1;", 7) < 0){
        return -1;
      }
    }
    unsigned long max = i + 4096 * 3 / 4;
    while(i < max){
      base64x3(buf + i, b64d);
      if(fbuf_putn(f, b64d, 4) < 0){
        return -1;
      }
      i += 3;
    }
    first = false;
    if(fbuf_putn(f, "\x1b\\", 2) < 0){
      return -1;
    }
  }
  if(!first){
    if(fbuf_putn(f, "\x1b_Gm=0;", 7) < 0){
      return -1;
    }
  }
  while(i < totw){
    if(totw - i < 3){
      base64final(buf + i, b64d, totw - i);
      if(fbuf_putn(f, b64d, 4) < 0){
        return -1;
      }
      i += totw - i;
    }else{
      base64x3(buf + i, b64d);
      if(fbuf_putn(f, b64d, 4) < 0){
        return -1;
      }
      i += 3;
    }
  }
  if(fbuf_putn(f, "\x1b\\", 2) < 0){
    return -1;
  }
  return 0;
}

static int
finalize_deflator(z_stream* zctx, fbuf* f, int dimy, int dimx){
  assert(0 == zctx->avail_in);
  int zret = deflate(zctx, Z_FINISH);
  if(zret != Z_STREAM_END){
    logerror("Error deflating (%d)\n", zret);
    return -1;
  }
  if(encode_and_chunkify(f, zctx, dimy, dimx)){
    return -1;
  }
  return 0;
}

// copy |encodeable| pixels to the deflate state
static int
add_to_deflator(z_stream* zctx, const uint32_t* src, int encodeable, bool wipe[static 3]){
  assert(0 == zctx->avail_in);
  uint32_t dst[3];
  zctx->next_in = (void*)dst;
  dst[0] = *src++;
  if(wipe[0] || rgba_trans_p(dst[0], 0)){
    ncpixel_set_a(&dst[0], 0);
  }
  if(encodeable > 1){
    dst[1] = *src++;
    if(wipe[1] || rgba_trans_p(dst[1], 0)){
      ncpixel_set_a(&dst[1], 0);
    }
    if(encodeable > 2){
      dst[2] = *src++;
      if(wipe[2] || rgba_trans_p(dst[2], 0)){
        ncpixel_set_a(&dst[2], 0);
      }
    }
  }
  zctx->avail_in = encodeable * 4;
  int zret = deflate(zctx, Z_NO_FLUSH);
  if(zret != Z_OK){
    logerror("Error deflating (%d)\n", zret);
    return -1;
  }
  return 0;
}

// writes to |*animated| based on normalized |level|
static int
prep_deflator(kitty_graphics_e level, z_stream* zctx, int pixy, int pixx,
              unsigned* animated){
  memset(zctx, 0, sizeof(*zctx));
  if(level >= KITTY_ANIMATION){
    int zret;
    // 2 seems to work well for things that are going to compress up
    // meaningfully at all, while not taking too much time.
    if((zret = deflateInit(zctx, 2)) != Z_OK){
      logerror("Couldn't get a deflate context (%d)\n", zret);
      return -1;
    }
    size_t blen = pixx * pixy * 4;
    unsigned long b = deflateBound(zctx, blen);
    unsigned char* buf = malloc(b);
    if(buf == NULL){
      deflateEnd(zctx);
      return -1;
    }
    zctx->avail_out = b;
    zctx->next_out = buf;
    *animated = true;
  }else{
    *animated = false;
  }
  return 0;
}

static void
destroy_deflator(unsigned animated, z_stream* zctx, int pixy, int pixx){
  if(animated){
    free(zctx_origbuf(zctx, pixy, pixx));
    deflateEnd(zctx);
  }
}

// we can only write 4KiB at a time. we're writing base64-encoded RGBA. each
// pixel is 4B raw (32 bits). each chunk of three pixels is then 12 bytes, or
// 16 base64-encoded bytes. 4096 / 16 == 256 3-pixel groups, or 768 pixels.
// closes |fp| on all paths.
static int
write_kitty_data(fbuf* f, int linesize, int leny, int lenx, int cols,
                 const uint32_t* data, const blitterargs* bargs,
                 tament* tam, int* parse_start, kitty_graphics_e level){
//fprintf(stderr, "drawing kitty %p\n", tam);
  if(linesize % sizeof(*data)){
    logerror("Stride (%d) badly aligned\n", linesize);
    return -1;
  }
  // we only deflate if we're using animation, since otherwise we need be able
  // to edit the encoded bitmap in-place for wipes/restores.
  z_stream zctx;
  unsigned animated;
  if(prep_deflator(level, &zctx, leny, lenx, &animated)){
    return -1;
  }
  bool translucent = bargs->flags & NCVISUAL_OPTION_BLEND;
  sprixel* s = bargs->u.pixel.spx;
  int sprixelid = s->id;
  int cdimy = s->cellpxy;
  int cdimx = s->cellpxx;
  uint32_t transcolor = bargs->transcolor;
  int total = leny * lenx; // total number of pixels (4 * total == bytecount)
  // number of 4KiB chunks we'll need
  int chunks = (total + (RGBA_MAXLEN - 1)) / RGBA_MAXLEN;
  int totalout = 0; // total pixels of payload out
  int y = 0; // position within source image (pixels)
  int x = 0;
  int targetout = 0; // number of pixels expected out after this chunk
//fprintf(stderr, "total: %d chunks = %d, s=%d,v=%d\n", total, chunks, lenx, leny);
  char out[17]; // three pixels base64 to no more than 17 bytes
  while(chunks--){
    // q=2 has been able to go on chunks other than the last chunk since
    // 2021-03, but there's no harm in this small bit of backwards compat.
    if(totalout == 0){
      // older versions of kitty will delete uploaded images when scrolling,
      // alas. see https://github.com/dankamongmen/notcurses/issues/1910 =[.
      // parse_start isn't used in animation mode, so no worries there.
      *parse_start = fbuf_printf(f, "\e_Gf=32,s=%d,v=%d,i=%d,p=1,a=t,%s",
                                 lenx, leny, sprixelid,
                                 animated ? "o=z,q=2" : chunks ? "m=1;" : "q=2;");
      if(*parse_start < 0){
        goto err;
      }
      // so if we're animated, we've printed q=2, but no semicolon to close
      // the control block, since we're not yet sure what m= to write. we've
      // otherwise written q=2; if we're the only chunk, and m=1; otherwise.
      // if we're *not* animated, we'll get q=2,m=0; below. otherwise, it's
      // handled following deflate.
    }else{
      if(!animated){
        if(fbuf_printf(f, "\e_G%sm=%d;", chunks ? "" : "q=2,", chunks ? 1 : 0) < 0){
          goto err;
        }
      }
    }
    if((targetout += RGBA_MAXLEN) > total){
      targetout = total;
    }
    while(totalout < targetout){
      int encodeable = targetout - totalout;
      if(encodeable > 3){
        encodeable = 3;
      }
      uint32_t source[3]; // we encode up to 3 pixels at a time
      bool wipe[3];
      for(int e = 0 ; e < encodeable ; ++e){
        if(x == lenx){
          x = 0;
          ++y;
        }
        const uint32_t* line = data + (linesize / sizeof(*data)) * y;
        source[e] = line[x];
        if(translucent){
          ncpixel_set_a(&source[e], ncpixel_a(source[e]) / 2);
        }
//fprintf(stderr, "%u/%u/%u -> %c%c%c%c %u %u %u %u\n", r, g, b, b64[0], b64[1], b64[2], b64[3], b64[0], b64[1], b64[2], b64[3]);
        int xcell = x / cdimx;
        int ycell = y / cdimy;
        int tyx = xcell + ycell * cols;
//fprintf(stderr, "Tyx: %d y: %d (%d) * %d x: %d (%d) state %d %p\n", tyx, y, y / cdimy, cols, x, x / cdimx, tam[tyx].state, tam[tyx].auxvector);
        // old-style animated auxvecs carry the entirety of the replacement
        // data in them. on the first pixel of the cell, ditch the previous
        // auxvec in its entirety, and copy over the entire cell.
        if(x % cdimx == 0 && y % cdimy == 0){
          if(level == KITTY_ANIMATION){
            uint8_t* tmp;
            tmp = kitty_anim_auxvec(leny, lenx, y, x, cdimy, cdimx,
                                    data, linesize, tam[tyx].auxvector,
                                    transcolor);
            if(tmp == NULL){
              goto err;
            }
            tam[tyx].auxvector = tmp;
          }else if(level == KITTY_SELFREF){
            if(tam[tyx].auxvector == NULL){
              tam[tyx].auxvector = malloc(sizeof(tam[tyx].state));
              if(tam[tyx].auxvector == NULL){
                goto err;
              }
            }
            memcpy(tam[tyx].auxvector, &tam[tyx].state, sizeof(tam[tyx].state));
          }
        }
        if(tam[tyx].state >= SPRIXCELL_ANNIHILATED){
          if(!animated){
            // this pixel is part of a cell which is currently wiped (alpha-nulled
            // out, to present a glyph "atop" it). we will continue to mark it
            // transparent, but we need to update the auxiliary vector.
            const int vyx = (y % cdimy) * cdimx + (x % cdimx);
            tam[tyx].auxvector[vyx] = ncpixel_a(source[e]);
          }
          if(rgba_trans_p(source[e], transcolor)){
            ncpixel_set_a(&source[e], 0); // in case it was transcolor
            if(x % cdimx == 0 && y % cdimy == 0){
              tam[tyx].state = SPRIXCELL_ANNIHILATED_TRANS;
            }
          }else{
            tam[tyx].state = SPRIXCELL_ANNIHILATED;
          }
          wipe[e] = 1;
        }else{
          wipe[e] = 0;
          if(rgba_trans_p(source[e], transcolor)){
            ncpixel_set_a(&source[e], 0); // in case it was transcolor
            if(x % cdimx == 0 && y % cdimy == 0){
              tam[tyx].state = SPRIXCELL_TRANSPARENT;
            }else if(tam[tyx].state == SPRIXCELL_OPAQUE_KITTY){
              tam[tyx].state = SPRIXCELL_MIXED_KITTY;
            }
          }else{
            if(x % cdimx == 0 && y % cdimy == 0){
              tam[tyx].state = SPRIXCELL_OPAQUE_KITTY;
            }else if(tam[tyx].state == SPRIXCELL_TRANSPARENT){
              tam[tyx].state = SPRIXCELL_MIXED_KITTY;
            }
          }
        }
        ++x;
      }
      totalout += encodeable;
      if(animated){
        if(add_to_deflator(&zctx, source, encodeable, wipe)){
          goto err;
        }
      }else{
        // we already took transcolor to alpha 0; there's no need to
        // check it again, so pass 0.
        base64_rgba3(source, encodeable, out, wipe, 0);
        if(fbuf_puts(f, out) < 0){
          goto err;
        }
      }
    }
    if(!animated){
      if(fbuf_putn(f, "\x1b\\", 2) < 0){
        goto err;
      }
    }
  }
  if(animated){
    if(finalize_deflator(&zctx, f, leny, lenx)){
      goto err;
    }
  }
  scrub_tam_boundaries(tam, leny, lenx, cdimy, cdimx);
  destroy_deflator(animated, &zctx, leny, lenx);
  return 0;

err:
  cleanup_tam(tam, (leny + cdimy - 1) / cdimy, (lenx + cdimx - 1) / cdimx);
  destroy_deflator(animated, &zctx, leny, lenx);
  return -1;
}

// with t=z, we can reference the original frame, and say "redraw this region",
// thus avoiding the need to carry the original data around in our auxvecs.
int kitty_rebuild_selfref(sprixel* s, int ycell, int xcell, uint8_t* auxvec){
  if(init_sprixel_animation(s)){
    return -1;
  }
  fbuf* f = &s->glyph;
  const int ystart = ycell * s->cellpxy;
  const int xstart = xcell * s->cellpxx;
  const int xlen = xstart + s->cellpxx > s->pixx ? s->pixx - xstart : s->cellpxx;
  const int ylen = ystart + s->cellpxy > s->pixy ? s->pixy - ystart : s->cellpxy;
  logdebug("rematerializing %u at %d/%d (%dx%d)\n", s->id, ycell, xcell, ylen, xlen);
  fbuf_printf(f, "\e_Ga=c,x=%d,y=%d,X=%d,Y=%d,w=%d,h=%d,i=%d,r=1,c=2,q=2;\x1b\\",
              xcell * s->cellpxx, ycell * s->cellpxy,
              xcell * s->cellpxx, ycell * s->cellpxy,
              xlen, ylen, s->id);
  const int tyx = xcell + ycell * s->dimx;
  memcpy(&s->n->tam[tyx].state, auxvec, sizeof(s->n->tam[tyx].state));
  s->invalidated = SPRIXEL_INVALIDATED;
  return 0;
}

int kitty_rebuild_animation(sprixel* s, int ycell, int xcell, uint8_t* auxvec){
  if(init_sprixel_animation(s)){
    return -1;
  }
  fbuf* f = &s->glyph;
  logdebug("rebuilding sprixel %u %d at %d/%d\n", s->id, s->invalidated, ycell, xcell);
  const int ystart = ycell * s->cellpxy;
  const int xstart = xcell * s->cellpxx;
  const int xlen = xstart + s->cellpxx > s->pixx ? s->pixx - xstart : s->cellpxx;
  const int ylen = ystart + s->cellpxy > s->pixy ? s->pixy - ystart : s->cellpxy;
  const int linesize = xlen * 4;
  const int total = xlen * ylen;
  const int tyx = xcell + ycell * s->dimx;
  int chunks = (total + (RGBA_MAXLEN - 1)) / RGBA_MAXLEN;
  int totalout = 0; // total pixels of payload out
  int y = 0; // position within source image (pixels)
  int x = 0;
  int targetout = 0; // number of pixels expected out after this chunk
//fprintf(stderr, "total: %d chunks = %d, s=%d,v=%d\n", total, chunks, lenx, leny);
  // FIXME this ought be factored out and shared with write_kitty_data()
  logdebug("placing %d/%d at %d/%d\n", ylen, xlen, ycell * s->cellpxy, xcell * s->cellpxx);
  while(chunks--){
    if(totalout == 0){
      if(fbuf_printf(f, "\e_Ga=f,x=%d,y=%d,s=%d,v=%d,i=%d,X=1,r=1,%s;",
                     xcell * s->cellpxx, ycell * s->cellpxy, xlen, ylen,
                     s->id, chunks ? "m=1" : "q=2") < 0){
        return -1;
      }
    }else{
      if(fbuf_putn(f, "\x1b_G", 3) < 0){
        return -1;
      }
      if(!chunks){
        if(fbuf_putn(f, "q=2,", 4) < 0){
          return -1;
        }
      }
      if(fbuf_putn(f, "m=", 2) < 0){
        return -1;
      }
      if(fbuf_putint(f, chunks ? 1 : 0) < 0){
        return -1;
      }
      if(fbuf_putc(f, ';') != 1){
        return -1;
      }
    }
    if((targetout += RGBA_MAXLEN) > total){
      targetout = total;
    }
    while(totalout < targetout){
      int encodeable = targetout - totalout;
      if(encodeable > 3){
        encodeable = 3;
      }
      uint32_t source[3]; // we encode up to 3 pixels at a time
      bool wipe[3];
      for(int e = 0 ; e < encodeable ; ++e){
        if(x == xlen){
          x = 0;
          ++y;
        }
        const uint32_t* line = (const uint32_t*)(auxvec + linesize * y);
        source[e] = line[x];
//fprintf(stderr, "%u/%u/%u -> %c%c%c%c %u %u %u %u\n", r, g, b, b64[0], b64[1], b64[2], b64[3], b64[0], b64[1], b64[2], b64[3]);
//fprintf(stderr, "Tyx: %d y: %d (%d) * %d x: %d (%d) state %d %p\n", tyx, y, y / cdimy, cols, x, x / cdimx, tam[tyx].state, tam[tyx].auxvector);
        wipe[e] = 0;
        if(rgba_trans_p(source[e], 0)){
          if(x % s->cellpxx == 0 && y % s->cellpxy == 0){
            s->n->tam[tyx].state = SPRIXCELL_TRANSPARENT;
          }else if(s->n->tam[tyx].state == SPRIXCELL_OPAQUE_KITTY){
            s->n->tam[tyx].state = SPRIXCELL_MIXED_KITTY;
          }
        }else{
          if(x % s->cellpxx == 0 && y % s->cellpxy == 0){
            s->n->tam[tyx].state = SPRIXCELL_OPAQUE_KITTY;
          }else if(s->n->tam[tyx].state == SPRIXCELL_TRANSPARENT){
            s->n->tam[tyx].state = SPRIXCELL_MIXED_KITTY;
          }
        }
        ++x;
      }
      totalout += encodeable;
      char out[17];
      base64_rgba3(source, encodeable, out, wipe, 0);
      if(fbuf_puts(f, out) < 0){
        return -1;
      }
    }
    if(fbuf_putn(f, "\x1b\\", 2) < 0){
      return -1;
    }
  }
//fprintf(stderr, "EMERGED WITH TAM STATE %d\n", s->n->tam[tyx].state);
  s->invalidated = SPRIXEL_INVALIDATED;
  return 0;
}
#undef RGBA_MAXLEN

// Kitty graphics blitter. Kitty can take in up to 4KiB at a time of (optionally
// deflate-compressed) 24bit RGB. Returns -1 on error, 1 on success.
static inline int
kitty_blit_core(ncplane* n, int linesize, const void* data, int leny, int lenx,
                const blitterargs* bargs, kitty_graphics_e level){
//fprintf(stderr, "IMAGE: start %p end %p\n", data, (const char*)data + leny * linesize);
  int cols = bargs->u.pixel.spx->dimx;
  int rows = bargs->u.pixel.spx->dimy;
  sprixel* s = bargs->u.pixel.spx;
  if(init_sprixel_animation(s)){
    return -1;
  }
  tament* tam = NULL;
  bool reuse = false;
  // if we have a sprixel attached to this plane, see if we can reuse it
  // (we need the same dimensions) and thus immediately apply its T-A table.
  if(n->tam){
    if(n->leny == rows && n->lenx == cols){
      tam = n->tam;
      reuse = true;
    }
  }
  int parse_start = 0;
  if(!reuse){
    tam = malloc(sizeof(*tam) * rows * cols);
    if(tam == NULL){
      goto error;
    }
    memset(tam, 0, sizeof(*tam) * rows * cols);
  }
  fbuf* f = &s->glyph;
  if(write_kitty_data(f, linesize, leny, lenx, cols, data,
                      bargs, tam, &parse_start, level)){
    goto error;
  }
  if(level == KITTY_ALWAYS_SCROLLS){
    s->animating = false;
  }
  // take ownership of |buf| and |tam| on success.
  if(plane_blit_sixel(s, &s->glyph, leny, lenx, parse_start, tam) < 0){
    goto error;
  }
  return 1;

error:
  if(!reuse){
    free(tam);
  }
  fbuf_free(&s->glyph);
  return -1;
}

int kitty_blit(ncplane* n, int linesize, const void* data, int leny, int lenx,
               const blitterargs* bargs){
  return kitty_blit_core(n, linesize, data, leny, lenx, bargs,
                         KITTY_ALWAYS_SCROLLS);
}

int kitty_blit_animated(ncplane* n, int linesize, const void* data,
                        int leny, int lenx, const blitterargs* bargs){
  return kitty_blit_core(n, linesize, data, leny, lenx, bargs,
                         KITTY_ANIMATION);
}

int kitty_blit_selfref(ncplane* n, int linesize, const void* data,
                       int leny, int lenx, const blitterargs* bargs){
  return kitty_blit_core(n, linesize, data, leny, lenx, bargs,
                         KITTY_SELFREF);
}

int kitty_remove(int id, fbuf* f){
  loginfo("Removing graphic %u\n", id);
  if(fbuf_printf(f, "\e_Ga=d,d=i,i=%d\e\\", id) < 0){
    return -1;
  }
  return 0;
}

// damages cells underneath the graphic which were OPAQUE
int kitty_scrub(const ncpile* p, sprixel* s){
//fprintf(stderr, "FROM: %d/%d state: %d s->n: %p\n", s->movedfromy, s->movedfromx, s->invalidated, s->n);
  for(int yy = s->movedfromy ; yy < s->movedfromy + s->dimy && yy < p->dimy ; ++yy){
    for(int xx = s->movedfromx ; xx < s->movedfromx + s->dimx && xx < p->dimx ; ++xx){
      const int ridx = yy * p->dimx + xx;
      struct crender *r = &p->crender[ridx];
      if(!r->sprixel){
        if(s->n){
//fprintf(stderr, "CHECKING %d/%d\n", yy - s->movedfromy, xx - s->movedfromx);
          sprixcell_e state = sprixel_state(s, yy - s->movedfromy + s->n->absy,
                                              xx - s->movedfromx + s->n->absx);
          if(state == SPRIXCELL_OPAQUE_KITTY){
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
  }
  return 0;
}

// returns the number of bytes written
int kitty_draw(const tinfo* ti, const ncpile* p, sprixel* s, fbuf* f,
               int y, int x){
  (void)ti;
  (void)p;
  (void)y;
  (void)x;
  bool animated = false;
  if(s->animating){ // active animation
    s->animating = false;
    animated = true;
  }
  int ret = s->glyph.used;
  logdebug("Writing out %zub for %u\n", s->glyph.used, s->id);
  if(ret){
    if(fbuf_putn(f, s->glyph.buf, s->glyph.used) < 0){
      ret = -1;
    }
  }
  if(animated){
    fbuf_free(&s->glyph);
  }
  s->invalidated = SPRIXEL_LOADED;
  return ret;
}

// returns -1 on failure, 0 on success (move bytes do not count for sprixel stats)
int kitty_move(sprixel* s, fbuf* f, unsigned noscroll){
  int ret = 0;
  if(fbuf_printf(f, "\e_Ga=p,i=%d,p=1,q=2%s\e\\", s->id,
                 noscroll ? ",C=1" : "") < 0){
    ret = -1;
  }
  s->invalidated = SPRIXEL_QUIESCENT;
  return ret;
}

// clears all kitty bitmaps
int kitty_clear_all(fbuf* f){
//fprintf(stderr, "KITTY UNIVERSAL ERASE\n");
  if(fbuf_putn(f, "\x1b_Ga=d,q=2\x1b\\", 12) < 0){
    return -1;
  }
  return 0;
}

int kitty_shutdown(fbuf* f){
  // FIXME need to close off any open kitty bitmap emission, or we will
  // lock up the terminal
  (void)f;
  return 0;
}
