#include "internal.h"

ncreader* ncreader_create(notcurses* nc, int rows, int cols, int y, int x){
  ncreader* nr = malloc(sizeof(*nr));
  if(nr){
    // FIXME
  }
  return nr;
}

// empty the ncreader of any user input, and home the cursor.
int ncreader_clear(ncreader* n){
  // FIXME
  return 0;
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
