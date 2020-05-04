#include "internal.h"

// ncvisual functionality independent of the underlying engine

ncvisual* ncvisual_from_plane(ncplane* n){
  int dimy, dimx;
  ncplane_dim_yx(n, &dimy, &dimx);
  ncvisual* ncv = ncvisual_create(1);
  return ncv;
}
