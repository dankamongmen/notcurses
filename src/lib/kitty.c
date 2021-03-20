#include "internal.h"

#define RGBA_MAXLEN 768 // 768 base64-encoded pixels in 4096 bytes
int sprite_kitty_cell_wipe(notcurses* nc, sprixel* s, int ycell, int xcell){
  if(ycell >= s->dimy){
    return -1;
  }
  if(xcell >= s->dimx){
    return -1;
  }
  int xpixels = nc->tcache.cellpixx;
  int ypixels = nc->tcache.cellpixy;
  int xpx = xpixels * xcell; // pixel coordinates where we start erasing
  int ypx = ypixels * ycell;
  char* c = s->glyph;
  // every pixel was 4 source bytes, 32 bits, 6.33 base64 bytes. every 3 input pixels is
  // 12 bytes (96 bits), an even 16 base64 bytes. there is chunking to worry about. there
  // are up to 768 pixels in a chunk.
  int chunks = (xcell + s->dimx * ycell) / RGBA_MAXLEN;
  do{
    while(*c != ';'){
      ++c;
    }
    ++c;
    if(chunks == 0){
      // we're in the proper chunk. find the pixel offset of the first
      // pixel (within the chunk).
      int offset = (xpx + s->dimx * ypx) % RGBA_MAXLEN;
      // skip the 16-byte pixel triples
      int bytes = (offset / 3) * 16;
      // FIXME
      return 0;
    }
  }while(--chunks);
  return -1;
}

static unsigned const char b64subs[] =
 "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

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

// we can only write 4KiB at a time. we're writing base64-encoded RGBA. each
// pixel is 4B raw (32 bits). each chunk of three pixels is then 12 bytes, or
// 16 base64-encoded bytes. 4096 / 16 == 256 3-pixel groups, or 768 pixels.
static int
write_kitty_data(FILE* fp, int linesize, int leny, int lenx,
                 const uint32_t* data, int sprixelid){
  if(linesize % sizeof(*data)){
    return -1;
  }
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
      fprintf(fp, "\e_Gf=32,s=%d,v=%d,i=%d,a=T,%c=1;", lenx, leny, sprixelid, chunks ? 'm' : 'q');
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
      fputs(out, fp);
    }
    fprintf(fp, "\e\\");
  }
  if(fclose(fp) == EOF){
    return -1;
  }
  return 0;
#undef RGBA_MAXLEN
}

// Kitty graphics blitter. Kitty can take in up to 4KiB at a time of (optionally
// deflate-compressed) 24bit RGB.
int kitty_blit_inner(ncplane* nc, int linesize, int leny, int lenx,
                     const void* data, const blitterargs* bargs){
  int rows = leny / bargs->pixel.celldimy + !!(leny % bargs->pixel.celldimy);
  int cols = lenx / bargs->pixel.celldimx + !!(lenx % bargs->pixel.celldimx);
  char* buf = NULL;
  size_t size = 0;
  FILE* fp = open_memstream(&buf, &size);
  if(fp == NULL){
    return -1;
  }
  if(write_kitty_data(fp, linesize, leny, lenx, data, bargs->pixel.sprixelid)){
    fclose(fp);
    free(buf);
    return -1;
  }
  if(plane_blit_sixel(nc, buf, size, rows, cols, bargs->pixel.sprixelid) < 0){
    free(buf);
    return -1;
  }
  free(buf);
  return 1;
}

int kitty_blit(ncplane* nc, int linesize, const void* data, int begy, int begx,
               int leny, int lenx, const blitterargs* bargs){
  (void)begy;
  (void)begx;
  int r = kitty_blit_inner(nc, linesize, leny, lenx, data, bargs);
  if(r < 0){
    return -1;
  }
  return r;
}
