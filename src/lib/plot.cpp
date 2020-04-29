#include "cpp.h"

using ncuplot = struct ncuplot {
  ncppplot<uint64_t> n;
};

using ncdplot = struct ncdplot {
  ncppplot<double> n;
};

extern "C" {

ncuplot* ncuplot_create(ncplane* n, const ncplot_options* opts, uint64_t miny, uint64_t maxy) {
  auto ret = new ncuplot;
  if(ret){
    if(ncppplot<uint64_t>::create(&ret->n, n, opts, miny, maxy)){
      return ret;
    }
    delete ret;
  }
  return nullptr;
}

ncplane* ncuplot_plane(ncuplot* n) {
  return n->n.ncp;
}

int ncuplot_add_sample(ncuplot* n, uint64_t x, uint64_t y) {
  return n->n.add_sample(x, y);
}

int ncuplot_set_sample(ncuplot* n, uint64_t x, uint64_t y) {
  return n->n.set_sample(x, y);
}

void ncuplot_destroy(ncuplot* n) {
  if(n){
    n->n.destroy();
    delete n;
  }
}

ncdplot* ncdplot_create(ncplane* n, const ncplot_options* opts, double miny, double maxy) {
  auto ret = new ncdplot;
  if(ret){
    if(ncppplot<double>::create(&ret->n, n, opts, miny, maxy)){
      return ret;
    }
    delete ret;
  }
  return nullptr;
}

ncplane* ncdplot_plane(ncdplot* n) {
  return n->n.ncp;
}

int ncdplot_add_sample(ncdplot* n, uint64_t x, double y) {
  return n->n.add_sample(x, y);
}

int ncdplot_set_sample(ncdplot* n, uint64_t x, double y) {
  return n->n.set_sample(x, y);
}

void ncdplot_destroy(ncdplot* n) {
  if(n){
    n->n.destroy();
    delete n;
  }
}

}
