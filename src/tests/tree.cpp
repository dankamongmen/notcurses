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

  // should be refused with a null items
  SUBCASE("BadTreeNoItems") {
    struct nctree_options opts = {
      .items = nullptr,
      .count = 2,
      .bchannels = 0,
      .nctreecb = treecb,
      .flags = 0,
    };
    auto treen = nctree_create(n_, &opts);
    REQUIRE(nullptr == treen);
  }

  // should be refused with a zero count
  SUBCASE("BadTreeNoCount") {
    struct nctree_options opts = {
      .items = {},
      .count = 0,
      .bchannels = 0,
      .nctreecb = treecb,
      .flags = 0,
    };
    auto treen = nctree_create(n_, &opts);
    REQUIRE(nullptr == treen);
  }

  // should be refused with a null callback
  SUBCASE("BadTreeNoCallback") {
    nctree_item items[] = {
      {
        .curry = strdup("item1"),
        .subs = nullptr,
        .subcount = 0,
      },
    };
    struct nctree_options opts = {
      .items = items,
      .count = sizeof(items) / sizeof(*items),
      .bchannels = 0,
      .nctreecb = nullptr,
      .flags = 0,
    };
    auto treen = nctree_create(n_, &opts);
    REQUIRE(nullptr == treen);
  }

  SUBCASE("CreateTree") {
    nctree_item subs[] = {
      {
        .curry = strdup("sub1-1"),
        .subs = nullptr,
        .subcount = 0,
      },{
        .curry = strdup("sub1-2"),
        .subs = nullptr,
        .subcount = 0,
      }
    };
    nctree_item items[] = {
      {
        .curry = strdup("item1"),
        .subs = nullptr,
        .subcount = 0,
      }, {
        .curry = strdup("item2"),
        .subs = subs,
        .subcount = 2,
      },
    };
    struct nctree_options opts = {
      .items = items,
      .count = sizeof(items) / sizeof(*items),
      .bchannels = 0,
      .nctreecb = treecb,
      .flags = 0,
    };
    const ncplane_options nopts = {
      .y = 0, .x = 0, .rows = 3, .cols = ncplane_dim_y(n_),
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
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

  CHECK(0 == notcurses_stop(nc_));
}
