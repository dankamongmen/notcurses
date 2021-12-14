#include <limits.h>
#include <notcurses/notcurses.h>
#include "structure.h"

#define HIERARCHY_MAX 3

typedef struct docnode {
  char* title;
  int line;
  docstruct_e level;
} docnode;

typedef struct docstructure {
  struct nctree* nct;
  // one entry for each hierarchy level + terminator
  unsigned curpath[HIERARCHY_MAX + 1];
} docstructure;

static int
docstruct_callback(struct ncplane* n, void* curry, int i){
  if(n){
    docnode* dn = curry;
    ncplane_printf(n, "%s", dn->title);
    ncplane_resize_simple(n, 1, ncplane_dim_x(n));
    (void)i;
  }
  return 0;
}

docstructure* docstructure_create(struct ncplane* n){
  docstructure* ds = malloc(sizeof(*ds));
  if(ds == NULL){
    return NULL;
  }
  const int ROWS = 7;
  const int COLDIV = 4;
  ncplane_options nopts = {
    .rows = ROWS,
    .cols = ncplane_dim_x(n) / COLDIV,
    .y = ncplane_dim_y(n) - ROWS,
    .x = ncplane_dim_x(n) - (ncplane_dim_x(n) / COLDIV) - 1,
    .flags = NCPLANE_OPTION_AUTOGROW, // autogrow to right
  };
  struct ncplane* p = ncplane_create(n, &nopts);
  if(p == NULL){
    return NULL;
  }
  uint64_t channels = NCCHANNELS_INITIALIZER(0, 0, 0, 0x99, 0xed, 0xc3);
  ncplane_set_base(p, "", 0, channels);
  nctree_options topts = {
    .nctreecb = docstruct_callback,
  };
  if((ds->nct = nctree_create(p, &topts)) == NULL){
    free(ds);
    return NULL;
  }
  for(unsigned z = 0 ; z < sizeof(ds->curpath) / sizeof(*ds->curpath) ; ++z){
    ds->curpath[z] = UINT_MAX;
  }
  return ds;
}

void docstructure_free(docstructure* ds){
  if(ds){
    nctree_destroy(ds->nct);
    free(ds);
  }
}

static void
docnode_free(docnode* dn){
  if(dn){
    free(dn->title);
    free(dn);
  }
}

static docnode*
docnode_create(const char* title, int line, docstruct_e level){
  docnode* dn = malloc(sizeof(*dn));
  if(dn == NULL){
    return NULL;
  }
  if((dn->title = strdup(title)) == NULL){
    free(dn);
    return NULL;
  }
  dn->level = level;
  dn->line = line;
  return dn;
}

// add the specified [sub]section to the document strucure. |line| refers to
// the row on the display plane, *not* the line in the original content.
int docstructure_add(docstructure* ds, const char* title, int line,
                     docstruct_e level){
  unsigned addpath[sizeof(ds->curpath) / sizeof(*ds->curpath)];
  if(level < 0 || (unsigned)level >= sizeof(addpath) / sizeof(*addpath) - 1){
    fprintf(stderr, "invalid level %d\n", level);
    return -1;
  }
  docnode* dn = docnode_create(title, line, level);
  if(dn == NULL){
    return -1;
  }
  unsigned z = 0;
  while(z <= level){
    if((addpath[z] = ds->curpath[z]) == UINT_MAX){
      ds->curpath[z] = 0;
      addpath[z] = 0;
    }
    ++z;
  }
  addpath[z] = UINT_MAX;
  struct nctree_item nitem = {
    .curry = dn,
  };
  if(nctree_add(ds->nct, addpath, &nitem)){
    docnode_free(dn);
    return -1;
  }
  ds->curpath[z] = addpath[z];
  if(nctree_redraw(ds->nct)){
    return -1;
  }
  return 0;
}
