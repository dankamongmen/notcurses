// the iterm2 graphics protocol is based entirely around containerized formats
// https://iterm2.com/documentation-images.html

#include <stdio.h>
#include "internal.h"
#include "termdesc.h"
#include "sprite.h"
#include "png.h"

// yank a cell out of the PNG by setting all of its alphas to 0. the alphas
// will be preserved in the auxvec.
int iterm_wipe(sprixel* s, int ycell, int xcell){
  return 0;
}

// build a cell of the PNG back up by copying auxvec alphas to it.
int iterm_rebuild(sprixel* s, int ycell, int xcell, uint8_t* auxvec){
  return 0;
}

// spit out the control sequence and data.
int iterm_draw(const ncpile *p, sprixel* s, FILE* out, int y, int x){
  return 0;
}

// damage any cells underneath the graphic, destroying it.
int iterm_scrub(const ncpile* p, sprixel* s){
  return 0;
}

static int
write_iterm_graphic(sprixel* s, const void* data, int leny, int stride,
                    int lenx, int* parse_start){
  s->glyph = NULL;
  FILE* fp = open_memstream(&s->glyph, &s->glyphlen);
  if(fp == NULL){
    return -1;
  }
  if(ncfputs("\e]1337;inline=1:", fp) == EOF){
    goto err;
  }
  // FIXME won't we need to pass TAM into create_png_mmap()?
  size_t bsize;
  // FIXME we'll want a create_png_mmap() that takes a FILE*
  void* png = create_png_mmap(data, leny, stride, lenx, &bsize, -1);
  if(png == MAP_FAILED){
    goto err;
  }
  size_t encoded = 0;
  while(bsize){
    size_t plen = bsize > 4096 ? 4096 : bsize;
    // FIXME base64-encode
    if(fwrite((const char*)png + encoded, plen, 1, fp) != 1){
      munmap(png, bsize);
      goto err;
    }
    bsize -= plen;
    encoded += plen;
  }
  if(munmap(png, bsize)){
    goto err;
  }
  if(fclose(fp) == EOF){
    free(s->glyph);
    return -1;
  }
  return 0;

err:
  fclose(fp);
  free(s->glyph);
  s->glyph = NULL;
  return -1;
}

// create an iterm2 control sequence complete with base64-encoded PNG.
int iterm_blit(ncplane* n, int linesize, const void* data,
               int leny, int lenx, const blitterargs* bargs){
  int cols = bargs->u.pixel.spx->dimx;
  int rows = bargs->u.pixel.spx->dimy;
  sprixel* s = bargs->u.pixel.spx;
  tament* tam = NULL;
  bool reuse = false;
  void* png = NULL;
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
  if(write_iterm_graphic(s, data, leny, linesize, lenx, &parse_start)){
    goto error;
  }
  // FIXME set up glyph/glyphlen
  if(plane_blit_sixel(s, s->glyph, s->glyphlen, leny, lenx, parse_start, tam) < 0){
    goto error;
  }
  return 0;

error:
  if(!reuse){
    free(tam);
  }
  free(png);
  free(s->glyph);
  return -1;
}
