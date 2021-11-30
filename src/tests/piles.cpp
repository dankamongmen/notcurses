#include "main.h"

TEST_CASE("Piles") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  unsigned dimy, dimx;
  struct ncplane* n_ = notcurses_stddim_yx(nc_, &dimy, &dimx);
  REQUIRE(nullptr != n_);

  // you can't move the standard plane, but you can move root planes of other
  // piles. they ought go to the absolute specified location.
  SUBCASE("MovePileRoot") {
    struct ncplane_options nopts = {
      .y = 1, .x = 1,
      .rows = dimy - 2,
      .cols = dimx - 2,
      .userptr = nullptr,
      .name = "small",
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto np = ncpile_create(nc_, &nopts);
    REQUIRE(nullptr != np);
    CHECK(1 == ncplane_y(np));
    CHECK(1 == ncplane_x(np));
    CHECK(0 == ncplane_move_yx(np, 2, 2));
    CHECK(2 == ncplane_y(np));
    CHECK(2 == ncplane_x(np));
    CHECK(0 == ncplane_move_yx(np, -1, -1));
    CHECK(-1 == ncplane_y(np));
    CHECK(-1 == ncplane_x(np));
    CHECK(0 == ncpile_render(np));
    CHECK(0 == ncpile_rasterize(np));
    CHECK(0 == ncplane_destroy(np));
  }

  // create a plane bigger than the standard plane, and render it as a pile
  SUBCASE("SmallerPileRender") {
    struct ncplane_options nopts = {
      .y = 1, .x = 1,
      .rows = dimy - 2,
      .cols = dimx - 2,
      .userptr = nullptr,
      .name = "small",
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto np = ncpile_create(nc_, &nopts);
    REQUIRE(nullptr != np);
    CHECK(np == ncplane_parent_const(np));
    CHECK(1 == ncplane_y(np));
    CHECK(1 == ncplane_x(np));
    nccell c = NCCELL_CHAR_INITIALIZER('X');
    CHECK(0 < ncplane_polyfill_yx(np, 0, 0, &c));
    nccell o = NCCELL_CHAR_INITIALIZER('O');
    CHECK(0 < ncplane_polyfill_yx(n_, 0, 0, &o));
    CHECK(0 == ncpile_render(np));
    CHECK(0 == ncpile_render(n_));
    CHECK(0 == ncpile_rasterize(n_));
    uint16_t style;
    uint64_t chan;
    auto egc = notcurses_at_yx(nc_, 1, 1, &style, &chan);
    REQUIRE(nullptr != egc);
    CHECK(0 == strcmp(egc, "O"));
    free(egc);
    CHECK(0 == ncpile_rasterize(np));
    egc = notcurses_at_yx(nc_, 1, 1, &style, &chan);
    REQUIRE(nullptr != egc);
    CHECK(0 == strcmp(egc, "X"));
    free(egc);
    // notcurses_render() ought render the standard pile, back to "O"
    CHECK(0 == notcurses_render(nc_));
    egc = notcurses_at_yx(nc_, 1, 1, &style, &chan);
    REQUIRE(nullptr != egc);
    CHECK(0 == strcmp(egc, "O"));
    free(egc);
    ncplane_destroy(np);
  }

  // create a plane bigger than the standard plane, and render it as a pile
  SUBCASE("BiggerPileRender") {
    struct ncplane_options nopts = {
      -1, -1, dimy + 2, dimx + 2, nullptr, "big", nullptr, 0, 0, 0,
    };
    auto np = ncpile_create(nc_, &nopts);
    REQUIRE(nullptr != np);
    CHECK(np == ncplane_parent_const(np));
    CHECK(-1 == ncplane_y(np));
    CHECK(-1 == ncplane_x(np));
    nccell c = NCCELL_CHAR_INITIALIZER('X');
    CHECK(0 < ncplane_polyfill_yx(np, 0, 0, &c));
    nccell o = NCCELL_CHAR_INITIALIZER('O');
    CHECK(0 < ncplane_polyfill_yx(n_, 0, 0, &o));
    CHECK(0 == ncpile_render(np));
    CHECK(0 == ncpile_render(n_));
    CHECK(0 == ncpile_rasterize(n_));
    uint16_t style;
    uint64_t chan;
    auto egc = notcurses_at_yx(nc_, 0, 0, &style, &chan);
    REQUIRE(nullptr != egc);
    CHECK(0 == strcmp(egc, "O"));
    free(egc);
    CHECK(0 == ncpile_rasterize(np));
    egc = notcurses_at_yx(nc_, 0, 0, &style, &chan);
    REQUIRE(nullptr != egc);
    CHECK(0 == strcmp(egc, "X"));
    free(egc);
    // notcurses_render() ought render the standard pile, back to "O"
    CHECK(0 == notcurses_render(nc_));
    egc = notcurses_at_yx(nc_, 0, 0, &style, &chan);
    REQUIRE(nullptr != egc);
    CHECK(0 == strcmp(egc, "O"));
    free(egc);
    ncplane_destroy(np);
  }

  // create a new pile, and rotate subplanes through the root set
  SUBCASE("ShufflePile") {
    struct ncplane_options nopts = {
      1, 1, dimy - 2, dimx - 2, nullptr, "new1", nullptr, 0, 0, 0,
    };
    auto n1 = ncpile_create(nc_, &nopts);
    REQUIRE(nullptr != n1);
    CHECK(n1 == ncplane_parent_const(n1));
    nopts.name = "new2";
    auto n2 = ncplane_create(n1, &nopts);
    REQUIRE(nullptr != n2);
    CHECK(n1 == ncplane_parent_const(n2));
    nopts.name = "new3";
    auto n3 = ncplane_create(n1, &nopts);
    REQUIRE(nullptr != n3);
    CHECK(n1 == ncplane_parent_const(n3));
    CHECK(n3 == n1->blist);
    CHECK(&n3->bnext == n2->bprev);
    CHECK(n2 == *n2->bprev);
    CHECK(nullptr == n2->bnext);
    // we now have n1 -> { n3, n2 }
    // now rotate n2 into the root plane, not touching n3: { n3, n2 } -> n1
    CHECK(nullptr != ncplane_reparent(n1, n2));
    CHECK(n2 == ncplane_parent_const(n2));
    CHECK(n2 == ncplane_parent_const(n1));
    CHECK(n3 == ncplane_parent_const(n3));
    // now rotate n3 under n2, not touching n1: n2 -> { n1, n3 }
    CHECK(nullptr != ncplane_reparent(n3, n2));
    CHECK(n2 == ncplane_parent_const(n1));
    CHECK(n2 == ncplane_parent_const(n3));
    CHECK(n2 == ncplane_parent_const(n2));
    // now rotate n2 under n3, not touching n1: { n1, n3 } -> n2
    CHECK(nullptr != ncplane_reparent(n2, n3));
    CHECK(n1 == ncplane_parent_const(n1));
    CHECK(n3 == ncplane_parent_const(n3));
    CHECK(n3 == ncplane_parent_const(n2));
    // now rotate n3 under n1, not touching n2: { n1, n2 } -> n3 (start state)
    CHECK(nullptr != ncplane_reparent(n3, n1));
    CHECK(n1 == ncplane_parent_const(n1));
    CHECK(n1 == ncplane_parent_const(n3));
    CHECK(n2 == ncplane_parent_const(n2));
    ncplane_destroy(n3);
    ncplane_destroy(n2);
    ncplane_destroy(n1);
  }

  SUBCASE("ShufflePileFamilies") {
    struct ncplane_options nopts = {
      1, 1, dimy - 2, dimx - 2, nullptr, "new1", nullptr, 0, 0, 0,
    };
    auto n1 = ncpile_create(nc_, &nopts);
    REQUIRE(nullptr != n1);
    CHECK(n1 == ncplane_parent_const(n1));
    nopts.name = "new2";
    auto n2 = ncplane_create(n1, &nopts);
    REQUIRE(nullptr != n2);
    CHECK(n1 == ncplane_parent_const(n2));
    nopts.name = "new3";
    auto n3 = ncplane_create(n1, &nopts);
    REQUIRE(nullptr != n3);
    CHECK(n1 == ncplane_parent_const(n3));
    CHECK(n3 == n1->blist);
    CHECK(&n3->bnext == n2->bprev);
    CHECK(n2 == *n2->bprev);
    CHECK(nullptr == n2->bnext);
    nopts.name = "new4";
    auto n4 = ncplane_create(n2, &nopts);
    REQUIRE(nullptr != n4);
    CHECK(n2 == ncplane_parent_const(n4));
    // we now have n1 -> { n3, n2 } -> n4
    // n1 to any ought be refused
    CHECK(nullptr == ncplane_reparent_family(n1, n2));
    CHECK(nullptr == ncplane_reparent_family(n1, n3));
    CHECK(nullptr == ncplane_reparent_family(n1, n4));
    // n2 to n4 ought be refused
    CHECK(nullptr == ncplane_reparent_family(n2, n4));
    ncplane_destroy(n4);
    ncplane_destroy(n3);
    ncplane_destroy(n2);
    ncplane_destroy(n1);
  }

  // the position of a plane is relative to its parent plane. when a plane
  // is removed, the children ought be updated relative to their new parent.
  // grandchildren and further oughtn't be changed.
  SUBCASE("RemoveParentUpdatePos") {
    struct ncplane_options nopts = {
      .y = 10,
      .x = 10,
      .rows = 2,
      .cols = 2,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto gen1 = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != gen1);
    auto gen2 = ncplane_create(gen1, &nopts);
    REQUIRE(nullptr != gen2);
    auto gen3 = ncplane_create(gen2, &nopts);
    REQUIRE(nullptr != gen3);
    int y, x;
    ncplane_abs_yx(gen1, &y, &x);
    CHECK(10 == y);
    CHECK(10 == x);
    CHECK(10 == ncplane_y(gen1));
    CHECK(10 == ncplane_x(gen1));
    ncplane_abs_yx(gen2, &y, &x);
    CHECK(20 == y);
    CHECK(20 == x);
    CHECK(10 == ncplane_y(gen2));
    CHECK(10 == ncplane_x(gen2));
    ncplane_abs_yx(gen3, &y, &x);
    CHECK(30 == y);
    CHECK(30 == x);
    CHECK(10 == ncplane_y(gen3));
    CHECK(10 == ncplane_x(gen3));
    ncplane_destroy(gen1);
    ncplane_abs_yx(gen2, &y, &x);
    CHECK(20 == y); // should remain the same
    CHECK(20 == x);
    CHECK(20 == ncplane_y(gen2)); // should increase
    CHECK(20 == ncplane_x(gen2));
    ncplane_abs_yx(gen3, &y, &x);
    CHECK(30 == y); // should stay the same
    CHECK(30 == x);
    CHECK(10 == ncplane_y(gen3)); // should also stay the same
    CHECK(10 == ncplane_x(gen3));
    ncplane_destroy(gen2);
    ncplane_destroy(gen3);
  }

  // When a plane is reparented, its absy/absx ought be updated to reflect the
  // new parent (and its children ought also be updated). The absolute
  // positions ought not change.
  SUBCASE("ReparentUpdatePos") {
    struct ncplane_options nopts = {
      .y = 10,
      .x = 10,
      .rows = 2,
      .cols = 2,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto gen1 = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != gen1);
    auto gen2 = ncplane_create(gen1, &nopts);
    REQUIRE(nullptr != gen2);
    auto gen3 = ncplane_create(gen2, &nopts);
    REQUIRE(nullptr != gen3);
    int y, x;
    ncplane_abs_yx(gen1, &y, &x);
    CHECK(10 == y);
    CHECK(10 == x);
    CHECK(10 == ncplane_y(gen1));
    CHECK(10 == ncplane_x(gen1));
    ncplane_abs_yx(gen2, &y, &x);
    CHECK(20 == y);
    CHECK(20 == x);
    CHECK(10 == ncplane_y(gen2));
    CHECK(10 == ncplane_x(gen2));
    ncplane_abs_yx(gen3, &y, &x);
    CHECK(30 == y);
    CHECK(30 == x);
    CHECK(10 == ncplane_y(gen3));
    CHECK(10 == ncplane_x(gen3));
    CHECK(nullptr != ncplane_reparent(gen1, gen2));
    ncplane_abs_yx(gen2, &y, &x); // gen2 is now the parent
    CHECK(20 == y);
    CHECK(20 == x);
    CHECK(20 == ncplane_y(gen2));
    CHECK(20 == ncplane_x(gen2));
    ncplane_abs_yx(gen1, &y, &x); // gen1 is below and to the left of gen2
    CHECK(10 == y);
    CHECK(10 == x);
    CHECK(-10 == ncplane_y(gen1));
    CHECK(-10 == ncplane_x(gen1));
    ncplane_abs_yx(gen3, &y, &x);
    CHECK(30 == y); // should stay the same
    CHECK(30 == x);
    CHECK(10 == ncplane_y(gen3)); // remain the same; still a child of gen2
    CHECK(10 == ncplane_x(gen3));
    ncplane_destroy(gen1);
    ncplane_destroy(gen2);
    ncplane_destroy(gen3);
  }

  // common teardown
  CHECK(0 == notcurses_stop(nc_));
}
