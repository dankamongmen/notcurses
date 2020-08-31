#include "main.h"
#include "egcpool.h"

TEST_CASE("Cell") {
  if(!enforce_utf8()){
    return;
  }
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(nullptr != n_);

  SUBCASE("Loadchar") {
    cell c = CELL_TRIVIAL_INITIALIZER;
    REQUIRE(1 == cell_load(n_, &c, " "));
    CHECK(cell_simple_p(&c));
    cell_release(n_, &c);
  }

  SUBCASE("MultibyteWidth") {
    CHECK(0 == mbswidth(""));       // zero bytes, zero columns
    CHECK(-1 == mbswidth("\x7"));   // single byte, non-printable
    CHECK(1 == mbswidth(" "));      // single byte, one column
    CHECK(5 == mbswidth("abcde"));  // single byte, one column
    CHECK(1 == mbswidth("µ"));      // two bytes, one column
    CHECK(1 <= mbswidth("\xf0\x9f\xa6\xb2"));     // four bytes, two columns
    CHECK(3 <= mbswidth("平仮名")); // nine bytes, six columns
    CHECK(1 == mbswidth("\ufdfd")); // three bytes, ? columns, wcwidth() returns 1
  }

  SUBCASE("SetItalic") {
    cell c = CELL_TRIVIAL_INITIALIZER;
    int dimy, dimx;
    notcurses_term_dim_yx(nc_, &dimy, &dimx);
    cell_styles_set(&c, NCSTYLE_ITALIC);
    REQUIRE(1 == cell_load(n_, &c, "i"));
    cell_set_fg_rgb(&c, 255, 255, 255);
    ncplane_set_base_cell(n_, &c);
    cell_release(n_, &c);
    CHECK(0 == notcurses_render(nc_));
    cell_styles_off(&c, NCSTYLE_ITALIC);
  }

  SUBCASE("SetBold") {
    cell c = CELL_TRIVIAL_INITIALIZER;
    int dimy, dimx;
    notcurses_term_dim_yx(nc_, &dimy, &dimx);
    cell_styles_set(&c, NCSTYLE_BOLD);
    REQUIRE(1 == cell_load(n_, &c, "b"));
    cell_set_fg_rgb(&c, 255, 255, 255);
    ncplane_set_base_cell(n_, &c);
    cell_release(n_, &c);
    CHECK(0 == notcurses_render(nc_));
    cell_styles_off(&c, NCSTYLE_BOLD);
  }

  SUBCASE("SetUnderline") {
    cell c = CELL_TRIVIAL_INITIALIZER;
    int dimy, dimx;
    notcurses_term_dim_yx(nc_, &dimy, &dimx);
    cell_styles_set(&c, NCSTYLE_UNDERLINE);
    REQUIRE(1 == cell_load(n_, &c, "u"));
    cell_set_fg_rgb(&c, 255, 255, 255);
    ncplane_set_base_cell(n_, &c);
    cell_release(n_, &c);
    CHECK(0 == notcurses_render(nc_));
    cell_styles_off(&c, NCSTYLE_UNDERLINE);
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
    CHECK(cell_fg_default_p(&c));
    CHECK(cell_bg_default_p(&c));
    CHECK(CELL_ALPHA_OPAQUE == cell_fg_alpha(&c));
    CHECK(0 == cell_set_fg_alpha(&c, CELL_ALPHA_HIGHCONTRAST));
    CHECK(CELL_ALPHA_HIGHCONTRAST == cell_fg_alpha(&c));
    CHECK(!cell_fg_default_p(&c));
    CHECK(cell_bg_default_p(&c));
  }

  SUBCASE("CellSetBGAlpha"){
    cell c = CELL_TRIVIAL_INITIALIZER;
    CHECK(0 > cell_set_bg_alpha(&c, -1));
    CHECK(0 > cell_set_bg_alpha(&c, 4));
    CHECK(0 == cell_set_bg_alpha(&c, CELL_ALPHA_OPAQUE));
    CHECK(CELL_ALPHA_OPAQUE == cell_bg_alpha(&c));
    CHECK(0 != cell_set_bg_alpha(&c, CELL_ALPHA_HIGHCONTRAST));
    CHECK(0 == cell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT));
    CHECK(CELL_ALPHA_TRANSPARENT == cell_bg_alpha(&c));
    CHECK(cell_fg_default_p(&c));
    CHECK(!cell_bg_default_p(&c));
  }

  // white on a black background ought be unmolested for highcontrast
  SUBCASE("HighContrastWhiteOnBlackBackground"){
    cell c = CELL_CHAR_INITIALIZER('+');
    CHECK(0 == cell_set_fg_rgb(&c, 0xff, 0xff, 0xff));
    CHECK(0 == cell_set_fg_alpha(&c, CELL_ALPHA_HIGHCONTRAST));
    CHECK(0 == cell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT));
    auto np = ncplane_new(nc_, 1, 1, 0, 0, nullptr);
    REQUIRE(nullptr != np);
    CHECK(1 == ncplane_putc(np, &c));
    cell_load_char(np, &c, '*');
    CHECK(0 == cell_set_bg_rgb(&c, 0x0, 0x0, 0x0));
    CHECK(0 == cell_set_bg_alpha(&c, CELL_ALPHA_OPAQUE));
    CHECK(1 == ncplane_putc(n_, &c));
    CHECK(0 == notcurses_render(nc_));
    uint64_t channels, underchannels, overchannels;
    auto egc = notcurses_at_yx(nc_, 0, 0, nullptr, &channels);
    REQUIRE(nullptr != egc);
    auto negc = ncplane_at_yx(n_, 0, 0, nullptr, &underchannels);
    REQUIRE(nullptr != negc);
    auto topegc = ncplane_at_yx(np, 0, 0, nullptr, &overchannels);
    REQUIRE(nullptr != topegc);
    CHECK(channels_bg(channels) == channels_bg(underchannels));
    CHECK(channels_fg(channels) == channels_fg(overchannels));
    ncplane_destroy(np);
    free(topegc);
    free(negc);
    free(egc);
  }

  // white on a white background ought be changed for highcontrast
  SUBCASE("HighContrastWhiteOnWhiteBackground"){
    cell c = CELL_CHAR_INITIALIZER('+');
    CHECK(0 == cell_set_fg_rgb(&c, 0xff, 0xff, 0xff));
    CHECK(0 == cell_set_fg_alpha(&c, CELL_ALPHA_HIGHCONTRAST));
    CHECK(0 == cell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT));
    auto np = ncplane_new(nc_, 1, 1, 0, 0, nullptr);
    REQUIRE(nullptr != np);
    CHECK(1 == ncplane_putc(np, &c));
    cell_load_char(np, &c, '*');
    CHECK(0 == cell_set_bg_rgb(&c, 0xff, 0xff, 0xff));
    CHECK(0 == cell_set_bg_alpha(&c, CELL_ALPHA_OPAQUE));
    CHECK(1 == ncplane_putc(n_, &c));
    CHECK(0 == notcurses_render(nc_));
    uint64_t channels, underchannels, overchannels;
    auto egc = notcurses_at_yx(nc_, 0, 0, nullptr, &channels);
    REQUIRE(nullptr != egc);
    auto negc = ncplane_at_yx(n_, 0, 0, nullptr, &underchannels);
    REQUIRE(nullptr != negc);
    auto topegc = ncplane_at_yx(np, 0, 0, nullptr, &overchannels);
    REQUIRE(nullptr != topegc);
    CHECK(channels_bg(channels) == channels_bg(underchannels));
    CHECK(channels_fg(channels) < channels_fg(overchannels));
    ncplane_destroy(np);
    free(topegc);
    free(negc);
    free(egc);
  }

  // black on a black background must be changed for highcontrast
  SUBCASE("HighContrastBlackOnBlackBackground"){
    cell c = CELL_CHAR_INITIALIZER('+');
    CHECK(0 == cell_set_fg_rgb(&c, 0x0, 0x0, 0x0));
    CHECK(0 == cell_set_fg_alpha(&c, CELL_ALPHA_HIGHCONTRAST));
    CHECK(0 == cell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT));
    auto np = ncplane_new(nc_, 1, 1, 0, 0, nullptr);
    REQUIRE(nullptr != np);
    CHECK(1 == ncplane_putc(np, &c));
    cell_load_char(np, &c, '*');
    CHECK(0 == cell_set_bg_rgb(&c, 0x0, 0x0, 0x0));
    CHECK(0 == cell_set_bg_alpha(&c, CELL_ALPHA_OPAQUE));
    CHECK(1 == ncplane_putc(n_, &c));
    CHECK(0 == notcurses_render(nc_));
    uint64_t channels, underchannels, overchannels;
    auto egc = notcurses_at_yx(nc_, 0, 0, nullptr, &channels);
    REQUIRE(nullptr != egc);
    auto negc = ncplane_at_yx(n_, 0, 0, nullptr, &underchannels);
    REQUIRE(nullptr != negc);
    auto topegc = ncplane_at_yx(np, 0, 0, nullptr, &overchannels);
    REQUIRE(nullptr != topegc);
    CHECK(channels_bg(channels) == channels_bg(underchannels));
    CHECK(channels_fg(channels) > channels_fg(overchannels));
    ncplane_destroy(np);
    free(topegc);
    free(negc);
    free(egc);
  }

  // black on a white background ought be unmolested for highcontrast
  SUBCASE("HighContrastBlackOnWhiteBackground"){
    cell c = CELL_CHAR_INITIALIZER('+');
    CHECK(0 == cell_set_fg_rgb(&c, 0x0, 0x0, 0x0));
    CHECK(0 == cell_set_fg_alpha(&c, CELL_ALPHA_HIGHCONTRAST));
    CHECK(0 == cell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT));
    auto np = ncplane_new(nc_, 1, 1, 0, 0, nullptr);
    REQUIRE(nullptr != np);
    CHECK(1 == ncplane_putc(np, &c));
    cell_load_char(np, &c, '*');
    CHECK(0 == cell_set_bg_rgb(&c, 0xff, 0xff, 0xff));
    CHECK(0 == cell_set_bg_alpha(&c, CELL_ALPHA_OPAQUE));
    CHECK(1 == ncplane_putc(n_, &c));
    CHECK(0 == notcurses_render(nc_));
    uint64_t channels, underchannels, overchannels;
    auto egc = notcurses_at_yx(nc_, 0, 0, nullptr, &channels);
    REQUIRE(nullptr != egc);
    auto negc = ncplane_at_yx(n_, 0, 0, nullptr, &underchannels);
    REQUIRE(nullptr != negc);
    auto topegc = ncplane_at_yx(np, 0, 0, nullptr, &overchannels);
    REQUIRE(nullptr != topegc);
    CHECK(channels_bg(channels) == channels_bg(underchannels));
    CHECK(channels_fg(channels) == channels_fg(overchannels));
    ncplane_destroy(np);
    free(topegc);
    free(negc);
    free(egc);
  }

  // high contrast ought only be activated relevant to the background equal to
  // or below them, not above.
  SUBCASE("HighContrastBelowOnly"){
    cell c = CELL_TRIVIAL_INITIALIZER;
    // top has a background of white
    CHECK(0 == cell_set_bg_rgb(&c, 0xff, 0xff, 0xff));
    CHECK(0 == cell_set_fg_alpha(&c, CELL_ALPHA_TRANSPARENT));
    auto np = ncplane_new(nc_, 1, 1, 0, 0, nullptr);
    REQUIRE(nullptr != np);
    CHECK(1 == ncplane_putc(np, &c));
    cell_load_char(n_, &c, '*');
    // bottom has white foreground + HIGHCONTRAST, should remain white
    CHECK(0 == cell_set_fg_rgb(&c, 0xff, 0x0, 0xff));
    CHECK(0 == cell_set_fg_alpha(&c, CELL_ALPHA_HIGHCONTRAST));
    cell_set_bg_default(&c);
    CHECK(1 == ncplane_putc(n_, &c));
    CHECK(0 == notcurses_render(nc_));
    uint64_t channels, underchannels, overchannels;
    auto egc = notcurses_at_yx(nc_, 0, 0, nullptr, &channels);
    REQUIRE(nullptr != egc);
    auto negc = ncplane_at_yx(n_, 0, 0, nullptr, &underchannels);
    REQUIRE(nullptr != negc);
    auto topegc = ncplane_at_yx(np, 0, 0, nullptr, &overchannels);
    REQUIRE(nullptr != topegc);
    CHECK(channels_bg(channels) == channels_bg(overchannels));
    CHECK(channels_fg(channels) < channels_fg(underchannels));
    free(topegc);
    free(negc);
    free(egc);
  }

  // common teardown
  CHECK(0 == notcurses_stop(nc_));
}
