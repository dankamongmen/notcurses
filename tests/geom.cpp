#include "main.h"

TEST_CASE("Geometry") {

  if(getenv("TERM") == nullptr){
    return;
  }
  notcurses_options nopts{};
  nopts.inhibit_alternate_screen = true;
  nopts.suppress_banner = true;
  FILE* outfp_ = fopen("/dev/tty", "wb");
  REQUIRE(outfp_);
  notcurses* nc_ = notcurses_init(&nopts, outfp_);
  REQUIRE(nc_);
  ncplane* ncp_ = notcurses_stdplane(nc_);
  REQUIRE(ncp_);

  SUBCASE("Center") {
    const struct test {
      int leny, lenx;       // geometries
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
      auto n = ncplane_new(nc_, t->leny, t->lenx, 0, 0, nullptr);
      REQUIRE(n);
      cell tl = CELL_TRIVIAL_INITIALIZER; cell tr = CELL_TRIVIAL_INITIALIZER;
      cell bl = CELL_TRIVIAL_INITIALIZER; cell br = CELL_TRIVIAL_INITIALIZER;
      cell hl = CELL_TRIVIAL_INITIALIZER; cell vl = CELL_TRIVIAL_INITIALIZER;
      CHECK(0 == cells_double_box(n, 0, 0, &tl, &tr, &bl, &br, &hl, &vl));
      CHECK(0 <= ncplane_perimeter(n, &tl, &tr, &bl, &br, &hl, &vl, 0));
      CHECK(0 == notcurses_render(nc_));
      CHECK(0 == ncplane_destroy(n));
    }
  }

}
