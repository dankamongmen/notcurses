#include "notcurses/notcurses.h"

static int
callback(struct ncplane* ncp, void* curry, int dizzy){
  // FIXME
  (void)ncp;
  (void)curry;
  (void)dizzy;
  return 0;
}

static void
tree_ui(struct notcurses* nc, struct nctree* tree){
  ncinput ni;
  while(notcurses_getc_blocking(nc, &ni) != (char32_t)-1){
    fprintf(stderr, "ni: 0x%04x\n", ni.id);
  }
}

static struct nctree*
create_tree(struct notcurses* nc){
  struct nctree_options topts = {
    .nctreecb = callback,
    .flags = 0,
  };
  struct nctree* tree = nctree_create(notcurses_stdplane(nc), &topts);
  return tree;
}

int main(void){
  struct notcurses* nc = notcurses_init(NULL, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  struct nctree* tree = create_tree(nc);
  tree_ui(nc, tree);
  nctree_destroy(tree);
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}
