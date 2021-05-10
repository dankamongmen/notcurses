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
  unsigned maxdepth;        // maximum hierarchy level
  unsigned* currentpath;    // array of |maxdepth|+1 elements, ended by UINT_MAX
  int activerow;            // active row 0 <= activerow < dimy
  int indentcols;           // cols to indent per level
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
  // FIXME perhaps better to alloc a page at a time and take all items from
  // there, for better TLB performance?
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
goto_last_item(nctree* n){
  void* prev = NULL;
  void* r;
  while((r = nctree_next(n))){
    if(r == prev){
      return;
    }
    prev = r;
  }
}

static void
goto_first_item(nctree* n){
  n->currentpath[0] = 0;
  n->currentpath[1] = UINT_MAX;
  n->curitem = &n->items.subs[0];
  n->activerow = 0;
}

// the initial path ought point to the first item.
static int
prep_initial_path(nctree* n, unsigned maxdepth){
  n->currentpath = malloc(sizeof(*n->currentpath) * (maxdepth + 1));
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
    ret->cbfxn = opts->nctreecb;
    ret->indentcols = opts->indentcols;
    ret->maxdepth = 0;
    if(dup_tree_items(&ret->items, opts->items, opts->count, 0, &ret->maxdepth)){
      free(ret);
      return NULL;
    }
//fprintf(stderr, "MAXDEPTH: %u\n", ret->maxdepth);
    if(prep_initial_path(ret, ret->maxdepth)){
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
  if(opts->indentcols < 0){
    logerror(nc, "Can't indent negative columns\n");
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
nctree_prev_internal(nctree* n, unsigned* newpath){
  nctree_int_item* nii = &n->items;
  nctree_int_item* wedge = NULL; // tracks the rightmost non-zero path
  int idx = 0;
  while(newpath[idx] != UINT_MAX){
    nii = &nii->subs[newpath[idx]];
    if(idx == 0){
      wedge = &n->items;
    }else{// if(idx > 1){
      wedge = &wedge->subs[newpath[idx - 1]];
    }
    ++idx;
  }
  --idx;
  if(newpath[idx]){
    --newpath[idx];
    nii = &wedge->subs[newpath[idx]];
    ++idx;
//fprintf(stderr, "nii->subcount: %u idx: %d\n", nii->subcount, idx);
    while(nii->subcount){
      newpath[idx] = nii->subcount - 1;
      nii = &nii->subs[newpath[idx]];
      ++idx;
//fprintf(stderr, "nii->subcount: %u idx: %d\n", nii->subcount, idx);
    }
    newpath[idx] = UINT_MAX;
    return nii;
  }
  if(wedge == &n->items){
    return nii; // no change
  }
  newpath[idx] = UINT_MAX;
  return wedge;
}

void* nctree_prev(nctree* n){
  int rows = 0;
  if(n->curitem->ncp){
    rows = ncplane_dim_y(n->curitem->ncp);
  }
  nctree_int_item* tmp = nctree_prev_internal(n, n->currentpath);
  if(tmp != n->curitem){
    n->curitem = tmp;
    n->activerow -= rows;
    if(n->activerow < 0){
      n->activerow = 0;
    }
  }
  return n->curitem->curry;
}

// the next is either:
//  - an extension to the right, if subs are available, or
//  - a bump to the rightmost path component with subcount available, or
//  - the current item
static nctree_int_item*
nctree_next_internal(nctree* n, unsigned* newpath){
  nctree_int_item* nii = &n->items;
  nctree_int_item* wedge = NULL;  // tracks the rightmost with room in subs
  int idx = 0;
  int wedidx = 0;
  while(newpath[idx] != UINT_MAX){
    if(newpath[idx] < nii->subcount - 1){
      wedge = nii;
      wedidx = idx;
    }
    nii = &nii->subs[newpath[idx]];
    ++idx;
  }
  if(nii->subcount){
    newpath[idx] = 0;
    newpath[idx + 1] = UINT_MAX;
    return &nii->subs[newpath[idx]];
  }
  if(wedge){
    ++newpath[wedidx];
    newpath[wedidx + 1] = UINT_MAX;
    return &wedge->subs[newpath[wedidx]];
  }
  return nii;
}

void* nctree_next(nctree* n){
  int rows = 0;
  if(n->curitem->ncp){
    rows = ncplane_dim_y(n->curitem->ncp);
  }
  nctree_int_item* tmp = nctree_next_internal(n, n->currentpath);
  if(tmp != n->curitem){
    n->curitem = tmp;
    n->activerow += rows;
    if(n->activerow >= ncplane_dim_y(n->items.ncp)){
      n->activerow = ncplane_dim_y(n->items.ncp) - 1;
    }
  }
  return n->curitem->curry;
}

static int
tree_path_length(const unsigned* path){
  int len = 0;
  while(path[len] != UINT_MAX){
    ++len;
  }
  return len;
}

// draw the item. if *|frontiert| == *|frontierb|, we're the current item, and
// can use all the available space. if *|frontiert| < 0, draw down from
// *|frontierb|. otherwise, draw up from *|frontiert|.
static int
draw_tree_item(nctree* n, nctree_int_item* nii, const unsigned* path,
               int* frontiert, int* frontierb, int distance){
//fprintf(stderr, "drawing item ft: %d fb: %d %p\n", *frontiert, *frontierb, nii->ncp);
  if(!nii->ncp){
    const int startx = (tree_path_length(path) - 1) * n->indentcols;
    int ymin, ymax;
    if(*frontiert == *frontierb){
      ymin = 0;
      ymax = ncplane_dim_y(n->items.ncp) - 1;
    }else if(*frontiert < 0){
      ymin = *frontierb;
      ymax = ncplane_dim_y(n->items.ncp) - 1;
    }else{
      ymin = 0;
      ymax = *frontiert;
    }
//fprintf(stderr, "x: %d y: %d\n", startx, ymin);
    struct ncplane_options nopts = {
      .x = startx,
      .y = ymin,
      .cols = ncplane_dim_x(n->items.ncp) - startx,
      .rows = ymax - ymin + 1,
      .userptr = NULL,
      .name = NULL,
      .resizecb = NULL,
      .flags = 0,
    };
    nii->ncp = ncplane_create(n->items.ncp, &nopts);
    if(nii->ncp == NULL){
      return -1;
    }
  }else{
    // FIXME possibly enlarge nii->ncp?
  }
  if(ncplane_y(nii->ncp) <= *frontiert || *frontierb >= ncplane_dim_y(n->items.ncp)){
    ncplane_move_yx(nii->ncp, *frontiert, ncplane_x(nii->ncp));
  }else{
    ncplane_move_yx(nii->ncp, *frontierb, ncplane_x(nii->ncp));
  }
  int ret = n->cbfxn(nii->ncp, nii->curry, distance);
  if(ret < 0){
    return -1;
  }
  // FIXME shrink plane if it was enlarged
//fprintf(stderr, "ft: %d fb: %d %p ncplane_y: %d\n", *frontiert, *frontierb, nii->ncp, ncplane_y(nii->ncp));
  if(ncplane_y(nii->ncp) <= *frontiert){
    *frontiert = ncplane_y(nii->ncp) - 1;
  }
  if(ncplane_y(nii->ncp) + ncplane_dim_y(nii->ncp) > *frontierb){
    *frontierb = ncplane_y(nii->ncp) + ncplane_dim_y(nii->ncp);
  }
  return 0;
}

// iterate backwards from tmppath, destroying any ncplanes we find. they've
// been pushed off-screen. tmppath is changed as we iterate. nii will not be
// destroyed, only items above nii.
static int
destroy_above(nctree* n, nctree_int_item* nii, unsigned* path, int distance){
  nctree_int_item* tmpnii;
  while((tmpnii = nctree_prev_internal(n, path)) != nii){
    nii = tmpnii;
    --distance;
    if(nii->ncp){
      ncplane_destroy(nii->ncp);
      nii->ncp = NULL;
      n->cbfxn(nii->ncp, nii->curry, distance);
    }
  }
  return 0;
}

// iterate forwards from tmppath, destroying any ncplanes we find. they've
// been pushed off-screen. tmppath is changed as we iterate. nii will not be
// destroyed, only items below nii.
static int
destroy_below(nctree* n, nctree_int_item* nii, unsigned* path, int distance){
  nctree_int_item* tmpnii;
  while((tmpnii = nctree_next_internal(n, path)) != nii){
    nii = tmpnii;
    ++distance;
    if(nii->ncp){
      ncplane_destroy(nii->ncp);
      nii->ncp = NULL;
      n->cbfxn(nii->ncp, nii->curry, distance);
    }
  }
  return 0;
}

// tmppath ought be initialized with currentpath, but having size sufficient
// to hold n->maxdepth + 1 unsigneds.
static int
nctree_inner_redraw(nctree* n, unsigned* tmppath){
  ncplane* ncp = n->items.ncp;
  if(ncplane_cursor_move_yx(ncp, n->activerow, 0)){
    return -1;
  }
  int frontiert = n->activerow;
  int frontierb = n->activerow;
  nctree_int_item* nii = n->curitem;
  int distance = 0;
  // draw the focused item
  if(draw_tree_item(n, nii, tmppath, &frontiert, &frontierb, distance)){
    return -1;
  }
  nctree_int_item* tmpnii;
  // draw items above the current one
  while(frontiert >= 0){
    if((tmpnii = nctree_prev_internal(n, tmppath)) == nii){
      break;
    }
    nii = tmpnii;
    --distance;
    if(draw_tree_item(n, nii, tmppath, &frontiert, &frontierb, distance)){
      return -1;
    }
  }
  destroy_above(n, nii, tmppath, distance);
  // move items up if there is a gap at the top FIXME
  if(frontiert >= 0){
  }
  distance = 0;
  n->activerow = ncplane_y(n->curitem->ncp);
  nii = n->curitem;
  // draw items below the current one FIXME
  memcpy(tmppath, n->currentpath, sizeof(*tmppath) * (n->maxdepth + 1));
  while(frontierb < ncplane_dim_y(n->items.ncp)){
    if((tmpnii = nctree_next_internal(n, tmppath)) == nii){
      break;
    }
    nii = tmpnii;
    ++distance;
    if(draw_tree_item(n, nii, tmppath, &frontiert, &frontierb, distance)){
      return -1;
    }
  }
  destroy_below(n, nii, tmppath, distance);
  return 0;
}

int nctree_redraw(nctree* n){
  unsigned* tmppath = malloc(sizeof(*tmppath) * (n->maxdepth + 1));
  if(tmppath == NULL){
    return -1;
  }
  memcpy(tmppath, n->currentpath, sizeof(*tmppath) * (n->maxdepth + 1));
  int ret = nctree_inner_redraw(n, tmppath);
  free(tmppath);
  return ret;
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
    goto_last_item(n);
    return true;
  }
  // FIXME implement left, right, +, - (expand/collapse)
  return false;
}

void* nctree_focused(nctree* n){
  return n->curitem->curry;
}

void* nctree_goto(nctree* n, const unsigned* spec, int* failspec){
  (void)n;
  (void)spec;
  (void)failspec;
  // FIXME
  return NULL;
}
