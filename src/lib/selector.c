#include "internal.h"

// ideal body width given the ncselector's items and secondary/footer
static int
ncselector_body_width(const ncselector* n){
  int cols = 0;
  // the body is the maximum of
  //  * longop + longdesc + 5
  //  * secondary + 2
  //  * footer + 2
  if(n->footercols + 2 > cols){
    cols = n->footercols + 2;
  }
  if(n->secondarycols + 2 > cols){
    cols = n->secondarycols + 2;
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
  // if we have a title, we'll draw a riser. the riser is two rows tall, and
  // exactly four columns longer than the title, and aligned to the right. we
  // draw a rounded box. the body will blow part or all of the bottom away.
  int yoff = 0;
  if(n->title){
    size_t riserwidth = n->titlecols + 4;
    int offx = ncplane_align(n->ncp, NCALIGN_RIGHT, riserwidth);
    ncplane_cursor_move_yx(n->ncp, 0, offx);
    ncplane_rounded_box_sized(n->ncp, 0, n->boxchannels, 3, riserwidth, 0);
    n->ncp->channels = n->titlechannels;
    ncplane_printf_yx(n->ncp, 1, offx + 1, " %s ", n->title);
    yoff += 2;
  }
  int bodywidth = ncselector_body_width(n);
  int xoff = ncplane_align(n->ncp, NCALIGN_RIGHT, bodywidth);
  ncplane_cursor_move_yx(n->ncp, yoff, xoff);
  int dimy, dimx;
  ncplane_dim_yx(n->ncp, &dimy, &dimx);
  ncplane_rounded_box_sized(n->ncp, 0, n->boxchannels, dimy - yoff, bodywidth, 0);
  if(n->title){
    n->ncp->channels = n->boxchannels;
    ncplane_putegc_yx(n->ncp, 2, dimx - 1, "┤", NULL);
    if(bodywidth < dimx){
      ncplane_putegc_yx(n->ncp, 2, dimx - bodywidth, "┬", NULL);
    }
    if((n->titlecols + 4 != dimx) && n->titlecols > n->secondarycols){
      ncplane_putegc_yx(n->ncp, 2, dimx - (n->titlecols + 4), "┴", NULL);
    }
  }
  // There is always at least one space available on the right for the
  // secondary title and footer, but we'd prefer to use a few more if we can.
  if(n->secondary){
    int xloc = bodywidth - (n->secondarycols + 1) + xoff;
    if(n->secondarycols < bodywidth - 2){
      --xloc;
    }
    n->ncp->channels = n->footchannels;
    ncplane_putstr_yx(n->ncp, yoff, xloc, n->secondary);
  }
  if(n->footer){
    int xloc = bodywidth - (n->footercols + 1) + xoff;
    if(n->footercols < bodywidth - 2){
      --xloc;
    }
    n->ncp->channels = n->footchannels;
    ncplane_putstr_yx(n->ncp, dimy - 1, xloc, n->footer);
  }
  // Top line of body (background and possibly up arrow)
  ++yoff;
  ncplane_cursor_move_yx(n->ncp, yoff, xoff + 1);
  for(int i = xoff + 1 ; i < dimx - 1 ; ++i){
    ncplane_putc(n->ncp, &n->background);
  }
  const int bodyoffset = dimx - bodywidth + 2;
  if(n->maxdisplay && n->maxdisplay < n->itemcount){
    n->ncp->channels = n->descchannels;
    n->arrowx = bodyoffset + n->longop;
    ncplane_putegc_yx(n->ncp, yoff, n->arrowx, "↑", NULL);
  }else{
    n->arrowx = -1;
  }
  n->uarrowy = yoff;
  unsigned printidx = n->startdisp;
  unsigned printed = 0;
  for(yoff += 1 ; yoff < dimy - 2 ; ++yoff){
    if(n->maxdisplay && printed == n->maxdisplay){
      break;
    }
    ncplane_cursor_move_yx(n->ncp, yoff, xoff + 1);
    for(int i = xoff + 1 ; i < dimx - 1 ; ++i){
      ncplane_putc(n->ncp, &n->background);
    }
    n->ncp->channels = n->opchannels;
    if(printidx == n->selected){
      n->ncp->channels = (uint64_t)channels_bchannel(n->opchannels) << 32u | channels_fchannel(n->opchannels);
    }
    ncplane_printf_yx(n->ncp, yoff, bodyoffset + (n->longop - n->items[printidx].opcolumns), "%s", n->items[printidx].option);
    n->ncp->channels = n->descchannels;
    if(printidx == n->selected){
      n->ncp->channels = (uint64_t)channels_bchannel(n->descchannels) << 32u | channels_fchannel(n->descchannels);
    }
    ncplane_printf_yx(n->ncp, yoff, bodyoffset + n->longop, " %s", n->items[printidx].desc);
    if(++printidx == n->itemcount){
      printidx = 0;
    }
    ++printed;
  }
  // Bottom line of body (background and possibly down arrow)
  ncplane_cursor_move_yx(n->ncp, yoff, xoff + 1);
  for(int i = xoff + 1 ; i < dimx - 1 ; ++i){
    ncplane_putc(n->ncp, &n->background);
  }
  if(n->maxdisplay && n->maxdisplay < n->itemcount){
    n->ncp->channels = n->descchannels;
    ncplane_putegc_yx(n->ncp, yoff, n->arrowx, "↓", NULL);
  }
  n->darrowy = yoff;
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
  if(n->titlecols + 4 > cols){
    cols = n->titlecols + 4;
  }
  if(cols > dimx){ // insufficient width to display selector
    return -1;
  }
  *ncdimx = cols;
  return 0;
}

ncselector* ncselector_create(ncplane* n, int y, int x, const selector_options* opts){
  if(opts->defidx && opts->defidx >= opts->itemcount){
    return NULL;
  }
  ncselector* ns = malloc(sizeof(*ns));
  ns->title = opts->title ? strdup(opts->title) : NULL;
  ns->titlecols = opts->title ? mbswidth(opts->title) : 0;
  ns->secondary = opts->secondary ? strdup(opts->secondary) : NULL;
  ns->secondarycols = opts->secondary ? mbswidth(opts->secondary) : 0;
  ns->footer = opts->footer ? strdup(opts->footer) : NULL;
  ns->footercols = opts->footer ? mbswidth(opts->footer) : 0;
  ns->selected = opts->defidx;
  ns->startdisp = opts->defidx >= opts->maxdisplay ? opts->defidx - opts->maxdisplay + 1 : 0;
  ns->longop = 0;
  ns->maxdisplay = opts->maxdisplay;
  ns->longdesc = 0;
  ns->opchannels = opts->opchannels;
  ns->boxchannels = opts->boxchannels;
  ns->descchannels = opts->descchannels;
  ns->titlechannels = opts->titlechannels;
  ns->footchannels = opts->footchannels;
  ns->boxchannels = opts->boxchannels;
  ns->darrowy = ns->uarrowy = ns->arrowx = -1;
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
    int cols = mbswidth(src->option);
    ns->items[ns->itemcount].opcolumns = cols;
    if(cols > ns->longop){
      ns->longop = cols;
    }
    cols = mbswidth(src->desc);
    ns->items[ns->itemcount].desccolumns = cols;
    if(cols > ns->longdesc){
      ns->longdesc = cols;
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
  cell_init(&ns->background);
  uint64_t transchan = 0;
  channels_set_fg_alpha(&transchan, CELL_ALPHA_TRANSPARENT);
  channels_set_bg_alpha(&transchan, CELL_ALPHA_TRANSPARENT);
  ncplane_set_base(ns->ncp, transchan, 0, "");
  if(cell_prime(ns->ncp, &ns->background, " ", 0, opts->bgchannels) < 0){
    ncplane_destroy(ns->ncp);
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

const char* ncselector_selected(const ncselector* n){
  if(n->itemcount == 0){
    return NULL;
  }
  return n->items[n->selected].option;
}

const char* ncselector_previtem(ncselector* n){
  const char* ret = NULL;
  if(n->itemcount == 0){
    return ret;
  }
  if(n->selected == n->startdisp){
    if(n->startdisp-- == 0){
      n->startdisp = n->itemcount - 1;
    }
  }
  if(n->selected == 0){
    n->selected = n->itemcount;
  }
  --n->selected;
  ret = n->items[n->selected].option;
  ncselector_draw(n);
  return ret;
}

const char* ncselector_nextitem(ncselector* n){
  const char* ret = NULL;
  if(n->itemcount == 0){
    return NULL;
  }
  unsigned lastdisp = n->startdisp;
  lastdisp += n->maxdisplay && n->maxdisplay < n->itemcount ? n->maxdisplay : n->itemcount;
  --lastdisp;
  lastdisp %= n->itemcount;
  if(lastdisp == n->selected){
    if(++n->startdisp == n->itemcount){
      n->startdisp = 0;
    }
  }
  ++n->selected;
  if(n->selected == n->itemcount){
    n->selected = 0;
  }
  ret = n->items[n->selected].option;
  ncselector_draw(n);
  return ret;
}

bool ncselector_offer_input(ncselector* n, const ncinput* nc){
  if(nc->id == NCKEY_UP){
    ncselector_previtem(n);
    return true;
  }else if(nc->id == NCKEY_DOWN){
    ncselector_nextitem(n);
    return true;
  }else if(nc->id == NCKEY_SCROLL_UP){
    ncselector_previtem(n);
    return true;
  }else if(nc->id == NCKEY_SCROLL_DOWN){
    ncselector_nextitem(n);
    return true;
  }else if(nc->id == NCKEY_RELEASE && ncplane_mouseevent_p(n->ncp, nc)){
    int y = nc->y, x = nc->x;
    ncplane_translate(ncplane_stdplane(n->ncp), n->ncp, &y, &x);
    if(y == n->uarrowy && x == n->arrowx){
      ncselector_previtem(n);
      return true;
    }else if(y == n->darrowy && x == n->arrowx){
      ncselector_nextitem(n);
      return true;
    }else if(n->uarrowy < y && y < n->darrowy){
      // FIXME we probably only want to consider it a click if both the release
      // and the depress happened to be on us. for now, just check release.
      // FIXME verify that we're within the body walls!
      // FIXME verify we're on the left of the split?
      // FIXME verify that we're on a visible glyph?
      int cury =  (n->selected + n->itemcount - n->startdisp) % n->itemcount;
      int click = y - n->uarrowy - 1;
      while(click > cury){
        ncselector_nextitem(n);
        ++cury;
      }
      while(click < cury){
        ncselector_previtem(n);
        --cury;
      }
      return true;
    }
  }
  return false;
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
    cell_release(n->ncp, &n->background);
    ncplane_destroy(n->ncp);
    free(n->items);
    free(n->title);
    free(n->secondary);
    free(n->footer);
    free(n);
  }
}
