#include "internal.h"

// find the center coordinate of a plane, preferring the top/left in the
// case of an even number of rows/columns (in such a case, there will be one
// more cell to the bottom/right of the center than the top/left).
void ncplane_center(const ncplane* n, int* y, int* x){
  if(y){
    *y = ((n->leny - 1) / 2);
  }
  if(x){
    *x = ((n->lenx - 1) / 2);
  }
}
