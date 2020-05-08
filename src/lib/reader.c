#include "internal.h"

ncreader* ncreader_create(notcurses* nc, int y, int x, const ncreader_options* opts){
  if(opts->physrows <= 0 || opts->physcols <= 0){
    return NULL;
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
    nr->contents = strdup("");
    // FIXME
  }
  return nr;
}

// empty the ncreader of any user input, and home the cursor.
int ncreader_clear(ncreader* n){
  free(n->contents);
  ncplane_erase(n->ncp);
  return ncplane_cursor_move_yx(n->ncp, 0, 0);
}

ncplane* ncreader_plane(ncreader* n){
  return n->ncp;
}

bool ncreader_offer_input(ncreader* n, const ncinput* ni){
  if(nckey_supppuab_p(ni->id)){
    return false;
  }
  // FIXME add ni to n->content
  return true;
}

char* ncreader_contents(const ncreader* n){
  return strdup(n->contents);
}

void ncreader_destroy(ncreader* n, char** contents){
  if(n){
    if(contents){
      *contents = strdup(n->contents);
    }
    free(n->contents);
    ncplane_destroy(n->ncp);
    free(n);
  }
}
