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
      1, 1, dimy - 2, dimx - 2, nullptr, "big", nullptr, 0,
    };
    auto np = ncpile_create(nc_, &nopts);
    REQUIRE(nullptr != np);
    CHECK(np == ncplane_parent_const(np));
    CHECK(1 == ncplane_y(np));
    CHECK(1 == ncplane_x(np));
    cell c = CELL_CHAR_INITIALIZER('X');
    CHECK(0 < ncplane_polyfill_yx(np, 0, 0, &c));
    cell o = CELL_CHAR_INITIALIZER('O');
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
    cell c = CELL_CHAR_INITIALIZER('X');
    CHECK(0 < ncplane_polyfill_yx(np, 0, 0, &c));
    cell o = CELL_CHAR_INITIALIZER('O');
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

  // common teardown
  CHECK(0 == notcurses_stop(nc_));
}
