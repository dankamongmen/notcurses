#include "main.h"
#include "egcpool.h"

TEST_CASE("Cell") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  if(!notcurses_canutf8(nc_)){
    CHECK(0 == notcurses_stop(nc_));
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(nullptr != n_);

  SUBCASE("EGCs") {
    int cols, bytes;
    bytes = utf8_egc_len("é", &cols);
    CHECK(2 == bytes);
    CHECK(1 == cols);
    bytes = utf8_egc_len("\x41\u0301", &cols);
    CHECK(3 == bytes);
    CHECK(1 == cols);
    bytes = utf8_egc_len(" ி", &cols);
    CHECK(4 == bytes);
#ifdef __linux__
    CHECK(2 == cols);
#else
    CHECK(1 == cols);
#endif
    bytes = utf8_egc_len(" ि", &cols);
    CHECK(4 == bytes);
#ifdef __linux__
    CHECK(2 == cols);
#else
    CHECK(1 == cols);
#endif
    bytes = utf8_egc_len("◌̈", &cols);
    CHECK(5 == bytes);
    CHECK(1 == cols);
    bytes = utf8_egc_len("นี้", &cols);
    CHECK(9 == bytes);
    CHECK(1 == cols);
  }

  SUBCASE("Loadchar") {
    nccell c = CELL_TRIVIAL_INITIALIZER;
    CHECK(1 == nccell_load(n_, &c, " "));
    CHECK(cell_simple_p(&c));
    nccell_release(n_, &c);
  }

  SUBCASE("MultibyteWidth") {
    CHECK(0 == ncstrwidth(""));       // zero bytes, zero columns
    CHECK(-1 == ncstrwidth("\x7"));   // single byte, non-printable
    CHECK(1 == ncstrwidth(" "));      // single byte, one column
    CHECK(5 == ncstrwidth("abcde"));  // single byte, one column
    CHECK(1 == ncstrwidth("µ"));      // two bytes, one column
    CHECK(1 <= ncstrwidth("\U0001f982"));     // four bytes, two columns
    CHECK(3 <= ncstrwidth("平仮名")); // nine bytes, six columns
    CHECK(1 == ncstrwidth("\ufdfd")); // three bytes, ? columns, wcwidth() returns 1
  }

  // test combining characters and ZWJs
  SUBCASE("MultiglyphWidth") {
    CHECK(2 == ncstrwidth("\U0001F471"));
    CHECK(2 == ncstrwidth("\U0001F471\u200D"));
    CHECK(3 == ncstrwidth("\U0001F471\u200D\u2640")); // *not* a single EGC!
  }

  SUBCASE("SetItalic") {
    nccell c = CELL_TRIVIAL_INITIALIZER;
    int dimy, dimx;
    notcurses_term_dim_yx(nc_, &dimy, &dimx);
    nccell_set_styles(&c, NCSTYLE_ITALIC);
    CHECK(1 == nccell_load(n_, &c, "i"));
    nccell_set_fg_rgb8(&c, 255, 255, 255);
    ncplane_set_base_cell(n_, &c);
    nccell_release(n_, &c);
    CHECK(0 == notcurses_render(nc_));
    nccell_off_styles(&c, NCSTYLE_ITALIC);
  }

  SUBCASE("SetBold") {
    nccell c = CELL_TRIVIAL_INITIALIZER;
    int dimy, dimx;
    notcurses_term_dim_yx(nc_, &dimy, &dimx);
    nccell_set_styles(&c, NCSTYLE_BOLD);
    CHECK(1 == nccell_load(n_, &c, "b"));
    nccell_set_fg_rgb8(&c, 255, 255, 255);
    ncplane_set_base_cell(n_, &c);
    nccell_release(n_, &c);
    CHECK(0 == notcurses_render(nc_));
    nccell_off_styles(&c, NCSTYLE_BOLD);
  }

  SUBCASE("SetUnderline") {
    nccell c = CELL_TRIVIAL_INITIALIZER;
    int dimy, dimx;
    notcurses_term_dim_yx(nc_, &dimy, &dimx);
    nccell_set_styles(&c, NCSTYLE_UNDERLINE);
    CHECK(1 == nccell_load(n_, &c, "u"));
    nccell_set_fg_rgb8(&c, 255, 255, 255);
    ncplane_set_base_cell(n_, &c);
    nccell_release(n_, &c);
    CHECK(0 == notcurses_render(nc_));
    nccell_off_styles(&c, NCSTYLE_UNDERLINE);
  }

  /*SUBCASE("CellLoadTamil") {
    const char zerodeg[] = "\u0bb8\u0bc0\u0bb0\u0bc7\u0bb3\u0b95\u0bbf\u0b95\u0bbf\u0bb0\u0bbf";
    nccell c = CELL_TRIVIAL_INITIALIZER;
    size_t ulen = nccell_load(n_, &c, zerodeg);
    // First have U+0BB8 TAMIL LETTER SA U+0BC0 TAMIL VOWEL SIGN II
    // // e0 ae b8 e0 af 80
    CHECK(6 == ulen);
    ulen = nccell_load(n_, &c, zerodeg + ulen);
    // U+0BB0 TAMIL LETTER RA U+0BCB TAMIL VOWEL SIGN OO
    // e0 ae b0 e0 af 8b
    CHECK(6 == ulen);
    // FIXME
  }*/

  SUBCASE("CellSetFGAlpha"){
    nccell c = CELL_TRIVIAL_INITIALIZER;
    CHECK(0 > nccell_set_fg_alpha(&c, -1));
    CHECK(0 > nccell_set_fg_alpha(&c, 4));
    CHECK(0 == nccell_set_fg_alpha(&c, CELL_ALPHA_OPAQUE));
    CHECK(nccell_fg_default_p(&c));
    CHECK(nccell_bg_default_p(&c));
    CHECK(CELL_ALPHA_OPAQUE == nccell_fg_alpha(&c));
    CHECK(0 == nccell_set_fg_alpha(&c, CELL_ALPHA_HIGHCONTRAST));
    CHECK(CELL_ALPHA_HIGHCONTRAST == nccell_fg_alpha(&c));
    CHECK(!nccell_fg_default_p(&c));
    CHECK(nccell_bg_default_p(&c));
  }

  SUBCASE("CellSetBGAlpha"){
    nccell c = CELL_TRIVIAL_INITIALIZER;
    CHECK(0 > nccell_set_bg_alpha(&c, -1));
    CHECK(0 > nccell_set_bg_alpha(&c, 4));
    CHECK(0 == nccell_set_bg_alpha(&c, CELL_ALPHA_OPAQUE));
    CHECK(CELL_ALPHA_OPAQUE == nccell_bg_alpha(&c));
    CHECK(0 != nccell_set_bg_alpha(&c, CELL_ALPHA_HIGHCONTRAST));
    CHECK(0 == nccell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT));
    CHECK(CELL_ALPHA_TRANSPARENT == nccell_bg_alpha(&c));
    CHECK(nccell_fg_default_p(&c));
    CHECK(!nccell_bg_default_p(&c));
  }

  // white on a black background ought be unmolested for highcontrast
  SUBCASE("HighContrastWhiteOnBlackBackground"){
    nccell c = CELL_CHAR_INITIALIZER('+');
    CHECK(0 == nccell_set_fg_rgb8(&c, 0xff, 0xff, 0xff));
    CHECK(0 == nccell_set_fg_alpha(&c, CELL_ALPHA_HIGHCONTRAST));
    CHECK(0 == nccell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT));
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto np = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != np);
    CHECK(1 == ncplane_putc(np, &c));
    nccell_load_char(np, &c, '*');
    CHECK(0 == nccell_set_bg_rgb8(&c, 0x0, 0x0, 0x0));
    CHECK(0 == nccell_set_bg_alpha(&c, CELL_ALPHA_OPAQUE));
    CHECK(1 == ncplane_putc(n_, &c));
    CHECK(0 == notcurses_render(nc_));
    uint64_t channels, underchannels, overchannels;
    auto egc = notcurses_at_yx(nc_, 0, 0, nullptr, &channels);
    REQUIRE(nullptr != egc);
    auto negc = ncplane_at_yx(n_, 0, 0, nullptr, &underchannels);
    REQUIRE(nullptr != negc);
    auto topegc = ncplane_at_yx(np, 0, 0, nullptr, &overchannels);
    REQUIRE(nullptr != topegc);
    CHECK(ncchannels_bg_rgb(channels) == ncchannels_bg_rgb(underchannels));
    CHECK(ncchannels_fg_rgb(channels) == ncchannels_fg_rgb(overchannels));
    ncplane_destroy(np);
    free(topegc);
    free(negc);
    free(egc);
  }

  // white on a white background ought be changed for highcontrast
  SUBCASE("HighContrastWhiteOnWhiteBackground"){
    nccell c = CELL_CHAR_INITIALIZER('+');
    CHECK(0 == nccell_set_fg_rgb8(&c, 0xff, 0xff, 0xff));
    CHECK(0 == nccell_set_fg_alpha(&c, CELL_ALPHA_HIGHCONTRAST));
    CHECK(0 == nccell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT));
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto np = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != np);
    CHECK(1 == ncplane_putc(np, &c));
    nccell_load_char(np, &c, '*');
    CHECK(0 == nccell_set_bg_rgb8(&c, 0xff, 0xff, 0xff));
    CHECK(0 == nccell_set_bg_alpha(&c, CELL_ALPHA_OPAQUE));
    CHECK(1 == ncplane_putc(n_, &c));
    CHECK(0 == notcurses_render(nc_));
    uint64_t channels, underchannels, overchannels;
    auto egc = notcurses_at_yx(nc_, 0, 0, nullptr, &channels);
    REQUIRE(nullptr != egc);
    auto negc = ncplane_at_yx(n_, 0, 0, nullptr, &underchannels);
    REQUIRE(nullptr != negc);
    auto topegc = ncplane_at_yx(np, 0, 0, nullptr, &overchannels);
    REQUIRE(nullptr != topegc);
    CHECK(ncchannels_bg_rgb(channels) == ncchannels_bg_rgb(underchannels));
    CHECK(ncchannels_fg_rgb(channels) < ncchannels_fg_rgb(overchannels));
    ncplane_destroy(np);
    free(topegc);
    free(negc);
    free(egc);
  }

  // black on a black background must be changed for highcontrast
  SUBCASE("HighContrastBlackOnBlackBackground"){
    nccell c = CELL_CHAR_INITIALIZER('+');
    CHECK(0 == nccell_set_fg_rgb8(&c, 0x0, 0x0, 0x0));
    CHECK(0 == nccell_set_fg_alpha(&c, CELL_ALPHA_HIGHCONTRAST));
    CHECK(0 == nccell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT));
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto np = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != np);
    CHECK(1 == ncplane_putc(np, &c));
    nccell_load_char(np, &c, '*');
    CHECK(0 == nccell_set_bg_rgb8(&c, 0x0, 0x0, 0x0));
    CHECK(0 == nccell_set_bg_alpha(&c, CELL_ALPHA_OPAQUE));
    CHECK(1 == ncplane_putc(n_, &c));
    CHECK(0 == notcurses_render(nc_));
    uint64_t channels, underchannels, overchannels;
    auto egc = notcurses_at_yx(nc_, 0, 0, nullptr, &channels);
    REQUIRE(nullptr != egc);
    auto negc = ncplane_at_yx(n_, 0, 0, nullptr, &underchannels);
    REQUIRE(nullptr != negc);
    auto topegc = ncplane_at_yx(np, 0, 0, nullptr, &overchannels);
    REQUIRE(nullptr != topegc);
    CHECK(ncchannels_bg_rgb(channels) == ncchannels_bg_rgb(underchannels));
    CHECK(ncchannels_fg_rgb(channels) > ncchannels_fg_rgb(overchannels));
    ncplane_destroy(np);
    free(topegc);
    free(negc);
    free(egc);
  }

  // black on a white background ought be unmolested for highcontrast
  SUBCASE("HighContrastBlackOnWhiteBackground"){
    nccell c = CELL_CHAR_INITIALIZER('+');
    CHECK(0 == nccell_set_fg_rgb8(&c, 0x0, 0x0, 0x0));
    CHECK(0 == nccell_set_fg_alpha(&c, CELL_ALPHA_HIGHCONTRAST));
    CHECK(0 == nccell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT));
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto np = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != np);
    CHECK(1 == ncplane_putc(np, &c));
    nccell_load_char(np, &c, '*');
    CHECK(0 == nccell_set_bg_rgb8(&c, 0xff, 0xff, 0xff));
    CHECK(0 == nccell_set_bg_alpha(&c, CELL_ALPHA_OPAQUE));
    CHECK(1 == ncplane_putc(n_, &c));
    CHECK(0 == notcurses_render(nc_));
    uint64_t channels, underchannels, overchannels;
    auto egc = notcurses_at_yx(nc_, 0, 0, nullptr, &channels);
    REQUIRE(nullptr != egc);
    auto negc = ncplane_at_yx(n_, 0, 0, nullptr, &underchannels);
    REQUIRE(nullptr != negc);
    auto topegc = ncplane_at_yx(np, 0, 0, nullptr, &overchannels);
    REQUIRE(nullptr != topegc);
    CHECK(ncchannels_bg_rgb(channels) == ncchannels_bg_rgb(underchannels));
    CHECK(ncchannels_fg_rgb(channels) == ncchannels_fg_rgb(overchannels));
    ncplane_destroy(np);
    free(topegc);
    free(negc);
    free(egc);
  }

  // high contrast ought only be activated relevant to the background equal to
  // or below them, not above.
  SUBCASE("HighContrastBelowOnly"){
    nccell c = CELL_TRIVIAL_INITIALIZER;
    // top has a background of white
    CHECK(0 == nccell_set_bg_rgb8(&c, 0xff, 0xff, 0xff));
    CHECK(0 == nccell_set_fg_alpha(&c, CELL_ALPHA_TRANSPARENT));
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto np = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != np);
    CHECK(1 == ncplane_putc(np, &c));
    nccell_load_char(n_, &c, '*');
    // bottom has white foreground + HIGHCONTRAST, should remain white
    CHECK(0 == nccell_set_fg_rgb8(&c, 0xff, 0x0, 0xff));
    CHECK(0 == nccell_set_fg_alpha(&c, CELL_ALPHA_HIGHCONTRAST));
    nccell_set_bg_default(&c);
    CHECK(1 == ncplane_putc(n_, &c));
    CHECK(0 == notcurses_render(nc_));
    uint64_t channels, underchannels, overchannels;
    auto egc = notcurses_at_yx(nc_, 0, 0, nullptr, &channels);
    REQUIRE(nullptr != egc);
    auto negc = ncplane_at_yx(n_, 0, 0, nullptr, &underchannels);
    REQUIRE(nullptr != negc);
    auto topegc = ncplane_at_yx(np, 0, 0, nullptr, &overchannels);
    REQUIRE(nullptr != topegc);
    CHECK(ncchannels_bg_rgb(channels) == ncchannels_bg_rgb(overchannels));
    CHECK(ncchannels_fg_rgb(channels) < ncchannels_fg_rgb(underchannels));
    free(topegc);
    free(negc);
    free(egc);
  }

  SUBCASE("CellLoadCharPrinting") {
    nccell c = CELL_TRIVIAL_INITIALIZER;
    CHECK(1 == nccell_load_char(n_, &c, '*'));
    CHECK(0 == strcmp(nccell_extended_gcluster(n_, &c), "*"));
  }

  SUBCASE("CellLoadCharWhitespace") {
    nccell c = CELL_TRIVIAL_INITIALIZER;
    CHECK(1 == nccell_load_char(n_, &c, '\f'));
    CHECK(1 == nccell_load_char(n_, &c, '\n'));
    CHECK(1 == nccell_load_char(n_, &c, '\t'));
    CHECK(1 == nccell_load_char(n_, &c, ' '));
  }

  SUBCASE("CellLoadCharControl") {
    nccell c = CELL_TRIVIAL_INITIALIZER;
    CHECK(0 == nccell_load_char(n_, &c, '\0'));
    CHECK(-1 == nccell_load_char(n_, &c, 1));
    CHECK(-1 == nccell_load_char(n_, &c, '\b'));
  }

  SUBCASE("CellLoadEGC32") {
    nccell c = CELL_TRIVIAL_INITIALIZER;
    CHECK(0 == nccell_load_egc32(n_, &c, 0));
    CHECK(1 == nccell_load_egc32(n_, &c, 0x65));   // U+0061 LATIN SMALL LETTER A
    CHECK(2 == nccell_load_egc32(n_, &c, 0xb5c2)); // U+00B5 MICRO SIGN
    CHECK(4 == nccell_load_egc32(n_, &c, 0x82a69ff0)); // U+1F982 SCORPION
  }

  // common teardown
  CHECK(0 == notcurses_stop(nc_));
}
