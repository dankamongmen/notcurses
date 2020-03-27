#include <cstdlib>
#include <iostream>
#include "internal.h"
#include "main.h"

TEST_CASE("RenderTest") {
  if(!enforce_utf8()){
    return;
  }
  if(getenv("TERM") == nullptr){
    return;
  }
  notcurses_options nopts{};
  nopts.inhibit_alternate_screen = true;
  nopts.suppress_banner = true;
  FILE* outfp_ = fopen("/dev/tty", "wb");
  REQUIRE(outfp_);
  struct notcurses* nc_ = notcurses_init(&nopts, outfp_);
  REQUIRE(nc_);
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  // If an ncplane is moved atop the right half of a wide glyph, the entire
  // glyph should be oblitrated.
  SUBCASE("PlaneStompsWideGlyph"){
    cell c = CELL_TRIVIAL_INITIALIZER;
    char* egc;

    // print two wide glyphs on the standard plane
    int y, x;
    ncplane_yx(n_, &y, &x);
    CHECK(0 == y);
    CHECK(0 == x);
    CHECK(3 == ncplane_putstr(n_, "\xe5\x85\xa8"));
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == y);
    CHECK(2 == x);
    CHECK(3 == ncplane_putstr(n_, "\xe5\xbd\xa2"));
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == y);
    CHECK(4 == x);
    CHECK(!notcurses_render(nc_));

    // should be wide char 1
    REQUIRE(3 == ncplane_at_yx(n_, 0, 0, &c));
    egc = cell_egc_copy(n_, &c);
    REQUIRE(egc);
    CHECK(!strcmp("\xe5\x85\xa8", egc));
    CHECK(cell_double_wide_p(&c));
    free(egc);
    egc = notcurses_at_yx(nc_, 0, 0, &c.attrword, &c.channels);
    REQUIRE(egc);
    CHECK(!strcmp("\xe5\x85\xa8", egc));
    CHECK(cell_double_wide_p(&c));
    free(egc);
    cell_init(&c);
    // should be wide char 1 right side
    REQUIRE(0 == ncplane_at_yx(n_, 0, 1, &c));
    egc = cell_egc_copy(n_, &c);
    REQUIRE(egc);
    CHECK(!strcmp("", egc));
    CHECK(cell_double_wide_p(&c));
    free(egc);
    egc = notcurses_at_yx(nc_, 0, 1, &c.attrword, &c.channels);
    REQUIRE(egc);
    CHECK(!strcmp("", egc));
    CHECK(cell_double_wide_p(&c));
    free(egc);
    cell_init(&c);

    // should be wide char 2
    REQUIRE(3 == ncplane_at_yx(n_, 0, 2, &c));
    egc = cell_egc_copy(n_, &c);
    REQUIRE(egc);
    CHECK(!strcmp("\xe5\xbd\xa2", egc));
    CHECK(cell_double_wide_p(&c));
    free(egc);
    egc = notcurses_at_yx(nc_, 0, 2, &c.attrword, &c.channels);
    REQUIRE(egc);
    CHECK(!strcmp("\xe5\xbd\xa2", egc));
    CHECK(cell_double_wide_p(&c));
    free(egc);
    cell_init(&c);
    // should be wide char 2 right side
    CHECK(0 == ncplane_at_yx(n_, 0, 3, &c));
    egc = cell_egc_copy(n_, &c);
    REQUIRE(egc);
    CHECK(!strcmp("", egc));
    CHECK(cell_double_wide_p(&c));
    free(egc);
    egc = notcurses_at_yx(nc_, 0, 3, &c.attrword, &c.channels);
    REQUIRE(egc);
    CHECK(!strcmp("", egc));
    CHECK(cell_double_wide_p(&c));
    free(egc);
    cell_init(&c);

    struct ncplane* n = ncplane_new(nc_, 1, 2, 0, 1, nullptr);
    REQUIRE(n);
    CHECK(0 < ncplane_putstr(n, "AB"));
    CHECK(!notcurses_render(nc_));

    // should be nothing, having been stomped
    egc = notcurses_at_yx(nc_, 0, 0, &c.attrword, &c.channels);
    REQUIRE(egc);
    CHECK(!strcmp(" ", egc));
    free(egc);
    cell_init(&c);
    // should be character from higher plane
    egc = notcurses_at_yx(nc_, 0, 1, &c.attrword, &c.channels);
    REQUIRE(egc);
    CHECK(!strcmp("A", egc));
    free(egc);
    cell_init(&c);

    egc = notcurses_at_yx(nc_, 0, 2, &c.attrword, &c.channels);
    REQUIRE(egc);
    CHECK(!strcmp("B", egc));
    free(egc);
    cell_init(&c);

    // should be nothing, having been stomped
    egc = notcurses_at_yx(nc_, 0, 3, &c.attrword, &c.channels);
    REQUIRE(egc);
    CHECK(!strcmp(" ", egc));
    free(egc);
    cell_init(&c);

    CHECK(!ncplane_destroy(n));
  }


  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));
}
