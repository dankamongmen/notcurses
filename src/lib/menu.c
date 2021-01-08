#include "internal.h"

static void
free_menu_section(ncmenu_int_section* ms){
  for(int i = 0 ; i < ms->itemcount ; ++i){
    free(ms->items[i].desc);
    free(ms->items[i].shortdesc);
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
dup_menu_item(ncmenu_int_item* dst, const struct ncmenu_item* src){
#define ALTMOD "Alt+"
#define CTLMOD "Ctrl+"
  dst->disabled = false;
  if((dst->desc = strdup(src->desc)) == NULL){
    return -1;
  }
  if(!src->shortcut.id){
    dst->shortdesccols = 0;
    dst->shortdesc = NULL;
    return 0;
  }
  size_t bytes = 1; // NUL terminator
  if(src->shortcut.alt){
    bytes += strlen(ALTMOD);
  }
  if(src->shortcut.ctrl){
    bytes += strlen(CTLMOD);
  }
  mbstate_t ps;
  memset(&ps, 0, sizeof(ps));
  size_t shortsize = wcrtomb(NULL, src->shortcut.id, &ps);
  if(shortsize == (size_t)-1){
    free(dst->desc);
    return -1;
  }
  bytes += shortsize + 1;
  char* sdup = malloc(bytes);
  int n = snprintf(sdup, bytes, "%s%s", src->shortcut.alt ? ALTMOD : "",
                   src->shortcut.ctrl ? CTLMOD : "");
  if(n < 0 || (size_t)n >= bytes){
    free(sdup);
    free(dst->desc);
    return -1;
  }
  memset(&ps, 0, sizeof(ps));
  size_t mbbytes = wcrtomb(sdup + n, src->shortcut.id, &ps);
  if(mbbytes == (size_t)-1){ // shouldn't happen
    free(sdup);
    free(dst->desc);
    return -1;
  }
  sdup[n + mbbytes] = '\0';
  dst->shortdesc = sdup;
  dst->shortdesccols = ncstrwidth(dst->shortdesc);
  return 0;
#undef CTLMOD
#undef ALTMOD
}

static int
dup_menu_section(ncmenu_int_section* dst, const struct ncmenu_section* src){
  // we must reject any empty section
  if(src->itemcount == 0 || src->items == NULL){
    return -1;
  }
  dst->bodycols = 0;
  dst->itemselected = 0;
  dst->items = NULL;
  // we must reject any section which is entirely separators
  bool gotitem = false;
  dst->itemcount = 0;
  dst->enabled_item_count = 0;
  dst->items = malloc(sizeof(*dst->items) * src->itemcount);
  if(dst->items == NULL){
    return -1;
  }
  for(int i = 0 ; i < src->itemcount ; ++i){
    if(src->items[i].desc){
      if(dup_menu_item(&dst->items[i], &src->items[i])){
        while(i--){
          free(dst->items[i].desc);
        }
        free(dst->items);
        return -1;
      }
      gotitem = true;
      int cols = ncstrwidth(dst->items[i].desc);
      if(dst->items[i].shortdesc){
        cols += 2 + dst->items[i].shortdesccols; // two spaces minimum
      }
      if(cols > dst->bodycols){
        dst->bodycols = cols;
      }
      memcpy(&dst->items[i].shortcut, &src->items[i].shortcut, sizeof(dst->items[i].shortcut));
      if(mbstr_find_codepoint(dst->items[i].desc,
                              dst->items[i].shortcut.id,
                              &dst->items[i].shortcut_offset) < 0){
        dst->items[i].shortcut_offset = -1;
      }
    }else{
      dst->items[i].desc = NULL;
      dst->items[i].shortdesc = NULL;
    }
    ++dst->itemcount;
  }
  dst->enabled_item_count = dst->itemcount;
  if(!gotitem){
    while(dst->itemcount){
      free(dst->items[--dst->itemcount].desc);
    }
    free(dst->items);
    return -1;
  }
  return 0;
}

// Duplicates all menu sections in opts, adding their length to '*totalwidth'.
static int
dup_menu_sections(ncmenu* ncm, const ncmenu_options* opts, int* totalwidth, int* totalheight){
  if(opts->sectioncount == 0){
    return -1;
  }
  ncm->sections = malloc(sizeof(*ncm->sections) * opts->sectioncount);
  if(ncm->sections == NULL){
    return -1;
  }
  bool rightaligned = false; // can only right-align once. twice is error.
  int maxheight = 0;
  int maxwidth = *totalwidth;
  int xoff = 2;
  int i;
  for(i = 0 ; i < opts->sectioncount ; ++i){
    if(opts->sections[i].name){
      int cols = ncstrwidth(opts->sections[i].name);
      if(rightaligned){ // FIXME handle more than one right-aligned section
        ncm->sections[i].xoff = -(cols + 2);
      }else{
        ncm->sections[i].xoff = xoff;
      }
      if(cols < 0 || (ncm->sections[i].name = strdup(opts->sections[i].name)) == NULL){
        goto err;
      }
      if(dup_menu_section(&ncm->sections[i], &opts->sections[i])){
        free(ncm->sections[i].name);
        goto err;
      }
      if(ncm->sections[i].itemcount > maxheight){
        maxheight = ncm->sections[i].itemcount;
      }
      if(*totalwidth + cols + 2 > maxwidth){
        maxwidth = *totalwidth + cols + 2;
      }
      if(*totalwidth + ncm->sections[i].bodycols + 2 > maxwidth){
        maxwidth = *totalwidth + ncm->sections[i].bodycols + 2;
      }
      *totalwidth += cols + 2;
      memcpy(&ncm->sections[i].shortcut, &opts->sections[i].shortcut, sizeof(ncm->sections[i].shortcut));
      if(mbstr_find_codepoint(ncm->sections[i].name,
                              ncm->sections[i].shortcut.id,
                              &ncm->sections[i].shortcut_offset) < 0){
        ncm->sections[i].shortcut_offset = -1;
      }
      xoff += cols + 2;
    }else{ // divider; remaining sections are right-aligned
      if(rightaligned){
        goto err;
      }
      rightaligned = true;
      ncm->sections[i].name = NULL;
      ncm->sections[i].items = NULL;
      ncm->sections[i].itemcount = 0;
      ncm->sections[i].xoff = -1;
      ncm->sections[i].bodycols = 0;
      ncm->sections[i].itemselected = -1;
      ncm->sections[i].shortcut_offset = -1;
      ncm->sections[i].enabled_item_count = 0;
    }
  }
  if(ncm->sectioncount == 1 && rightaligned){
    goto err;
  }
  *totalwidth = maxwidth;
  *totalheight += maxheight + 2; // two rows of border
  return 0;

err:
  while(i--){
    free_menu_section(&ncm->sections[i]);
  }
  return -1;
}

// what section header, if any, is living at the provided x coordinate? solves
// by replaying the write_header() algorithm. returns -1 if no such section.
static int
section_x(const ncmenu* ncm, int x){
  int dimx = ncplane_dim_x(ncm->ncp);
  for(int i = 0 ; i < ncm->sectioncount ; ++i){
    if(!ncm->sections[i].name){
      continue;
    }
    if(ncm->sections[i].xoff < 0){ // right-aligned
      int pos = dimx + ncm->sections[i].xoff;
      if(x < pos){
        break;
      }
      if(x < pos + ncstrwidth(ncm->sections[i].name)){
        return i;
      }
    }else{
      if(x < ncm->sections[i].xoff){
        break;
      }
      if(x < ncm->sections[i].xoff + ncstrwidth(ncm->sections[i].name)){
        return i;
      }
    }
  }
  return -1;
}

static int
write_header(ncmenu* ncm){
  ncplane_set_channels(ncm->ncp, ncm->headerchannels);
  int dimy, dimx;
  ncplane_dim_yx(ncm->ncp, &dimy, &dimx);
  int xoff = 0; // 2-column margin on left
  int ypos = ncm->bottom ? dimy - 1 : 0;
  if(ncplane_cursor_move_yx(ncm->ncp, ypos, 0)){
    return -1;
  }
  nccell c = CELL_INITIALIZER(' ', 0, ncm->headerchannels);
  ncplane_set_styles(ncm->ncp, 0);
  if(ncplane_putc(ncm->ncp, &c) < 0){
    return -1;
  }
  if(ncplane_putc(ncm->ncp, &c) < 0){
    return -1;
  }
  for(int i = 0 ; i < ncm->sectioncount ; ++i){
    if(ncm->sections[i].name){
      ncplane_cursor_move_yx(ncm->ncp, ypos, xoff);
      int spaces = ncm->sections[i].xoff - xoff;
      if(ncm->sections[i].xoff < 0){ // right-aligned
        spaces = dimx + ncm->sections[i].xoff - xoff;
        if(spaces < 0){
          spaces = 0;
        }
      }
      xoff += spaces;
      while(spaces--){
        if(ncplane_putc(ncm->ncp, &c) < 0){
          return -1;
        }
      }
      if(ncm->sections[i].enabled_item_count <= 0){
        ncplane_set_channels(ncm->ncp, ncm->dissectchannels);
      }else{
        ncplane_set_channels(ncm->ncp, ncm->headerchannels);
      }
      if(ncplane_putstr_yx(ncm->ncp, ypos, xoff, ncm->sections[i].name) < 0){
        return -1;
      }
      if(ncm->sections[i].shortcut_offset >= 0){
        nccell cl = CELL_TRIVIAL_INITIALIZER;
        if(ncplane_at_yx_cell(ncm->ncp, ypos, xoff + ncm->sections[i].shortcut_offset, &cl) < 0){
          return -1;
        }
        cell_on_styles(&cl, NCSTYLE_UNDERLINE|NCSTYLE_BOLD);
        if(ncplane_putc_yx(ncm->ncp, ypos, xoff + ncm->sections[i].shortcut_offset, &cl) < 0){
          return -1;
        }
        cell_release(ncm->ncp, &cl);
      }
      xoff += ncstrwidth(ncm->sections[i].name);
    }
  }
  while(xoff < dimx){
    if(ncplane_putc_yx(ncm->ncp, ypos, xoff, &c) < 0){
      return -1;
    }
    ++xoff;
  }
  return 0;
}

static int
resize_menu(ncplane* n){
  const ncplane* parent = ncplane_parent_const(n);
  int dimx = ncplane_dim_x(parent);
  int dimy = ncplane_dim_y(n);
  if(ncplane_resize_simple(n, dimy, dimx)){
    return -1;
  }
  ncmenu* menu = ncplane_userptr(n);
  int unrolled = menu->unrolledsection;
  if(unrolled < 0){
    return write_header(menu);
  }
  ncplane_erase(n); // "rolls up" section without resetting unrolledsection
  return ncmenu_unroll(menu, unrolled);
}

ncmenu* ncmenu_create(ncplane* n, const ncmenu_options* opts){
  ncmenu_options zeroed = {};
  if(!opts){
    opts = &zeroed;
  }
  if(opts->sectioncount <= 0 || !opts->sections){
    logerror(ncplane_notcurses(n), "Invalid %d-ary section information\n", opts->sectioncount);
    return NULL;
  }
  if(opts->flags >= (NCMENU_OPTION_HIDING << 1u)){
    logwarn(ncplane_notcurses(n), "Provided unsupported flags %016jx\n", (uintmax_t)opts->flags);
  }
  int totalheight = 1;
  int totalwidth = 2; // start with two-character margin on the left
  ncmenu* ret = malloc(sizeof(*ret));
  ret->sectioncount = opts->sectioncount;
  ret->sections = NULL;
  int dimy, dimx;
  ncplane_dim_yx(notcurses_stdplane(ncplane_notcurses(n)), &dimy, &dimx);
  if(ret){
    ret->bottom = !!(opts->flags & NCMENU_OPTION_BOTTOM);
    if(dup_menu_sections(ret, opts, &totalwidth, &totalheight) == 0){
      ret->headerwidth = totalwidth;
      if(totalwidth < dimx){
        totalwidth = dimx;
      }
      struct ncplane_options nopts = {
        .y = ret->bottom ? dimy - totalheight : 0,
        .x = 0,
        .rows = totalheight,
        .cols = totalwidth,
        .userptr = ret,
        .name = "menu",
        .resizecb = resize_menu,
      };
      ret->ncp = ncplane_create(n, &nopts);
      if(ret->ncp){
        ret->unrolledsection = -1;
        ret->headerchannels = opts->headerchannels;
        ret->dissectchannels = opts->headerchannels;
        channels_set_fg_rgb(&ret->dissectchannels, 0xdddddd);
        ret->sectionchannels = opts->sectionchannels;
        ret->disablechannels = ret->sectionchannels;
        channels_set_fg_rgb(&ret->disablechannels, 0xdddddd);
        nccell c = CELL_TRIVIAL_INITIALIZER;
        cell_set_fg_alpha(&c, CELL_ALPHA_TRANSPARENT);
        cell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT);
        ncplane_set_base_cell(ret->ncp, &c);
        cell_release(ret->ncp, &c);
        if(write_header(ret) == 0){
          return ret;
        }
        ncplane_destroy(ret->ncp);
      }
      free_menu_sections(ret);
    }
    free(ret);
  }
  logerror(ncplane_notcurses(n), "Error creating ncmenu\n");
  return NULL;
}

static inline int
section_height(const ncmenu* n, int sectionidx){
  return n->sections[sectionidx].itemcount + 2;
}

static inline int
section_width(const ncmenu* n, int sectionidx){
  return n->sections[sectionidx].bodycols + 2;
}

int ncmenu_unroll(ncmenu* n, int sectionidx){
  if(ncmenu_rollup(n)){ // roll up any unrolled section
    return -1;
  }
  if(sectionidx < 0 || sectionidx >= n->sectioncount){
    logerror(ncplane_notcurses(n->ncp), "Unrolled invalid sectionidx %d\n", sectionidx);
    return -1;
  }
  if(n->sections[sectionidx].enabled_item_count <= 0){
    return 0;
  }
  if(n->sections[sectionidx].name == NULL){
    return -1;
  }
  n->unrolledsection = sectionidx;
  int dimy, dimx;
  ncplane_dim_yx(n->ncp, &dimy, &dimx);
  const int height = section_height(n, sectionidx);
  const int width = section_width(n, sectionidx);
  int xpos = n->sections[sectionidx].xoff < 0 ?
    dimx + (n->sections[sectionidx].xoff - 2) : n->sections[sectionidx].xoff;
  if(xpos + width >= dimx){
    xpos = dimx - (width + 2);
  }
  int ypos = n->bottom ? dimy - height - 1 : 1;
  if(ncplane_cursor_move_yx(n->ncp, ypos, xpos)){
    return -1;
  }
  if(ncplane_rounded_box_sized(n->ncp, 0, n->headerchannels, height, width, 0)){
    return -1;
  }
  const ncmenu_int_section* sec = &n->sections[sectionidx];
  for(int i = 0 ; i < sec->itemcount ; ++i){
    ++ypos;
    if(sec->items[i].desc){
      // FIXME the user ought be able to configure the disabled channel
      if(!sec->items[i].disabled){
        ncplane_set_channels(n->ncp, n->sectionchannels);
      }else{
        ncplane_set_channels(n->ncp, n->disablechannels);
      }
      if(i == sec->itemselected){
        ncplane_set_styles(n->ncp, NCSTYLE_REVERSE);
      }else{
        ncplane_set_styles(n->ncp, 0);
      }
      int cols = ncplane_putstr_yx(n->ncp, ypos, xpos + 1, sec->items[i].desc);
      if(cols < 0){
        return -1;
      }
      // we need pad out the remaining columns of this line with spaces. if
      // there's a shortcut description, we align it to the right, printing
      // spaces only through the start of the aligned description.
      int thiswidth = width;
      if(sec->items[i].shortdesc){
        thiswidth -= sec->items[i].shortdesccols;
      }
      // print any necessary padding spaces
      for(int j = cols + 1 ; j < thiswidth - 1 ; ++j){
        if(ncplane_putchar(n->ncp, ' ') < 0){
          return -1;
        }
      }
      if(sec->items[i].shortdesc){
        if(ncplane_putstr(n->ncp, sec->items[i].shortdesc) < 0){
          return -1;
        }
      }
      if(sec->items[i].shortcut_offset >= 0){
        nccell cl = CELL_TRIVIAL_INITIALIZER;
        if(ncplane_at_yx_cell(n->ncp, ypos, xpos + 1 + sec->items[i].shortcut_offset, &cl) < 0){
          return -1;
        }
        cell_on_styles(&cl, NCSTYLE_UNDERLINE|NCSTYLE_BOLD);
        if(ncplane_putc_yx(n->ncp, ypos, xpos + 1 + sec->items[i].shortcut_offset, &cl) < 0){
          return -1;
        }
        cell_release(n->ncp, &cl);
      }
    }else{
      n->ncp->channels = n->headerchannels;
      ncplane_set_styles(n->ncp, 0);
      if(ncplane_putegc_yx(n->ncp, ypos, xpos, "├", NULL) < 0){
        return -1;
      }
      for(int j = 1 ; j < width - 1 ; ++j){
        if(ncplane_putegc(n->ncp, "─", NULL) < 0){
          return -1;
        }
      }
      if(ncplane_putegc(n->ncp, "┤", NULL) < 0){
        return -1;
      }
    }
  }
  return 0;
}

int ncmenu_rollup(ncmenu* n){
  if(n->unrolledsection < 0){
    return 0;
  }
  n->unrolledsection = -1;
  ncplane_erase(n->ncp);
  return write_header(n);
}

int ncmenu_nextsection(ncmenu* n){
  int nextsection = n->unrolledsection;
  // FIXME probably best to detect cycles
  do{
    if(++nextsection == n->sectioncount){
      nextsection = 0;
    }
  }while(n->sections[nextsection].name == NULL ||
         n->sections[nextsection].enabled_item_count == 0);
  return ncmenu_unroll(n, nextsection);
}

int ncmenu_prevsection(ncmenu* n){
  int prevsection = n->unrolledsection;
  // FIXME probably best to detect cycles
  do{
    if(--prevsection < 0){
      prevsection = n->sectioncount - 1;
    }
  }while(n->sections[prevsection].name == NULL ||
         n->sections[prevsection].enabled_item_count == 0);
  return ncmenu_unroll(n, prevsection);
}

int ncmenu_nextitem(ncmenu* n){
  if(n->unrolledsection == -1){
    if(ncmenu_unroll(n, 0)){
      return -1;
    }
  }
  ncmenu_int_section* sec = &n->sections[n->unrolledsection];
  // FIXME probably best to detect cycles
  do{
    if(++sec->itemselected == sec->itemcount){
      sec->itemselected = 0;
    }
  }while(!sec->items[sec->itemselected].desc || sec->items[sec->itemselected].disabled);
  return ncmenu_unroll(n, n->unrolledsection);
}

int ncmenu_previtem(ncmenu* n){
  if(n->unrolledsection == -1){
    if(ncmenu_unroll(n, 0)){
      return -1;
    }
  }
  ncmenu_int_section* sec = &n->sections[n->unrolledsection];
  // FIXME probably best to detect cycles
  do{
    if(sec->itemselected-- == 0){
      sec->itemselected = sec->itemcount - 1;
    }
  }while(!sec->items[sec->itemselected].desc || sec->items[sec->itemselected].disabled);
  return ncmenu_unroll(n, n->unrolledsection);
}

const char* ncmenu_selected(const ncmenu* n, ncinput* ni){
  if(n->unrolledsection < 0){
    return NULL;
  }
  const struct ncmenu_int_section* sec = &n->sections[n->unrolledsection];
  const int itemidx = sec->itemselected;
  if(ni){
    memcpy(ni, &sec->items[itemidx].shortcut, sizeof(*ni));
  }
  return sec->items[itemidx].desc;
}

const char* ncmenu_mouse_selected(const ncmenu* n, const ncinput* click,
                                  ncinput* ni){
  if(click->id != NCKEY_RELEASE){
    return NULL;
  }
  int y, x, dimy, dimx;
  struct ncplane* nc = n->ncp;
  y = click->y;
  x = click->x;
  ncplane_dim_yx(nc, &dimy, &dimx);
  if(!ncplane_translate_abs(nc, &y, &x)){
    return NULL;
  }
  // FIXME section_x() works only off the section header lengths, meaning that
  // if we click an item outside of those columns covered by the header, it will
  // read as a -1 from section_x(). we want to instead get the unrolled section,
  // find its boundaries, and verify that we are within them.
  int i = section_x(n, x);
  if(i < 0 || i != n->unrolledsection){
    return NULL;
  }
  const struct ncmenu_int_section* sec = &n->sections[n->unrolledsection];
  if(y < 2 || y - 2 >= sec->itemcount){
    return NULL;
  }
  const int itemidx = y - 2;
  if(ni){
    memcpy(ni, &sec->items[itemidx].shortcut, sizeof(*ni));
  }
  return sec->items[itemidx].desc;
}

bool ncmenu_offer_input(ncmenu* n, const ncinput* nc){
  // we can't actually select menu items in this function, since we need to
  // invoke an arbitrary function as a result.
  if(nc->id == NCKEY_RELEASE){
    int y, x, dimy, dimx;
    y = nc->y;
    x = nc->x;
    ncplane_dim_yx(n->ncp, &dimy, &dimx);
    if(!ncplane_translate_abs(n->ncp, &y, &x)){
      return false;
    }
    if(y != (n->bottom ? dimy - 1 : 0)){
      return false;
    }
    int i = section_x(n, x);
    if(i < 0 || i == n->unrolledsection){
      ncmenu_rollup(n);
    }else{
      ncmenu_unroll(n, i);
    }
    return true;
  }
  for(int si = 0 ; si < n->sectioncount ; ++si){
    const ncmenu_int_section* sec = &n->sections[si];
    if(sec->enabled_item_count == 0){
      continue;
    }
    if(!ncinput_equal_p(&sec->shortcut, nc)){
      continue;
    }
    ncmenu_unroll(n, si);
    return true;
  }
  if(n->unrolledsection < 0){ // all following need an unrolled section
    return false;
  }
  if(nc->id == NCKEY_LEFT){
    if(ncmenu_prevsection(n)){
      return false;
    }
    return true;
  }else if(nc->id == NCKEY_RIGHT){
    if(ncmenu_nextsection(n)){
      return false;
    }
    return true;
  }else if(nc->id == NCKEY_UP || nc->id == NCKEY_SCROLL_UP){
    if(ncmenu_previtem(n)){
      return false;
    }
    return true;
  }else if(nc->id == NCKEY_DOWN || nc->id == NCKEY_SCROLL_DOWN){
    if(ncmenu_nextitem(n)){
      return false;
    }
    return true;
  }else if(nc->id == NCKEY_ESC){
    ncmenu_rollup(n);
    return true;
  }
  return false;
}

// FIXME we probably ought implement this with a trie or something
int ncmenu_item_set_status(ncmenu* n, const char* section, const char* item,
                           bool enabled){
  for(int si = 0 ; si < n->sectioncount ; ++si){
    struct ncmenu_int_section* sec = &n->sections[si];
    if(strcmp(sec->name, section) == 0){
      for(int ii = 0 ; ii < sec->itemcount ; ++ii){
        struct ncmenu_int_item* i = &sec->items[ii];
        if(strcmp(i->desc, item) == 0){
          const bool changed = i->disabled == enabled;
          i->disabled = !enabled;
          if(changed){
            if(i->disabled){
              if(--sec->enabled_item_count == 0){
                write_header(n);
              }
            }else{
              if(++sec->enabled_item_count == 1){
                write_header(n);
              }
            }
            if(n->unrolledsection == si){
              if(sec->enabled_item_count == 0){
                ncmenu_rollup(n);
              }else{
                ncmenu_unroll(n, n->unrolledsection);
              }
            }
          }
          return 0;
        }
      }
      break;
    }
  }
  return -1;
}

ncplane* ncmenu_plane(ncmenu* menu){
  return menu->ncp;
}

int ncmenu_destroy(ncmenu* n){
  int ret = 0;
  if(n){
    free_menu_sections(n);
    ncplane_destroy(n->ncp);
    free(n);
  }
  return ret;
}
