#include "internal.h"

ncreader* ncreader_create(notcurses* nc, int y, int x, const ncreader_options* opts){
  if(opts->physrows <= 0 || opts->physcols <= 0){
    return NULL;
  }
  if(opts->egc == NULL){
    return NULL;
  }
  ncreader* nr = malloc(sizeof(*nr));
  if(nr){
    nr->ncp = ncplane_new(nc, opts->physrows, opts->physcols, y, x, NULL);
    if(!nr->ncp){
      free(nr);
      return NULL;
    }
    if(ncplane_set_base(nr->ncp, opts->egc, opts->eattrword, opts->echannels) <= 0){
      ncreader_destroy(nr);
      return NULL;
    }
    // FIXME
  }
  return nr;
}

// empty the ncreader of any user input, and home the cursor.
int ncreader_clear(ncreader* n){
  ncplane_erase(n->ncp);
  return ncplane_cursor_move_yx(n->ncp, 0, 0);
}

ncplane* ncreader_plane(ncreader* n){
  return n->ncp;
}

void ncreader_destroy(ncreader* n){
  if(n){
    ncplane_destroy(n->ncp);
    free(n);
  }
}
