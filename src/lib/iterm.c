// the iterm2 graphics protocol is based entirely around containerized formats
// https://iterm2.com/documentation-images.html

#include <stdio.h>
#include "termdesc.h"
#include "sprite.h"

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
int iterm_draw(const struct ncpile *p, sprixel* s, FILE* out, int y, int x){
  return 0;
}

// damage any cells underneath the graphic, destroying it.
int iterm_scrub(const struct ncpile* p, sprixel* s){
  return 0;
}

// create an iterm2 control sequence complete with base64-encoded PNG.
int iterm_blit(struct ncplane* nc, int linesize, const void* data,
               int leny, int lenx, const struct blitterargs* bargs){
  return 0;
}
