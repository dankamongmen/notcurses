#include "notcurses.h"
#include "internal.h"

ncselector* ncselector_create(ncplane* n, int y, int x, const struct selector_options* opts){
  ncselector* ns = malloc(sizeof(*ns));
  ns->title = opts->title ? strdup(opts->title) : NULL;
  ns->secondary = opts->secondary ? strdup(opts->secondary) : NULL;
  ns->footer = opts->footer ? strdup(opts->footer) : NULL;
  if(!(ns->ncp = ncplane_new(n->nc, opts->ylen, opts->xlen, y, x, NULL))){
    free(ns->title); free(ns->secondary); free(ns->footer);
    free(n);
    return NULL;
  }
  ns->selected = 0;
  ns->startdisp = 0;
  ns->longop = 0;
  if(opts->itemcount){
    if(!(ns->items = malloc(sizeof(*ns->items) * opts->itemcount))){
      ncplane_destroy(ns->ncp);
      free(ns->title); free(ns->secondary); free(ns->footer);
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
      free(ns->title); free(ns->secondary); free(ns->footer);
      free(ns);
      return NULL;
    }
  }
  return ns;
}

ncselector* ncselector_aligned(ncplane* n, int y, ncalign_e align, const struct selector_options* opts);

int ncselector_additem(ncselector* n, const struct selector_item* item){
  size_t newsize = sizeof(*n->items) * (n->itemcount + 1);
  struct selector_item* items = realloc(n->items, newsize);
  if(!items){
    return -1;
  }
  n->items = items;
  n->items[n->itemcount].option = strdup(item->option);
  n->items[n->itemcount].desc = strdup(item->desc);
  ++n->itemcount;
  return 0;
}

int ncselector_delitem(ncselector* n, const char* item){
  for(unsigned idx = 0 ; idx < n->itemcount ; ++idx){
    if(strcmp(n->items[idx].option, item) == 0){ // found it
      free(n->items[idx].option);
      free(n->items[idx].desc);
      if(idx < n->itemcount - 1){
        memmove(n->items + idx, n->items + idx + 1, sizeof(*n->items) * (n->itemcount - idx - 1));
      }else{
        if(idx){
          --n->selected;
        }
      }
      --n->itemcount;
      // FIXME redraw
      return 0;
    }
  }
  return -1; // wasn't found
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
