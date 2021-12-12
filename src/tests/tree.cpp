#include "main.h"
#include <iostream>

int treecb(struct ncplane* n, void* curry, int pos){
  ncplane_printf_yx(n, 0, 0, "item: %s pos: %d",
                    static_cast<const char*>(curry), pos);
  return 0;
}

TEST_CASE("Tree") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);
  REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));

  SUBCASE("RefuseStandardPlane") {
    struct nctree_options opts{};
    opts.nctreecb = treecb;
    auto treen = nctree_create(n_, &opts);
    REQUIRE(nullptr == treen);
    CHECK(0 == notcurses_render(nc_));
  }

  // trivial tree with null items
  SUBCASE("TreeNoItems") {
    struct ncplane_options nopts{};
    nopts.rows = 7;
    nopts.cols = 20;
    auto p = ncplane_create(n_, &nopts);
    REQUIRE(p);
    struct nctree_options opts{};
    opts.nctreecb = treecb;
    auto treen = nctree_create(p, &opts);
    REQUIRE(treen);
    CHECK(0 == notcurses_render(nc_));
    nctree_destroy(treen);
  }

  nctree_item subs[] = {
    {
      .curry = strdup("sub1-0"),
      .subs = nullptr,
      .subcount = 0,
    },{
      .curry = strdup("sub1-1"),
      .subs = nullptr,
      .subcount = 0,
    }
  };

  nctree_item items[] = {
    {
      .curry = strdup("item0"),
      .subs = nullptr,
      .subcount = 0,
    }, {
      .curry = strdup("item1"),
      .subs = subs,
      .subcount = 2,
    },
  };

  // should be refused with a null callback
  SUBCASE("BadTreeNoCallback") {
    struct nctree_options opts = {
      .items = items,
      .count = sizeof(items) / sizeof(*items),
      .nctreecb = nullptr,
      .indentcols = 1,
      .flags = 0,
    };
    auto treen = nctree_create(n_, &opts);
    REQUIRE(nullptr == treen);
  }

  // should be refused with negative indentcols
  SUBCASE("BadTreeNoCallback") {
    struct nctree_options opts = {
      .items = items,
      .count = sizeof(items) / sizeof(*items),
      .nctreecb = treecb,
      .indentcols = -1,
      .flags = 0,
    };
    auto treen = nctree_create(n_, &opts);
    REQUIRE(nullptr == treen);
  }

  SUBCASE("Create") {
    struct nctree_options opts = {
      .items = items,
      .count = sizeof(items) / sizeof(*items),
      .nctreecb = treecb,
      .indentcols = 1,
      .flags = 0,
    };
    const ncplane_options nopts = {
      .y = 0, .x = 0, .rows = 3, .cols = ncplane_dim_y(n_),
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto treen = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != treen);
    auto tree = nctree_create(treen, &opts);
    REQUIRE(nullptr != tree);
    CHECK(0 == notcurses_render(nc_));
    CHECK(treen == nctree_plane(tree));
    CHECK(items[0].curry == nctree_focused(tree));
    nctree_destroy(tree);
  }

  SUBCASE("Traverse") {
    struct nctree_options opts = {
      .items = items,
      .count = sizeof(items) / sizeof(*items),
      .nctreecb = treecb,
      .indentcols = 2,
      .flags = 0,
    };
    const ncplane_options nopts = {
      .y = 0, .x = 0, .rows = 3, .cols = ncplane_dim_y(n_),
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto treen = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != treen);
    auto tree = nctree_create(treen, &opts);
    REQUIRE(nullptr != tree);
    CHECK(0 == notcurses_render(nc_));

    CHECK(treen == nctree_plane(tree));
    CHECK(items[0].curry == nctree_focused(tree));

    CHECK(items[0].curry == nctree_prev(tree));
    CHECK(items[0].curry == nctree_focused(tree));

    CHECK(items[1].curry == nctree_next(tree));
    CHECK(items[1].curry == nctree_focused(tree));

    CHECK(items[1].subs[0].curry == nctree_next(tree));
    CHECK(items[1].subs[0].curry == nctree_focused(tree));

    CHECK(items[1].subs[1].curry == nctree_next(tree));
    CHECK(items[1].subs[1].curry == nctree_focused(tree));

    CHECK(items[1].subs[1].curry == nctree_next(tree));
    CHECK(items[1].subs[1].curry == nctree_focused(tree));

    CHECK(items[1].subs[0].curry == nctree_prev(tree));
    CHECK(items[1].subs[0].curry == nctree_focused(tree));

    CHECK(items[1].curry == nctree_prev(tree));
    CHECK(items[1].curry == nctree_focused(tree));

    CHECK(items[0].curry == nctree_prev(tree));
    CHECK(items[0].curry == nctree_focused(tree));

    CHECK(items[0].curry == nctree_prev(tree));
    CHECK(items[0].curry == nctree_focused(tree));

    nctree_destroy(tree);
  }

  free(subs[0].curry);
  free(subs[1].curry);
  free(items[0].curry);
  free(items[1].curry);

  nctree_item alphaUs[] = {
    {
      .curry = const_cast<char*>("²¹⁴U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²¹⁵U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²¹⁶U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²¹⁷U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²¹⁸U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²¹⁹U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²²¹U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²²²U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²²⁸U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²²⁹U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³⁰U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³¹U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³²U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³³U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³⁴U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³⁵U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³⁶U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³⁸U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²⁴⁰U"),
      .subs = NULL,
      .subcount = 0,
    },
  };

  nctree_item alphaU = {
    .curry = const_cast<char*>("ɑ-emitting U"),
    .subs = alphaUs,
    .subcount = sizeof(alphaUs) / sizeof(*alphaUs),
  };

  nctree_item doubleUs[] = {
    {
      .curry = const_cast<char*>("²³⁰U"),
      .subs = NULL,
      .subcount = 0,
    }
  };

  nctree_item doubleU = {
    .curry = const_cast<char*>("ββ-emitting U"),
    .subs = doubleUs,
    .subcount = sizeof(doubleUs) / sizeof(*doubleUs),
  };

  nctree_item doubleminusUs[] = {
    {
      .curry = const_cast<char*>("²³⁸U"),
      .subs = NULL,
      .subcount = 0,
    }
  };

  nctree_item doubleminusU = {
    .curry = const_cast<char*>("β−β−-emitting U"),
    .subs = doubleminusUs,
    .subcount = sizeof(doubleminusUs) / sizeof(*doubleminusUs),
  };

  nctree_item betaminusUs[] = {
    {
      .curry = const_cast<char*>("²³⁹U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²⁴⁰U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²⁴²U"),
      .subs = NULL,
      .subcount = 0,
    }
  };

  nctree_item betaminusPus[] = {
    {
      .curry = const_cast<char*>("²⁴¹Pu"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²⁴³Pu"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²⁴⁵Pu"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²⁴⁶Pu"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²⁴⁷Pu"),
      .subs = NULL,
      .subcount = 0,
    }
  };

  nctree_item betaminus[] = {
    {
      .curry = const_cast<char*>("β−-emitting U"),
      .subs = betaminusUs,
      .subcount = sizeof(betaminusUs) / sizeof(*betaminusUs),
    }, {
      .curry = const_cast<char*>("β−-emitting Pu"),
      .subs = betaminusPus,
      .subcount = sizeof(betaminusPus) / sizeof(*betaminusPus),
    },
  };

  nctree_item betaplusUs[] = {
    {
      .curry = const_cast<char*>("²²²U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²²⁷U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²²⁹U"),
      .subs = NULL,
      .subcount = 0,
    },
  };

  nctree_item betaplus = {
    .curry = const_cast<char*>("β-emitting U"),
    .subs = betaplusUs,
    .subcount = sizeof(betaplusUs) / sizeof(*betaplusUs),
  };

  nctree_item gammaUs[] = {
    {
      .curry = const_cast<char*>("²¹⁶Uᵐ"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³⁴Uᵐ"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³⁵Uᵐ"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³⁶Uᵐ¹"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³⁶Uᵐ²"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³⁸Uᵐ"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³⁹Uᵐ¹"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³⁹Uᵐ²"),
      .subs = NULL,
      .subcount = 0,
    },
  };

  nctree_item gammas = {
    .curry = const_cast<char*>("γ-emitting U"),
    .subs = gammaUs,
    .subcount = sizeof(gammaUs) / sizeof(*gammaUs),
  };

  nctree_item sfissionUs[] = {
    {
      .curry = const_cast<char*>("²³⁰U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³²U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³³U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³⁴U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³⁵U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³⁶U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³⁸U"),
      .subs = NULL,
      .subcount = 0,
    },
  };

  nctree_item sfissions = {
    .curry = const_cast<char*>("spontaneously fissioning U"),
    .subs = sfissionUs,
    .subcount = sizeof(sfissionUs) / sizeof(*sfissionUs),
  };

  nctree_item ecaptureUs[] = {
    {
      .curry = const_cast<char*>("²²⁸U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³¹U"),
      .subs = NULL,
      .subcount = 0,
    },
  };

  nctree_item ecaptures = {
    .curry = const_cast<char*>("electron capturing U"),
    .subs = ecaptureUs,
    .subcount = sizeof(ecaptureUs) / sizeof(*ecaptureUs),
  };

  nctree_item cdecayUs[] = {
    {
      .curry = const_cast<char*>("²³²U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³³U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³⁴U"),
      .subs = NULL,
      .subcount = 0,
    }, {
      .curry = const_cast<char*>("²³⁵U"),
      .subs = NULL,
      .subcount = 0,
    },
  };

  nctree_item cdecays = {
    .curry = const_cast<char*>("cluster decaying U"),
    .subs = cdecayUs,
    .subcount = sizeof(cdecayUs) / sizeof(*cdecayUs),
  };

  nctree_item radUs[] = {
    {
      .curry = const_cast<char*>("ɑ emitters"),
      .subs = &alphaU,
      .subcount = 1,
    }, {
      .curry = const_cast<char*>("β− emitters"),
      .subs = betaminus,
      .subcount = sizeof(betaminus) / sizeof(*betaminus),
    }, {
      .curry = const_cast<char*>("β−β− emitters"),
      .subs = &doubleminusU,
      .subcount = 1,
    }, {
      .curry = const_cast<char*>("ββ emitters"),
      .subs = &doubleU,
      .subcount = 1,
    }, {
      .curry = const_cast<char*>("β emitters"),
      .subs = &betaplus,
      .subcount = 1,
    }, {
      .curry = const_cast<char*>("γ emitters"),
      .subs = &gammas,
      .subcount = 1,
    }, {
      .curry = const_cast<char*>("spontaneous fissions"),
      .subs = &sfissions,
      .subcount = 1,
    }, {
      .curry = const_cast<char*>("cluster decays"),
      .subs = &cdecays,
      .subcount = 1,
    }, {
      .curry = const_cast<char*>("electron captures"),
      .subs = &ecaptures,
      .subcount = 1,
    },
  };

  nctree_item rads = {
    .curry = const_cast<char*>("radiating isotopes"),
    .subs = radUs,
    .subcount = sizeof(radUs) / sizeof(*radUs),
  };

  SUBCASE("TraverseLongList") {
    struct ncplane_options nopts{};
    nopts.rows = 7;
    nopts.cols = 20;
    auto p = ncplane_create(n_, &nopts);
    REQUIRE(p);
    struct nctree_options topts = {
      .items = &rads,
      .count = 1,
      .nctreecb = treecb,
      .indentcols = 2,
      .flags = 0,
    };
    struct nctree* tree = nctree_create(p, &topts);
    REQUIRE(nullptr != tree);
    CHECK(0 == notcurses_render(nc_));
    void* curry = nctree_focused(tree);
    void* first = curry;
    CHECK(nullptr != curry);
    void* tmpcurry;
    while( (tmpcurry = nctree_next(tree)) ){
      if(tmpcurry == curry){
        break;
      }
      curry = tmpcurry;
    }
    CHECK(nullptr != tmpcurry);
    while( (tmpcurry = nctree_prev(tree)) ){
      if(tmpcurry == curry){
        break;
      }
      curry = tmpcurry;
    }
    CHECK(nullptr != tmpcurry);
    CHECK(curry == first);
    nctree_destroy(tree);
  }

  CHECK(0 == notcurses_stop(nc_));
}
