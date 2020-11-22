#include "internal.h"

static void
ncpile_debug(const ncpile* p, FILE* debugfp){
  const ncplane* n = p->top;
  const ncplane* prev = NULL;
  int planeidx = 0;
  while(n){
    fprintf(debugfp, "%04d off y: %3d x: %3d geom y: %3d x: %3d curs y: %3d x: %3d %p %.8s\n",
            planeidx, n->absy, n->absx, n->leny, n->lenx, n->y, n->x, n, n->name);
    if(n->boundto || n->bnext || n->bprev || n->blist){
      fprintf(debugfp, " bound %p → %p ← %p binds %p\n",
              n->boundto, n->bnext, n->bprev, n->blist);
    }
    if(n->bprev && (*n->bprev != n)){
      fprintf(stderr, " WARNING: expected *->bprev %p, got %p\n", n, *n->bprev);
    }
    if(n->above != prev){
      fprintf(stderr, " WARNING: expected ->above %p, got %p\n", prev, n->above);
    }
    prev = n;
    n = n->below;
    ++planeidx;
  }
  if(p->bottom != prev){
    fprintf(stderr, " WARNING: expected ->bottom %p, got %p\n", prev, p->bottom);
  }
}

void notcurses_debug(notcurses* nc, FILE* debugfp){
  const ncpile* p = ncplane_pile(nc->stdplane);
  fprintf(debugfp, " ************************** notcurses debug state *****************************\n");
  const ncpile* p0 = p;
  do{
    ncpile_debug(p0, debugfp);
    p0 = p0->next;
  }while(p != p0);
  fprintf(debugfp, " ******************************************************************************\n");
}
