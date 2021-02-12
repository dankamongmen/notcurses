#include "main.h"
#include <iostream>

int treecb(struct ncplane* n, void* curry, int pos){
  // FIXME draw to the ncplane
  fprintf(stderr, "n: %p curry: %p pos: %d", n, curry, pos);
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
  SUBCASE("BadTreeNoPointer") {
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
  SUBCASE("BadTreeNoCount") {
    struct nctree_options opts = {
      .items = {},
      .count = 1,
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
        .curry = nullptr,
        .subs = nullptr,
        .subcount = 0,
      },{
        .curry = nullptr,
        .subs = nullptr,
        .subcount = 0,
      }
    };
    nctree_item items[] = {
      {
        .curry = nullptr,
        .subs = nullptr,
        .subcount = 0,
      }, {
        .curry = nullptr,
        .subs = subs,
        .subcount = 2,
      },
    };
    struct nctree_options opts = {
      .items = items,
      .count = 2,
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
    nctree_destroy(tree);
  }

  CHECK(0 == notcurses_stop(nc_));
}
