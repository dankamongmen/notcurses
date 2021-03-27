#include "main.h"
#include <cstdlib>
#include <iostream>

TEST_CASE("ZAxis") {
  auto nc_ = testing_notcurses();
  REQUIRE(nullptr != nc_);
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
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 2,
      .cols = 2,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* np = ncplane_create(n_, &nopts);
    REQUIRE(np);
    CHECK(ncplane_move_below(n_, n_));
    CHECK(ncplane_move_above(n_, n_));
    CHECK(ncplane_move_below(np, np));
    CHECK(ncplane_move_above(np, np));
  }

  // new planes ought be on the top
  SUBCASE("NewPlaneOnTop") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 2,
      .cols = 2,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* np = ncplane_create(n_, &nopts);
    REQUIRE(np);
    struct ncplane* top = notcurses_top(nc_);
    CHECK(np == top);
    CHECK(n_ == ncplane_below(top));
    CHECK(!ncplane_below(n_));
  }

  // "move" top plane to top. everything ought remain the same.
  SUBCASE("TopToTop") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 2,
      .cols = 2,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* np = ncplane_create(n_, &nopts);
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
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 2,
      .cols = 2,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* np = ncplane_create(n_, &nopts);
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
    nccell cat = CELL_TRIVIAL_INITIALIZER;
    nccell c = CELL_CHAR_INITIALIZER('x');
    REQUIRE(!cell_set_fg_rgb8(&c, 0xff, 0, 0));
    REQUIRE(1 == ncplane_putc(n_, &c));
    CHECK(!notcurses_render(nc_));
    REQUIRE(!ncplane_cursor_move_yx(n_, 0, 0));
    REQUIRE(1 == ncplane_at_cursor_cell(n_, &cat));
    REQUIRE(cell_simple_p(&cat));
    REQUIRE(0 == strcmp("x", cell_extended_gcluster(n_, &c)));
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 2,
      .cols = 2,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* n2 = ncplane_create(n_, &nopts);
    REQUIRE(1 == cell_load(n2, &c, "y"));
    REQUIRE(!cell_set_fg_rgb8(&c, 0, 0xff, 0));
    REQUIRE(1 == ncplane_putc(n2, &c));
    CHECK_EQ(0, notcurses_render(nc_));
    REQUIRE(!ncplane_cursor_move_yx(n2, 0, 0));
    REQUIRE(1 == ncplane_at_cursor_cell(n2, &cat));
    REQUIRE(0 == strcmp("y", cell_extended_gcluster(n_, &c)));
    struct ncplane* n3 = ncplane_create(n_, &nopts);
    REQUIRE(1 == cell_load(n3, &c, "z"));
    REQUIRE(!cell_set_fg_rgb8(&c, 0, 0, 0xff));
    REQUIRE(1 == ncplane_putc(n3, &c));
    CHECK(!notcurses_render(nc_));
    REQUIRE(!ncplane_cursor_move_yx(n3, 0, 0));
    REQUIRE(1 == ncplane_at_cursor_cell(n3, &cat));
    REQUIRE(0 == strcmp("z", cell_extended_gcluster(n_, &c)));
    // FIXME testing damage requires notcurses keeping a copy of the screen....
    // FIXME move y atop z
    // FIXME inspect
    // FIXME move z atop y
    // FIXME inspect
  }

  SUBCASE("DropPlanes") {
    struct ncplane_options nopts = {
      .y = 1,
      .x = 1,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto p = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != p);
    CHECK(notcurses_top(nc_) == p);
    CHECK(0 == notcurses_render(nc_));
    notcurses_drop_planes(nc_);
    CHECK(notcurses_top(nc_) != p);
    CHECK(0 == notcurses_render(nc_));
  }

  CHECK(0 == notcurses_stop(nc_));
}
