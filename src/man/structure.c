#include <limits.h>
#include <notcurses/notcurses.h>
#include "structure.h"

typedef struct docnode {
  char* title;
  int y;
} docnode;

typedef struct docstructure {
  struct ncplane* n;
  unsigned count;
  docnode** nodes;
  unsigned curnode;
  int cury;
  bool visible;
  bool movedown;     // our last move was down
} docstructure;

static int
docbar_redraw(docstructure *ds){
  ncplane_erase(ds->n);
  for(unsigned c = ds->curnode ; c < ds->count ; ++c){
    if(c == ds->curnode){
      ncplane_set_styles(ds->n, NCSTYLE_BOLD);
      ncplane_set_fg_rgb(ds->n, 0x00ffb0);
    }
    if(ncplane_putstr(ds->n, ds->nodes[c]->title) <= 0){
      return 0;
    }
    ncplane_set_styles(ds->n, NCSTYLE_NONE);
    ncplane_set_fg_rgb(ds->n, 0x00b0b0);
    if(notcurses_canutf8(ncplane_notcurses(ds->n))){
      if(ncplane_putstr(ds->n, " ∎ ") <= 0){
        return 0;
      }
    }else{
      if(ncplane_putstr(ds->n, " | ") <= 0){
        return 0;
      }
    }
  }
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
  uint64_t channels = NCCHANNELS_INITIALIZER(0x0, 0xb0, 0xb0, 0x06, 0x11, 0x2f);
  ncplane_set_base(p, " ", 0, channels);
  ncplane_set_channels(p, channels);
  ds->n = p;
  ds->visible = true;
  ds->count = 0;
  ds->nodes = NULL;
  ds->movedown = false;
  ds->cury = 0;
  ds->curnode = 0;
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

static void
docnode_free(docnode* dn){
  if(dn){
    free(dn->title);
    free(dn);
  }
}

void docstructure_free(docstructure* ds){
  if(ds){
    while(ds->count--){
      docnode_free(ds->nodes[ds->count]);
    }
    free(ds->nodes);
    free(ds);
  }
}

static docnode*
docnode_create(const char* title, int y){
  docnode* dn = malloc(sizeof(*dn));
  if(dn == NULL){
    return NULL;
  }
  if((dn->title = strdup(title)) == NULL){
    free(dn);
    return NULL;
  }
  dn->y = -y;
  return dn;
}

// add the specified [sub]section to the document strucure. |line| refers to
// the row on the display plane, *not* the line in the original content.
int docstructure_add(docstructure* ds, const char* title, int y){
  docnode* dn = docnode_create(title, y);
  if(dn == NULL){
    return -1;
  }
  unsigned newcount = ds->count + 1;
  docnode** newdn = realloc(ds->nodes, newcount * sizeof(*ds->nodes));
  if(newdn == NULL){
    docnode_free(dn);
    return -1;
  }
  ds->nodes = newdn;
  ds->nodes[ds->count] = dn;
  ds->count = newcount;
  if(docbar_redraw(ds)){
    return -1;
  }
  return 0;
}

// returns corresponding y
int docstructure_prev(docstructure* ds){
  if(ds->curnode){
    --ds->curnode;
  }
  return ds->nodes[ds->curnode]->y;
}

int docstructure_next(docstructure* ds){
  if(ds->curnode + 1 < ds->count){
    ++ds->curnode;
  }
  return ds->nodes[ds->curnode]->y;
}

int docstructure_move(docstructure* ds, int newy, unsigned movedown){
  ds->movedown = movedown;
  if(movedown){
    while(ds->curnode + 1 < ds->count){
      if(newy < ds->nodes[ds->curnode + 1]->y){
        docstructure_next(ds);
      }else{
        break;
      }
    }
  }else{
    while(ds->curnode){
      if(newy > ds->nodes[ds->curnode - 1]->y){
        docstructure_prev(ds);
      }else{
        break;
      }
    }
  }
  ds->cury = newy;
  if(docbar_redraw(ds)){
    return -1;
  }
  return 0;
}
