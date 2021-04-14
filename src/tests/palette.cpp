#include "main.h"

TEST_CASE("Palette256") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  SUBCASE("AllocAndFree") {
    palette256* p = palette256_new(nc_);
    REQUIRE(nullptr != p);
    palette256_free(p);
  }

  SUBCASE("SetIndexZero") {
    palette256* p = palette256_new(nc_);
    REQUIRE(nullptr != p);
    palette256_set_rgb8(p, 0, 0x80, 0x90, 0xa0);
    unsigned r, g, b;
    palette256_get_rgb8(p, 0, &r, &g, &b);
    CHECK(r == 0x80);
    CHECK(g == 0x90);
    CHECK(b == 0xa0);
    palette256_free(p);
  }

  SUBCASE("SetIndex255") {
    palette256* p = palette256_new(nc_);
    REQUIRE(nullptr != p);
    palette256_set_rgb8(p, 255, 0xa0, 0x70, 0x50);
    unsigned r, g, b;
    palette256_get_rgb8(p, 255, &r, &g, &b);
    CHECK(r == 0xa0);
    CHECK(g == 0x70);
    CHECK(b == 0x50);
    palette256_free(p);
  }

  // when we set a palette index, it ought change us from using default
  SUBCASE("FAttributes") {
    nccell c = CELL_TRIVIAL_INITIALIZER;
    CHECK(cell_fg_default_p(&c));
    nccell_set_fg_alpha(&c, CELL_ALPHA_TRANSPARENT);
    CHECK(0 == cell_set_fg_palindex(&c, 0x20));
    CHECK(!cell_fg_default_p(&c));
    CHECK(cell_fg_palindex_p(&c));
    CHECK(CELL_ALPHA_OPAQUE == cell_fg_alpha(&c));
    CHECK(0x20 == cell_fg_palindex(&c));
  }

  SUBCASE("BAttributes") {
    nccell c = CELL_TRIVIAL_INITIALIZER;
    CHECK(cell_bg_default_p(&c));
    nccell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT);
    CHECK(0 == cell_set_bg_palindex(&c, 0x20));
    CHECK(!cell_bg_default_p(&c));
    CHECK(cell_bg_palindex_p(&c));
    CHECK(CELL_ALPHA_OPAQUE == cell_bg_alpha(&c));
    CHECK(0x20 == cell_bg_palindex(&c));
  }

  // write it to an ncplane, and verify attributes via reflection
  SUBCASE("PutCAttrs") {
    nccell c = CELL_TRIVIAL_INITIALIZER;
    CHECK(1 == nccell_load_char(n_, &c, 'X'));
    CHECK(0 == cell_set_fg_palindex(&c, 0x20));
    CHECK(0 == cell_set_bg_palindex(&c, 0x40));
    CHECK(1 == ncplane_putc_yx(n_, 0, 0, &c));
    nccell_release(n_, &c);
    nccell r = CELL_TRIVIAL_INITIALIZER;
    CHECK(0 < ncplane_at_yx_cell(n_, 0, 0, &r));
    CHECK(cell_fg_palindex_p(&r));
    CHECK(cell_bg_palindex_p(&r));
    CHECK(CELL_ALPHA_OPAQUE == cell_fg_alpha(&r));
    CHECK(CELL_ALPHA_OPAQUE == cell_bg_alpha(&r));
    CHECK(0x20 == cell_fg_palindex(&r));
    CHECK(0x40 == cell_bg_palindex(&r));
    nccell_release(n_, &r);
  }

  SUBCASE("RenderCAttrs") {
    nccell c = CELL_TRIVIAL_INITIALIZER;
    nccell_load_char(n_, &c, 'X');
    CHECK(0 == cell_set_fg_palindex(&c, 0x20));
    CHECK(0 == cell_set_bg_palindex(&c, 0x40));
    CHECK(0 == ncplane_set_fg_palindex(n_, 0x20));
    CHECK(0 == ncplane_set_bg_palindex(n_, 0x40));
    CHECK(0 < ncplane_putc_yx(n_, 0, 0, &c));
    nccell_release(n_, &c);
    CHECK(0 == notcurses_render(nc_));
    nccell r = CELL_TRIVIAL_INITIALIZER;
    CHECK(nullptr != notcurses_at_yx(nc_, 0, 0, &r.stylemask, &r.channels));
    CHECK(cell_fg_palindex_p(&r));
    CHECK(cell_bg_palindex_p(&r));
    CHECK(CELL_ALPHA_OPAQUE == cell_fg_alpha(&r));
    CHECK(CELL_ALPHA_OPAQUE == cell_bg_alpha(&r));
    CHECK(0x20 == cell_fg_palindex(&r));
    CHECK(0x40 == cell_bg_palindex(&r));
    nccell_release(n_, &r);
  }

  // common teardown
  CHECK(0 == notcurses_stop(nc_));
}
