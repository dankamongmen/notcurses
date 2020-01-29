#include "notcurses.h"
#include "internal.h"

ncselector* ncselector_create(ncplane* n, int y, int x, const struct selector_options* opts){
  ncselector* ns = malloc(sizeof(*ns));
  if(!(ns->ncp = ncplane_new(n->nc, opts->ylen, opts->xlen, y, x, NULL))){
    free(n);
    return NULL;
  }
  ns->selected = 0;
  ns->startdisp = 0;
  ns->longop = 0;
  if(opts->itemcount){
    if(!(ns->items = malloc(sizeof(*ns->items) * opts->itemcount))){
      ncplane_destroy(ns->ncp);
      free(n);
      return NULL;
    }
  }else{
    ns->items = NULL;
  }
  for(ns->itemcount = 0 ; ns->itemcount < opts->itemcount ; ++ns->itemcount){
    const struct selector_item* src = &opts->items[ns->itemcount];
    if(strlen(src->option) > ns->longop){
      ns->longop = strlen(src->option);
    }
    ns->items[ns->itemcount].option = strdup(src->option);
    ns->items[ns->itemcount].desc = strdup(src->desc);
    if(!(ns->items[ns->itemcount].desc && ns->items[ns->itemcount].option)){
      do{
        free(ns->items[ns->itemcount].option);
        free(ns->items[ns->itemcount].desc);
      }while(ns->itemcount-- >= 1);
      free(ns->items);
      ncplane_destroy(ns->ncp);
      return NULL;
    }
  }
  ns->title = opts->title ? strdup(opts->title) : NULL;
  ns->secondary = opts->secondary ? strdup(opts->secondary) : NULL;
  ns->footer = opts->footer ? strdup(opts->footer) : NULL;
  return ns;
}

void ncselector_destroy(ncselector* n, char** item){
  if(n){
    if(item){
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
