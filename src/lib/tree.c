#include "internal.h"

typedef struct nctree_int_item {
  void* curry;
  ncplane* n;
  unsigned subcount;
  struct nctree_int_item* subs;
} nctree_int_item;

typedef struct nctree {
  int (*cbfxn)(ncplane*, void*, int);
  nctree_int_item* items;
  unsigned itemcount;
  uint64_t bchannels;
} nctree;

static void
free_tree_items(nctree_int_item* iarray){
  for(unsigned c = 0 ; c < iarray->subcount ; ++c){
    free_tree_items(&iarray->subs[c]);
  }
  free(iarray->subs);
}

static nctree_int_item*
dup_tree_items(const nctree_item* items, unsigned count){
  nctree_int_item* ret = malloc(sizeof(*ret) * count);
  if(ret){
    for(unsigned c = 0 ; c < count ; ++c){
      nctree_int_item* nii = &ret[c];
      nii->curry = items[c].curry;
      nii->n = NULL;
      nii->subcount = items[c].subcount;
      if((nii->subs = dup_tree_items(items[c].subs, nii->subcount)) == NULL){
        while(c--){
          free_tree_items(&ret[c]);
        }
        free(ret);
        return NULL;
      }
    }
  }
  return ret;
}

static nctree*
nctree_inner_create(ncplane* n, const struct nctree_options* opts){
  nctree* ret = malloc(sizeof(*ret));
  if(ret){
    ret->bchannels = opts->bchannels;
    ret->cbfxn = opts->nctreecb;
    ret->itemcount = opts->count;
    ret->items = dup_tree_items(opts->items, ret->itemcount);
    if(ret->items == NULL){
      logerror(ncplane_notcurses(n), "Couldn't duplicate tree items\n");
      free(ret);
      return NULL;
    }
  }
  return ret;
}

nctree* nctree_create(ncplane* n, const struct nctree_options* opts){
  notcurses* nc = ncplane_notcurses(n);
  if(opts->flags){
    logwarn(nc, "Passed invalid flags 0x%016jx\n", (uint64_t)opts->flags);
    return NULL;
  }
  if(opts->count == 0 || opts->items == NULL){
    logerror(nc, "Can't create empty tree\n");
    return NULL;
  }
  if(opts->nctreecb == NULL){
    logerror(nc, "Can't use NULL callback\n");
    return NULL;
  }
  return nctree_inner_create(n, opts);
}

void nctree_destroy(nctree* n){
  if(n){
    for(unsigned c = 0 ; c < n->itemcount ; ++c){
      free_tree_items(&n->items[c]);
    }
    free(n->items);
    free(n);
  }
}
