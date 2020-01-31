#include "notcurses.h"
#include "internal.h"

// ideal body width given the ncselector's items and secondary/footer
static size_t
ncselector_body_width(const ncselector* n){
  size_t cols = 0;
  // the body is the maximum of
  //  * longop + longdesc + 5
  //  * secondary + 2
  //  * footer + 2
  if(n->footer && strlen(n->footer) + 2 > cols){
    cols = strlen(n->footer) + 2;
  }
  if(n->secondary && strlen(n->secondary) + 2 > cols){
    cols = strlen(n->secondary) + 2;
  }
  if(n->longop + n->longdesc + 5 > cols){
    cols = n->longop + n->longdesc + 5;
  }
  return cols;
}

// redraw the selector widget in its entirety
static int
ncselector_draw(ncselector* n){
  ncplane_erase(n->ncp);
  uint64_t channels = 0;
  channels_set_fg(&channels, 0x4040f0); // FIXME allow configuration
  // if we have a title, we'll draw a riser. the riser is two rows tall, and
  // exactly four columns longer than the title, and aligned to the right. we
  // draw a rounded box. the body will blow part or all of the bottom away.
  int yoff = 0;
  if(n->title){
    size_t riserwidth = strlen(n->title) + 4;
    int offx = ncplane_align(n->ncp, NCALIGN_RIGHT, riserwidth);
    ncplane_cursor_move_yx(n->ncp, 0, offx);
    ncplane_rounded_box_sized(n->ncp, 0, channels, 3, riserwidth, 0);
    ncplane_cursor_move_yx(n->ncp, 1, offx + 2);
    ncplane_putstr(n->ncp, n->title); // FIXME allow styling configuration
    yoff += 2;
  }
  size_t bodywidth = ncselector_body_width(n);
  int xoff = ncplane_align(n->ncp, NCALIGN_RIGHT, bodywidth);
  ncplane_cursor_move_yx(n->ncp, yoff, xoff);
  int dimy, dimx;
  ncplane_dim_yx(n->ncp, &dimy, &dimx);
  ncplane_rounded_box_sized(n->ncp, 0, channels, dimy - yoff, bodywidth, 0);
  unsigned printidx = n->startdisp;
  int bodyoffset = dimx - bodywidth + 2;
  unsigned printed = 0;
  for(yoff += 2 ; yoff < dimy - 2 ; ++yoff){
    if(n->maxdisplay && printed == n->maxdisplay){
      break;
    }
    if(printidx == n->selected){
      ncplane_styles_on(n->ncp, CELL_STYLE_REVERSE);
    }
    ncplane_printf_yx(n->ncp, yoff, bodyoffset, "%*.*s %s", (int)n->longop,
                      (int)n->longop, n->items[printidx].option,
                      n->items[printidx].desc);
    if(printidx == n->selected){
      ncplane_styles_off(n->ncp, CELL_STYLE_REVERSE);
    }
    if(++printidx == n->itemcount){
      printidx = 0;
    }
    ++printed;
  }
  return notcurses_render(n->ncp->nc);
}

// calculate the necessary dimensions based off properties of the selector and
// the containing screen FIXME should be based on containing ncplane
static int
ncselector_dim_yx(notcurses* nc, const ncselector* n, int* ncdimy, int* ncdimx){
  int rows = 0, cols = 0; // desired dimensions
  int dimy, dimx; // dimensions of containing screen
  notcurses_term_dim_yx(nc, &dimy, &dimx);
  if(n->title){ // header adds two rows for riser
    rows += 2;
  }
  // we have a top line, a bottom line, two lines of margin, and must be able
  // to display at least one row beyond that, so require five more
  rows += 5;
  if(rows > dimy){ // insufficient height to display selector
    return -1;
  }
  rows += (!n->maxdisplay || n->maxdisplay > n->itemcount ? n->itemcount : n->maxdisplay) - 1; // rows necessary to display all options
  if(rows > dimy){ // claw excess back
    rows = dimy;
  }
  *ncdimy = rows;
  cols = ncselector_body_width(n);
  // the riser, if it exists, is header + 4. the cols are the max of these two.
  if(n->title && strlen(n->title) + 4 > (size_t)cols){
    cols = strlen(n->title) + 4;
  }
  if(cols > dimx){ // insufficient width to display selector
    return -1;
  }
  *ncdimx = cols;
  return 0;
}

ncselector* ncselector_create(ncplane* n, int y, int x, const selector_options* opts){
  ncselector* ns = malloc(sizeof(*ns));
  ns->title = opts->title ? strdup(opts->title) : NULL;
  ns->secondary = opts->secondary ? strdup(opts->secondary) : NULL;
  ns->footer = opts->footer ? strdup(opts->footer) : NULL;
  ns->selected = 0;
  ns->startdisp = 0;
  ns->longop = 0;
  ns->maxdisplay = opts->maxdisplay;
  ns->longdesc = 0;
  if(opts->itemcount){
    if(!(ns->items = malloc(sizeof(*ns->items) * opts->itemcount))){
      free(ns->title); free(ns->secondary); free(ns->footer);
      free(n);
      return NULL;
    }
  }else{
    ns->items = NULL;
  }
  for(ns->itemcount = 0 ; ns->itemcount < opts->itemcount ; ++ns->itemcount){
    const struct selector_item* src = &opts->items[ns->itemcount];
    if(strlen(src->option) > ns->longop){
      ns->longop = strlen(src->option);
    }
    if(strlen(src->desc) > ns->longdesc){
      ns->longdesc = strlen(src->desc);
    }
    ns->items[ns->itemcount].option = strdup(src->option);
    ns->items[ns->itemcount].desc = strdup(src->desc);
    if(!(ns->items[ns->itemcount].desc && ns->items[ns->itemcount].option)){
      free(ns->items[ns->itemcount].option);
      free(ns->items[ns->itemcount].desc);
      goto freeitems;
    }
  }
  int dimy, dimx;
  if(ncselector_dim_yx(n->nc, ns, &dimy, &dimx)){
    goto freeitems;
  }
  if(!(ns->ncp = ncplane_new(n->nc, dimy, dimx, y, x, NULL))){
    goto freeitems;
  }
  ncselector_draw(ns); // deal with error here?
  return ns;

freeitems:
  while(ns->itemcount--){
    free(ns->items[ns->itemcount].option);
    free(ns->items[ns->itemcount].desc);
  }
  free(ns->items);
  free(ns->title); free(ns->secondary); free(ns->footer);
  free(ns);
  return NULL;
}

ncselector* ncselector_aligned(ncplane* n, int y, ncalign_e align, const selector_options* opts);

int ncselector_additem(ncselector* n, const struct selector_item* item){
  size_t newsize = sizeof(*n->items) * (n->itemcount + 1);
  struct selector_item* items = realloc(n->items, newsize);
  if(!items){
    return -1;
  }
  n->items = items;
  n->items[n->itemcount].option = strdup(item->option);
  n->items[n->itemcount].desc = strdup(item->desc);
  ++n->itemcount;
  return ncselector_draw(n);
}

int ncselector_delitem(ncselector* n, const char* item){
  for(unsigned idx = 0 ; idx < n->itemcount ; ++idx){
    if(strcmp(n->items[idx].option, item) == 0){ // found it
      free(n->items[idx].option);
      free(n->items[idx].desc);
      if(idx < n->itemcount - 1){
        memmove(n->items + idx, n->items + idx + 1, sizeof(*n->items) * (n->itemcount - idx - 1));
      }else{
        if(idx){
          --n->selected;
        }
      }
      --n->itemcount;
      return ncselector_draw(n);
    }
  }
  return -1; // wasn't found
}

ncplane* ncselector_plane(ncselector* n){
  return n->ncp;
}

char* ncselector_selected(const ncselector* n){
  if(n->itemcount == 0){
    return NULL;
  }
  return strdup(n->items[n->selected].option);
}

void ncselector_previtem(ncselector* n, char** newitem){
fprintf(stderr, "MAX: %u COUNT: %u SELECTED: %u START: %u\n", n->maxdisplay, n->itemcount, n->selected, n->startdisp);
  if(n->selected == n->startdisp && n->maxdisplay && n->maxdisplay < n->itemcount){
    if(n->startdisp-- == 0){
      n->startdisp = n->itemcount - 1;
    }
  }
  if(n->selected == 0){
    n->selected = n->itemcount;
  }
  --n->selected;
  if(newitem && n->itemcount){
    *newitem = strdup(n->items[n->selected].option);
  }
  ncselector_draw(n);
}

void ncselector_nextitem(ncselector* n, char** newitem){
  ++n->selected;
  if(n->maxdisplay && n->maxdisplay < n->itemcount){
    if((n->startdisp + n->maxdisplay) % n->itemcount == (n->selected % n->itemcount)){
      if(++n->startdisp == n->itemcount){
        n->startdisp = 0;
      }
    }
  }
  if(n->selected == n->itemcount){
    n->selected = 0;
  }
  if(newitem && n->itemcount){
    *newitem = strdup(n->items[n->selected].option);
  }
  ncselector_draw(n);
}

void ncselector_destroy(ncselector* n, char** item){
  if(n){
    if(item){
      *item = n->items[n->selected].option;
      n->items[n->selected].option = NULL;
    }
    while(n->itemcount--){
      free(n->items[n->itemcount].option);
      free(n->items[n->itemcount].desc);
    }
    free(n->items);
    free(n->title);
    free(n->secondary);
    free(n->footer);
    free(n);
  }
}
