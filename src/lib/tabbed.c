#include "internal.h"

static bool
tab_is_before(nctabbed* nt, nctab* t, nctab* relativeto){
  nctab* tb = relativeto;
  while(tb != nt->leftmost){
    tb = tb->prev;
    if(t == tb){
      return true;
    }
  }
  return false;
}

void nctabbed_redraw(nctabbed* nt){
  nctab* t;
  int drawn_cols = 0;
  int rows, cols;
  if(nt->tabcount == 0){
    // no tabs = nothing to draw
    ncplane_erase(nt->hp);
    return;
  }
  // update sizes for planes
  ncplane_dim_yx(nt->ncp, &rows, &cols);
  if(nt->opts.flags & NCTABBED_OPTION_BOTTOM){
    ncplane_resize_simple(nt->hp, -1, cols);
    ncplane_resize_simple(nt->p, rows - 1, cols);
    ncplane_move_yx(nt->hp, rows - 2, 0);
  }else{
    ncplane_resize_simple(nt->hp, -1, cols);
    ncplane_resize_simple(nt->p, rows - 1, cols);
  }
  // the callback draws the tab contents
  nt->selected->cb(nt->selected, nt->p, nt->selected->curry);
  // now we draw the headers
  t = nt->leftmost;
  ncplane_erase(nt->hp);
  ncplane_set_channels(nt->hp, nt->opts.hdrchan);
  do{
    if(t == nt->selected){
      ncplane_set_channels(nt->hp, nt->opts.selchan);
      drawn_cols += ncplane_putstr(nt->hp, t->name);
      ncplane_set_channels(nt->hp, nt->opts.hdrchan);
    }else{
      drawn_cols += ncplane_putstr(nt->hp, t->name);
    }
    // avoid drawing the separator after the last tab or when we ran out of space
    if(t->next != nt->leftmost || drawn_cols >= cols){
      // FIXME maybe have the separator configurable thru opts
      drawn_cols += ncplane_putchar(nt->hp, ' ');
    }
    t = t->next;
  }while(t != nt->leftmost && drawn_cols < cols);
}

void nctabbed_ensure_selected_header_visible(nctabbed* nt){
  nctab* t = nt->leftmost;
  int cols = ncplane_dim_x(nt->hp);
  int takencols = 0;
  if(!t){
    return;
  }
//fprintf(stderr, "ensuring selected header visible\n");
  do{
    if(t == nt->selected){
      break;
    }
    takencols += t->namecols + 1; // separator width
    if(takencols >= cols){
//fprintf(stderr, "not enough space, rotating\n");
      takencols -= nt->leftmost->namecols + 1; // separator width
      nctabbed_rotate(nt, -1);
    }
    t = t->next;
//fprintf(stderr, "iteration over: takencols = %d, cols = %d\n", takencols, cols);
  }while(t != nt->leftmost);
//fprintf(stderr, "ensuring done\n");
}

static bool
nctabbed_validate_opts(ncplane* n, const nctabbed_options* opts){
  if(!n){
    return false;
  }
  if(opts->flags > NCTABBED_OPTION_BOTTOM){
    return false;
  }
  return true;
}

nctab* nctabbed_selected(nctabbed* nt){
  return nt->selected;
}

nctab* nctabbed_leftmost(nctabbed* nt){
  return nt->leftmost;
}

int nctabbed_tabcount(nctabbed* nt){
  return nt->tabcount;
}

ncplane* nctabbed_plane(nctabbed* nt){
  return nt->ncp;
}

ncplane* nctabbed_content_plane(nctabbed* nt){
  return nt->p;
}

const char* nctab_name(nctab* t){
  return t->name;
}

int nctab_name_width(nctab* t){
  return t->namecols;
}

void* nctab_userptr(nctab* t){
  return t->curry;
}

nctab* nctab_next(nctab* t){
  return t->next;
}

nctab* nctab_prev(nctab* t){
  return t->prev;
}

nctabbed* nctabbed_create(ncplane* n, const nctabbed_options* topts){
  nctabbed_options zeroed = {};
  ncplane_options nopts = {};
  int nrows, ncols;
  nctabbed* nt;
  if(!topts){
    topts = &zeroed;
  }
  if(!nctabbed_validate_opts(n, topts)){
    return NULL;
  }
  if((nt = malloc(sizeof(*nt))) == NULL){
    return NULL;
  }
  nt->ncp = n;
  nt->leftmost = nt->selected = NULL;
  nt->tabcount = 0;
  memcpy(&nt->opts, topts, sizeof(*topts));
  ncplane_dim_yx(n, &nrows, &ncols);
  if(topts->flags & NCTABBED_OPTION_BOTTOM){
    nopts.y = nopts.x = 0;
    nopts.cols = ncols;
    nopts.rows = nrows - 1;
    if((nt->p = ncplane_create(n, &nopts)) == NULL){
      ncplane_genocide(n);
      free(nt);
      return NULL;
    }
    nopts.y = nrows - 2;
    nopts.rows = 1;
    if((nt->hp = ncplane_create(n, &nopts)) == NULL){
      ncplane_genocide(n);
      free(nt);
      return NULL;
    }
  }else{
    nopts.y = nopts.x = 0;
    nopts.cols = ncols;
    nopts.rows = 1;
    if((nt->hp = ncplane_create(n, &nopts)) == NULL){
      ncplane_genocide(n);
      free(nt);
      return NULL;
    }
    nopts.y = 1;
    nopts.rows = nrows - 1;
    if((nt->p = ncplane_create(n, &nopts)) == NULL){
      ncplane_genocide(n);
      free(nt);
      return NULL;
    }
  }
  return nt;
}

nctab* nctabbed_add(nctabbed* nt, nctab* after, nctab* before, tabcb cb,
                    const char* name, void* opaque){
  nctab* t;
  if(after && before){
    if(after->prev != before || before->next != after){
      logerror(ncplane_notcurses(nt->ncp), "bad before (%p) / after (%p) spec\n", before, after);
      return NULL;
    }
  }else if(!after && !before){
    // add it to the right of the selected tab
    after = nt->selected;
  }
  if((t = malloc(sizeof(*t))) == NULL){
    return NULL;
  }
  if((t->name = strdup(name)) == NULL){
    free(t);
    return NULL;
  }
  if((t->namecols = ncstrwidth(name)) < 0){
    free(t->name);
    free(t);
    return NULL;
  }
  if(after){
    t->next = after->next;
    t->prev = after;
    after->next = t;
    t->next->prev = t;
  }else if(before){
    t->next = before;
    t->prev = before->prev;
    before->prev = t;
    t->prev->next = t;
  }else{
    // the first tab
    t->prev = t->next = t;
    nt->leftmost = nt->selected = t;
  }
  t->cb = cb;
  t->curry = opaque;
  ++nt->tabcount;
  return t;
}

int nctabbed_del(nctabbed* nt, nctab* t){
  if(!t){
    return -1;
  }
  if(nt->tabcount == 1){
    nt->leftmost = nt->selected = NULL;
  }else{
    if(nt->selected == t){
      nt->selected = t->next;
    }
    if(nt->leftmost == t){
      nt->leftmost = t->next;
    }
    t->next->prev = t->prev;
    t->prev->next = t->next;
  }
  free(t);
  --nt->tabcount;
  return 0;
}

void nctabbed_rotate(nctabbed* nt, int amt){
  if(amt > 0){
    for(int i = 0 ; i < amt ; ++i){
      nt->leftmost = nt->leftmost->prev;
    }
  }else{
    for(int i = 0 ; i < -amt ; ++i){
      nt->leftmost = nt->leftmost->next;
    }
  }
}

nctab* nctabbed_next(nctabbed* nt){
  if(nt->tabcount == 0){
    return NULL;
  }
  nt->selected = nt->selected->next;
  return nt->selected;
}

nctab* nctabbed_prev(nctabbed* nt){
  if(nt->tabcount == 0){
    return NULL;
  }
  nt->selected = nt->selected->prev;
  return nt->selected;
}

void nctabbed_destroy(nctabbed* nt){
  if(!nt){
    return;
  }
  nctab* t = nt->leftmost;
  nctab* tmp;
  t->prev->next = NULL;
  while(t){
    tmp = t->next;
    free(t->name);
    free(t);
    t = tmp;
  }
  ncplane_genocide(nt->ncp);
  free(nt);
}
