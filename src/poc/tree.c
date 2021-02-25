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
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²¹⁸U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²¹⁹U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²²¹U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²²²U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²²⁸U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²²⁹U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁰U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³¹U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³²U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³³U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁴U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁵U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁶U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁸U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²⁴⁰U",
  }
};

static nctree_item alphaU = {
  .subs = alphaUs,
  .subcount = sizeof(alphaUs) / sizeof(*alphaUs),
  .curry = "ɑ-emitting U",
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
  .curry = "ββ-emitting U",
};

static nctree_item doubleminusUs[] = {
  {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁸U",
  }
};

static nctree_item doubleminusU = {
  .subs = doubleminusUs,
  .subcount = sizeof(doubleminusUs) / sizeof(*doubleminusUs),
  .curry = "β−β−-emitting U",
};

static nctree_item betaminusUs[] = {
  {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁹U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²⁴⁰U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²⁴²U",
  }
};

static nctree_item betaminusPus[] = {
  {
    .subs = NULL,
    .subcount = 0,
    .curry = "²⁴¹Pu",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²⁴³Pu",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²⁴⁵Pu",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²⁴⁶Pu",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²⁴⁷Pu",
  }
};

static nctree_item betaminus[] = {
  {
    .subs = betaminusUs,
    .subcount = sizeof(betaminusUs) / sizeof(*betaminusUs),
    .curry = "β−-emitting U",
  }, {
    .subs = betaminusPus,
    .subcount = sizeof(betaminusPus) / sizeof(*betaminusPus),
    .curry = "β−-emitting Pu",
  },
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
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²²⁹U",
  },
};

static nctree_item betaplus = {
  .subs = betaplusUs,
  .subcount = sizeof(betaplusUs) / sizeof(*betaplusUs),
  .curry = "β-emitting U",
};

static nctree_item gammaUs[] = {
  {
    .subs = NULL,
    .subcount = 0,
    .curry = "²¹⁶Uᵐ",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁴Uᵐ",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁵Uᵐ",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁶Uᵐ¹",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁶Uᵐ²",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁸Uᵐ",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁹Uᵐ¹",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁹Uᵐ²",
  },
};

static nctree_item gammas = {
  .subs = gammaUs,
  .subcount = sizeof(gammaUs) / sizeof(*gammaUs),
  .curry = "γ-emitting U",
};

static nctree_item sfissionUs[] = {
  {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁰U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³²U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³³U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁴U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁵U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁶U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁸U",
  },
};

static nctree_item sfissions = {
  .subs = sfissionUs,
  .subcount = sizeof(sfissionUs) / sizeof(*sfissionUs),
  .curry = "spontaneously fissioning U",
};

static nctree_item ecaptureUs[] = {
  {
    .subs = NULL,
    .subcount = 0,
    .curry = "²²⁸U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³¹U",
  },
};

static nctree_item ecaptures = {
  .subs = ecaptureUs,
  .subcount = sizeof(ecaptureUs) / sizeof(*ecaptureUs),
  .curry = "electron capturing U",
};

static nctree_item cdecayUs[] = {
  {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³²U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³³U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁴U",
  }, {
    .subs = NULL,
    .subcount = 0,
    .curry = "²³⁵U",
  },
};

static nctree_item cdecays = {
  .subs = cdecayUs,
  .subcount = sizeof(cdecayUs) / sizeof(*cdecayUs),
  .curry = "cluster decaying U",
};

static nctree_item radUs[] = {
  {
    .subs = &alphaU,
    .subcount = 1,
    .curry = "ɑ emitters",
  }, {
    .subs = betaminus,
    .subcount = sizeof(betaminus) / sizeof(*betaminus),
    .curry = "β− emitters",
  }, {
    .subs = &doubleminusU,
    .subcount = 1,
    .curry = "β−β− emitters",
  }, {
    .subs = &doubleU,
    .subcount = 1,
    .curry = "ββ emitters",
  }, {
    .subs = &betaplus,
    .subcount = 1,
    .curry = "β emitters",
  }, {
    .subs = &gammas,
    .subcount = 1,
    .curry = "γ emitters",
  }, {
    .subs = &sfissions,
    .subcount = 1,
    .curry = "spontaneous fissions",
  }, {
    .subs = &cdecays,
    .subcount = 1,
    .curry = "cluster decays",
  }, {
    .subs = &ecaptures,
    .subcount = 1,
    .curry = "electron captures",
  },
};

static nctree_item rads = {
  .subs = radUs,
  .subcount = sizeof(radUs) / sizeof(*radUs),
  .curry = "radiating isotopes",
};

static int
callback(struct ncplane* ncp, void* curry, int dizzy){
  if(ncp == NULL){
    return 0;
  }
  const float totylen = ncplane_dim_y(ncplane_parent_const(ncp));
  if(ncplane_dim_y(ncp) > 1){
    if(ncplane_resize_simple(ncp, 1, ncplane_dim_x(ncp))){
      return -1;
    }
  }
  ncplane_cursor_move_yx(ncp, 0, 0);
  uint64_t channels = 0;
  if(dizzy == 0){
    channels_set_bg_rgb(&channels, 0x006080);
    ncplane_set_fg_rgb(ncp, 0xffffff);
  }else if(dizzy < 0){
    float f = -dizzy / totylen;
    channels_set_bg_rgb8(&channels, 0, 0x60 - 0x60 * f, 0);
    ncplane_set_fg_rgb(ncp, 0xbbbbbb);
  }else if(dizzy > 0){
    float f = dizzy / totylen;
    channels_set_bg_rgb8(&channels, 0, 0x60 - 0x60 * f, 0);
    ncplane_set_fg_rgb(ncp, 0xbbbbbb);
  }
  ncplane_set_base(ncp, "", 0, channels);
  ncplane_putstr(ncp, curry);
  return 0;
}

static int
tree_ui(struct notcurses* nc, struct nctree* tree){
  ncinput ni;
  while(notcurses_getc_blocking(nc, &ni) != (char32_t)-1){
    if(nctree_offer_input(tree, &ni)){
      if(nctree_redraw(tree)){
        return -1;
      }
      if(notcurses_render(nc)){
        return -1;
      }
      continue;
    }
    if(ni.id == 'q'){
      return 0;
    }
  }
  return -1;
}

static struct nctree*
create_tree(struct notcurses* nc){
  struct nctree_options topts = {
    .items = &rads,
    .count = 1,
    .nctreecb = callback,
    .indentcols = 2,
    .flags = 0,
  };
  struct nctree* tree = nctree_create(notcurses_stdplane(nc), &topts);
  if(tree){
    notcurses_render(nc);
  }
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
  int r = tree_ui(nc, tree);
  nctree_destroy(tree);
  notcurses_stop(nc);
  if(r){
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
