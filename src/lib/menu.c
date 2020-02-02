#include "internal.h"

static void
free_menu_section(struct menu_section* ms){
  for(int i = 0 ; i < ms->itemcount ; ++i){
    free(ms->items[i].desc);
  }
  free(ms->items);
  free(ms->name);
}

static void
free_menu_sections(ncmenu* ncm){
  for(int i = 0 ; i < ncm->sectioncount ; ++i){
    free_menu_section(&ncm->sections[i]);
  }
  free(ncm->sections);
}

static int
dup_menu_section(struct menu_section* dst, const struct menu_section* src){
  dst->items = NULL;
  if(src->itemcount){
    dst->items = malloc(sizeof(*dst->items) * src->itemcount);
    if(dst->items == NULL){
      return -1;
    }
    for(int i = 0 ; i < src->itemcount ; ++i){
      if((dst->items[i].desc = strdup(src->items[i].desc)) == NULL){
        while(--i){
          free(&dst->items[i].desc);
        }
      }
    }
  }
  return 0;
}

static int
dup_menu_items(ncmenu* ncm, const menu_options* opts){
  ncm->sections = NULL;
  if(opts->sectioncount){
    ncm->sections = malloc(sizeof(*ncm->sections) * opts->sectioncount);
    if(ncm->sections == NULL){
      return -1;
    }
    for(int i = 0 ; i < opts->sectioncount ; ++i){
      if((ncm->sections[i].name = strdup(opts->sections[i].name)) == NULL){
        while(--i){
          free_menu_section(&ncm->sections[i]);
        }
      }
      if(dup_menu_section(&opts->sections[i], &ncm->sections[i])){
        free(ncm->sections[i].name);
        while(--i){
          free_menu_section(&ncm->sections[i]);
        }
        return -1;
      }
    }
  }
  return 0;
}

ncmenu* ncmenu_create(notcurses* nc, const menu_options* opts){
  if(opts->sectioncount < 0){
    return NULL;
  }else if(opts->sectioncount == 0 && opts->sections){
    return NULL;
  }else if(opts->sectioncount && !opts->sections){
    return NULL;
  }
  int totalheight = 1;
  int totalwidth = 2;
  // FIXME calaculate maximum dimensions
  ncmenu* ret = malloc(sizeof(*ret));
  ret->sectioncount = opts->sectioncount;
  ret->sections = NULL;
  int dimy = ncplane_dim_y(notcurses_stdplane(nc));
  int ypos = opts->bottom ? dimy - 1 : 0;
  if(ret){
    if(dup_menu_items(ret, opts) == 0){
      ret->ncp = ncplane_new(nc, totalheight, totalwidth, ypos, 0, NULL);
      if(ret->ncp){
        ret->unrolledsection = -1;
        ret->headerchannels = opts->headerchannels;
        ret->sectionchannels = opts->sectionchannels;
        return ret;
      }
      free_menu_sections(ret);
    }
    free(ret);
  }
  return NULL;
}

int ncmenu_unroll(ncmenu* n, int sectionidx){
  if(sectionidx < 0 || sectionidx >= n->sectioncount){
    return -1;
  }
  if(ncmenu_rollup(n)){ // roll up any unroled section
    return -1;
  }
  // FIXME
  return 0;
}

int ncmenu_rollup(ncmenu* n){
  if(n->unrolledsection < 0){
    return 0;
  }
  // FIXME
  return 0;
}

int ncmenu_destroy(ncmenu* n){
  int ret = 0;
  if(n){
    free_menu_sections(n);
  }
  return ret;
}
