#include "internal.h"

// we can only write 4KiB at a time
static int
write_kitty_data(FILE* fp, int linesize, int leny, int lenx, const uint32_t* data){
  if(linesize % sizeof(*data)){
    return -1;
  }
  // FIXME must write m=1 for initial chunks, m=0 for final (assuming > 1)
  fprintf(fp, "\e_Gf=24,s=%d,v=%d;", lenx, leny);
  // FIXME need to base64 encode payload. each 3B RGB goes to a 4B base64
  for(int y = 0 ; y < leny ; ++y){
    const uint32_t* line = data + (linesize / sizeof(*data)) * y;
    for(int x = 0 ; x < lenx ; ++x){
      uint32_t pixel = line[x];
      unsigned r = ncpixel_r(pixel);
      unsigned g = ncpixel_g(pixel);
      unsigned b = ncpixel_b(pixel);
      unsigned char b64[4] = {
        ((r & 0xfc) >> 2) + 'A',
        ((r & 0x3 << 2) | ((g & 0xf0) >> 4)) + 'A',
        (((g & 0xf) << 2) | ((b & 0xc0) >> 6)) + 'A',
        (b & 0x3f) + 'A'
      };
// this isn't the correct base64 distribution FIXME
fprintf(stderr, "%u/%u/%u -> %c%c%c%c %u %u %u %u\n", r, g, b, b64[0], b64[1], b64[2], b64[3], b64[0], b64[1], b64[2], b64[3]);
      fprintf(fp, "%c%c%c%c", b64[0], b64[1], b64[2], b64[3]);
    }
  }
  fprintf(fp, "\e\\");
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
  int r = kitty_blit_inner(nc, placey, placex, linesize, leny, lenx, cellpixx, data);
  if(r < 0){
    return -1;
  }
  return r;
}
