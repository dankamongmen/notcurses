#include "main.h"
#include <cstdlib>
#include <iostream>

TEST_CASE("ZAxis") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  SUBCASE("StdPlaneOnly") {
    struct ncplane* top = notcurses_top(nc_);
    CHECK(n_ == top);
    CHECK(!ncplane_below(top));
  }

  // if you want to move the plane which is already top+bottom to either, go ahead
  SUBCASE("StdPlaneOnanism") {
    ncplane_move_top(n_);
    struct ncplane* top = notcurses_top(nc_);
    CHECK(n_ == top);
    CHECK(!ncplane_below(top));
    ncplane_move_bottom(n_);
    CHECK(!ncplane_below(n_));
  }

  // you can't place a plane above or below itself, stdplane or otherwise
  SUBCASE("NoMoveSelf") {
    struct ncplane* np = ncplane_new(nc_, 2, 2, 0, 0, nullptr);
    REQUIRE(np);
    CHECK(ncplane_move_below(n_, n_));
    CHECK(ncplane_move_above(n_, n_));
    CHECK(ncplane_move_below(np, np));
    CHECK(ncplane_move_above(np, np));
  }

  // new planes ought be on the top
  SUBCASE("NewPlaneOnTop") {
    struct ncplane* np = ncplane_new(nc_, 2, 2, 0, 0, nullptr);
    REQUIRE(np);
    struct ncplane* top = notcurses_top(nc_);
    CHECK(np == top);
    CHECK(n_ == ncplane_below(top));
    CHECK(!ncplane_below(n_));
  }

  // "move" top plane to top. everything ought remain the same.
  SUBCASE("TopToTop") {
    struct ncplane* np = ncplane_new(nc_, 2, 2, 0, 0, nullptr);
    REQUIRE(np);
    struct ncplane* top = notcurses_top(nc_);
    CHECK(np == top);
    CHECK(n_ == ncplane_below(top));
    CHECK(!ncplane_below(n_));
    ncplane_move_top(np);
    // verify it
    top = notcurses_top(nc_);
    CHECK(np == top);
    CHECK(n_ == ncplane_below(top));
    CHECK(!ncplane_below(n_));
  }

  // move top plane to bottom, and verify enumeration
  SUBCASE("TopToBottom") {
    struct ncplane* np = ncplane_new(nc_, 2, 2, 0, 0, nullptr);
    REQUIRE(np);
    struct ncplane* top = notcurses_top(nc_);
    CHECK(np == top);
    CHECK(n_ == ncplane_below(top));
    CHECK(!ncplane_below(n_));
    ncplane_move_bottom(np);
    top = notcurses_top(nc_);
    CHECK(n_ == top);
    CHECK(np == ncplane_below(top));
    CHECK(!ncplane_below(np));
  }

  // verify that moving one above another, with no other changes, is reflected at
  // render time (requires explicit damage maintenance from move functionality).
  SUBCASE("ZAxisDamage") {
    cell cat = CELL_TRIVIAL_INITIALIZER;
    cell c = CELL_SIMPLE_INITIALIZER('x');
    REQUIRE(!cell_set_fg_rgb(&c, 0xff, 0, 0));
    REQUIRE(1 == ncplane_putc(n_, &c));
    CHECK(!notcurses_render(nc_));
    REQUIRE(!ncplane_cursor_move_yx(n_, 0, 0));
    REQUIRE(1 == ncplane_at_cursor_cell(n_, &cat));
    REQUIRE(cell_simple_p(&cat));
    REQUIRE('x' == cat.gcluster);
    struct ncplane* n2 = ncplane_new(nc_, 2, 2, 0, 0, nullptr);
    REQUIRE(1 == cell_load(n2, &c, "y"));
    REQUIRE(!cell_set_fg_rgb(&c, 0, 0xff, 0));
    REQUIRE(1 == ncplane_putc(n2, &c));
    CHECK_EQ(0, notcurses_render(nc_));
    REQUIRE(!ncplane_cursor_move_yx(n2, 0, 0));
    REQUIRE(1 == ncplane_at_cursor_cell(n2, &cat));
    REQUIRE('y' == cat.gcluster);
    struct ncplane* n3 = ncplane_new(nc_, 2, 2, 0, 0, nullptr);
    REQUIRE(1 == cell_load(n3, &c, "z"));
    REQUIRE(!cell_set_fg_rgb(&c, 0, 0, 0xff));
    REQUIRE(1 == ncplane_putc(n3, &c));
    CHECK(!notcurses_render(nc_));
    REQUIRE(!ncplane_cursor_move_yx(n3, 0, 0));
    REQUIRE(1 == ncplane_at_cursor_cell(n3, &cat));
    REQUIRE('z' == cat.gcluster);
    // FIXME testing damage requires notcurses keeping a copy of the screen....
    // FIXME move y atop z
    // FIXME inspect
    // FIXME move z atop y
    // FIXME inspect
  }

  SUBCASE("DropPlanes") {
    auto p = ncplane_new(nc_, 1, 1, 1, 1, nullptr);
    REQUIRE(nullptr != p);
    CHECK(notcurses_top(nc_) == p);
    CHECK(0 == notcurses_render(nc_));
    notcurses_drop_planes(nc_);
    CHECK(notcurses_top(nc_) != p);
    CHECK(0 == notcurses_render(nc_));
  }

  CHECK(0 == notcurses_stop(nc_));
}
