#include "main.h"

TEST_CASE("Palette") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  SUBCASE("AllocAndFree") {
    ncpalette* p = ncpalette_new(nc_);
    REQUIRE(nullptr != p);
    ncpalette_free(p);
  }

  SUBCASE("SetIndexZero") {
    ncpalette* p = ncpalette_new(nc_);
    REQUIRE(nullptr != p);
    CHECK(0 == ncpalette_set_rgb8(p, 0, 0x80, 0x90, 0xa0));
    unsigned r, g, b;
    ncpalette_get_rgb8(p, 0, &r, &g, &b);
    CHECK(r == 0x80);
    CHECK(g == 0x90);
    CHECK(b == 0xa0);
    ncpalette_free(p);
  }

  SUBCASE("SetIndex255") {
    ncpalette* p = ncpalette_new(nc_);
    REQUIRE(nullptr != p);
    CHECK(0 == ncpalette_set_rgb8(p, 255, 0xa0, 0x70, 0x50));
    unsigned r, g, b;
    ncpalette_get_rgb8(p, 255, &r, &g, &b);
    CHECK(r == 0xa0);
    CHECK(g == 0x70);
    CHECK(b == 0x50);
    ncpalette_free(p);
  }

  // when we set a palette index, it ought change us from using default
  SUBCASE("FAttributes") {
    nccell c = NCCELL_TRIVIAL_INITIALIZER;
    CHECK(nccell_fg_default_p(&c));
    CHECK(0 == nccell_set_fg_alpha(&c, NCALPHA_TRANSPARENT));
    CHECK(0 == nccell_set_fg_palindex(&c, 0x20));
    CHECK(!nccell_fg_default_p(&c));
    CHECK(nccell_fg_palindex_p(&c));
    CHECK(NCALPHA_OPAQUE == nccell_fg_alpha(&c));
    CHECK(0x20 == nccell_fg_palindex(&c));
  }

  SUBCASE("BAttributes") {
    nccell c = NCCELL_TRIVIAL_INITIALIZER;
    CHECK(nccell_bg_default_p(&c));
    CHECK(0 == nccell_set_bg_alpha(&c, NCALPHA_TRANSPARENT));
    CHECK(0 == nccell_set_bg_palindex(&c, 0x20));
    CHECK(!nccell_bg_default_p(&c));
    CHECK(nccell_bg_palindex_p(&c));
    CHECK(NCALPHA_OPAQUE == nccell_bg_alpha(&c));
    CHECK(0x20 == nccell_bg_palindex(&c));
  }

  // write it to an ncplane, and verify attributes via reflection
  SUBCASE("PutCAttrs") {
    nccell c = NCCELL_TRIVIAL_INITIALIZER;
    CHECK(1 == nccell_load_char(n_, &c, 'X'));
    CHECK(0 == nccell_set_fg_palindex(&c, 0x20));
    CHECK(0 == nccell_set_bg_palindex(&c, 0x40));
    CHECK(1 == ncplane_putc_yx(n_, 0, 0, &c));
    nccell_release(n_, &c);
    nccell r = NCCELL_TRIVIAL_INITIALIZER;
    CHECK(0 < ncplane_at_yx_cell(n_, 0, 0, &r));
    CHECK(nccell_fg_palindex_p(&r));
    CHECK(nccell_bg_palindex_p(&r));
    CHECK(NCALPHA_OPAQUE == nccell_fg_alpha(&r));
    CHECK(NCALPHA_OPAQUE == nccell_bg_alpha(&r));
    CHECK(0x20 == nccell_fg_palindex(&r));
    CHECK(0x40 == nccell_bg_palindex(&r));
    nccell_release(n_, &r);
  }

  SUBCASE("RenderCAttrs") {
    nccell c = NCCELL_TRIVIAL_INITIALIZER;
    nccell_load_char(n_, &c, 'X');
    CHECK(0 == nccell_set_fg_palindex(&c, 0x20));
    CHECK(0 == nccell_set_bg_palindex(&c, 0x40));
    CHECK(0 == nccell_set_fg_alpha(&c, NCALPHA_OPAQUE));
    CHECK(0 == nccell_set_bg_alpha(&c, NCALPHA_OPAQUE));
    CHECK(0 < ncplane_putc_yx(n_, 0, 0, &c));
    nccell_release(n_, &c);
    CHECK(0 == notcurses_render(nc_));
    nccell r = NCCELL_TRIVIAL_INITIALIZER;
    auto egc = notcurses_at_yx(nc_, 0, 0, &r.stylemask, &r.channels);
    CHECK(nullptr != egc);
    free(egc);
    CHECK(nccell_fg_palindex_p(&r));
    CHECK(nccell_bg_palindex_p(&r));
    CHECK(NCALPHA_OPAQUE == nccell_fg_alpha(&r));
    CHECK(NCALPHA_OPAQUE == nccell_bg_alpha(&r));
    CHECK(0x20 == nccell_fg_palindex(&r));
    CHECK(0x40 == nccell_bg_palindex(&r));
    nccell_release(n_, &r);
  }

  SUBCASE("HorizontalLineAttrs") {
    nccell c = NCCELL_TRIVIAL_INITIALIZER;
    REQUIRE(1 == nccell_load_char(n_, &c, '-'));
    REQUIRE(0 == nccell_set_fg_palindex(&c, 6));
    REQUIRE(0 == nccell_set_bg_palindex(&c, 8));
    REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));
    REQUIRE(3 == ncplane_hline(n_, &c, 3));
    nccell_release(n_, &c);
    for(int x = 0 ; x < 3 ; ++x){
      nccell r = NCCELL_TRIVIAL_INITIALIZER;
      REQUIRE(1 == ncplane_at_yx_cell(n_, 0, x, &r));
      CHECK(nccell_fg_palindex_p(&r));
      CHECK(nccell_bg_palindex_p(&r));
      CHECK(6 == nccell_fg_palindex(&r));
      CHECK(8 == nccell_bg_palindex(&r));
      nccell_release(n_, &r);
    }
  }

  SUBCASE("VerticalLineAttrs") {
    nccell c = NCCELL_TRIVIAL_INITIALIZER;
    REQUIRE(1 == nccell_load_char(n_, &c, '|'));
    REQUIRE(0 == nccell_set_fg_palindex(&c, 6));
    REQUIRE(0 == nccell_set_bg_palindex(&c, 8));
    REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));
    REQUIRE(3 == ncplane_vline(n_, &c, 3));
    nccell_release(n_, &c);
    for(int y = 0 ; y < 3 ; ++y){
      nccell r = NCCELL_TRIVIAL_INITIALIZER;
      REQUIRE(1 == ncplane_at_yx_cell(n_, y, 0, &r));
      CHECK(nccell_fg_palindex_p(&r));
      CHECK(nccell_bg_palindex_p(&r));
      CHECK(6 == nccell_fg_palindex(&r));
      CHECK(8 == nccell_bg_palindex(&r));
      nccell_release(n_, &r);
    }
  }

  // common teardown
  CHECK(0 == notcurses_stop(nc_));
}
