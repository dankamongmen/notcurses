#include <array>
#include <cstdlib>
#include "main.h"

const char SNAKE[] = u8"\xf0\x9f\x90\x8d"; // U+1F40D SNAKE
const char SCORPION[] = u8"\xf0\x9f\xa6\x82"; // U+1F982 SCORPION
const char FROG[] = u8"\xf0\x9f\x90\xb8"; // U+1F438 FROG FACE

TEST_CASE("Wide") {
  auto nc_ = testing_notcurses();
  REQUIRE(nullptr != nc_);
  if(!notcurses_canutf8(nc_)){
    CHECK(0 == notcurses_stop(nc_));
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  // Verify we can emit a wide character, and it advances the cursor by 2
  SUBCASE("EmitWideAsian") {
    const char* w = u8"\u5168";
    CHECK(0 < ncplane_putegc(n_, w, nullptr));
    unsigned x, y;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == y);
    CHECK(2 == x);
    CHECK(0 == notcurses_render(nc_));
  }

  // Verify a wide character is rejected with cursor on the last column
  SUBCASE("RejectWideAsian") {
    const char* w = u8"\u5168";
    unsigned dimx;
    ncplane_dim_yx(n_, nullptr, &dimx);
    CHECK(0 < ncplane_putegc_yx(n_, 0, dimx - 3, w, nullptr));
    unsigned x, y;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == y);
    CHECK(dimx - 1 == x);
    // now it ought be rejected
    CHECK(0 > ncplane_putegc(n_, w, nullptr));
    // cursor ought remain where it was
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == y);
    CHECK(dimx - 1 == x);
    CHECK(0 == notcurses_render(nc_));
  }

  // Verify a wide character is rejected when placed on the last column
  SUBCASE("RejectWideAsianPlaced") {
    const char* w = u8"\u5168";
    unsigned dimx;
    ncplane_dim_yx(n_, nullptr, &dimx);
    // now it ought be rejected
    CHECK(0 > ncplane_putegc_yx(n_, 0, dimx - 1, w, nullptr));
    // cursor ought remain where it was
    unsigned y, x;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == y);
    CHECK(0 == x);
    CHECK(0 == notcurses_render(nc_));
  }

  // half-width, double-width, huge-yet-narrow, all that crap
  SUBCASE("PlaneAtCursorInsane"){
    const char EGC0[] = u8"\uffe0"; // fullwidth cent sign ￠
    const char EGC1[] = u8"\u00c5"; // neutral A with ring above Å
    const char EGC2[] = u8"\u20a9"; // half-width won ₩
    const char EGC3[] = u8"\u212b"; // ambiguous angstrom Å
    const char EGC4[] = u8"\ufdfd"; // neutral yet huge bismillah ﷽
    std::array<nccell, 5> tcells;
    for(auto & tcell : tcells){
      nccell_init(&tcell);
    }
    CHECK(1 < nccell_load(n_, &tcells[0], EGC0));
    CHECK(1 < nccell_load(n_, &tcells[1], EGC1));
    CHECK(1 < nccell_load(n_, &tcells[2], EGC2));
    CHECK(1 < nccell_load(n_, &tcells[3], EGC3));
    CHECK(1 < nccell_load(n_, &tcells[4], EGC4));
    for(auto & tcell : tcells){
      CHECK(0 < ncplane_putc(n_, &tcell));
    }
    CHECK(0 == notcurses_render(nc_));
    int x = 0;
    for(auto & tcell : tcells){
      CHECK(0 == ncplane_cursor_move_yx(n_, 0, x));
      nccell testcell = NCCELL_TRIVIAL_INITIALIZER;
      CHECK(0 < ncplane_at_cursor_cell(n_, &testcell));
      CHECK(!strcmp(nccell_extended_gcluster(n_, &tcell), nccell_extended_gcluster(n_, &testcell)));
      CHECK(0 == testcell.stylemask);
      wchar_t w;
      CHECK(0 < mbtowc(&w, nccell_extended_gcluster(n_, &tcell), MB_CUR_MAX));
      if(wcwidth(w) == 2){
        CHECK(2 == testcell.width);
        ++x;
      }else{
        CHECK(0 == testcell.channels);
      }
      ++x;
    }
  }

  // Placing a wide char to the immediate left of any other char ought
  // obliterate that cell.
  SUBCASE("WideCharAnnihilatesRight") {
    const char* w = FROG;
    const char* wbashed = SCORPION;
    const char bashed = 'X';
    CHECK(0 < ncplane_putegc_yx(n_, 0, 1, wbashed, nullptr));
    CHECK(0 < ncplane_putchar_yx(n_, 1, 1, bashed));
    unsigned x, y;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(1 == y);
    CHECK(2 == x);
    CHECK(0 < ncplane_putegc_yx(n_, 0, 0, w, nullptr));
    nccell c = NCCELL_TRIVIAL_INITIALIZER;
    CHECK(0 < nccell_load(n_, &c, w));
    CHECK(0 < ncplane_putc_yx(n_, 1, 0, &c));
    nccell_release(n_, &c);
    ncplane_at_yx_cell(n_, 0, 0, &c);
    CHECK(0 == strcmp(nccell_extended_gcluster(n_, &c), FROG));
    ncplane_at_yx_cell(n_, 0, 1, &c);
    CHECK(ncstrwidth(FROG, NULL, NULL) == 1 + nccell_double_wide_p(&c)); // should be wide
    ncplane_at_yx_cell(n_, 0, 2, &c);
    CHECK(0 == strlen(nccell_extended_gcluster(n_, &c))); // should be nothing
    ncplane_at_yx_cell(n_, 1, 0, &c);
    CHECK(0 == strcmp(nccell_extended_gcluster(n_, &c), FROG));
    ncplane_at_yx_cell(n_, 1, 1, &c);
    CHECK(ncstrwidth(FROG, NULL, NULL) == 1 + nccell_double_wide_p(&c)); //should be wide
    CHECK(0 == notcurses_render(nc_)); // should be nothing
  }

  // Placing a wide char on the right half of a wide char ought obliterate the
  // original wide char.
  SUBCASE("WideCharAnnihilatesWideLeft") {
    const char* w = SNAKE;
    const char* wbashed = SCORPION;
    int cols1 = ncplane_putegc_yx(n_, 0, 0, wbashed, nullptr);
    CHECK(0 < cols1);
    int cols2 = ncplane_putegc_yx(n_, 0, 1, w, nullptr);
    CHECK(0 < cols2);
    unsigned x, y;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == y);
    CHECK(1 + cols2 == x);
    nccell c = NCCELL_TRIVIAL_INITIALIZER;
    ncplane_at_yx_cell(n_, 0, 0, &c);
    if(cols1 > 1){
      CHECK(0 == c.gcluster); // should be nothing
    }
    ncplane_at_yx_cell(n_, 0, 1, &c);
    CHECK(0 == strcmp(nccell_extended_gcluster(n_, &c), SNAKE));
    ncplane_at_yx_cell(n_, 0, 2, &c);
    CHECK(ncstrwidth(SNAKE, NULL, NULL) == 1 + nccell_double_wide_p(&c)); // should be wide
    CHECK(0 == notcurses_render(nc_));
  }

  // Placing a normal char on either half of a wide char ought obliterate
  // the original wide char.
  SUBCASE("WideCharsAnnihilated") {
    const char* cc = "X";
    const char* wbashedl = SNAKE;
    const char* wbashedr = SCORPION;
    CHECK(0 < ncplane_putegc_yx(n_, 0, 0, wbashedl, nullptr));
    CHECK(0 < ncplane_putegc_yx(n_, 0, 2, wbashedr, nullptr));
    CHECK(1 == ncplane_putchar_yx(n_, 0, 1, *cc));
    CHECK(1 == ncplane_putchar_yx(n_, 0, 2, *cc));
    unsigned x, y;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == y);
    CHECK(3 == x);
    nccell c = NCCELL_TRIVIAL_INITIALIZER;
    ncplane_at_yx_cell(n_, 0, 0, &c);
    if(ncstrwidth(wbashedl, NULL, NULL) > 1){
      CHECK(0 == c.gcluster); // should be nothing
    }
    ncplane_at_yx_cell(n_, 0, 1, &c);
    CHECK(0 == strcmp(cc, nccell_extended_gcluster(n_, &c))); // should be 'X'
    ncplane_at_yx_cell(n_, 0, 2, &c);
    CHECK(0 == strcmp(cc, nccell_extended_gcluster(n_, &c))); // should be 'X'
    ncplane_at_yx_cell(n_, 0, 3, &c);
    if(ncstrwidth(wbashedr, NULL, NULL) > 1){
      CHECK(0 == strlen(nccell_extended_gcluster(n_, &c))); // should be nothing
    }
    CHECK(0 == notcurses_render(nc_));
  }

  // But placing something to the immediate right of any glyph, that is not a
  // problem. Ensure it is so.
  SUBCASE("AdjacentCharsSafe") {
    const char* cc = "X";
    const char* wsafel = SNAKE;
    const char* wsafer = SCORPION;
    CHECK(0 < ncplane_putegc_yx(n_, 0, 0, wsafel, nullptr));
    CHECK(0 < ncplane_putegc_yx(n_, 0, 3, wsafer, nullptr));
    CHECK(1 == ncplane_putchar_yx(n_, 0, 2, *cc));
    unsigned x, y;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == y);
    CHECK(3 == x);
    nccell c = NCCELL_TRIVIAL_INITIALIZER;
    ncplane_at_yx_cell(n_, 0, 0, &c);
    CHECK(0 == strcmp(nccell_extended_gcluster(n_, &c), SNAKE));
    ncplane_at_yx_cell(n_, 0, 1, &c);
    CHECK(ncstrwidth(SNAKE, NULL, NULL) == 1 + nccell_double_wide_p(&c));
    ncplane_at_yx_cell(n_, 0, 2, &c);
    CHECK(0 == strcmp(cc, nccell_extended_gcluster(n_, &c))); // should be 'X'
    ncplane_at_yx_cell(n_, 0, 3, &c);
    CHECK(0 == strcmp(nccell_extended_gcluster(n_, &c), SCORPION));
    ncplane_at_yx_cell(n_, 0, 4, &c);
    CHECK(ncstrwidth(SCORPION, NULL, NULL) == 1 + nccell_double_wide_p(&c));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("BoxedWideGlyph") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 3,
      .cols = 4,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* ncp = ncplane_create(n_, &nopts);
    REQUIRE(ncp);
    unsigned dimx, dimy;
    ncplane_dim_yx(n_, &dimy, &dimx);
    CHECK(0 == ncplane_rounded_box_sized(ncp, 0, 0, 3, 4, 0));
    CHECK(ncstrwidth(SCORPION, NULL, NULL) == ncplane_putegc_yx(ncp, 1, 1, SCORPION, nullptr));
    CHECK(0 == notcurses_render(nc_));
    nccell c = NCCELL_TRIVIAL_INITIALIZER;
    CHECK(0 < ncplane_at_yx_cell(ncp, 1, 0, &c));
    CHECK(!strcmp(nccell_extended_gcluster(ncp, &c), "│"));
    nccell_release(ncp, &c);
    char* egc = notcurses_at_yx(nc_, 1, 0, &c.stylemask, &c.channels);
    REQUIRE(egc);
    CHECK(!strcmp(egc, "│"));
    free(egc);
    CHECK(0 < ncplane_at_yx_cell(ncp, 1, 3, &c));
    CHECK(!strcmp(nccell_extended_gcluster(ncp, &c), "│"));
    nccell_release(ncp, &c);
    egc = notcurses_at_yx(nc_, 1, 3, &c.stylemask, &c.channels);
    REQUIRE(egc);
    CHECK(!strcmp(egc, "│"));
    free(egc);
    CHECK(0 == ncplane_destroy(ncp));
  }

  SUBCASE("RenderWides") {
    CHECK(0 <= ncplane_putstr(n_, u8"\u5f62\u5168"));
    nccell c = NCCELL_TRIVIAL_INITIALIZER;
    ncplane_at_yx_cell(n_, 0, 0, &c);
    CHECK(nccell_double_wide_p(&c));
    ncplane_at_yx_cell(n_, 0, 1, &c);
    CHECK(nccell_double_wide_p(&c));
    ncplane_at_yx_cell(n_, 0, 2, &c);
    CHECK(nccell_double_wide_p(&c));
    ncplane_at_yx_cell(n_, 0, 3, &c);
    CHECK(nccell_double_wide_p(&c));
    ncplane_at_yx_cell(n_, 0, 4, &c);
    CHECK(!nccell_double_wide_p(&c));
    CHECK(0 == notcurses_render(nc_));
    auto egc = notcurses_at_yx(nc_, 0, 0, &c.stylemask, &c.channels);
    REQUIRE(nullptr != egc);
    CHECK(0 == strcmp(u8"\u5f62", egc));
    free(egc);
    egc = notcurses_at_yx(nc_, 0, 1, &c.stylemask, &c.channels);
    REQUIRE(nullptr != egc);
    CHECK(0 == strcmp("", egc));
    free(egc);
    egc = notcurses_at_yx(nc_, 0, 2, &c.stylemask, &c.channels);
    REQUIRE(nullptr != egc);
    CHECK(0 == strcmp(u8"\u5168", egc));
    free(egc);
    egc = notcurses_at_yx(nc_, 0, 3, &c.stylemask, &c.channels);
    REQUIRE(nullptr != egc);
    free(egc);
    egc = notcurses_at_yx(nc_, 0, 4, &c.stylemask, &c.channels);
    REQUIRE(nullptr != egc);
    free(egc);
  }

  // If an ncplane is moved atop the right half of a wide glyph, the entire
  // glyph should be obliterated.
  SUBCASE("PlaneStompsWideGlyph"){
    nccell c = NCCELL_TRIVIAL_INITIALIZER;
    char* egc;

    // print two wide glyphs on the standard plane
    int py, px;
    ncplane_yx(n_, &py, &px);
    CHECK(0 == py);
    CHECK(0 == px);
    CHECK(2 == ncplane_putstr(n_, u8"\xe5\x85\xa8")); // \u5168 (全)
    unsigned y, x;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == y);
    CHECK(2 == x);
    CHECK(2 == ncplane_putstr(n_, u8"\xe5\xbd\xa2")); // \u5f62 (形)
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == y);
    CHECK(4 == x);
    CHECK(!notcurses_render(nc_));

    // should be wide char 1
    CHECK(3 == ncplane_at_yx_cell(n_, 0, 0, &c));
    CHECK(!strcmp(u8"\xe5\x85\xa8", nccell_extended_gcluster(n_, &c)));
    CHECK(nccell_double_wide_p(&c));
    egc = notcurses_at_yx(nc_, 0, 0, &c.stylemask, &c.channels);
    REQUIRE(egc);
    CHECK(!strcmp(u8"\xe5\x85\xa8", egc));
    CHECK(nccell_double_wide_p(&c));
    free(egc);
    nccell_init(&c);
    // should be wide char 1 right side
    REQUIRE(0 == ncplane_at_yx_cell(n_, 0, 1, &c));
    CHECK(!strcmp("", nccell_extended_gcluster(n_, &c)));
    CHECK(nccell_double_wide_p(&c));
    egc = notcurses_at_yx(nc_, 0, 1, &c.stylemask, &c.channels);
    REQUIRE(egc);
    CHECK(!strcmp("", egc));
    CHECK(nccell_double_wide_p(&c));
    free(egc);
    nccell_init(&c);

    // should be wide char 2
    REQUIRE(3 == ncplane_at_yx_cell(n_, 0, 2, &c));
    CHECK(!strcmp(u8"\xe5\xbd\xa2", nccell_extended_gcluster(n_, &c)));
    CHECK(nccell_double_wide_p(&c));
    egc = notcurses_at_yx(nc_, 0, 2, &c.stylemask, &c.channels);
    REQUIRE(egc);
    CHECK(!strcmp(u8"\xe5\xbd\xa2", egc));
    CHECK(nccell_double_wide_p(&c));
    free(egc);
    nccell_init(&c);
    // should be wide char 2 right side
    CHECK(0 == ncplane_at_yx_cell(n_, 0, 3, &c));
    CHECK(!strcmp("", nccell_extended_gcluster(n_, &c)));
    CHECK(nccell_double_wide_p(&c));
    egc = notcurses_at_yx(nc_, 0, 3, &c.stylemask, &c.channels);
    REQUIRE(egc);
    CHECK(!strcmp("", egc));
    CHECK(nccell_double_wide_p(&c));
    free(egc);
    nccell_init(&c);

    struct ncplane_options nopts = {
      .y = 0,
      .x = 1,
      .rows = 1,
      .cols = 2,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* n = ncplane_create(n_, &nopts);
    REQUIRE(n);
    CHECK(0 < ncplane_putstr(n, "AB"));
    CHECK(!notcurses_render(nc_));

    // should be nothing, having been stomped
    egc = notcurses_at_yx(nc_, 0, 0, &c.stylemask, &c.channels);
    REQUIRE(egc);
    CHECK(0 == strcmp(" ", egc));
    free(egc);
    nccell_init(&c);
    // should be character from higher plane
    egc = notcurses_at_yx(nc_, 0, 1, &c.stylemask, &c.channels);
    REQUIRE(egc);
    CHECK(0 == strcmp("A", egc));
    free(egc);
    nccell_init(&c);

    egc = notcurses_at_yx(nc_, 0, 2, &c.stylemask, &c.channels);
    REQUIRE(egc);
    CHECK(0 == strcmp("B", egc));
    free(egc);
    nccell_init(&c);

    // should be nothing, having been stomped
    egc = notcurses_at_yx(nc_, 0, 3, &c.stylemask, &c.channels);
    REQUIRE(egc);
    CHECK(0 == strcmp("", egc));
    free(egc);
    nccell_init(&c);

    CHECK(0 == ncplane_destroy(n));
  }

  // Render a translucent plane atop a wide glyph, and check the colors on both
  // cells. See https://github.com/dankamongmen/notcurses/issues/362.
  SUBCASE("OverWide") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 3,
      .cols = 4,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* p = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != p);
    nccell c = NCCELL_CHAR_INITIALIZER('X');
    CHECK(0 == ncplane_perimeter(p, &c, &c, &c, &c, &c, &c, 0));
    ncplane_set_bg_rgb8(n_, 0x20, 0x20, 0x20);
    CHECK(2 == ncplane_putegc_yx(n_, 1, 1, "六", nullptr));
    uint64_t channels = 0;
    ncchannels_set_bg_alpha(&channels, NCALPHA_BLEND);
    ncchannels_set_bg_rgb8(&channels, 0x80, 0xf0, 0x10);
    CHECK(1 == ncplane_set_base(p, " ", 0, channels));
    CHECK(0 == notcurses_render(nc_));
    uint16_t stylemask;
    uint64_t chanleft, chanright;
    char* egc = notcurses_at_yx(nc_, 1, 1, &stylemask, &chanleft);
    REQUIRE(nullptr != egc);
    CHECK(0 == strcmp(" ", egc));
    free(egc);
    egc = notcurses_at_yx(nc_, 1, 2, &stylemask, &chanright);
    REQUIRE(nullptr != egc);
    CHECK(0 == strcmp(" ", egc));
    free(egc);
    nccell cl = NCCELL_TRIVIAL_INITIALIZER, cr = NCCELL_TRIVIAL_INITIALIZER;
    CHECK(3 == ncplane_at_yx_cell(n_, 1, 1, &cl));
    CHECK(0 == strcmp(u8"六", nccell_extended_gcluster(n_, &cl)));
    CHECK(0 == ncplane_at_yx_cell(n_, 1, 2, &cr));
    CHECK(cell_simple_p(&cr));
    CHECK(0 == cr.gcluster);
    nccell_release(n_, &cl);
    nccell_release(n_, &cr);
    CHECK(chanright == chanleft);
    ncplane_destroy(p);
  }

  // drag a plane of narrow chars across a plane of wide glyphs
  SUBCASE("NarrowPlaneAtopWide") {
    notcurses_cursor_disable(nc_);
    CHECK(0 == ncplane_set_fg_rgb8(n_, 0xff, 0, 0xff));
    // start the 1x4 top plane at 0, 4
    struct ncplane_options nopts = {
      .y = 0,
      .x = 4,
      .rows = 1,
      .cols = 4,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* topp = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != topp);
    CHECK(0 == ncplane_set_bg_rgb8(topp, 0, 0xff, 0));
    CHECK(4 == ncplane_putstr(topp, "abcd"));
    CHECK(8 == ncplane_putstr(n_, u8"六六六六"));
    CHECK(0 == notcurses_render(nc_));
    uint16_t stylemask;
    uint64_t channels;
    char* egc;
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 0, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 1, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 2, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 3, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 4, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "a"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 5, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "b"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 6, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "c"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 7, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "d"));
    free(egc);
    CHECK(0 == ncplane_move_yx(topp, 0, 3));
    CHECK(0 == notcurses_render(nc_));
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 0, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 1, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 2, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, " "));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 3, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "a"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 4, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "b"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 5, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "c"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 6, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "d"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 7, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    CHECK(0 == ncplane_move_yx(topp, 0, 2));
    CHECK(0 == notcurses_render(nc_));
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 0, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 1, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 2, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "a"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 3, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "b"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 4, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "c"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 5, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "d"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 6, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 7, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    CHECK(0 == ncplane_move_yx(topp, 0, 1));
    CHECK(0 == notcurses_render(nc_));
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 0, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, " "));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 1, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "a"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 2, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "b"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 3, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "c"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 4, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "d"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 5, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 6, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 7, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    CHECK(0 == ncplane_move_yx(topp, 0, 0));
    CHECK(0 == notcurses_render(nc_));
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 0, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "a"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 1, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "b"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 2, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "c"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 3, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "d"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 4, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 5, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 6, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 7, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    ncplane_destroy(topp);
  }

  // drag a plane of wide glyphs across a plane of wide glyphs
  SUBCASE("WidePlaneAtopWide") {
    CHECK(0 == ncplane_set_fg_rgb8(n_, 0xff, 0, 0xff));
    // start the 1x4 top plane at 0, 4
    struct ncplane_options nopts = {
      .y = 0,
      .x = 4,
      .rows = 1,
      .cols = 4,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* topp = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != topp);
    CHECK(0 == ncplane_set_bg_rgb8(topp, 0, 0xff, 0));
    CHECK(4 == ncplane_putstr(topp, u8"次次"));
    CHECK(0 == ncplane_cursor_move_yx(n_, 0, 0));
    CHECK(8 == ncplane_putstr(n_, u8"六六六六"));
    CHECK(0 == notcurses_render(nc_));
    uint16_t stylemask;
    uint64_t channels;
    char* egc;
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 0, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 1, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 2, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 3, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 4, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"次"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 5, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 6, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"次"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 7, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    CHECK(0 == ncplane_move_yx(topp, 0, 3));
    CHECK(0 == notcurses_render(nc_));
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 0, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 1, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 2, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, " "));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 3, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"次"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 4, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 5, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"次"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 6, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 7, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    CHECK(0 == ncplane_move_yx(topp, 0, 2));
    CHECK(0 == notcurses_render(nc_));
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 0, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 1, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 2, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"次"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 3, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 4, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"次"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 5, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 6, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 7, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    CHECK(0 == ncplane_move_yx(topp, 0, 1));
    CHECK(0 == notcurses_render(nc_));
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 0, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, " "));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 1, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"次"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 2, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 3, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"次"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 4, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 5, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 6, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 7, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    CHECK(0 == ncplane_move_yx(topp, 0, 0));
    CHECK(0 == notcurses_render(nc_));
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 0, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"次"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 1, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 2, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"次"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 3, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 4, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 5, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 6, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, u8"六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 7, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    ncplane_destroy(topp);
  }

  // drag a plane of wide glyphs across a plane of narrow glyphs
  SUBCASE("WidePlaneAtopNarrow") {
    CHECK(0 == ncplane_set_fg_rgb8(n_, 0xff, 0, 0xff));
    // start the 1x4 top plane at 0, 4
    struct ncplane_options nopts = {
      .y = 0,
      .x = 4,
      .rows = 1,
      .cols = 4,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* topp = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != topp);
    CHECK(0 == ncplane_set_bg_rgb8(topp, 0, 0xff, 0));
    CHECK(4 == ncplane_putstr(topp, u8"次次"));
    CHECK(8 == ncplane_putstr(n_, "abcdefgh"));
    CHECK(0 == notcurses_render(nc_));
    uint16_t stylemask;
    uint64_t channels;
    char* egc;
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 0, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "a"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 1, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "b"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 2, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "c"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 3, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "d"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 4, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "次"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 5, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 6, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "次"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 7, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    CHECK(0 == ncplane_move_yx(topp, 0, 3));
    CHECK(0 == notcurses_render(nc_));
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 0, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "a"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 1, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "b"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 2, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "c"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 3, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "次"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 4, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 5, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "次"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 6, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 7, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "h"));
    free(egc);
    CHECK(0 == ncplane_move_yx(topp, 0, 2));
    CHECK(0 == notcurses_render(nc_));
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 0, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "a"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 1, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "b"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 2, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "次"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 3, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 4, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "次"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 5, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 6, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "g"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 7, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "h"));
    free(egc);
    CHECK(0 == ncplane_move_yx(topp, 0, 1));
    CHECK(0 == notcurses_render(nc_));
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 0, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "a"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 1, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "次"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 2, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 3, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "次"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 4, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 5, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "f"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 6, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "g"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 7, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "h"));
    free(egc);
    CHECK(0 == ncplane_move_yx(topp, 0, 0));
    CHECK(0 == notcurses_render(nc_));
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 0, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "次"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 1, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 2, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "次"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 3, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 4, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "e"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 5, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "f"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 6, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "g"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 7, &stylemask, &channels)));
    CHECK(0 == strcmp(egc, "h"));
    free(egc);
    ncplane_destroy(topp);
  }

  // Verify that we can use long (4-byte) UTF-encoded characters together with
  // styles without a problem (since the stylemask holds both styling bits and
  // the NUL backstop for long inlined UTF8).
  // U+1F427 PINCHED FINGERS → UTF8(f0 9f a4 8c)
  SUBCASE("ItalicEmoji") {
    nccell c = NCCELL_TRIVIAL_INITIALIZER;
    nccell_load(n_, &c, "\U0001F427");
    CHECK(0xa7909ff0 == htole(c.gcluster));
    nccell_on_styles(&c, NCSTYLE_ITALIC);
    CHECK(4 == strlen(nccell_extended_gcluster(n_, &c)));
    CHECK(0 == strcmp("\U0001F427", nccell_extended_gcluster(n_, &c)));
    CHECK(0 < ncplane_putc_yx(n_, 0, 0, &c));
    CHECK(0 == notcurses_render(nc_));
    uint16_t stylemask;
    uint64_t channels;
    auto egc = notcurses_at_yx(nc_, 0, 0, &stylemask, &channels);
    REQUIRE(nullptr != egc);
    CHECK(4 == strlen(egc));
    CHECK(0 == strcmp("\U0001F427", egc));
    free(egc);
    CHECK(NCSTYLE_ITALIC == stylemask);
  }

  SUBCASE("StyleMaxEmoji") {
    nccell c = NCCELL_TRIVIAL_INITIALIZER;
    nccell_load(n_, &c, "\U0001F427");
    CHECK(0xa7909ff0 == htole(c.gcluster));
    nccell_on_styles(&c, NCSTYLE_MASK);
    CHECK(4 == strlen(nccell_extended_gcluster(n_, &c)));
    CHECK(0 == strcmp("\U0001F427", nccell_extended_gcluster(n_, &c)));
    CHECK(0 < ncplane_putc_yx(n_, 0, 0, &c));
    CHECK(0 == notcurses_render(nc_));
    uint16_t stylemask;
    uint64_t channels;
    auto egc = notcurses_at_yx(nc_, 0, 0, &stylemask, &channels);
    REQUIRE(nullptr != egc);
    CHECK(4 == strlen(egc));
    CHECK(0 == strcmp("\U0001F427", egc));
    free(egc);
    CHECK(NCSTYLE_MASK == stylemask);
  }

  // a higher glyph ought not be annihilated by a lower wide glyph
  SUBCASE("HigherGlyphAbides") {
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
    auto high = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != high);
    CHECK(0 < ncplane_putchar_yx(high, 0, 0, 'a'));
    CHECK(0 < ncplane_putegc_yx(n_, 0, 0, "全", nullptr));
    CHECK(0 == notcurses_render(nc_));
    auto egc = notcurses_at_yx(nc_, 0, 0, nullptr, nullptr);
    REQUIRE(nullptr != egc);
    CHECK(0 == strcmp(egc, "a"));
    free(egc);
    egc = notcurses_at_yx(nc_, 0, 1, nullptr, nullptr);
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    CHECK(0 == ncplane_move_yx(high, 0, 1));
    CHECK(0 == notcurses_render(nc_));
    egc = notcurses_at_yx(nc_, 0, 0, nullptr, nullptr);
    REQUIRE(nullptr != egc);
    CHECK(0 == strcmp(egc, " "));
    free(egc);
    egc = notcurses_at_yx(nc_, 0, 1, nullptr, nullptr);
    CHECK(0 == strcmp(egc, "a"));
    free(egc);
    ncplane_destroy(high);
  }

  // fill the screen with un-inlineable EGCs
  SUBCASE("OfflineEGCs") {
    nccell c = NCCELL_TRIVIAL_INITIALIZER;
    const char egc[] = u8"\U0001F471\u200D\u2640"; // all one EGC
    CHECK(0 < nccell_load(n_, &c, egc));
    ncplane_set_scrolling(n_, true);
    unsigned dimx, dimy;
    ncplane_dim_yx(n_, &dimy, &dimx);
    unsigned cy;
    do{
      CHECK(0 < ncplane_putc(n_, &c));
      unsigned cx;
      ncplane_cursor_yx(n_, &cy, &cx);
      if(cx + 2 >= dimx){
        CHECK(0 < ncplane_putchar(n_, '\n'));
        ncplane_cursor_yx(n_, &cy, &cx);
      }
    }while(cy + 1 < dimy);
    nccell_release(n_, &c);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("Putwc") {
    wchar_t w = L'\u2658';
    CHECK(0 == ncplane_set_fg_rgb(n_, 0xff00ff));
    CHECK(0 < ncplane_putwc(n_, w));
    CHECK(0 == ncplane_set_fg_rgb(n_, 0x00ff00));
    CHECK(0 == notcurses_render(nc_));
    uint16_t stylemask;
    uint64_t channels;
    char* egc = notcurses_at_yx(nc_, 0, 0, &stylemask, &channels);
    REQUIRE(egc);
    CHECK(0 == stylemask);
    CHECK(0xff00ff == ncchannels_fg_rgb(channels));
    CHECK(0 == strcmp("\u2658", egc));
    free(egc);
  }

  SUBCASE("Putwcstained") {
    ncplane_home(n_);
    wchar_t w = L'\u2658'; // white chess knight (♘)
    CHECK(0 == ncplane_set_fg_rgb(n_, 0xff00ff));
    CHECK(0 < ncplane_putwc(n_, w));
    CHECK(0 == ncplane_set_fg_rgb(n_, 0x00ff00));
    ncplane_home(n_);
    CHECK(0 < ncplane_putwc_stained(n_, w));
    CHECK(0 == notcurses_render(nc_));
    uint16_t stylemask;
    uint64_t channels;
    char* egc = notcurses_at_yx(nc_, 0, 0, &stylemask, &channels);
    REQUIRE(egc);
    CHECK(0 == stylemask);
    CHECK(0xff00ff == ncchannels_fg_rgb(channels));
    CHECK(0 == strcmp("\u2658", egc));
    free(egc);
  }

  CHECK(0 == notcurses_stop(nc_));

}
