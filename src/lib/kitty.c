#include "internal.h"

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
// How to reclaim a section once we no longer want to draw text above it is
// an open question; we'd presumably need to store the original alphas in
// the T-A matrix.
//
// It has some interesting features of which we do not yet take advantage:
//  * in-terminal scaling of image data (we prescale)
//  * subregion display of a transmitted bitmap
//  * an animation protocol we should probably use for video, and definitely
//     ought use for cell wiping
//  * movement (redisplay at another position) of loaded bitmaps
//
// https://sw.kovidgoyal.net/kitty/graphics-protocol.html
//
// We are unlikely to ever use several features: direct PNG support (only
// works for PNG), transfer via shared memory (only works locally),
// compression (only helps on easy graphics), or offsets within a cell.

// lookup table for base64
static unsigned const char b64subs[] =
 "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// convert a base64 character into its equivalent integer 0..63
static inline int
b64idx(char b64){
  if(b64 >= 'A' && b64 <= 'Z'){
    return b64 - 'A';
  }else if(b64 >= 'a' && b64 <= 'z'){
    return b64 - 'a';
  }else if(b64 >= '0' && b64 <= '9'){
    return b64 - '0';
  }else if(b64 == '+'){
    return 62;
  }else{
    return 63;
  }
}

// every 3 RGBA pixels (96 bits) become 16 base64-encoded bytes (128 bits). if
// there are only 2 pixels available, those 64 bits become 12 bytes. if there
// is only 1 pixel available, those 32 bits become 8 bytes. (pcount + 1) * 4
// bytes are used, plus a null terminator. we thus must receive 17.
static void
base64_rgba3(const uint32_t* pixels, size_t pcount, char* b64){
  uint32_t pixel = *pixels++;
  unsigned r = ncpixel_r(pixel);
  unsigned g = ncpixel_g(pixel);
  unsigned b = ncpixel_b(pixel);
  unsigned a = rgba_trans_p(ncpixel_a(pixel)) ? 0 : 255;
  b64[0] = b64subs[(r & 0xfc) >> 2];
  b64[1] = b64subs[(r & 0x3 << 4) | ((g & 0xf0) >> 4)];
  b64[2] = b64subs[((g & 0xf) << 2) | ((b & 0xc0) >> 6)];
  b64[3] = b64subs[b & 0x3f];
  b64[4] = b64subs[(a & 0xfc) >> 2];
  if(pcount == 1){
    b64[5] = b64subs[(a & 0x3) << 4];
    b64[6] = '=';
    b64[7] = '=';
    b64[8] = '\0';
    return;
  }
  b64[5] = (a & 0x3) << 4;
  pixel = *pixels++;
  r = ncpixel_r(pixel);
  g = ncpixel_g(pixel);
  b = ncpixel_b(pixel);
  a = rgba_trans_p(ncpixel_a(pixel)) ? 0 : 255;
  b64[5] = b64subs[b64[5] | ((r & 0xf0) >> 4)];
  b64[6] = b64subs[((r & 0xf) << 2) | ((g & 0xc0) >> 6u)];
  b64[7] = b64subs[g & 0x3f];
  b64[8] = b64subs[(b & 0xfc) >> 2];
  b64[9] = b64subs[((b & 0x3) << 4) | ((a & 0xf0) >> 4)];
  if(pcount == 2){
    b64[10] = b64subs[(a & 0xf) << 2];
    b64[11] = '=';
    b64[12] = '\0';
    return;
  }
  b64[10] = (a & 0xf) << 2;
  pixel = *pixels;
  r = ncpixel_r(pixel);
  g = ncpixel_g(pixel);
  b = ncpixel_b(pixel);
  a = rgba_trans_p(ncpixel_a(pixel)) ? 0 : 255;
  b64[10] = b64subs[b64[10] | ((r & 0xc0) >> 6)];
  b64[11] = b64subs[r & 0x3f];
  b64[12] = b64subs[(g & 0xfc) >> 2];
  b64[13] = b64subs[((g & 0x3) << 4) | ((b & 0xf0) >> 4)];
  b64[14] = b64subs[((b & 0xf) << 2) | ((a & 0xc0) >> 6)];
  b64[15] = b64subs[a & 0x3f];
  b64[16] = '\0';
}

// null out part of a triplet (a triplet is 3 pixels, which map to 12 bytes, which map to
// 16 bytes when base64 encoded). skip the initial |skip| pixels, and null out a maximum
// of |max| pixels after that. returns the number of pixels nulled out. |max| must be
// positive. |skip| must be non-negative, and less than 3. |pleft| is the number of pixels
// available in the chunk.
static inline int
kitty_null(char* triplet, int skip, int max, int pleft){
//fprintf(stderr, "SKIP/MAX/PLEFT %d/%d/%d\n", skip, max, pleft);
  if(pleft > 3){
    pleft = 3;
  }
  if(max + skip > pleft){
    max = pleft - skip;
  }
//fprintf(stderr, "alpha-nulling %d after %d\n", max, skip);
  if(skip == 0){
    if(max == 1){
      memset(triplet, b64subs[0], 5);
      triplet[5] = b64subs[b64idx(triplet[5]) & 0xf];
    }else if(max == 2){
      memset(triplet, b64subs[0], 10);
      triplet[10] = b64subs[b64idx(triplet[10]) & 0x3];
    }else{ // max == 3
      memset(triplet, b64subs[0], 16);
    }
  }else if(skip == 1){
    if(max == 1){
      triplet[5] = b64subs[b64idx(triplet[5]) & 0x30];
      memset(triplet + 6, b64subs[0], 4);
      triplet[10] = b64subs[b64idx(triplet[10]) & 0x3];
    }else{
      triplet[5] = b64subs[b64idx(triplet[5]) & 0x30];
      memset(triplet + 6, b64subs[0], 10);
    }
  }else{ // skip == 2
    if(max == 1){
      triplet[10] = b64subs[b64idx(triplet[10]) & 0xf];
      memset(triplet + 11, b64subs[0], 5);
    }
  }
  return max;
}

#define RGBA_MAXLEN 768 // 768 base64-encoded pixels in 4096 bytes
int sprite_kitty_cell_wipe(const notcurses* nc, sprixel* s, int ycell, int xcell){
  const int totalpixels = s->pixy * s->pixx;
  const int xpixels = nc->tcache.cellpixx;
  const int ypixels = nc->tcache.cellpixy;
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
  char* c = s->glyph + s->parse_start;
//fprintf(stderr, "TARGET AREA: %d x %d @ %dx%d of %d/%d (%d/%d) len %zu\n", targy, targx, ycell, xcell, s->dimy, s->dimx, s->pixy, s->pixx, strlen(c));
  // every pixel was 4 source bytes, 32 bits, 6.33 base64 bytes. every 3 input pixels is
  // 12 bytes (96 bits), an even 16 base64 bytes. there is chunking to worry about. there
  // are up to 768 pixels in a chunk.
  int nextpixel = (s->pixx * ycell * ypixels) + (xpixels * xcell);
  int thisrow = targx;
  int chunkedhandled = 0;
  const int chunks = totalpixels / RGBA_MAXLEN + !!(totalpixels % RGBA_MAXLEN);
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
      int chomped = kitty_null(c + tripbytes, tripskip, thisrow, inchunk - triples * 3);
      assert(chomped >= 0);
      thisrow -= chomped;
//fprintf(stderr, "POSTCHIMP CHOMP: %d pixoffset: %d next: %d tripbytes: %d tripskip: %d thisrow: %d\n", chomped, pixoffset, nextpixel, tripbytes, tripskip, thisrow);
      if(thisrow == 0){
//fprintf(stderr, "CLEARED ROW, TARGY: %d\n", targy - 1);
        if(--targy == 0){
          return 0;
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

// we can only write 4KiB at a time. we're writing base64-encoded RGBA. each
// pixel is 4B raw (32 bits). each chunk of three pixels is then 12 bytes, or
// 16 base64-encoded bytes. 4096 / 16 == 256 3-pixel groups, or 768 pixels.
static sprixcell_e*
write_kitty_data(FILE* fp, int rows, int cols, int linesize, int leny, int lenx,
                 const uint32_t* data, int sprixelid, int* parse_start){
  if(linesize % sizeof(*data)){
    return NULL;
  }
  sprixcell_e* tacache = malloc(sizeof(*tacache) * rows * cols);
  if(tacache == NULL){
    return NULL;
  }
  memset(tacache, 0, sizeof(*tacache) * rows * cols);
  (void)tacache; // FIXME populate tacache with 1s for cells with transparency
  int total = leny * lenx; // total number of pixels (4 * total == bytecount)
  // number of 4KiB chunks we'll need
  int chunks = (total + (RGBA_MAXLEN - 1)) / RGBA_MAXLEN;
  int totalout = 0; // total pixels of payload out
  int y = 0; // position within source image
  int x = 0;
  int targetout = 0; // number of pixels expected out after this chunk
//fprintf(stderr, "total: %d chunks = %d, s=%d,v=%d\n", total, chunks, lenx, leny);
  while(chunks--){
    if(totalout == 0){
      *parse_start = fprintf(fp, "\e_Gf=32,s=%d,v=%d,i=%d,a=T,%c=1;",
                             lenx, leny, sprixelid, chunks ? 'm' : 'q');
    }else{
      fprintf(fp, "\e_G%sm=%d;", chunks ? "" : "q=1,", chunks ? 1 : 0);
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
      for(int e = 0 ; e < encodeable ; ++e){
        if(x == lenx){
          x = 0;
          ++y;
        }
        const uint32_t* line = data + (linesize / sizeof(*data)) * y;
        source[e] = line[x];
//fprintf(stderr, "%u/%u/%u -> %c%c%c%c %u %u %u %u\n", r, g, b, b64[0], b64[1], b64[2], b64[3], b64[0], b64[1], b64[2], b64[3]);
        ++x;
      }
      totalout += encodeable;
      char out[17];
      base64_rgba3(source, encodeable, out);
      ncfputs(out, fp);
    }
    fprintf(fp, "\e\\");
  }
  if(fclose(fp) == EOF){
    free(tacache);
    return NULL;
  }
  return tacache;
}
#undef RGBA_MAXLEN

// Kitty graphics blitter. Kitty can take in up to 4KiB at a time of (optionally
// deflate-compressed) 24bit RGB. Returns -1 on error, 1 on success.
int kitty_blit(ncplane* nc, int linesize, const void* data,
               int leny, int lenx, const blitterargs* bargs){
  int rows = leny / bargs->u.pixel.celldimy + !!(leny % bargs->u.pixel.celldimy);
  int cols = lenx / bargs->u.pixel.celldimx + !!(lenx % bargs->u.pixel.celldimx);
  char* buf = NULL;
  size_t size = 0;
  FILE* fp = open_memstream(&buf, &size);
  if(fp == NULL){
    return -1;
  }
  int parse_start = 0;
  // closes fp on success
  sprixcell_e* tacache = write_kitty_data(fp, rows, cols, linesize, leny, lenx, data,
                                          bargs->u.pixel.sprixelid, &parse_start);
  if(tacache == NULL){
    fclose(fp);
    free(buf);
    return -1;
  }
  // takes ownership of |buf| on success
  if(plane_blit_sixel(nc, buf, size, bargs->placey, bargs->placex,
                      rows, cols, bargs->u.pixel.sprixelid, leny, lenx,
                      parse_start, tacache) < 0){
    free(tacache);
    free(buf);
    return -1;
  }
  return 1;
}

// clears all kitty bitmaps
int sprite_kitty_init(int fd){
  return tty_emit("\e_Ga=d\e\\", fd);
}

// removes the kitty bitmap graphic identified by s->id
int sprite_kitty_annihilate(const notcurses* nc, const ncpile* p, FILE* out, sprixel* s){
  (void)p;
  (void)nc;
  if(fprintf(out, "\e_Ga=d,d=i,i=%d\e\\", s->id) < 0){
    return 0;
  }
  return 0;
}
