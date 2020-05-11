#include "internal.h"

void notcurses_debug(notcurses* nc, FILE* debugfp){
  const ncplane* n = nc->top;
  int planeidx = 0;
  fprintf(debugfp, "********************* notcurses debug state *********************\n");
  while(n){
    fprintf(stderr, "%02d] abs: %d/%d len: %d/%d cursor: %d/%d %p\n", d,
            p->absy, p->absx, p->leny, p->lenx, p->y, p->x, p);
    n = n->z;
  }
  fprintf(debugfp, "*****************************************************************\n");
}
