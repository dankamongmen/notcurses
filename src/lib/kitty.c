#include "internal.h"

static unsigned const char b64subs[] =
 "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// we can only write 4KiB at a time
static int
write_kitty_data(FILE* fp, int linesize, int leny, int lenx, const uint32_t* data){
#define KITTY_MAXLEN 4096 // 4096B maximum payload
  if(linesize % sizeof(*data)){
    return -1;
  }
  int total = leny * lenx * 4; // 3B RGB -> 4B Base64, total bytes
  int chunks = (total + (KITTY_MAXLEN - 1)) / KITTY_MAXLEN;
  int totalout = 0; // total bytes of payload out
  int y = 0;
  int x = 0;
  int targetout = 0;
//fprintf(stderr, "total: %d chunks = %d, s=%d,v=%d\n", total, chunks, lenx, leny);
  while(chunks--){
    if(totalout == 0){
      fprintf(fp, "\e_Gf=24,s=%d,v=%d,a=T%s;", lenx, leny, chunks > 1 ? ",m=1" : "");
    }else{
      fprintf(fp, "\e_Gm=%d;", chunks ? 1 : 0);
    }
    if((targetout += KITTY_MAXLEN) > total){
      targetout = total;
    }
    while(totalout < targetout){
      if(x == lenx){
        x = 0;
        ++y;
      }
      const uint32_t* line = data + (linesize / sizeof(*data)) * y;
      uint32_t pixel = line[x];
      unsigned r = ncpixel_r(pixel);
      unsigned g = ncpixel_g(pixel);
      unsigned b = ncpixel_b(pixel);
      unsigned char b64[4] = {
        b64subs[((r & 0xfc) >> 2)],
        b64subs[((r & 0x3 << 4) | ((g & 0xf0) >> 4))],
        b64subs[(((g & 0xf) << 2) | ((b & 0xc0) >> 6))],
        b64subs[(b & 0x3f)]
      };
//fprintf(stderr, "%u/%u/%u -> %c%c%c%c %u %u %u %u\n", r, g, b, b64[0], b64[1], b64[2], b64[3], b64[0], b64[1], b64[2], b64[3]);
      fprintf(fp, "%c%c%c%c", b64[0], b64[1], b64[2], b64[3]);
      totalout += 4;
      ++x;
    }
    fprintf(fp, "\e\\");
  }
  if(fclose(fp) == EOF){
    return -1;
  }
  return 0;
}

// Kitty graphics blitter. Kitty can take in up to 4KiB at a time of (optionally
// deflate-compressed) 24bit RGB.
int kitty_blit_inner(ncplane* nc, int placey, int placex, int linesize,
                     int leny, int lenx, unsigned cellpixx, const void* data){
  unsigned width = lenx / cellpixx + !!(lenx % cellpixx);
  char* buf = NULL;
  size_t size = 0;
  FILE* fp = open_memstream(&buf, &size);
  if(fp == NULL){
    return -1;
  }
  if(write_kitty_data(fp, linesize, leny, lenx, data)){
    fclose(fp);
    free(buf);
    return -1;
  }
  nccell* c = ncplane_cell_ref_yx(nc, placey, placex);
  if(pool_blit_direct(&nc->pool, c, buf, size, width) < 0){
    free(buf);
    return -1;
  }
  free(buf);
  return 1;
}


int kitty_blit(ncplane* nc, int placey, int placex, int linesize,
               const void* data, int begy, int begx,
               int leny, int lenx, unsigned cellpixx){
  (void)begy;
  (void)begx;
//fprintf(stderr, "s=%d,v=%d\n", lenx, leny);
  int r = kitty_blit_inner(nc, placey, placex, linesize, leny, lenx, cellpixx, data);
  if(r < 0){
    return -1;
  }
  return r;
}
