#include "cpp.h"

typedef struct ncplot {
  ncppplot<uint64_t>* n;
} ncplot;

extern "C" {

ncplot* ncplot_create(ncplane* n, const ncplot_options* opts){
  auto ret = new ncplot;
  if(ret){
    if( (ret->n = ncppplot<uint64_t>::create(n, opts)) ){
      return ret;
    }
    free(ret);
  }
  return nullptr;
}

ncplane* ncplot_plane(ncplot* n){
  return n->n->ncp;
}

int ncplot_add_sample(ncplot* n, uint64_t x, uint64_t y){
  return n->n->add_sample(x, y);
}

int ncplot_set_sample(ncplot* n, uint64_t x, uint64_t y){
  return n->n->set_sample(x, y);
}

void ncplot_destroy(ncplot* n){
  if(n){
    n->n->destroy();
    delete n;
  }
}

}
