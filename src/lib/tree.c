#include "internal.h"

// these are never allocated themselves, but always as arrays of object
typedef struct nctree_int_item {
  void* curry;
  ncplane* ncp;
  unsigned subcount;
  struct nctree_int_item* subs;
} nctree_int_item;

typedef struct nctree {
  int (*cbfxn)(ncplane*, void*, int);
  nctree_int_item items; // topmost set of items, holds widget plane
  unsigned* currentpath; // array of |maxdepth|+1 elements, ended by UINT_MAX
  unsigned maxdepth;     // binds the path length
  int activerow;         // active row 0 <= activerow < dimy
  uint64_t bchannels;    // border glyph channels
} nctree;

// recursively free an array of nctree_int_item; nctree_int_item structs are
// never individually free()d, just their innards
static void
free_tree_items(nctree_int_item* iarray){
  for(unsigned c = 0 ; c < iarray->subcount ; ++c){
    free_tree_items(&iarray->subs[c]);
  }
  ncplane_destroy(iarray->ncp);
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
    if(nii->curry == NULL){
      while(c--){
        free_tree_items(&fill->subs[c]);
      }
      free(fill->subs);
      return -1;
    }
    nii->ncp = NULL;
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

// the initial path ought point to the first item. maxdepth must be set.
static int
prep_initial_path(nctree* n){
  n->currentpath = malloc(sizeof(*n->currentpath) * (n->maxdepth + 1));
  if(n->currentpath == NULL){
    return -1;
  }
  const nctree_int_item* nii = &n->items;
  int c = 0;
  while(nii->subcount){
    n->currentpath[c++] = 0;
    nii = &nii->subs[0];
  }
  n->currentpath[c] = UINT_MAX;
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
//fprintf(stderr, "MAXDEPTH: %u\n", ret->maxdepth);
    if(prep_initial_path(ret)){
      free_tree_items(&ret->items);
      free(ret);
      return NULL;
    }
    ret->items.ncp = n;
    ret->items.curry = NULL;
    ret->activerow = 0;
    nctree_redraw(ret);
  }
  return ret;
}

nctree* nctree_create(ncplane* n, const struct nctree_options* opts){
  notcurses* nc = ncplane_notcurses(n);
  if(opts->flags){
    logwarn(nc, "Passed invalid flags 0x%016jx\n", (uint64_t)opts->flags);
    goto error;
  }
  if(opts->count == 0 || opts->items == NULL){
    logerror(nc, "Can't create empty tree\n");
    goto error;
  }
  if(opts->nctreecb == NULL){
    logerror(nc, "Can't use NULL callback\n");
    goto error;
  }
  nctree* ret = nctree_inner_create(n, opts);
  if(ret == NULL){
    goto error;
  }
  return ret;

error:
  ncplane_destroy(n);
  return NULL;
}

void nctree_destroy(nctree* n){
  if(n){
    free_tree_items(&n->items);
    free(n);
  }
}

// Returns the ncplane on which this nctree lives.
ncplane* nctree_plane(nctree* n){
  return n->items.ncp;
}

// first we go out to the path's end, marked by UINT_MAX.
// there, start falling back, until we find a path element that is not 0
// (if there are no such elements, we're at the first item).
// if |*breakpoint| is set, we're solved--propagate the return.
// otherwise, if we are greater than 0, decrement, set |*breakpoint|,
//  and call forward to the greatest subelement, returning it.
// otherwise, we are equal to 0, and things are unsolved;
//  propagate the return, unless we're the penultimate, in which case return
//  our own curry;
// return our curry unless |*breakpoint| is set.
// FIXME rewrite as iteration -- we already have all the state we need!

static void*
nctree_chase_max(const nctree_int_item* nii, unsigned* path, int idx){
  if(nii->subcount == 0){
    path[idx] = UINT_MAX;
    return NULL;
  }
  path[idx] = nii->subcount - 1;
  void* ret = nctree_chase_max(&nii->subs[path[idx]], path, idx + 1);
  return ret ? ret : nii->curry;
}

static void*
nctree_prior_recursive(const nctree_int_item* nii, unsigned* path, int idx){
fprintf(stderr, "PPATH[%d]: %u %p COUNT: %u\n", idx, path[idx], nii->curry, nii->subcount);
  void* ret = NULL;
  if(nii->subcount){
    ret = nctree_prior_recursive(&nii->subs[path[idx]], path, idx + 1);
  }
  if(ret){
    return ret;
  }
  if(path[idx]){
    --path[idx];
    ret = nctree_chase_max(&nii->subs[path[idx]], path, idx + 1);
  }
  return ret ? ret : nii->curry;
}

// move to the prior path, updating the path in-place, returning its curry.
void* nctree_prev(nctree* n){
  void* ret = nctree_prior_recursive(&n->items.subs[n->currentpath[0]], n->currentpath, 0);
  if(n->activerow > 0){
    --n->activerow; // FIXME deduct size of previous?
  }
  nctree_redraw(n); // FIXME maybe require explicit redraws?
  return ret;
}

// the next is either:
//  - an extension to the right, if subs are available, or
//  - a bump to the rightmost path component with subcount available, or
//  - the current item
void* nctree_next(nctree* n){
  nctree_int_item* nii = &n->items;
  nctree_int_item* wedge = NULL;  // tracks the rightmost with room in subs
  int idx = 0;
  int wedidx = 0;
  while(n->currentpath[idx] != UINT_MAX){
    if(n->currentpath[idx] < nii->subcount - 1){
      wedge = nii;
      wedidx = idx;
    }
    nii = &nii->subs[n->currentpath[idx]];
    ++idx;
  }
  if(nii->subcount){
    n->currentpath[idx] = 0;
    n->currentpath[idx + 1] = UINT_MAX;
    return nii->subs[n->currentpath[idx]].curry;
  }
  if(wedge){
    ++n->currentpath[wedidx];
    n->currentpath[wedidx + 1] = UINT_MAX;
    return wedge->subs[n->currentpath[wedidx]].curry;
  }
  return nii->curry;
}

int nctree_redraw(nctree* n){
  // FIXME start at n->activerow with the currentpath. for each, until we run
  // out or fill the screen, check that it has an ncplane defined. if not,
  // create one. pass it to the callback with the curry.
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
  // FIXME implement left, right, +, - (expand/collapse)
  return false;
}

void* nctree_focused(nctree* n){
  int idx = 0;
  const nctree_int_item* nii = &n->items;
  while(n->currentpath[idx] != UINT_MAX){
    assert(n->currentpath[idx] < nii->subcount);
    nii = &nii->subs[n->currentpath[idx]];
    ++idx;
  }
//fprintf(stderr, "FOCUSED: %s\n", nii->curry);
  return nii->curry;
}

void* nctree_goto(nctree* n, const int* spec, size_t specdepth, int* failspec){
  // FIXME
}
