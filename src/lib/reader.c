#include "internal.h"

ncreader* ncreader_create(ncplane* n, int y, int x, const ncreader_options* opts){
  if(opts->physrows <= 0 || opts->physcols <= 0){
    logerror(n->nc, "Provided illegal geometry %dx%d\n", opts->physcols, opts->physrows);
    return NULL;
  }
  if(opts->flags > NCREADER_OPTION_HORSCROLL){
    logerror(n->nc, "Provided unsupported flags %016lx\n", opts->flags);
    return NULL;
  }
  ncreader* nr = malloc(sizeof(*nr));
  if(nr){
    nr->ncp = ncplane_new(n->nc, opts->physrows, opts->physcols, y, x, NULL);
    if(!nr->ncp){
      free(nr);
      return NULL;
    }
    // do *not* bind it to the visible plane; we always want it offscreen,
    // to the upper left of the true origin
    if((nr->textarea = ncplane_new(n->nc, opts->physrows, opts->physcols, -opts->physrows, -opts->physcols, NULL)) == NULL){
      ncplane_destroy(nr->ncp);
      free(nr);
      return NULL;
    }
    const char* egc = opts->egc ? opts->egc : "_";
    if(ncplane_set_base(nr->ncp, egc, opts->eattrword, opts->echannels) <= 0){
      ncreader_destroy(nr, NULL);
      return NULL;
    }
    nr->horscroll = opts->flags & NCREADER_OPTION_HORSCROLL;
    nr->xproject = 0;
    nr->tchannels = opts->tchannels;
    nr->tattrs = opts->tattrword;
    ncplane_set_channels(nr->ncp, opts->tchannels);
    ncplane_set_attr(nr->ncp, opts->tattrword);
  }
  return nr;
}

// empty both planes of all input, and home the cursors.
int ncreader_clear(ncreader* n){
  ncplane_erase(n->ncp);
  ncplane_erase(n->textarea);
  n->xproject = 0;
  return 0;
}

ncplane* ncreader_plane(ncreader* n){
  return n->ncp;
}

// copy the viewed area down from the textarea
static int
ncreader_redraw(ncreader* n){
  int ret = 0;
fprintf(stderr, "redraw: xproj %d\n", n->xproject);
notcurses_debug(n->ncp->nc, stderr);
  assert(n->xproject >= 0);
  assert(n->textarea->lenx >= n->ncp->lenx);
  assert(n->textarea->leny >= n->ncp->leny);
  for(int y = 0 ; y < n->ncp->leny ; ++y){
    const int texty = y;
    for(int x = 0 ; x < n->ncp->lenx ; ++x){
      const int textx = x + n->xproject;
      const cell* src = &n->textarea->fb[nfbcellidx(n->textarea, texty, textx)];
      cell* dst = &n->ncp->fb[nfbcellidx(n->ncp, y, x)];
fprintf(stderr, "projecting %d/%d [%s] to %d/%d [%s]\n", texty, textx, cell_extended_gcluster(n->textarea, src), y, x, cell_extended_gcluster(n->ncp, dst));
      if(cellcmp_and_dupfar(&n->ncp->pool, dst, n->textarea, src) < 0){
        ret = -1;
      }
    }
  }
  return ret;
}

// try to move left. does not move past the start of the textarea, but will
// try to move up and to the end of the previous row if not on the top row.
// if on the left side of the viewarea, but not the left side of the textarea,
// scrolls left. returns true if a move was made.
int ncreader_move_left(ncreader* n){
  int viewx = n->ncp->x;
  int textx = n->textarea->x;
  int y = n->ncp->y;
fprintf(stderr, "moving left: tcurs: %dx%d vcurs: %dx%d xproj: %d\n", y, textx, y, viewx, n->xproject);
  if(textx == 0){
    // are we on the first column of the textarea? if so, we must also be on
    // the first column of the viewarea. try to move up.
    if(y == 0){
      return -1; // no move possible
    }
    viewx = n->textarea->lenx - 1; // FIXME find end of particular row
    --y;
    textx = viewx;
  }else{
    // if we're on the first column of the viewarea, but not the first column
    // of the textarea, we must be able to scroll to the left. do so.
    // if we're not on the last column anywhere, move cursor right everywhere.
    if(viewx < n->ncp->leny - 1){
      ++viewx;
    }else{
      ++n->xproject;
    }
    ++textx;
  }
  ncplane_cursor_move_yx(n->textarea, y, textx);
  ncplane_cursor_move_yx(n->ncp, y, viewx);
fprintf(stderr, "moved right: tcurs: %dx%d vcurs: %dx%d xproj: %d\n", y, textx, y, viewx, n->xproject);
  return 0;
}

// only writing can enlarge the textarea. movement can pan, but not enlarge.
int ncreader_write_egc(ncreader* n, const char* egc){
  const int cols = mbswidth(egc);
  if(cols < 0){
    logerror(n->ncp->nc, "Fed illegal UTF-8 [%s]\n", egc);
    return -1;
  }
  if(n->textarea->x >= n->textarea->lenx - (cols + 1)){
    if(n->horscroll){
      // FIXME resize
    }
  }
  // use ncplane_putegc on both planes because it'll get cursor movement right
  if(ncplane_putegc(n->textarea, egc, NULL) < 0){
    return -1;
  }
  if(ncplane_putegc(n->ncp, egc, NULL) < 0){
    return -1;
  }
  // FIXME pan right if necessary
  return 0;
}

// we pass along:
//  * anything with Alt
//  * anything with Ctrl, except 'U' (which clears all input)
//  * anything synthesized, save arrow keys and backspace
bool ncreader_offer_input(ncreader* n, const ncinput* ni){
  int x = n->textarea->x;
  int y = n->textarea->y;
  if(ni->alt){ // pass on all alts
    return false;
  }
  if(ni->ctrl){
    if(ni->id == 'U'){
      ncplane_erase(n->ncp);
      return true;
    }
    return false; // pass on all other ctrls
  }
  if(ni->id == NCKEY_BACKSPACE){
    if(n->textarea->x == 0){
      if(n->textarea->y){
        y = n->textarea->y - 1;
        x = n->textarea->lenx - 1;
      }
    }else{
      --x;
    }
    ncplane_putegc_yx(n->textarea, y, x, "", NULL);
    ncplane_cursor_move_yx(n->textarea, y, x);
    ncreader_redraw(n);
    return true;
  }
  // FIXME deal with multicolumn EGCs -- probably extract these and make them
  // general ncplane_cursor_{left, right, up, down}()
  if(ni->id == NCKEY_LEFT){
    ncreader_move_left(n);
    ncreader_redraw(n);
    return true;
  }else if(ni->id == NCKEY_RIGHT){
    if(x == n->textarea->lenx - 1){
      if(y < n->textarea->leny - 1){
        ++y;
        x = 0;
      }
    }else{
      ++x;
    }
    ncplane_cursor_move_yx(n->textarea, y, x);
    ncreader_redraw(n);
    return true;
  }else if(ni->id == NCKEY_UP){
    if(y == 0){
      x = 0;
    }else{
      --y;
    }
    ncplane_cursor_move_yx(n->textarea, y, x);
    ncreader_redraw(n);
    return true;
  }else if(ni->id == NCKEY_DOWN){
    if(y >= n->textarea->leny){
      x = n->textarea->lenx - 1;
    }else{
      ++y;
    }
    ncplane_cursor_move_yx(n->textarea, y, x);
    ncreader_redraw(n);
    return true;
  }else if(nckey_supppuab_p(ni->id)){
    return false;
  }
  // FIXME need to collect full EGCs
  char wbuf[WCHAR_MAX_UTF8BYTES + 1];
  // FIXME breaks for wint_t < 32bits
  if(snprintf(wbuf, sizeof(wbuf), "%lc", (wint_t)ni->id) < (int)sizeof(wbuf)){
    ncreader_write_egc(n, wbuf);
    ncreader_redraw(n);
  }
  return true;
}

char* ncreader_contents(const ncreader* n){
  return ncplane_contents(n->ncp, 0, 0, -1, -1);
}

void ncreader_destroy(ncreader* n, char** contents){
  if(n){
    if(contents){
      *contents = ncreader_contents(n);
    }
    ncplane_destroy(n->textarea);
    ncplane_destroy(n->ncp);
    free(n);
  }
}
