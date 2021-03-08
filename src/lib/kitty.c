#include "internal.h"

// we can only write 4KiB at a time
static int
write_kitty_data(FILE* fp, int linesize, int leny, int lenx, const uint32_t* data){
  if(linesize % sizeof(*data)){
    return -1;
  }
  fprintf(fp, "\e_Gf=24,s=%d,v=%d;", lenx, leny);
  for(int y = 0 ; y < leny ; ++y){
    const uint32_t* line = data + linesize / sizeof(*data);
    for(int x = 0 ; x < lenx ; ++x){
      uint32_t pixel = line[x];
      fprintf(fp, "%u%u%u", ncpixel_r(pixel), ncpixel_g(pixel), ncpixel_b(pixel));
    }
  }
  fprintf(fp, "\e\\");
  // FIXME need to base64 encode this
  if(fclose(fp) == EOF){
    return -1;
  }
  return 0;
}

// Kitty graphics blitter. Kitty can take in up to 4KiB at a time of (optionally
// deflate-compressed) 24bit RGB.
int kitty_blit_inner(ncplane* nc, int placey, int placex, int linesize,
                     int leny, int lenx, unsigned cellpixx, const void* data){
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
  unsigned width = lenx / cellpixx + !!(lenx % cellpixx);
fprintf(stderr, "SIZE: %zu WIDTH: %u\n", size, width);
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
