#include "sixel/sixel.h"
#include <notcurses/notcurses.h>

// represents a sixel generated from some image
typedef struct sixel {
  char* escape; // nul-terminated escape suitable for writing to the terminal
  // there is both the true pixel geometry, and the sprixel-padded pixel
  // geometry--the latter always has a height which is a multiple of six.
  unsigned pixy, pixx;           // original pixel geometry
  unsigned sprixpixy, sprixpixx; // sprixel-padded pixel geometry
  // we might only occupy a portion of the final column and row of cells.
  unsigned celly, cellx;         // cell geometry
  unsigned colorregs_avail;      // color registers available
  unsigned colorregs_used;       // color registers used
} sixel;

typedef struct sixelctx {
  struct notcurses* nc;
} sixelctx;

sixelctx* libncsixel_init(void){
  sixelctx* sctx = malloc(sizeof(*sctx));
  if(sctx == NULL){
    return NULL;
  }
  struct notcurses_options nopts = {
    .flags = NCOPTION_NO_ALTERNATE_SCREEN |
             NCOPTION_PRESERVE_CURSOR |
             NCOPTION_SUPPRESS_BANNERS |
             NCOPTION_DRAIN_INPUT,
  };
  if((sctx->nc = notcurses_init(&nopts, NULL)) == NULL){
    free(sctx);
    return NULL;
  }
  return sctx;
}

sixel* libncsixel_encode(sixelctx* sctx, const char* file, unsigned colorregs){
  (void)sctx;
  (void)file;
  (void)colorregs;
  return NULL;
}

void libncsixel_stop(sixelctx* sctx){
  if(sctx){
    notcurses_stop(sctx->nc);
    free(sctx);
  }
}
