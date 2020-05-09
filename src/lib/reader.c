#include "internal.h"

ncreader* ncreader_create(notcurses* nc, int y, int x, const ncreader_options* opts){
  if(opts->physrows <= 0 || opts->physcols <= 0){
    return NULL;
  }
  if(opts->scroll){
    return NULL; // FIXME not yet handled
  }
  ncreader* nr = malloc(sizeof(*nr));
  if(nr){
    nr->ncp = ncplane_new(nc, opts->physrows, opts->physcols, y, x, NULL);
    if(!nr->ncp){
      free(nr);
      return NULL;
    }
    const char* egc = opts->egc ? opts->egc : "_";
    if(ncplane_set_base(nr->ncp, egc, opts->eattrword, opts->echannels) <= 0){
      ncreader_destroy(nr, NULL);
      return NULL;
    }
    nr->tchannels = opts->tchannels;
    nr->tattrs = opts->tattrword;
    ncplane_set_channels(nr->ncp, opts->tchannels);
    ncplane_set_attr(nr->ncp, opts->tattrword);
  }
  return nr;
}

// empty the ncreader of any user input, and home the cursor.
int ncreader_clear(ncreader* n){
  ncplane_erase(n->ncp);
  return 0;
}

ncplane* ncreader_plane(ncreader* n){
  return n->ncp;
}

// we pass along:
//  * anything with Alt
//  * anything with Ctrl, except 'U' (which clears all input)
//  * anything synthesized, save arrow keys and backspace
bool ncreader_offer_input(ncreader* n, const ncinput* ni){
  int x = n->ncp->x;
  int y = n->ncp->y;
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
    if(n->ncp->x == 0){
      if(n->ncp->y){
        y = n->ncp->y - 1;
        x = n->ncp->lenx - 1;
      }
    }else{
      --x;
    }
    ncplane_putegc_yx(n->ncp, y, x, "", NULL);
    ncplane_cursor_move_yx(n->ncp, y, x);
    return true;
  }
  // FIXME deal with multicolumn EGCs -- probably extract these and make them
  // general ncplane_cursor_{left, right, up, down}()
  if(ni->id == NCKEY_LEFT){
    if(x == 0){
      if(y){
        x = n->ncp->lenx - 1;
        --y;
      }
    }else{
      --x;
    }
    ncplane_cursor_move_yx(n->ncp, y, x);
    return true;
  }else if(ni->id == NCKEY_RIGHT){
    if(x == n->ncp->lenx - 1){
      if(y < n->ncp->leny - 1){
        ++y;
        x = 0;
      }
    }else{
      ++x;
    }
    ncplane_cursor_move_yx(n->ncp, y, x);
    return true;
  }else if(ni->id == NCKEY_UP){
    if(y == 0){
      x = 0;
    }else{
      --y;
    }
    ncplane_cursor_move_yx(n->ncp, y, x);
    return true;
  }else if(ni->id == NCKEY_DOWN){
    if(y >= n->ncp->leny){
      x = n->ncp->lenx - 1;
    }else{
      ++y;
    }
    ncplane_cursor_move_yx(n->ncp, y, x);
    return true;
  }else if(nckey_supppuab_p(ni->id)){
    return false;
  }
  // FIXME need to collect full EGCs
  char wbuf[WCHAR_MAX_UTF8BYTES + 1];
  // FIXME breaks for wint_t < 32bits
  if(snprintf(wbuf, sizeof(wbuf), "%lc", (wint_t)ni->id) < (int)sizeof(wbuf)){
    if(ncplane_putegc(n->ncp, wbuf, NULL) > 0){
      if(n->ncp->x == n->ncp->lenx && n->ncp->y < n->ncp->leny - 1){
        ncplane_cursor_move_yx(n->ncp, n->ncp->y + 1, 0);
      }
    }
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
    ncplane_destroy(n->ncp);
    free(n);
  }
}
