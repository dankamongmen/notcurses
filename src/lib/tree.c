#include "internal.h"

// these are never allocated themselves, but always as arrays of object
typedef struct nctree_int_item {
  void* curry;
  ncplane* n;
  unsigned subcount;
  struct nctree_int_item* subs;
} nctree_int_item;

typedef struct nctree {
  ncplane* ncp;
  int (*cbfxn)(ncplane*, void*, int);
  nctree_int_item items;
  // FIXME need to track item we're on, probably via array of uints + sentinel?
  unsigned maxdepth;
  unsigned activerow;
  uint64_t bchannels;
} nctree;

// recursively free an array of nctree_int_item; nctree_int_item structs are
// never individually free()d, just their innards
static void
free_tree_items(nctree_int_item* iarray){
  for(unsigned c = 0 ; c < iarray->subcount ; ++c){
    free_tree_items(&iarray->subs[c]);
  }
  ncplane_destroy(iarray->n);
  free(iarray->subs);
}

// allocates a |count|-sized array of nctree_int_items, and fills |fill| in,
// using |items|. updates |*maxdepth| when appropriate.
static int
dup_tree_items(nctree_int_item* fill, const nctree_item* items, unsigned count, unsigned depth,
               unsigned* maxdepth){
  fill->subcount = count;
  fill->subs = malloc(sizeof(*fill->subs) * count);
  if(fill->subs == NULL){
    return -1;
  }
  for(unsigned c = 0 ; c < fill->subcount ; ++c){
    nctree_int_item* nii = &fill->subs[c];
    nii->curry = items[c].curry;
    nii->n = NULL;
    if(dup_tree_items(nii, items[c].subs, items[c].subcount, depth + 1, maxdepth)){
      while(c--){
        free_tree_items(&fill->subs[c]);
      }
      free(fill->subs);
      return -1;
    }
  }
  if(depth > *maxdepth){
    *maxdepth = depth;
  }
  return 0;
}

static nctree*
nctree_inner_create(ncplane* n, const struct nctree_options* opts){
  nctree* ret = malloc(sizeof(*ret));
  if(ret){
    ret->bchannels = opts->bchannels;
    ret->cbfxn = opts->nctreecb;
    ret->maxdepth = 0;
    if(dup_tree_items(&ret->items, opts->items, opts->count, 0, &ret->maxdepth)){
      free(ret);
      return NULL;
    }
    ret->items.n = NULL;
    ret->items.curry = NULL;
    ret->activerow = 0;
    ret->ncp = n;
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
    free_tree_items(&n->items);
    free(n);
  }
}

// Returns the ncplane on which this nctree lives.
ncplane* nctree_plane(nctree* n){
  return n->ncp;
}

int nctree_redraw(nctree* n){
  // FIXME
  return 0;
}

bool nctree_offer_input(nctree* n, const ncinput* ni){
  if(ni->id == NCKEY_UP){
    nctree_prev(n);
    return true;
  }else if(ni->id == NCKEY_DOWN){
    nctree_next(n);
    return true;
  }else if(ni->id == NCKEY_PGUP){
    nctree_prev(n); // more FIXME
    return true;
  }else if(ni->id == NCKEY_PGDOWN){
    nctree_next(n); // more FIXME
    return true;
  }else if(ni->id == NCKEY_HOME){
    nctree_prev(n); // more FIXME
    return true;
  }else if(ni->id == NCKEY_END){
    nctree_next(n); // more FIXME
    return true;
  }
  // FIXME implement left, right, +, -
  return false;
}

void* nctree_focused(nctree* n){
  // FIXME
}

void* nctree_next(nctree* n){
  // FIXME
}

void* nctree_prev(nctree* n){
  // FIXME
}

void* nctree_goto(nctree* n, const int* spec, size_t specdepth, int* failspec){
  // FIXME
}
