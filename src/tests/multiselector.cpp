#include "main.h"
#include <cstring>
#include <iostream>

TEST_CASE("Multiselectors") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);
  REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));

  // multiselector can't be bound to the standard plane
  SUBCASE("RefuseStandardPlane") {
    ncmultiselector_options s{};
    struct ncmultiselector* ns = ncmultiselector_create(n_, &s);
    REQUIRE(nullptr == ns);
  }

  // create a multiselector, but don't explicitly destroy it, thus testing the
  // context shutdown cleanup path
  SUBCASE("ImplicitDestroy") {
    ncmultiselector_options s{};
    struct ncplane_options nopts{};
    nopts.rows = 1;
    nopts.cols = 1;
    struct ncplane* n = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != n);
    struct ncmultiselector* ns = ncmultiselector_create(n, &s);
    REQUIRE(ns);
    CHECK(0 == notcurses_render(nc_));
  }

  // now do the same, but with a plane we have created.
  SUBCASE("RefuseBoundCreatedPlane") {
    struct ncplane_options nopts{};
    nopts.rows = ncplane_dim_y(n_);
    nopts.cols = ncplane_dim_x(n_);
    auto ncp = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != ncp);
    ncmultiselector_options s{};
    struct ncmultiselector* ns = ncmultiselector_create(ncp, &s);
    REQUIRE(ns);
    CHECK(0 == notcurses_render(nc_));
    struct ncmultiselector* fail = ncmultiselector_create(ncp, &s);
    CHECK(nullptr == fail);
    CHECK(0 == notcurses_render(nc_));
  }

  CHECK(0 == notcurses_stop(nc_));
}
