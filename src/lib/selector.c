#include "notcurses.h"

ncselector* ncselector_create(ncplane* n, int y, int x, const struct selector_options* opts){
  ncselector* n = malloc(sizeof(*n));
  // FIXME do crap
  return n;
}

void ncselector_destroy(ncselector* n, char** item){
  if(n){
    if(*item){
      *item = n->items[n->selected].option;
      n->items[n->selected].option = NULL;
    }
    while(n->itemcount--){
      free(n->items[n->itemcount].option);
      free(n->items[n->itemcount].desc);
    }
    free(n->items);
    free(n->title);
    free(n->secondary);
    free(n->footer);
    free(n);
  }
}
