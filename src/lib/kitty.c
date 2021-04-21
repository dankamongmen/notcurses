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
static inline void
base64_rgba3(const uint32_t* pixels, size_t pcount, char* b64, bool wipe[static 3],
             uint32_t transcolor){
  uint32_t pixel = *pixels++;
  unsigned r = ncpixel_r(pixel);
  unsigned g = ncpixel_g(pixel);
  unsigned b = ncpixel_b(pixel);
  // we go ahead and take advantage of kitty's ability to reproduce 8-bit
  // alphas by copying it in directly, rather than mapping to {0, 255}.
  unsigned a = ncpixel_a(pixel);
  if(wipe[0] || rgba_trans_p(pixel, transcolor)){
    a = 0;
  }
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
  a = wipe[1] ? 0 : rgba_trans_p(pixel, transcolor) ? 0 : 255;
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
  a = wipe[2] ? 0 : rgba_trans_p(pixel, transcolor) ? 0 : 255;
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
int kitty_wipe(const notcurses* nc, sprixel* s, int ycell, int xcell){
  if(s->n->tacache[s->dimx * ycell + xcell] == SPRIXCELL_ANNIHILATED){
//fprintf(stderr, "CACHED WIPE %d %d/%d\n", s->id, ycell, xcell);
    return 0; // already annihilated, needn't draw glyph in kitty
  }
//fprintf(stderr, "NEW WIPE %d %d/%d\n", s->id, ycell, xcell);
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
          //s->invalidated = SPRIXEL_INVALIDATED;
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
// closes |fp| on all paths.
static int
write_kitty_data(FILE* fp, int linesize, int leny, int lenx,
                 int cols, const uint32_t* data, int cdimy, int cdimx,
                 int sprixelid, sprixcell_e* tacache, int* parse_start,
                 uint32_t transcolor){
  if(linesize % sizeof(*data)){
    fclose(fp);
    return -1;
  }
  int total = leny * lenx; // total number of pixels (4 * total == bytecount)
  // number of 4KiB chunks we'll need
  int chunks = (total + (RGBA_MAXLEN - 1)) / RGBA_MAXLEN;
  int totalout = 0; // total pixels of payload out
  int y = 0; // position within source image (pixels)
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
      bool wipe[3];
      for(int e = 0 ; e < encodeable ; ++e){
        if(x == lenx){
          x = 0;
          ++y;
        }
        const uint32_t* line = data + (linesize / sizeof(*data)) * y;
        source[e] = line[x];
//fprintf(stderr, "%u/%u/%u -> %c%c%c%c %u %u %u %u\n", r, g, b, b64[0], b64[1], b64[2], b64[3], b64[0], b64[1], b64[2], b64[3]);
        int xcell = x / cdimx;
        int ycell = y / cdimy;
        int tyx = xcell + ycell * cols;
//fprintf(stderr, "Tyx: %d y: %d (%d) * %d x: %d (%d)\n", tyx, y, y / cdimy, cols, x, x / cdimx);
        if(tacache[tyx] == SPRIXCELL_ANNIHILATED){
          wipe[e] = 1;
        }else{
          wipe[e] = 0;
          if(rgba_trans_p(source[e], transcolor)){
            if(x % cdimx == 0 && y % cdimy == 0){
              tacache[tyx] = SPRIXCELL_TRANSPARENT;
            }else if(tacache[tyx] == SPRIXCELL_OPAQUE){
              tacache[tyx] = SPRIXCELL_MIXED;
            }
          }else{
            if(x % cdimx == 0 && y % cdimy == 0){
              tacache[tyx] = SPRIXCELL_OPAQUE;
            }else if(tacache[tyx] == SPRIXCELL_TRANSPARENT){
              tacache[tyx] = SPRIXCELL_MIXED;
            }
          }
        }
        ++x;
      }
      totalout += encodeable;
      char out[17];
      base64_rgba3(source, encodeable, out, wipe, transcolor);
      ncfputs(out, fp);
    }
    fprintf(fp, "\e\\");
  }
  if(fclose(fp) == EOF){
    return -1;
  }
  scrub_tam_boundaries(tacache, leny, lenx, cdimy, cdimx);
  return 0;
}
#undef RGBA_MAXLEN

// Kitty graphics blitter. Kitty can take in up to 4KiB at a time of (optionally
// deflate-compressed) 24bit RGB. Returns -1 on error, 1 on success.
int kitty_blit(ncplane* n, int linesize, const void* data,
               int leny, int lenx, const blitterargs* bargs){
  int cols = bargs->u.pixel.spx->dimx;
  int rows = bargs->u.pixel.spx->dimy;
  char* buf = NULL;
  size_t size = 0;
  FILE* fp = open_memstream(&buf, &size);
  if(fp == NULL){
    return -1;
  }
  sprixcell_e* tacache = NULL;
  bool reuse = false;
  // if we have a sprixel attached to this plane, see if we can reuse it
  // (we need the same dimensions) and thus immediately apply its T-A table.
  if(n->tacache){
    if(n->tacachey == rows && n->tacachex == cols){
      tacache = n->tacache;
      reuse = true;
    }
  }
  int parse_start = 0;
  if(!reuse){
    tacache = malloc(sizeof(*tacache) * rows * cols);
    if(tacache == NULL){
      fclose(fp);
      free(buf);
      return -1;
    }
    memset(tacache, 0, sizeof(*tacache) * rows * cols);
  }
  // closes fp on all paths
  if(write_kitty_data(fp, linesize, leny, lenx, cols, data,
                      bargs->u.pixel.celldimy, bargs->u.pixel.celldimx,
                      bargs->u.pixel.spx->id, tacache, &parse_start,
                      bargs->transcolor)){
    if(!reuse){
      free(tacache);
    }
    free(buf);
    return -1;
  }
  // take ownership of |buf| and |tacache| on success
  if(plane_blit_sixel(bargs->u.pixel.spx, buf, size, rows, cols,
                      bargs->placey, bargs->placex,
                      leny, lenx, parse_start, tacache) < 0){
    if(!reuse){
      free(tacache);
    }
    free(buf);
    return -1;
  }
  return 1;
}

// removes the kitty bitmap graphic identified by s->id, and damages those
// cells which weren't SPRIXCEL_OPAQUE
int kitty_delete(const notcurses* nc, const ncpile* p, FILE* out, sprixel* s){
  (void)p;
  (void)nc;
  if(fprintf(out, "\e_Ga=d,d=i,i=%d\e\\", s->id) < 0){
    return -1;
  }
//fprintf(stderr, "MOVED FROM: %d/%d\n", s->movedfromy, s->movedfromx);
  for(int yy = s->movedfromy ; yy < s->movedfromy + s->dimy && yy < p->dimy ; ++yy){
    for(int xx = s->movedfromx ; xx < s->movedfromx + s->dimx && xx < p->dimx ; ++xx){
      struct crender *r = &p->crender[yy * p->dimx + xx];
      if(s->n){
        const ncplane* stdn = notcurses_stdplane_const(nc);
//fprintf(stderr, "CHECKING %d/%d\n", yy - s->movedfromy, xx - s->movedfromx);
        sprixcell_e state = sprixel_state(s, yy - s->movedfromy + s->n->absy - stdn->absy,
                                             xx - s->movedfromx + s->n->absx - stdn->absx);
        if(state == SPRIXCELL_OPAQUE){
          r->s.damaged = 1;
        }else if(s->invalidated == SPRIXEL_MOVED){
          // ideally, we wouldn't damage our annihilated sprixcells, but if
          // we're being annihilated only during this cycle, we need to go
          // ahead and damage it.
          r->s.damaged = 1;
        }
      }else{
        r->s.damaged = 1;
      }
    }
  }
  return 0;
}

int kitty_draw(const notcurses* nc, const ncpile* p, sprixel* s, FILE* out){
  (void)nc;
  (void)p;
  (void)out;
  int ret = 0;
  if(fwrite(s->glyph, s->glyphlen, 1, out) != 1){
    ret = -1;
  }
  s->invalidated = SPRIXEL_QUIESCENT;
  return ret;
}

// clears all kitty bitmaps
int kitty_init(int fd){
  return tty_emit("\e_Ga=d\e\\", fd);
}

int kitty_shutdown(int fd){
  // FIXME need to close off any open kitty bitmap emission, or we will
  // lock up the terminal
  (void)fd;
  return 0;
}
