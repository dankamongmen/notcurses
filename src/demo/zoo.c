#include "demo.h"

int zoo_demo(struct notcurses* nc){
  struct ncplane* n = notcurses_stdplane(nc);
  ncselector_options sopts = {
    .maxdisplay = 4,
    .title = "single-item selector",
  };
  struct ncselector* selector = ncselector_create(n, 1, 1, &sopts);
  if(selector == NULL){
    return -1;
  }
  DEMO_RENDER(nc);
  // FIXME swoop a selector in from the right
  demo_nanosleep(nc, &demodelay);
  ncselector_destroy(selector, NULL);
  // FIXME swoop a multiselector in from the left
  DEMO_RENDER(nc);
  return 0;
}
