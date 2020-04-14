#include <array>
#include <cstdlib>
#include "main.h"
#include "internal.h"

TEST_CASE("Wide") {
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

  // Verify we can emit a wide character, and it advances the cursor by 2
  SUBCASE("EmitWideAsian") {
    if(!enforce_utf8()){
      return;
    }
    const char* w = "\u5168";
    int sbytes = 0;
    CHECK(0 < ncplane_putegc(n_, w, &sbytes));
    int x, y;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == y);
    CHECK(2 == x);
    CHECK(0 == notcurses_render(nc_));
  }

  // Verify a wide character is rejected on the last column
  SUBCASE("RejectWideAsian") {
    if(!enforce_utf8()){
      return;
    }
    const char* w = "\u5168";
    int sbytes = 0;
    int dimx;
    ncplane_dim_yx(n_, NULL, &dimx);
    CHECK(0 < ncplane_putegc_yx(n_, 0, dimx - 3, w, &sbytes));
    int x, y;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == y);
    CHECK(dimx - 1 == x);
    // now it ought be rejected
    CHECK(0 > ncplane_putegc(n_, w, &sbytes));
    CHECK(0 == y);
    CHECK(dimx - 1 == x);
    CHECK(0 == notcurses_render(nc_));
  }

  // half-width, double-width, huge-yet-narrow, all that crap
  SUBCASE("PlaneAtCursorInsane"){
    const char EGC0[] = "\uffe0"; // fullwidth cent sign ￠
    const char EGC1[] = "\u00c5"; // neutral A with ring above Å
    const char EGC2[] = "\u20a9"; // half-width won ₩
    const char EGC3[] = "\u212b"; // ambiguous angstrom Å
    const char EGC4[] = "\ufdfd"; // neutral yet huge bismillah ﷽
    std::array<cell, 5> tcells;
    for(auto i = 0u ; i < tcells.size() ; ++i){
      cell_init(&tcells[i]);
    }
    REQUIRE(1 < cell_load(n_, &tcells[0], EGC0));
    REQUIRE(1 < cell_load(n_, &tcells[1], EGC1));
    REQUIRE(1 < cell_load(n_, &tcells[2], EGC2));
    REQUIRE(1 < cell_load(n_, &tcells[3], EGC3));
    REQUIRE(1 < cell_load(n_, &tcells[4], EGC4));
    for(auto i = 0u ; i < tcells.size() ; ++i){
      REQUIRE(0 < ncplane_putc(n_, &tcells[i]));
    }
    CHECK(0 == notcurses_render(nc_));
    int x = 0;
    for(auto i = 0u ; i < tcells.size() ; ++i){
      CHECK(0 == ncplane_cursor_move_yx(n_, 0, x));
      cell testcell = CELL_TRIVIAL_INITIALIZER;
      REQUIRE(0 < ncplane_at_cursor(n_, &testcell));
      CHECK(!strcmp(cell_extended_gcluster(n_, &tcells[i]),
                    cell_extended_gcluster(n_, &testcell)));
      CHECK(0 == testcell.attrword);
      wchar_t w;
      REQUIRE(0 < mbtowc(&w, cell_extended_gcluster(n_, &tcells[i]), MB_CUR_MAX));
      if(wcwidth(w) == 2){
        CHECK(CELL_WIDEASIAN_MASK == testcell.channels);
        ++x;
      }else{
        CHECK(0 == testcell.channels);
      }
      ++x;
    }
  }

  // Placing a wide char to the immediate left of any other char ought obliterate
  // that cell.
  SUBCASE("WideCharAnnihilatesRight") {
    const char* w = "\xf0\x9f\x90\xb8"; // U+1F438 FROG FACE
    const char* wbashed = "\xf0\x9f\xa6\x82"; // U+1F982 SCORPION
    const char bashed = 'X';
    int sbytes = 0;
    CHECK(0 < ncplane_putegc_yx(n_, 0, 1, wbashed, &sbytes));
    CHECK(0 < ncplane_putsimple_yx(n_, 1, 1, bashed));
    int x, y;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(1 == y);
    CHECK(2 == x);
    CHECK(0 < ncplane_putegc_yx(n_, 0, 0, w, &sbytes));
    CHECK(0 < ncplane_putegc_yx(n_, 1, 0, w, &sbytes));
    cell c = CELL_TRIVIAL_INITIALIZER;
    ncplane_at_yx(n_, 0, 0, &c);
    const char* wres = extended_gcluster(n_, &c);
    CHECK(0 == strcmp(wres, "\xf0\x9f\x90\xb8")); // should be frog
    ncplane_at_yx(n_, 0, 1, &c);
    CHECK(cell_double_wide_p(&c)); // should be wide
    ncplane_at_yx(n_, 0, 2, &c);
    CHECK(0 == c.gcluster); // should be nothing
    ncplane_at_yx(n_, 1, 0, &c);
    wres = extended_gcluster(n_, &c);
    CHECK(0 == strcmp(wres, "\xf0\x9f\x90\xb8")); // should be frog
    ncplane_at_yx(n_, 1, 1, &c);
    CHECK(cell_double_wide_p(&c)); //should be wide
    ncplane_at_yx(n_, 0, 2, &c);
    CHECK(0 == c.gcluster);
    CHECK(0 == notcurses_render(nc_)); // should be nothing
  }

  // Placing a wide char on the right half of a wide char ought obliterate the
  // original wide char.
  SUBCASE("WideCharAnnihilatesWideLeft") {
    const char* w = "\xf0\x9f\x90\x8d";
    const char* wbashed = "\xf0\x9f\xa6\x82";
    int sbytes = 0;
    CHECK(0 < ncplane_putegc_yx(n_, 0, 0, wbashed, &sbytes));
    CHECK(0 < ncplane_putegc_yx(n_, 0, 1, w, &sbytes));
    int x, y;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == y);
    CHECK(3 == x);
    cell c = CELL_TRIVIAL_INITIALIZER;
    ncplane_at_yx(n_, 0, 0, &c);
    CHECK(0 == c.gcluster); // should be nothing
    ncplane_at_yx(n_, 0, 1, &c);
    const char* wres = extended_gcluster(n_, &c);
    CHECK(0 == strcmp(wres, "\xf0\x9f\x90\x8d")); // should be snake
    ncplane_at_yx(n_, 0, 2, &c);
    CHECK(cell_double_wide_p(&c)); // should be wide
    CHECK(0 == notcurses_render(nc_));
  }

  // Placing a normal char on either half of a wide char ought obliterate
  // the original wide char.
  SUBCASE("WideCharsAnnihilated") {
    const char cc = 'X';
    const char* wbashedl = "\xf0\x9f\x90\x8d";
    const char* wbashedr = "\xf0\x9f\xa6\x82";
    int sbytes = 0;
    CHECK(0 < ncplane_putegc_yx(n_, 0, 0, wbashedl, &sbytes));
    CHECK(0 < ncplane_putegc_yx(n_, 0, 2, wbashedr, &sbytes));
    CHECK(1 == ncplane_putsimple_yx(n_, 0, 1, cc));
    CHECK(1 == ncplane_putsimple_yx(n_, 0, 2, cc));
    int x, y;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == y);
    CHECK(3 == x);
    cell c = CELL_TRIVIAL_INITIALIZER;
    ncplane_at_yx(n_, 0, 0, &c);
    CHECK(0 == c.gcluster); // should be nothing
    ncplane_at_yx(n_, 0, 1, &c);
    CHECK(cc == c.gcluster); // should be 'X'
    ncplane_at_yx(n_, 0, 2, &c);
    CHECK(cc == c.gcluster); // should be 'X"
    ncplane_at_yx(n_, 0, 3, &c);
    CHECK(0 == c.gcluster); // should be nothing
    CHECK(0 == notcurses_render(nc_));
  }

  // But placing something to the immediate right of any glyph, that is not a
  // problem. Ensure it is so.
  SUBCASE("AdjacentCharsSafe") {
    const char cc = 'X';
    const char* wsafel = "\xf0\x9f\x90\x8d";
    const char* wsafer = "\xf0\x9f\xa6\x82";
    int sbytes = 0;
    CHECK(0 < ncplane_putegc_yx(n_, 0, 0, wsafel, &sbytes));
    CHECK(0 < ncplane_putegc_yx(n_, 0, 3, wsafer, &sbytes));
    CHECK(1 == ncplane_putsimple_yx(n_, 0, 2, cc));
    int x, y;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == y);
    CHECK(3 == x);
    cell c = CELL_TRIVIAL_INITIALIZER;
    ncplane_at_yx(n_, 0, 0, &c);
    const char* wres = extended_gcluster(n_, &c);
    CHECK(0 == strcmp(wres, "\xf0\x9f\x90\x8d")); // should be snake
    ncplane_at_yx(n_, 0, 1, &c);
    CHECK(cell_double_wide_p(&c)); // should be snake
    ncplane_at_yx(n_, 0, 2, &c);
    CHECK(cc == c.gcluster); // should be 'X'
    ncplane_at_yx(n_, 0, 3, &c);
    wres = extended_gcluster(n_, &c);
    CHECK(0 == strcmp(wres, "\xf0\x9f\xa6\x82")); // should be scorpion
    ncplane_at_yx(n_, 0, 4, &c);
    CHECK(cell_double_wide_p(&c)); // should be scorpion
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("BoxedWideGlyph") {
    struct ncplane* ncp = ncplane_new(nc_, 3, 4, 0, 0, nullptr);
    REQUIRE(ncp);
    int dimx, dimy;
    ncplane_dim_yx(n_, &dimy, &dimx);
    CHECK(0 == ncplane_rounded_box_sized(ncp, 0, 0, 3, 4, 0));
    CHECK(2 == ncplane_putegc_yx(ncp, 1, 1, "\xf0\x9f\xa6\x82", NULL)); // scorpion
    CHECK(0 == notcurses_render(nc_));
    cell c = CELL_TRIVIAL_INITIALIZER;
    REQUIRE(0 < ncplane_at_yx(ncp, 1, 0, &c));
    CHECK(!strcmp(cell_extended_gcluster(ncp, &c), "│"));
    cell_release(ncp, &c);
    char* egc = notcurses_at_yx(nc_, 1, 0, &c.attrword, &c.channels);
    REQUIRE(egc);
    CHECK(!strcmp(egc, "│"));
    free(egc);
    REQUIRE(0 < ncplane_at_yx(ncp, 1, 3, &c));
    CHECK(!strcmp(cell_extended_gcluster(ncp, &c), "│"));
    cell_release(ncp, &c);
    egc = notcurses_at_yx(nc_, 1, 3, &c.attrword, &c.channels);
    REQUIRE(egc);
    CHECK(!strcmp(egc, "│"));
    free(egc);
    CHECK(0 == ncplane_destroy(ncp));
  }

  SUBCASE("RenderWides") {
    CHECK(0 <= ncplane_putstr(n_, "\xe5\xbd\xa2\xe5\x85\xa8"));
    cell c = CELL_TRIVIAL_INITIALIZER;
    ncplane_at_yx(n_, 0, 0, &c);
    CHECK(cell_double_wide_p(&c));
    ncplane_at_yx(n_, 0, 1, &c);
    CHECK(cell_double_wide_p(&c));
    ncplane_at_yx(n_, 0, 2, &c);
    CHECK(cell_double_wide_p(&c));
    ncplane_at_yx(n_, 0, 3, &c);
    CHECK(cell_double_wide_p(&c));
    ncplane_at_yx(n_, 0, 4, &c);
    CHECK(!cell_double_wide_p(&c));
    REQUIRE(0 == notcurses_render(nc_));
    notcurses_at_yx(nc_, 0, 0, &c.attrword, &c.channels);
    CHECK(0 != (c.channels & 0x8000000080000000ull));
    notcurses_at_yx(nc_, 0, 1, &c.attrword, &c.channels);
    CHECK(0 != (c.channels & 0x8000000080000000ull));
    notcurses_at_yx(nc_, 0, 2, &c.attrword, &c.channels);
    CHECK(0 != (c.channels & 0x8000000080000000ull));
    notcurses_at_yx(nc_, 0, 3, &c.attrword, &c.channels);
    CHECK(0 != (c.channels & 0x8000000080000000ull));
    notcurses_at_yx(nc_, 0, 4, &c.attrword, &c.channels);
    CHECK(!(c.channels & 0x8000000080000000ull));
  }

  // Render a translucent plane atop a wide glyph, and check the colors on both
  // cells. See https://github.com/dankamongmen/notcurses/issues/362.
  SUBCASE("OverWide") {
    struct ncplane* p = ncplane_new(nc_, 3, 4, 0, 0, nullptr);
    REQUIRE(nullptr != p);
    cell c = CELL_SIMPLE_INITIALIZER('X');
    CHECK(0 == ncplane_perimeter(p, &c, &c, &c, &c, &c, &c, 0));
    ncplane_set_bg_rgb(n_, 0x20, 0x20, 0x20);
    int sbytes;
    CHECK(2 == ncplane_putegc_yx(n_, 1, 1, "六", &sbytes));
    uint64_t channels = 0;
    channels_set_bg_alpha(&channels, CELL_ALPHA_BLEND);
    channels_set_bg_rgb(&channels, 0x80, 0xf0, 0x10);
    CHECK(1 == ncplane_set_base(p, channels, 0, " "));
    CHECK(0 == notcurses_render(nc_));
    uint32_t attrword;
    uint64_t chanleft, chanright;
    char* egc = notcurses_at_yx(nc_, 1, 1, &attrword, &chanleft);
    REQUIRE(nullptr != egc);
    CHECK(0 == strcmp(" ", egc));
    free(egc);
    egc = notcurses_at_yx(nc_, 1, 2, &attrword, &chanright);
    REQUIRE(nullptr != egc);
    CHECK(0 == strcmp(" ", egc));
    free(egc);
    cell cl = CELL_TRIVIAL_INITIALIZER, cr = CELL_TRIVIAL_INITIALIZER;
    CHECK(3 == ncplane_at_yx(n_, 1, 1, &cl));
    REQUIRE(cell_extended_gcluster(n_, &cl));
    CHECK(0 == strcmp("六", extended_gcluster(n_, &cl)));
    CHECK(0 == ncplane_at_yx(n_, 1, 2, &cr));
    REQUIRE(cell_simple_p(&cr));
    CHECK(0 == cr.gcluster);
    cell_release(n_, &cl);
    cell_release(n_, &cr);
    CHECK(chanright == chanleft);
    ncplane_destroy(p);
  }

  // drag a plane of narrow chars across a plane of wide glyphs
  SUBCASE("NarrowPlaneAtopWide") {
    notcurses_cursor_disable(nc_);
    CHECK(0 == ncplane_set_fg_rgb(n_, 0xff, 0, 0xff));
    // start the 1x4 top plane at 0, 4
    struct ncplane* topp = ncplane_new(nc_, 1, 4, 0, 4, nullptr);
    REQUIRE(nullptr != topp);
    CHECK(0 == ncplane_set_bg_rgb(topp, 0, 0xff, 0));
    CHECK(4 == ncplane_putstr(topp, "abcd"));
    CHECK(12 == ncplane_putstr(n_, "六六六六"));
    CHECK(0 == notcurses_render(nc_));
    uint32_t attrword;
    uint64_t channels;
    char* egc;
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 0, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 1, &attrword, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 2, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 3, &attrword, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 4, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "a"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 5, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "b"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 6, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "c"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 7, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "d"));
    free(egc);
    CHECK(0 == ncplane_move_yx(topp, 0, 3));
    CHECK(0 == notcurses_render(nc_));
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 0, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 1, &attrword, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 2, &attrword, &channels)));
    CHECK(0 == strcmp(egc, " "));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 3, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "a"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 4, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "b"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 5, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "c"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 6, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "d"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 7, &attrword, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    CHECK(0 == ncplane_move_yx(topp, 0, 2));
    CHECK(0 == notcurses_render(nc_));
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 0, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 1, &attrword, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 2, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "a"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 3, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "b"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 4, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "c"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 5, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "d"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 6, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 7, &attrword, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    CHECK(0 == ncplane_move_yx(topp, 0, 1));
    CHECK(0 == notcurses_render(nc_));
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 0, &attrword, &channels)));
    CHECK(0 == strcmp(egc, " "));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 1, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "a"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 2, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "b"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 3, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "c"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 4, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "d"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 5, &attrword, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 6, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "六")); // FIXME returns -1?!?!
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 7, &attrword, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    CHECK(0 == ncplane_move_yx(topp, 0, 0));
    CHECK(0 == notcurses_render(nc_));
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 0, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "a"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 1, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "b"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 2, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "c"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 3, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "d"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 4, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 5, &attrword, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 6, &attrword, &channels)));
    CHECK(0 == strcmp(egc, "六"));
    free(egc);
    REQUIRE((egc = notcurses_at_yx(nc_, 0, 7, &attrword, &channels)));
    CHECK(0 == strcmp(egc, ""));
    free(egc);
    ncplane_destroy(topp);
  }

  // drag a plane of wide glyphs across a plane of narrow glyphs
  SUBCASE("WidePlaneAtopNarrow") {
    CHECK(0 == ncplane_set_fg_rgb(n_, 0xff, 0, 0xff));
    // start the 1x4 top plane at 0, 4
    struct ncplane* topp = ncplane_new(nc_, 1, 4, 0, 4, nullptr);
    REQUIRE(nullptr != topp);
    CHECK(0 == ncplane_set_bg_rgb(topp, 0, 0xff, 0));
    CHECK(6 == ncplane_putstr(topp, "次次"));
    CHECK(9 == ncplane_putstr(n_, "六六六"));
    CHECK(0 == notcurses_render(nc_));
    // FIXME
    ncplane_destroy(topp);
  }

  // drag a plane of wide glyphs across a plane of wide glyphs
  SUBCASE("WidePlaneAtopWide") {
    CHECK(0 == ncplane_set_fg_rgb(n_, 0xff, 0, 0xff));
    // start the 1x4 top plane at 0, 4
    struct ncplane* topp = ncplane_new(nc_, 1, 4, 0, 4, nullptr);
    REQUIRE(nullptr != topp);
    CHECK(0 == ncplane_set_bg_rgb(topp, 0, 0xff, 0));
    CHECK(6 == ncplane_putstr(topp, "次次"));
    CHECK(6 == ncplane_putstr(n_, "abcdef"));
    CHECK(0 == notcurses_render(nc_));
    // FIXME
    ncplane_destroy(topp);
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));

}
