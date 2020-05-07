#include "internal.h"

ncreader* ncreader_create(int rows, int cols, int y, int x){
  // FIXME
}

// empty the ncreader of any user input, and home the cursor.
int ncreader_clear(ncreader* n){
  // FIXME
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
