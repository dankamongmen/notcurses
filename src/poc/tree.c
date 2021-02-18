#include "notcurses/notcurses.h"

static nctree_item alphaUs[] = {
  {
    .subs = NULL,
    .subcount = 0,
    .curry = "²¹⁴U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²¹⁵U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²¹⁶U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²¹⁷U",
  },
};

static nctree_item alphaU = {
  .subs = alphaUs,
  .subcount = sizeof(alphaUs) / sizeof(*alphaUs),
};

static nctree_item alphas = {
  .subs = &alphaU,
  .subcount = 1,
};

static nctree_item doubleUs[] = {
  {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁰U",
  }
};

static nctree_item doubleU = {
  .subs = doubleUs,
  .subcount = sizeof(doubleUs) / sizeof(*doubleUs),
};

static nctree_item doublebeta = {
  .subs = &doubleU,
  .subcount = 1,
};

static nctree_item betaminus = {
};

static nctree_item betaplusUs[] = {
  {
    .subs = NULL,
    .subcount = 0,
    .curry = "²²²U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²²⁷U",
  },
};

static nctree_item betaplus = {
  .subs = betaplusUs,
  .subcount = sizeof(betaplusUs) / sizeof(*betaplusUs),
};

static nctree_item gammas = {
};

static nctree_item sfissions = {
};

static nctree_item radUs[] = {
  {
    .subs = &alphas,
    .subcount = 1,
  }, {
    .subs = &doublebeta,
    .subcount = 1,
  }, {
    .subs = &betaminus,
    .subcount = 0,
  }, {
    .subs = &betaplus,
    .subcount = 0,
  }, {
    .subs = &gammas,
    .subcount = 0,
  }, {
    .subs = &sfissions,
    .subcount = 0,
  },
};

static nctree_item rads = {
  .subs = radUs,
  .subcount = sizeof(radUs) / sizeof(*radUs),
};

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
    (void)tree; // FIXME
  }
}

static struct nctree*
create_tree(struct notcurses* nc){
  struct nctree_options topts = {
    .items = &rads,
    .count = 1,
    .nctreecb = callback,
    .flags = 0,
  };
  struct nctree* tree = nctree_create(notcurses_stdplane(nc), &topts);
  return tree;
}

int main(void){
  struct notcurses_options nopts = {
    .loglevel = NCLOGLEVEL_WARNING,
  };
  struct notcurses* nc = notcurses_init(&nopts, NULL);
  if(nc == NULL){
    return EXIT_FAILURE;
  }
  struct nctree* tree = create_tree(nc);
  if(tree == NULL){
    notcurses_stop(nc);
    return EXIT_FAILURE;
  }
  tree_ui(nc, tree);
  nctree_destroy(tree);
  notcurses_stop(nc);
  return EXIT_SUCCESS;
}
