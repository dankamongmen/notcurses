#include "main.h"

TEST_CASE("Piles") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  int dimy, dimx;
  struct ncplane* n_ = notcurses_stddim_yx(nc_, &dimy, &dimx);
  REQUIRE(nullptr != n_);

  // create a plane bigger than the standard plane, and render it as a pile
  SUBCASE("SmallerPileRender") {
    struct ncplane_options nopts = {
      1, 1, dimy - 2, dimx - 2, nullptr, "small", nullptr, 0,
    };
    auto np = ncpile_create(nc_, &nopts);
    REQUIRE(nullptr != np);
    CHECK(np == ncplane_parent_const(np));
    CHECK(1 == ncplane_y(np));
    CHECK(1 == ncplane_x(np));
    nccell c = CELL_CHAR_INITIALIZER('X');
    CHECK(0 < ncplane_polyfill_yx(np, 0, 0, &c));
    nccell o = CELL_CHAR_INITIALIZER('O');
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
      -1, -1, dimy + 2, dimx + 2, nullptr, "big", nullptr, 0,
    };
    auto np = ncpile_create(nc_, &nopts);
    REQUIRE(nullptr != np);
    CHECK(np == ncplane_parent_const(np));
    CHECK(-1 == ncplane_y(np));
    CHECK(-1 == ncplane_x(np));
    nccell c = CELL_CHAR_INITIALIZER('X');
    CHECK(0 < ncplane_polyfill_yx(np, 0, 0, &c));
    nccell o = CELL_CHAR_INITIALIZER('O');
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
      1, 1, dimy - 2, dimx - 2, nullptr, "new1", nullptr, 0,
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
      1, 1, dimy - 2, dimx - 2, nullptr, "new1", nullptr, 0,
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

  // common teardown
  CHECK(0 == notcurses_stop(nc_));
}
