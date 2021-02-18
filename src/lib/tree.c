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
  nctree_int_item items;    // topmost set of items, holds widget plane
  nctree_int_item* curitem; // item addressed by the path
  unsigned* currentpath;    // array of |maxdepth|+1 elements, ended by UINT_MAX
  unsigned maxdepth;        // binds the path length
  int activerow;            // active row 0 <= activerow < dimy
  uint64_t bchannels;       // border glyph channels
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

static void
goto_first_item(nctree* n){
  nctree_int_item* nii = &n->items;
  int c = 0;
  while(nii->subcount){
    n->currentpath[c++] = 0;
    nii = &nii->subs[0];
  }
  n->currentpath[c] = UINT_MAX;
  n->curitem = nii;
  n->activerow = 0;
}

// the initial path ought point to the first item. maxdepth must be set.
static int
prep_initial_path(nctree* n){
  n->currentpath = malloc(sizeof(*n->currentpath) * (n->maxdepth + 1));
  if(n->currentpath == NULL){
    return -1;
  }
  goto_first_item(n);
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
    nctree_redraw(ret);
  }
  return ret;
}

nctree* nctree_create(ncplane* n, const struct nctree_options* opts){
  notcurses* nc = ncplane_notcurses(n);
  if(opts->flags){
    logwarn(nc, "Passed invalid flags 0x%016jx\n", (uint64_t)opts->flags);
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
    logerror(nc, "Couldn't prepare nctree\n");
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

// the prev is either:
//   the item to the left, if the last path component is 0, or
//   a drop from the rightmost non-zero path component, extended out to the right, or
//   the current item
// so we can always just go to the last path component, act there, and possibly
// extend it out to the maximal topright.
static nctree_int_item*
nctree_prev_internal(nctree* n){
  nctree_int_item* nii = &n->items;
  nctree_int_item* wedge = NULL; // tracks the rightmost non-zero path
  int idx = 0;
  while(n->currentpath[idx] != UINT_MAX){
    nii = &nii->subs[n->currentpath[idx]];
    if(idx == 0){
      wedge = &n->items;
    }else{// if(idx > 1){
      wedge = &wedge->subs[n->currentpath[idx - 1]];
    }
    ++idx;
  }
  if(n->currentpath[idx - 1]){
    --n->currentpath[idx - 1];
    nii = &wedge->subs[n->currentpath[idx - 1]];
    while(nii->subcount){
      n->currentpath[idx - 1] = nii->subcount - 1;
      nii = &nii->subs[n->currentpath[idx - 1]];
      ++idx;
    }
    n->currentpath[idx] = UINT_MAX;
    return nii;
  }
  if(wedge == &n->items){
    return nii; // no change
  }
  n->currentpath[idx - 1] = UINT_MAX;
  return wedge;
}

void* nctree_prev(nctree* n){
  n->curitem = nctree_prev_internal(n);
  return n->curitem->curry;
}

// the next is either:
//  - an extension to the right, if subs are available, or
//  - a bump to the rightmost path component with subcount available, or
//  - the current item
static nctree_int_item*
nctree_next_internal(nctree* n){
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
    return &nii->subs[n->currentpath[idx]];
  }
  if(wedge){
    ++n->currentpath[wedidx];
    n->currentpath[wedidx + 1] = UINT_MAX;
    return &wedge->subs[n->currentpath[wedidx]];
  }
  return nii;
}

void* nctree_next(nctree* n){
  // FIXME update n->activerow, redraw
  n->curitem = nctree_next_internal(n);
  return n->curitem->curry;
}

int nctree_redraw(nctree* n){
  ncplane* ncp = n->items.ncp;
  if(ncplane_cursor_move_yx(ncp, n->activerow, 0)){
    return -1;
  }
  int frontiert = n->activerow;
  int frontierb = n->activerow;
  nctree_int_item* nii = n->curitem;
  while(frontiert > 0 || frontierb < ncplane_dim_y(n->items.ncp)){
    if(!nii->ncp){
      struct ncplane_options nopts = {
        .x = 0, // FIXME
        .y = 0, // FIXME
        .cols = ncplane_dim_x(n->items.ncp), // FIXME
        .rows = ncplane_dim_y(n->items.ncp), // FIXME
        .userptr = NULL,
        .name = NULL,
        .resizecb = NULL, // FIXME
        .flags = 0,
      };
      nii->ncp = ncplane_create(n->items.ncp, &nopts);
      if(nii->ncp == NULL){
        return -1;
      }
    }
    n->cbfxn(nii->ncp, nii->curry, 0);
    // FIXME start with the currentpath. for each, until we run
    // out or fill the screen, check that it has an ncplane defined. if not,
    // create one. pass it to the callback with the curry.
    // FIXME or maybe just track the top visible item, and always start from there?
    --frontiert; // FIXME placeholders to break loop
    ++frontierb;
  }
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
    goto_first_item(n);
    return true;
  }else if(ni->id == NCKEY_END){
    nctree_next(n); // more FIXME
    return true;
  }
  // FIXME implement left, right, +, - (expand/collapse)
  return false;
}

void* nctree_focused(nctree* n){
  return n->curitem->curry;
}

/*
void* nctree_goto(nctree* n, const int* spec, size_t specdepth, int* failspec){
  // FIXME
}
*/
