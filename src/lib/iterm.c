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
  size_t bsize;
  png = create_png_mmap(data, leny, linesize, lenx, &bsize, -1);
  if(png == NULL){
    goto error;
  }
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
