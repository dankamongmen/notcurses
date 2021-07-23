// the iterm2 graphics protocol is based entirely around containerized formats
// https://iterm2.com/documentation-images.html

#include <stdio.h>
#include "internal.h"
#include "termdesc.h"
#include "sprite.h"
#include "fbuf.h"
#include "png.h"

// yank a cell out of the PNG by setting all of its alphas to 0. the alphas
// will be preserved in the auxvec.
int iterm_wipe(sprixel* s, int ycell, int xcell){
  (void)s;     // FIXME
  (void)ycell;
  (void)xcell;
  return -1;
}

// build a cell of the PNG back up by copying auxvec alphas to it.
int iterm_rebuild(sprixel* s, int ycell, int xcell, uint8_t* auxvec){
  (void)s;      // FIXME
  (void)ycell;
  (void)xcell;
  (void)auxvec;
  return -1;
}

// spit out the control sequence and data.
int iterm_draw(const tinfo* ti, const ncpile *p, sprixel* s, FILE* out, int y, int x){
  (void)ti;
  if(p){
    if(goto_location(p->nc, out, y, x)){
      return -1;
    }
  }
  if(fwrite(s->glyph.buf, s->glyph.used, 1, out) != 1){
    return -1;
  }
  return s->glyph.used;
}

static int
write_iterm_graphic(const void* data, int leny, int stride, int lenx, fbuf *f){
  if(fbuf_puts(f, "\e]1337;File=inline=1:") < 0){
    return -1;
  }
  // FIXME won't we need to pass TAM into write_png_b64()?
  if(write_png_b64(data, leny, stride, lenx, f)){
    return -1;
  }
  if(fbuf_puts(f, "\x1b\\") < 0){
    return -1;
  }
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
      return -1;
    }
    memset(tam, 0, sizeof(*tam) * rows * cols);
  }
  if(fbuf_init(&s->glyph)){
    free(tam);
    return -1;
  }
  if(write_iterm_graphic(data, leny, linesize, lenx, &s->glyph)){
    if(!reuse){
      free(tam);
    }
    fbuf_free(&s->glyph);
    return -1;
  }
  scrub_tam_boundaries(tam, leny, lenx, s->cellpxy, s->cellpxx);
  if(plane_blit_sixel(s, &s->glyph, leny, lenx, parse_start, tam) < 0){
    if(!reuse){
      free(tam);
    }
    fbuf_free(&s->glyph);
    return -1;
  }
  return 1;

error:
  if(!reuse){
    free(tam);
  }
  free(s->glyph);
  return -1;
}
