#include <limits.h>
#include <notcurses/notcurses.h>
#include "structure.h"

typedef struct docnode {
  char* title;
  int line;
  docstruct_e level;
  int y;
} docnode;

typedef struct docstructure {
  struct ncplane* n;
  unsigned count;
  char** strings;
  bool visible;
} docstructure;

static int
docbar_redraw(docstructure *dn){
  ncplane_erase(dn->n);
  // FIXME redraw
  return 0;
}

static int
docbar_resize(struct ncplane* n){
  unsigned width = ncplane_dim_x(ncplane_parent(n));
  if(ncplane_resize_simple(n, 1, width)){
    return -1;
  }
  return docbar_redraw(ncplane_userptr(n));
}

docstructure* docstructure_create(struct ncplane* n){
  docstructure* ds = malloc(sizeof(*ds));
  if(ds == NULL){
    return NULL;
  }
  ncplane_options nopts = {
    .rows = 1,
    .cols = ncplane_dim_x(n),
    .y = 0,
    .userptr = ds,
    .resizecb = docbar_resize,
  };
  struct ncplane* p = ncplane_create(n, &nopts);
  if(p == NULL){
    return NULL;
  }
  uint64_t channels = NCCHANNELS_INITIALIZER(0xff, 0xff, 0xff, 0x06, 0x11, 0x2f);
  ncplane_set_base(p, " ", 0, channels);
  ds->n = p;
  ds->visible = true;
  ds->count = 0;
  ds->strings = NULL;
  docbar_redraw(ds);
  return ds;
}

// to show the structure menu, it ought be on top. otherwise, the page plane
// ought be below the bar, which ought be on top.
void docstructure_toggle(struct ncplane* p, struct ncplane* b, docstructure* ds){
  if(!(ds->visible = !ds->visible)){
    ncplane_move_top(p);
    ncplane_move_top(b);
  }else{
    ncplane_move_bottom(p);
    ncplane_move_bottom(notcurses_stdplane(ncplane_notcurses(p)));
  }
}

void docstructure_free(docstructure* ds){
  if(ds){
    while(ds->count--){
      free(ds->strings[ds->count]);
    }
    free(ds->strings);
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
docnode_create(const char* title, int line, docstruct_e level, int y){
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
  dn->y = -y;
  return dn;
}

// add the specified [sub]section to the document strucure. |line| refers to
// the row on the display plane, *not* the line in the original content.
int docstructure_add(docstructure* ds, const char* title, int line,
                     docstruct_e level, int y){
  docnode* dn = docnode_create(title, line, level, y);
  if(dn == NULL){
    return -1;
  }
  char* s = strdup(title);
  if(s == NULL){
    return -1;
  }
  unsigned newcount = ds->count + 1;
  char** newstr = realloc(ds->strings, newcount * sizeof(*ds->strings));
  if(newstr == NULL){
    docnode_free(dn);
    free(s);
    return -1;
  }
  ds->strings = newstr;
  ds->strings[ds->count] = s;
  ds->count = newcount;
  if(docbar_redraw(ds)){
    return -1;
  }
  return 0;
}

int docstructure_move(docstructure* ds, int newy){
  docnode* pdn = NULL;
  docnode* dn;
  /* FIXME
  if(newy > ds->cury){
    while((dn = nctree_prev(ds->nct)) != pdn){
      if(dn->y > newy){
        dn = nctree_next(ds->nct);
        break;
      }
      pdn = dn;
    }
  }else if(newy < ds->cury){
    while((dn = nctree_next(ds->nct)) != pdn){
      if(dn->y < newy){
        dn = nctree_prev(ds->nct);
        break;
      }
      pdn = dn;
    }
  }
  ds->cury = newy;
  */
  if(docbar_redraw(ds)){
    return -1;
  }
  return 0;
}
