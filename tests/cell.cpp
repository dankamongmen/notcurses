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
    CHECK(1 == cols);
    bytes = utf8_egc_len(" ि", &cols);
    CHECK(4 == bytes);
    CHECK(1 == cols);
    bytes = utf8_egc_len("◌̈", &cols);
    CHECK(5 == bytes);
    CHECK(1 == cols);
    bytes = utf8_egc_len("นี้", &cols);
    CHECK(9 == bytes);
    CHECK(1 == cols);
  }

  SUBCASE("Loadchar") {
    cell c = CELL_TRIVIAL_INITIALIZER;
    CHECK(1 == cell_load(n_, &c, " "));
    CHECK(cell_simple_p(&c));
    cell_release(n_, &c);
  }

  SUBCASE("MultibyteWidth") {
    CHECK(0 == ncstrwidth(""));       // zero bytes, zero columns
    CHECK(-1 == ncstrwidth("\x7"));   // single byte, non-printable
    CHECK(1 == ncstrwidth(" "));      // single byte, one column
    CHECK(5 == ncstrwidth("abcde"));  // single byte, one column
    CHECK(1 == ncstrwidth("µ"));      // two bytes, one column
    CHECK(1 <= ncstrwidth("\xf0\x9f\xa6\xb2"));     // four bytes, two columns
    CHECK(3 <= ncstrwidth("平仮名")); // nine bytes, six columns
    CHECK(1 == ncstrwidth("\ufdfd")); // three bytes, ? columns, wcwidth() returns 1
  }

  // test combining characters and ZWJs
  SUBCASE("MultiglyphWidth") {
#ifdef __linux__
    CHECK(2 == ncstrwidth("\U0001F471"));
    CHECK(2 == ncstrwidth("\U0001F471\u200D"));
    CHECK(3 == ncstrwidth("\U0001F471\u200D\u2640")); // *not* a single EGC!
#else
    // FreeBSD doesn't think these wide
    CHECK(1 == ncstrwidth("\U0001F471"));
    CHECK(1 == ncstrwidth("\U0001F471\u200D"));
    CHECK(2 == ncstrwidth("\U0001F471\u200D\u2640")); // *not* a single EGC!
#endif
  }

  SUBCASE("SetItalic") {
    cell c = CELL_TRIVIAL_INITIALIZER;
    int dimy, dimx;
    notcurses_term_dim_yx(nc_, &dimy, &dimx);
    cell_set_styles(&c, NCSTYLE_ITALIC);
    CHECK(1 == cell_load(n_, &c, "i"));
    cell_set_fg_rgb8(&c, 255, 255, 255);
    ncplane_set_base_cell(n_, &c);
    cell_release(n_, &c);
    CHECK(0 == notcurses_render(nc_));
    cell_off_styles(&c, NCSTYLE_ITALIC);
  }

  SUBCASE("SetBold") {
    cell c = CELL_TRIVIAL_INITIALIZER;
    int dimy, dimx;
    notcurses_term_dim_yx(nc_, &dimy, &dimx);
    cell_set_styles(&c, NCSTYLE_BOLD);
    CHECK(1 == cell_load(n_, &c, "b"));
    cell_set_fg_rgb8(&c, 255, 255, 255);
    ncplane_set_base_cell(n_, &c);
    cell_release(n_, &c);
    CHECK(0 == notcurses_render(nc_));
    cell_off_styles(&c, NCSTYLE_BOLD);
  }

  SUBCASE("SetUnderline") {
    cell c = CELL_TRIVIAL_INITIALIZER;
    int dimy, dimx;
    notcurses_term_dim_yx(nc_, &dimy, &dimx);
    cell_set_styles(&c, NCSTYLE_UNDERLINE);
    CHECK(1 == cell_load(n_, &c, "u"));
    cell_set_fg_rgb8(&c, 255, 255, 255);
    ncplane_set_base_cell(n_, &c);
    cell_release(n_, &c);
    CHECK(0 == notcurses_render(nc_));
    cell_off_styles(&c, NCSTYLE_UNDERLINE);
  }

  /*SUBCASE("CellLoadTamil") {
    const char zerodeg[] = "\u0bb8\u0bc0\u0bb0\u0bc7\u0bb3\u0b95\u0bbf\u0b95\u0bbf\u0bb0\u0bbf";
    cell c = CELL_TRIVIAL_INITIALIZER;
    size_t ulen = cell_load(n_, &c, zerodeg);
    // First have U+0BB8 TAMIL LETTER SA U+0BC0 TAMIL VOWEL SIGN II
    // // e0 ae b8 e0 af 80
    CHECK(6 == ulen);
    ulen = cell_load(n_, &c, zerodeg + ulen);
    // U+0BB0 TAMIL LETTER RA U+0BCB TAMIL VOWEL SIGN OO
    // e0 ae b0 e0 af 8b
    CHECK(6 == ulen);
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
    CHECK(0 == cell_set_fg_rgb8(&c, 0xff, 0xff, 0xff));
    CHECK(0 == cell_set_fg_alpha(&c, CELL_ALPHA_HIGHCONTRAST));
    CHECK(0 == cell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT));
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
    };
    auto np = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != np);
    CHECK(1 == ncplane_putc(np, &c));
    cell_load_char(np, &c, '*');
    CHECK(0 == cell_set_bg_rgb8(&c, 0x0, 0x0, 0x0));
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
    CHECK(channels_bg_rgb(channels) == channels_bg_rgb(underchannels));
    CHECK(channels_fg_rgb(channels) == channels_fg_rgb(overchannels));
    ncplane_destroy(np);
    free(topegc);
    free(negc);
    free(egc);
  }

  // white on a white background ought be changed for highcontrast
  SUBCASE("HighContrastWhiteOnWhiteBackground"){
    cell c = CELL_CHAR_INITIALIZER('+');
    CHECK(0 == cell_set_fg_rgb8(&c, 0xff, 0xff, 0xff));
    CHECK(0 == cell_set_fg_alpha(&c, CELL_ALPHA_HIGHCONTRAST));
    CHECK(0 == cell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT));
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
    };
    auto np = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != np);
    CHECK(1 == ncplane_putc(np, &c));
    cell_load_char(np, &c, '*');
    CHECK(0 == cell_set_bg_rgb8(&c, 0xff, 0xff, 0xff));
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
    CHECK(channels_bg_rgb(channels) == channels_bg_rgb(underchannels));
    CHECK(channels_fg_rgb(channels) < channels_fg_rgb(overchannels));
    ncplane_destroy(np);
    free(topegc);
    free(negc);
    free(egc);
  }

  // black on a black background must be changed for highcontrast
  SUBCASE("HighContrastBlackOnBlackBackground"){
    cell c = CELL_CHAR_INITIALIZER('+');
    CHECK(0 == cell_set_fg_rgb8(&c, 0x0, 0x0, 0x0));
    CHECK(0 == cell_set_fg_alpha(&c, CELL_ALPHA_HIGHCONTRAST));
    CHECK(0 == cell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT));
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
    };
    auto np = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != np);
    CHECK(1 == ncplane_putc(np, &c));
    cell_load_char(np, &c, '*');
    CHECK(0 == cell_set_bg_rgb8(&c, 0x0, 0x0, 0x0));
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
    CHECK(channels_bg_rgb(channels) == channels_bg_rgb(underchannels));
    CHECK(channels_fg_rgb(channels) > channels_fg_rgb(overchannels));
    ncplane_destroy(np);
    free(topegc);
    free(negc);
    free(egc);
  }

  // black on a white background ought be unmolested for highcontrast
  SUBCASE("HighContrastBlackOnWhiteBackground"){
    cell c = CELL_CHAR_INITIALIZER('+');
    CHECK(0 == cell_set_fg_rgb8(&c, 0x0, 0x0, 0x0));
    CHECK(0 == cell_set_fg_alpha(&c, CELL_ALPHA_HIGHCONTRAST));
    CHECK(0 == cell_set_bg_alpha(&c, CELL_ALPHA_TRANSPARENT));
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
    };
    auto np = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != np);
    CHECK(1 == ncplane_putc(np, &c));
    cell_load_char(np, &c, '*');
    CHECK(0 == cell_set_bg_rgb8(&c, 0xff, 0xff, 0xff));
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
    CHECK(channels_bg_rgb(channels) == channels_bg_rgb(underchannels));
    CHECK(channels_fg_rgb(channels) == channels_fg_rgb(overchannels));
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
    CHECK(0 == cell_set_bg_rgb8(&c, 0xff, 0xff, 0xff));
    CHECK(0 == cell_set_fg_alpha(&c, CELL_ALPHA_TRANSPARENT));
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
    };
    auto np = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != np);
    CHECK(1 == ncplane_putc(np, &c));
    cell_load_char(n_, &c, '*');
    // bottom has white foreground + HIGHCONTRAST, should remain white
    CHECK(0 == cell_set_fg_rgb8(&c, 0xff, 0x0, 0xff));
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
    CHECK(channels_bg_rgb(channels) == channels_bg_rgb(overchannels));
    CHECK(channels_fg_rgb(channels) < channels_fg_rgb(underchannels));
    free(topegc);
    free(negc);
    free(egc);
  }

  SUBCASE("CellLoadCharPrinting") {
    cell c = CELL_TRIVIAL_INITIALIZER;
    CHECK(1 == cell_load_char(n_, &c, '*'));
    CHECK(0 == strcmp(cell_extended_gcluster(n_, &c), "*"));
  }

  SUBCASE("CellLoadCharWhitespace") {
    cell c = CELL_TRIVIAL_INITIALIZER;
    CHECK(1 == cell_load_char(n_, &c, '\f'));
    CHECK(1 == cell_load_char(n_, &c, '\n'));
    CHECK(1 == cell_load_char(n_, &c, '\t'));
    CHECK(1 == cell_load_char(n_, &c, ' '));
  }

  SUBCASE("CellLoadCharControl") {
    cell c = CELL_TRIVIAL_INITIALIZER;
    CHECK(0 == cell_load_char(n_, &c, '\0'));
    CHECK(-1 == cell_load_char(n_, &c, 1));
    CHECK(-1 == cell_load_char(n_, &c, '\b'));
  }

  // common teardown
  CHECK(0 == notcurses_stop(nc_));
}
