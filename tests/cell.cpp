#include <notcurses.h>
#include "egcpool.h"
#include "main.h"

TEST_CASE("Cell") {
  // common initialization
  if(getenv("TERM") == nullptr){
    return;
  }
  FILE* outfp_{};
  outfp_ = fopen("/dev/tty", "wb");
  REQUIRE(nullptr != outfp_);
  notcurses_options nopts{};
  nopts.inhibit_alternate_screen = true;
  nopts.suppress_banner = true;
  struct notcurses* nc_ = notcurses_init(&nopts, outfp_);
  REQUIRE(nullptr != nc_);
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(nullptr != n_);

  SUBCASE("LoadSimple") {
    cell c = CELL_TRIVIAL_INITIALIZER;
    REQUIRE(1 == cell_load(n_, &c, " "));
    CHECK(cell_simple_p(&c));
    cell_release(n_, &c);
  }

SUBCASE("SetItalic") {
  cell c = CELL_TRIVIAL_INITIALIZER;
    int dimy, dimx;
    notcurses_term_dim_yx(nc_, &dimy, &dimx);
    cell_styles_set(&c, CELL_STYLE_ITALIC);
    REQUIRE(1 == cell_load(n_, &c, "i"));
    cell_set_fg_rgb(&c, 255, 255, 255);
    ncplane_set_default(n_, &c);
    cell_release(n_, &c);
    CHECK(0 == notcurses_render(nc_));
    cell_styles_off(&c, CELL_STYLE_ITALIC);
  }

  SUBCASE("SetBold") {
    cell c = CELL_TRIVIAL_INITIALIZER;
    int dimy, dimx;
    notcurses_term_dim_yx(nc_, &dimy, &dimx);
    cell_styles_set(&c, CELL_STYLE_BOLD);
    REQUIRE(1 == cell_load(n_, &c, "b"));
    cell_set_fg_rgb(&c, 255, 255, 255);
    ncplane_set_default(n_, &c);
    cell_release(n_, &c);
    CHECK(0 == notcurses_render(nc_));
    cell_styles_off(&c, CELL_STYLE_BOLD);
  }

  SUBCASE("SetUnderline") {
    cell c = CELL_TRIVIAL_INITIALIZER;
    int dimy, dimx;
    notcurses_term_dim_yx(nc_, &dimy, &dimx);
    cell_styles_set(&c, CELL_STYLE_UNDERLINE);
    REQUIRE(1 == cell_load(n_, &c, "u"));
    cell_set_fg_rgb(&c, 255, 255, 255);
    ncplane_set_default(n_, &c);
    cell_release(n_, &c);
    CHECK(0 == notcurses_render(nc_));
    cell_styles_off(&c, CELL_STYLE_UNDERLINE);
  }

/*  SUBCASE("CellLoadTamil") {
  const char zerodeg[] = "\u0bb8\u0bc0\u0bb0\u0bc7\u0bb3\u0b95\u0bbf\u0b95\u0bbf\u0bb0\u0bbf";
  cell c = CELL_TRIVIAL_INITIALIZER;
  size_t ulen = cell_load(n_, &c, zerodeg);
  // First have U+0BB8 TAMIL LETTER SA U+0BC0 TAMIL VOWEL SIGN II
  // // e0 ae b8 e0 af 80
  REQUIRE(6 == ulen);
  ulen = cell_load(n_, &c, zerodeg + ulen);
  // U+0BB0 TAMIL LETTER RA U+0BCB TAMIL VOWEL SIGN OO
  // e0 ae b0 e0 af 8b
  REQUIRE(6 == ulen);
  // FIXME
  }*/

  SUBCASE("CellSetFGAlpha"){
    cell c = CELL_TRIVIAL_INITIALIZER;
    CHECK(0 > cell_set_fg_alpha(&c, -1));
    CHECK(0 > cell_set_fg_alpha(&c, 4));
    CHECK(0 == cell_set_fg_alpha(&c, CELL_ALPHA_OPAQUE));
    CHECK(CELL_ALPHA_OPAQUE == cell_get_fg_alpha(&c));
    CHECK(0 == cell_set_fg_alpha(&c, CELL_ALPHA_HIGHCONTRAST));
    CHECK(CELL_ALPHA_HIGHCONTRAST == cell_get_fg_alpha(&c));
    CHECK(cell_fg_default_p(&c));
    CHECK(cell_bg_default_p(&c));
  }

  SUBCASE("CellSetBGAlpha"){
    cell c = CELL_TRIVIAL_INITIALIZER;
    CHECK(0 > cell_set_bg_alpha(&c, -1));
    CHECK(0 > cell_set_bg_alpha(&c, 4));
    CHECK(0 == cell_set_bg_alpha(&c, CELL_ALPHA_OPAQUE));
    CHECK(CELL_ALPHA_OPAQUE == cell_get_bg_alpha(&c));
    CHECK(0 != cell_set_bg_alpha(&c, CELL_ALPHA_HIGHCONTRAST));
    CHECK(0 == cell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT));
    CHECK(CELL_ALPHA_TRANSPARENT == cell_get_bg_alpha(&c));
    CHECK(cell_fg_default_p(&c));
    CHECK(cell_bg_default_p(&c));
  }

  // common teardown
  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));
}

