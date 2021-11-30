#include "main.h"

TEST_CASE("Geometry") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  SUBCASE("Center") {
    const struct test {
      unsigned leny, lenx;  // geometries
      int centy, centx;     // pre-calculated centers
    } tests[] = {
      { 1, 1, 0, 0, },
      { 1, 2, 0, 0, },
      { 3, 1, 1, 0, }, { 1, 3, 0, 1, }, { 2, 3, 0, 1, }, { 3, 2, 1, 0, }, { 3, 3, 1, 1, },
      { 4, 1, 1, 0, }, { 1, 4, 0, 1, }, { 2, 4, 0, 1, }, { 4, 2, 0, 1, }, { 3, 4, 1, 1, },
      { 4, 3, 1, 1, }, { 4, 4, 1, 1, }, { 4, 4, 1, 1, },
      { 5, 1, 2, 0, }, { 1, 5, 0, 2, }, { 2, 5, 0, 2, }, { 5, 2, 2, 1, }, { 3, 5, 1, 2, },
      { 5, 3, 2, 1, }, { 4, 5, 1, 2, }, { 5, 4, 2, 1, }, { 5, 5, 2, 2, },
      { 0, 0, 0, 0, }
    }, *t;
    for(t = tests ; !t->leny ; ++t){
      struct ncplane_options nopts = {
        .y = 0,
        .x = 0,
        .rows = t->leny,
        .cols = t->lenx,
        .userptr = nullptr,
        .name = nullptr,
        .resizecb = nullptr,
        .flags = 0,
        .margin_b = 0, .margin_r = 0,
      };
      auto n = ncplane_create(n_, &nopts);
      REQUIRE(n);
      nccell tl = NCCELL_TRIVIAL_INITIALIZER, tr = NCCELL_TRIVIAL_INITIALIZER;
      nccell bl = NCCELL_TRIVIAL_INITIALIZER, br = NCCELL_TRIVIAL_INITIALIZER;
      nccell hl = NCCELL_TRIVIAL_INITIALIZER, vl = NCCELL_TRIVIAL_INITIALIZER;
      CHECK(0 == nccells_double_box(n, 0, 0, &tl, &tr, &bl, &br, &hl, &vl));
      CHECK(0 <= ncplane_perimeter(n, &tl, &tr, &bl, &br, &hl, &vl, 0));
      CHECK(0 == notcurses_render(nc_));
      int y, x;
      ncplane_center(n, &y, &x);
      CHECK(y == t->centy);
      CHECK(x == t->centx);
      CHECK(0 == ncplane_destroy(n));
    }
  }

  SUBCASE("CenterAbs") {
    const struct test {
      unsigned leny, lenx;  // geometries
      int absy, absx;       // location of the origin
      int centy, centx;     // pre-calculated centers
    } tests[] = {
      { 1, 1, 10, 20, 0, 0, },
      { 1, 2, 10, 20, 0, 0, },
      { 3, 1, 10, 20, 1, 0, }, { 1, 3, 10, 20, 0, 1, }, { 2, 3, 10, 20, 0, 1, },
      { 3, 2, 10, 20, 1, 0, }, { 3, 3, 10, 20, 1, 1, },
      { 4, 1, 10, 20, 1, 0, }, { 1, 4, 10, 20, 0, 1, }, { 2, 4, 10, 20, 0, 1, },
      { 4, 2, 10, 20, 0, 1, }, { 3, 4, 10, 20, 1, 1, },
      { 4, 3, 10, 20, 1, 1, }, { 4, 4, 10, 20, 1, 1, }, { 4, 4, 10, 20, 1, 1, },
      { 5, 1, 10, 20, 2, 0, }, { 1, 5, 10, 20, 0, 2, }, { 2, 5, 10, 20, 0, 2, },
      { 5, 2, 10, 20, 2, 1, }, { 3, 5, 10, 20, 1, 2, },
      { 5, 3, 10, 20, 2, 1, }, { 4, 5, 10, 20, 1, 2, }, { 5, 4, 10, 20, 2, 1, },
      { 5, 5, 10, 20, 2, 2, },
      { 0, 0, 10, 20, 0, 0, }
    }, *t;
    for(t = tests ; !t->leny ; ++t){
      struct ncplane_options nopts = {
        .y = t->absy,
        .x = t->absx,
        .rows = t->leny,
        .cols = t->lenx,
        .userptr = nullptr,
        .name = nullptr,
        .resizecb = nullptr,
        .flags = 0,
        .margin_b = 0, .margin_r = 0,
      };
      auto n = ncplane_create(n_, &nopts);
      REQUIRE(n);
      nccell tl = NCCELL_TRIVIAL_INITIALIZER, tr = NCCELL_TRIVIAL_INITIALIZER;
      nccell bl = NCCELL_TRIVIAL_INITIALIZER, br = NCCELL_TRIVIAL_INITIALIZER;
      nccell hl = NCCELL_TRIVIAL_INITIALIZER, vl = NCCELL_TRIVIAL_INITIALIZER;
      CHECK(0 == nccells_double_box(n, 0, 0, &tl, &tr, &bl, &br, &hl, &vl));
      CHECK(0 <= ncplane_perimeter(n, &tl, &tr, &bl, &br, &hl, &vl, 0));
      CHECK(0 == notcurses_render(nc_));
      int y, x;
      ncplane_center_abs(n, &y, &x);
      CHECK(y == t->centy + t->absy);
      CHECK(x == t->centx + t->absx);
      ncplane_center(n, &y, &x);
      CHECK(y == t->centy);
      CHECK(x == t->centx);
      CHECK(0 == ncplane_destroy(n));
    }
  }

  CHECK(0 == notcurses_stop(nc_));

}
