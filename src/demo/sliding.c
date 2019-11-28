#include <stdlib.h>
#include <string.h>
#include <notcurses.h>
#include "demo.h"

// FIXME do the bigger dimension on the screen's bigger dimension
#define CHUNKS_VERT 8
#define CHUNKS_HORZ 16

// break whatever's on the screen into panels and shift them around like a
// sliding puzzle. FIXME once we have copying, anyway. until then, just use
// background colors.
int sliding_puzzle_demo(struct notcurses* nc){
  int ret = -1, z;
  int maxx, maxy;
  int chunky, chunkx;
  notcurses_term_dimyx(nc, &maxy, &maxx);
  if(maxy < CHUNKS_VERT || maxx < CHUNKS_HORZ){
    fprintf(stderr, "Terminal too small, need at least %dx%d\n",
            CHUNKS_HORZ, CHUNKS_VERT);
    return -1;
  } 
  // we want an 8x8 grid of chunks. the leftover space will be unused
  chunky = maxy / CHUNKS_VERT;
  chunkx = maxx / CHUNKS_HORZ;
  int chunkcount = CHUNKS_VERT * CHUNKS_HORZ;
  struct ncplane** chunks = malloc(sizeof(*chunks) * chunkcount);
  if(chunks == NULL){
    return -1;
  }
  memset(chunks, 0, sizeof(*chunks) * chunkcount);
  int cy, cx;
  for(cy = 0 ; cy < CHUNKS_VERT ; ++cy){
    for(cx = 0 ; cx < CHUNKS_HORZ ; ++cx){
      const int idx = cy * CHUNKS_HORZ + cx;
      chunks[idx] =
        notcurses_newplane(nc, chunky, chunkx, cy * chunky, cx * chunkx, NULL);
      if(chunks[idx] == NULL){
        goto done;
      }
    }
  }
  ret = 0;

done:
  for(z = 0 ; z < chunkcount ; ++z){
    ncplane_destroy(nc, chunks[z]);
  }
  free(chunks);
  return ret;
}
