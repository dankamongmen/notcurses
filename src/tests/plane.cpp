#include <cstdlib>
#include "main.h"

void BoxPermutationsRounded(struct notcurses* nc, struct ncplane* n, unsigned edges) {
  unsigned dimx, dimy;
  ncplane_dim_yx(n, &dimy, &dimx);
  REQUIRE(2 < dimy);
  REQUIRE(47 < dimx);
  // we'll try all 16 boxmasks in 3x3 configurations in a 1x16 map
  unsigned boxmask = edges << NCBOXCORNER_SHIFT;
  for(auto x0 = 0 ; x0 < 16 ; ++x0){
    CHECK(0 == ncplane_cursor_move_yx(n, 0, x0 * 3));
    CHECK(0 == ncplane_rounded_box_sized(n, 0, 0, 3, 3, boxmask));
    ++boxmask;
  }
  CHECK(0 == notcurses_render(nc));
}

TEST_CASE("Plane") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  if(!notcurses_canutf8(nc_)){
    CHECK(0 == notcurses_stop(nc_));
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  // Starting position ought be 0, 0 (the origin)
  SUBCASE("StdPlanePosition") {
    unsigned x, y;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == x);
    CHECK(0 == y);
  }

  // Dimensions of the standard plane ought be the same as those of the context
  SUBCASE("StdPlaneDimensions") {
    unsigned cols, rows;
    notcurses_term_dim_yx(nc_, &rows, &cols);
    unsigned ncols, nrows;
    ncplane_dim_yx(n_, &nrows, &ncols);
    CHECK(rows == nrows);
    CHECK(cols == ncols);
  }

  // Verify that we can move to all four coordinates of the standard plane
  SUBCASE("MoveStdPlaneDimensions") {
    unsigned cols, rows;
    notcurses_term_dim_yx(nc_, &rows, &cols);
    CHECK(0 == ncplane_cursor_move_yx(n_, 0, 0));
    unsigned x, y;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(y == 0);
    CHECK(x == 0);
    CHECK(0 == ncplane_cursor_move_yx(n_, rows - 1, 0));
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(y == rows - 1);
    CHECK(x == 0);
    CHECK(0 == ncplane_cursor_move_yx(n_, rows - 1, cols - 1));
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(y == rows - 1);
    CHECK(x == cols - 1);
    CHECK(0 == ncplane_cursor_move_yx(n_, 0, cols - 1));
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(y == 0);
    CHECK(x == cols - 1);
  }

  // Verify that we can move to all four coordinates of the standard plane
  SUBCASE("MoveBeyondPlaneFails") {
    unsigned cols, rows;
    notcurses_term_dim_yx(nc_, &rows, &cols);
    CHECK(0 > ncplane_cursor_move_yx(n_, -2, 0));
    CHECK(0 > ncplane_cursor_move_yx(n_, -2, -2));
    CHECK(0 > ncplane_cursor_move_yx(n_, 0, -2));
    CHECK(0 > ncplane_cursor_move_yx(n_, rows - 1, -2));
    CHECK(0 > ncplane_cursor_move_yx(n_, rows, 0));
    CHECK(0 > ncplane_cursor_move_yx(n_, rows + 1, 0));
    CHECK(0 > ncplane_cursor_move_yx(n_, rows, cols));
    CHECK(0 > ncplane_cursor_move_yx(n_, -2, cols - 1));
    CHECK(0 > ncplane_cursor_move_yx(n_, 0, cols));
    CHECK(0 > ncplane_cursor_move_yx(n_, 0, cols + 1));
  }

  SUBCASE("SetPlaneRGB") {
    CHECK(0 == ncplane_set_fg_rgb8(n_, 0, 0, 0));
    CHECK(0 == ncplane_set_fg_rgb8(n_, 255, 255, 255));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("RejectBadRGB") {
    CHECK(0 > ncplane_set_fg_rgb8(n_, -1, 0, 0));
    CHECK(0 > ncplane_set_fg_rgb8(n_, 0, -1, 0));
    CHECK(0 > ncplane_set_fg_rgb8(n_, 0, 0, -1));
    CHECK(0 > ncplane_set_fg_rgb8(n_, -1, -1, -1));
    CHECK(0 > ncplane_set_fg_rgb8(n_, 256, 255, 255));
    CHECK(0 > ncplane_set_fg_rgb8(n_, 255, 256, 255));
    CHECK(0 > ncplane_set_fg_rgb8(n_, 255, 255, 256));
    CHECK(0 > ncplane_set_fg_rgb8(n_, 256, 256, 256));
  }

  SUBCASE("StandardPlaneChild") {
    struct ncplane_options nopts{};
    nopts.rows = ncplane_dim_y(n_);
    nopts.cols = ncplane_dim_x(n_);
    auto n = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != n);
    CHECK(ncplane_dim_y(n) == ncplane_dim_y(n_));
    CHECK(ncplane_dim_x(n) == ncplane_dim_x(n_));
    CHECK(0 == ncplane_destroy(n));
  }

  // Verify we can emit a NUL character, and it advances the cursor after
  // wiping out whatever we printed it atop.
  SUBCASE("EmitNULCell") {
    nccell c = NCCELL_CHAR_INITIALIZER('a');
    CHECK(0 < ncplane_putc_yx(n_, 0, 0, &c));
    auto egc = ncplane_at_yx(n_, 0, 0, nullptr, nullptr);
    CHECK(0 == strcmp("a", egc));
    free(egc);
    unsigned y, x;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == y);
    CHECK(1 == x);
    CHECK(0 == notcurses_render(nc_));
    c = NCCELL_TRIVIAL_INITIALIZER;
    CHECK(0 < ncplane_putc_yx(n_, 0, 0, &c));
    egc = ncplane_at_yx(n_, 0, 0, nullptr, nullptr);
    CHECK(0 == strcmp("", egc));
    free(egc);
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == y);
    CHECK(1 == x);
    CHECK(0 == notcurses_render(nc_));
  }

  // Verify we can emit a multibyte character, and it advances the cursor
  SUBCASE("EmitCell") {
    const char cchar[] = "✔";
    nccell c{};
    CHECK(strlen(cchar) == nccell_load(n_, &c, cchar));
    CHECK(0 < ncplane_putc(n_, &c));
    unsigned x, y;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == y);
    CHECK(1 == x);
    CHECK(0 == notcurses_render(nc_));
  }

  // Verify we can emit a wchar_t, and it advances the cursor
  SUBCASE("EmitWcharT") {
    const wchar_t* w = L"✔";
    CHECK(0 < ncplane_putwegc(n_, w, nullptr));
    unsigned x, y;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == y);
    CHECK(1 == x);
    CHECK(0 == notcurses_render(nc_));
  }

  // Verify we can emit a multibyte string, and it advances the cursor
  SUBCASE("EmitStr") {
    const char* ss[] = { "Σιβυλλα τι θελεις;",
                         " respondebat illa:",
                         " αποθανειν θελω.", NULL };
    for(const char** s = ss ; *s ; ++s){
      CHECK(0 == ncplane_cursor_move_yx(n_, s - ss, 0));
      int wrote = ncplane_putstr(n_, *s);
      CHECK(ncstrwidth(*s, NULL, NULL) == wrote);
    }
    unsigned x, y;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(2 == y);
    CHECK(10 <= x);
    CHECK(0 == notcurses_render(nc_));
  }

  // Verify we can emit a wide string, and it advances the cursor
  SUBCASE("EmitWideStr") {
    const wchar_t* ss[] = { L"Σιβυλλα τι θελεις;",
                            L" respondebat illa:",
                            L" αποθανειν θελω.", NULL };
    for(const wchar_t** s = ss ; *s ; ++s){
      CHECK(0 == ncplane_cursor_move_yx(n_, s - ss, 0));
      int wrote = ncplane_putwstr(n_, *s);
      CHECK(0 < wrote);
    }
    unsigned x, y;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(2 == y);
    CHECK(10 <= x);
    CHECK(0 == notcurses_render(nc_));
  }

  // some aren't yet proper wcwidth() on ubuntu FIXME
  //"🐯🐰🐱🐲🐳🐴🐵🐶🐷🐹🐺🐻🐼🦉🐊🦕🦖🐬🐙🦠🦀";
  SUBCASE("EmitEmojiStr") {
    const wchar_t e[] =
      L"🍺🚬🌿💉💊🔫💣🤜🤛🐌🐎🐑🐒🐔🐗🐘🐙🐚"
      "🐛🐜🐝🐞🐟🐠🐡🐢🐣🐤🐥🐦🐧🐨🐩🐫🐬🐭🐮";
    CHECK(!ncplane_set_scrolling(n_, true));
    int wrote = ncplane_putwstr(n_, e);
    CHECK(0 < wrote);
    unsigned x, y;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK_LE(0, y);
    CHECK(1 <= x); // FIXME tighten in on this
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("HorizontalLines") {
    unsigned x, y;
    ncplane_dim_yx(n_, &y, &x);
    REQUIRE(0 < y);
    REQUIRE(0 < x);
    nccell c{};
    nccell_load(n_, &c, "-");
    for(unsigned yidx = 0 ; yidx < y ; ++yidx){
      CHECK(0 == ncplane_cursor_move_yx(n_, yidx, 1));
      CHECK(x - 2 == ncplane_hline(n_, &c, x - 2));
      unsigned posx, posy;
      ncplane_cursor_yx(n_, &posy, &posx);
      CHECK(yidx == posy);
      CHECK(x - 1 == posx);
    }
    nccell_release(n_, &c);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("VerticalLines") {
    unsigned x, y;
    ncplane_dim_yx(n_, &y, &x);
    REQUIRE(0 < y);
    REQUIRE(0 < x);
    nccell c{};
    nccell_load(n_, &c, "|");
    for(unsigned xidx = 1 ; xidx < x - 1 ; ++xidx){
      CHECK(0 == ncplane_cursor_move_yx(n_, 1, xidx));
      CHECK(y - 2 == ncplane_vline(n_, &c, y - 2));
      unsigned posx, posy;
      ncplane_cursor_yx(n_, &posy, &posx);
      CHECK(y - 2 == posy);
      CHECK(xidx == posx - 1);
    }
    nccell_release(n_, &c);
    CHECK(0 == notcurses_render(nc_));
  }

  // reject attempts to draw boxes beyond the boundaries of the ncplane
  SUBCASE("BadlyPlacedBoxen") {
    unsigned x, y;
    ncplane_dim_yx(n_, &y, &x);
    REQUIRE(2 < y);
    REQUIRE(2 < x);
    nccell ul{}, ll{}, lr{}, ur{}, hl{}, vl{};
    REQUIRE(0 == nccells_rounded_box(n_, 0, 0, &ul, &ur, &ll, &lr, &hl, &vl));
    CHECK_GT(0, ncplane_box(n_, &ul, &ur, &ll, &lr, &hl, &vl, y + 1, x + 1, 0));
    CHECK(0 == ncplane_cursor_move_yx(n_, 1, 0));
    CHECK_GT(0, ncplane_box(n_, &ul, &ur, &ll, &lr, &hl, &vl, y, x, 0));
    CHECK(0 == ncplane_cursor_move_yx(n_, 0, 1));
    CHECK_GT(0, ncplane_box(n_, &ul, &ur, &ll, &lr, &hl, &vl, y, x, 0));
    CHECK(0 == ncplane_cursor_move_yx(n_, y - 1, x - 1));
    CHECK_GT(0, ncplane_box(n_, &ul, &ur, &ll, &lr, &hl, &vl, 2, 2, 0));
    CHECK(0 == ncplane_cursor_move_yx(n_, y - 2, x - 1));
    CHECK_GT(0, ncplane_box(n_, &ul, &ur, &ll, &lr, &hl, &vl, 2, 2, 0));
    CHECK(0 == ncplane_cursor_move_yx(n_, y - 1, x - 2));
    CHECK_GT(0, ncplane_box(n_, &ul, &ur, &ll, &lr, &hl, &vl, 2, 2, 0));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("BoxPermutationsRoundedZeroEdges") {
    BoxPermutationsRounded(nc_, n_, 0);
  }

  SUBCASE("BoxPermutationsRoundedOneEdges") {
    BoxPermutationsRounded(nc_, n_, 1);
  }

  SUBCASE("BoxPermutationsRoundedTwoEdges") {
    BoxPermutationsRounded(nc_, n_, 2);
  }

  SUBCASE("BoxPermutationsRoundedThreeEdges") {
    BoxPermutationsRounded(nc_, n_, 3);
  }

  SUBCASE("BoxPermutationsDouble") {
    unsigned dimx, dimy;
    ncplane_dim_yx(n_, &dimy, &dimx);
    REQUIRE(2 < dimx);
    REQUIRE(47 < dimx);
    // we'll try all 16 boxmasks in 3x3 configurations in a 1x16 map
    unsigned boxmask = 0;
    for(auto x0 = 0 ; x0 < 16 ; ++x0){
      CHECK(0 == ncplane_cursor_move_yx(n_, 0, x0 * 3));
      CHECK(0 == ncplane_double_box_sized(n_, 0, 0, 3, 3, boxmask));
      ++boxmask;
    }
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("PerimeterRoundedBox") {
    unsigned x, y;
    ncplane_dim_yx(n_, &y, &x);
    REQUIRE(2 < y);
    REQUIRE(2 < x);
    REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));
    CHECK(0 == ncplane_rounded_box(n_, 0, 0, y - 1, x - 1, 0));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("PerimeterRoundedBoxSized") {
    unsigned x, y;
    ncplane_dim_yx(n_, &y, &x);
    REQUIRE(2 < y);
    REQUIRE(2 < x);
    REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));
    CHECK(0 == ncplane_rounded_box_sized(n_, 0, 0, y, x, 0));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("PerimeterDoubleBox") {
    unsigned x, y;
    ncplane_dim_yx(n_, &y, &x);
    REQUIRE(2 < y);
    REQUIRE(2 < x);
    REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));
    CHECK(0 == ncplane_double_box(n_, 0, 0, y - 1, x - 1, 0));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("PerimeterDoubleBoxSized") {
    unsigned x, y;
    ncplane_dim_yx(n_, &y, &x);
    REQUIRE(2 < y);
    REQUIRE(2 < x);
    REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));
    CHECK(0 == ncplane_double_box_sized(n_, 0, 0, y, x, 0));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("EraseScreen") {
    ncplane_erase(n_);
    CHECK(0 == notcurses_render(nc_));
  }

  // we're gonna run both a composed latin a with grave, and then a latin a with
  // a combining nonspacing grave
  SUBCASE("CellLoadCombining") {
    const char* w1 = "à"; // U+00E0, U+0000         (c3 a0)
    const char* w2 = "à"; // U+0061, U+0300, U+0000 (61 cc 80)
    const char* w3 = "a"; // U+0061, U+0000         (61)
    nccell cell1 = NCCELL_TRIVIAL_INITIALIZER;
    nccell cell2 = NCCELL_TRIVIAL_INITIALIZER;
    nccell cell3 = NCCELL_TRIVIAL_INITIALIZER;
    auto u1 = nccell_load(n_, &cell1, w1);
    auto u2 = nccell_load(n_, &cell2, w2);
    auto u3 = nccell_load(n_, &cell3, w3);
    REQUIRE(2 == u1);
    REQUIRE(3 == u2);
    REQUIRE(1 == u3);
    nccell_release(n_, &cell1);
    nccell_release(n_, &cell2);
    nccell_release(n_, &cell3);
  }

  SUBCASE("CellDuplicateCombining") {
    const char* w1 = "à"; // U+00E0, U+0000         (c3 a0)
    const char* w2 = "à"; // U+0061, U+0300, U+0000 (61 cc 80)
    const char* w3 = "a"; // U+0061, U+0000         (61)
    nccell cell1 = NCCELL_TRIVIAL_INITIALIZER;
    nccell cell2 = NCCELL_TRIVIAL_INITIALIZER;
    nccell cell3 = NCCELL_TRIVIAL_INITIALIZER;
    auto u1 = nccell_load(n_, &cell1, w1);
    auto u2 = nccell_load(n_, &cell2, w2);
    auto u3 = nccell_load(n_, &cell3, w3);
    REQUIRE(2 == u1);
    REQUIRE(3 == u2);
    REQUIRE(1 == u3);
    nccell cell4 = NCCELL_TRIVIAL_INITIALIZER;
    nccell cell5 = NCCELL_TRIVIAL_INITIALIZER;
    nccell cell6 = NCCELL_TRIVIAL_INITIALIZER;
    CHECK(0 == nccell_duplicate(n_, &cell4, &cell1));
    CHECK(0 == nccell_duplicate(n_, &cell5, &cell2));
    CHECK(0 == nccell_duplicate(n_, &cell6, &cell3));
    nccell_release(n_, &cell1);
    nccell_release(n_, &cell2);
    nccell_release(n_, &cell3);
    nccell_release(n_, &cell4);
    nccell_release(n_, &cell5);
    nccell_release(n_, &cell6);
  }

  SUBCASE("CellMultiColumn") {
    const char* w1 = "\xf0\x9f\x91\xa9"; // U+1F469 WOMAN
    const char* w2 = "N";
    nccell c1 = NCCELL_TRIVIAL_INITIALIZER;
    nccell c2 = NCCELL_TRIVIAL_INITIALIZER;
    auto u1 = nccell_load(n_, &c1, w1);
    auto u2 = nccell_load(n_, &c2, w2);
    REQUIRE(0 < u1);
    REQUIRE(0 < u2);
    REQUIRE(strlen(w1) == u1);
    REQUIRE(strlen(w2) == u2);
    CHECK(ncstrwidth(w1, NULL, NULL) == 1 + nccell_double_wide_p(&c1));
    CHECK_FALSE(nccell_double_wide_p(&c2));
    nccell_release(n_, &c1);
    nccell_release(n_, &c2);
  }

  // verifies that the initial userptr is what we provided, that it is a nullptr
  // for the standard plane, and that we can change it.
  SUBCASE("UserPtr") {
    CHECK(nullptr == ncplane_userptr(n_));
    unsigned x, y;
    void* sentinel = &x;
    notcurses_term_dim_yx(nc_, &y, &x);
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = y,
      .cols = x,
      .userptr = sentinel,
      .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* ncp = ncplane_create(n_, &nopts);
    REQUIRE(ncp);
    CHECK(&x == ncplane_userptr(ncp));
    CHECK(sentinel == ncplane_set_userptr(ncp, nullptr));
    CHECK(nullptr == ncplane_userptr(ncp));
    sentinel = &y;
    CHECK(nullptr == ncplane_set_userptr(ncp, sentinel));
    CHECK(&y == ncplane_userptr(ncp));
    CHECK(0 == ncplane_destroy(ncp));
  }

  // create a new plane, the same size as the terminal, and verify that it
  // occupies the same dimensions as the standard plane.
  SUBCASE("NewPlaneSameSize") {
    unsigned x, y;
    notcurses_term_dim_yx(nc_, &y, &x);
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = y,
      .cols = x,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* ncp = ncplane_create(n_, &nopts);
    REQUIRE(ncp);
    unsigned px, py;
    ncplane_dim_yx(ncp, &py, &px);
    CHECK(y == py);
    CHECK(x == px);
    unsigned sx, sy;
    ncplane_dim_yx(n_, &sy, &sx);
    CHECK(sy == py);
    CHECK(sx == px);
    CHECK(0 == ncplane_destroy(ncp));
  }

  // create a new plane, the same size as the terminal, and verify that it
  // occupies the same dimensions as the standard plane, but on another pile.
  SUBCASE("NewPileSameSize") {
    unsigned x, y;
    notcurses_term_dim_yx(nc_, &y, &x);
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = y,
      .cols = x,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* ncp = ncpile_create(nc_, &nopts);
    REQUIRE(ncp);
    unsigned px, py;
    ncplane_dim_yx(ncp, &py, &px);
    CHECK(y == py);
    CHECK(x == px);
    unsigned sx, sy;
    ncplane_dim_yx(n_, &sy, &sx);
    CHECK(sy == py);
    CHECK(sx == px);
    // ensure that the new plane is not on our zaxis
    CHECK(notcurses_top(nc_) == n_);
    CHECK(notcurses_bottom(nc_) == n_);
    // ensure the new plane has null above and below, and is bound to itself
    CHECK(ncplane_above(ncp) == nullptr);
    CHECK(ncplane_below(ncp) == nullptr);
    CHECK(ncplane_parent_const(ncp) == ncp);
    CHECK(0 == ncplane_destroy(ncp));
  }

  SUBCASE("ShrinkPlane") {
    unsigned maxx, maxy;
    int x = 0, y = 0;
    notcurses_term_dim_yx(nc_, &maxy, &maxx);
    struct ncplane_options nopts = {
      .y = y,
      .x = x,
      .rows = maxy,
      .cols = maxx,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* newp = ncplane_create(n_, &nopts);
    REQUIRE(newp);
    CHECK(0 == notcurses_render(nc_));
    while(y > 4 && x > 4){
      maxx -= 2;
      maxy -= 2;
      ++x;
      ++y;
      REQUIRE(0 == ncplane_resize(newp, 1, 1, maxy, maxx, 1, 1, maxy, maxx));
      CHECK(0 == notcurses_render(nc_));
      // FIXME check dims, pos
    }
    while(y > 4){
      maxy -= 2;
      ++y;
      REQUIRE(0 == ncplane_resize(newp, 1, 0, maxy, maxx, 1, 0, maxy, maxx));
      CHECK(0 == notcurses_render(nc_));
      // FIXME check dims, pos
    }
    while(x > 4){
      maxx -= 2;
      ++x;
      REQUIRE(0 == ncplane_resize(newp, 0, 1, maxy, maxx, 0, 1, maxy, maxx));
      CHECK(0 == notcurses_render(nc_));
      // FIXME check dims, pos
    }
    REQUIRE(0 == ncplane_resize(newp, 0, 0, 0, 0, 0, 0, 2, 2));
    CHECK(0 == notcurses_render(nc_));
    // FIXME check dims, pos
    REQUIRE(0 == ncplane_destroy(newp));
  }

  SUBCASE("GrowPlane") {
    unsigned maxx = 2, maxy = 2;
    int x = 0, y = 0;
    unsigned dimy, dimx;
    notcurses_term_dim_yx(nc_, &dimy, &dimx);
    x = dimx / 2 - 1;
    y = dimy / 2 - 1;
    struct ncplane_options nopts = {
      .y = y,
      .x = x,
      .rows = maxy,
      .cols = maxx,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* newp = ncplane_create(n_, &nopts);
    REQUIRE(newp);
    while(dimx - maxx > 4 && dimy - maxy > 4){
      maxx += 2;
      maxy += 2;
      --x;
      --y;
      REQUIRE(0 == ncplane_resize(newp, 1, 1, maxy - 3, maxx - 3, 1, 1, maxy, maxx));
      // FIXME check dims, pos
    }
    while(y < static_cast<int>(dimy)){
      ++maxy;
      if(y){
        ++y;
      }
      REQUIRE(0 == ncplane_resize(newp, 1, 0, maxy - 2, maxx, 1, 0, maxy, maxx));
      // FIXME check dims, pos
    }
    while(x < static_cast<int>(dimx)){
      ++maxx;
      if(x){
        ++x;
      }
      REQUIRE(0 == ncplane_resize(newp, 0, 1, maxy, maxx - 2, 0, 1, maxy, maxx));
      // FIXME check dims, pos
    }
    REQUIRE(0 == ncplane_resize(newp, 0, 0, 0, 0, 0, 0, dimy, dimx));
    // FIXME check dims, pos
    REQUIRE(0 == ncplane_destroy(newp));
  }

  // we ought be able to see what we're about to render, or have just rendered, or
  // in any case whatever's in the virtual framebuffer for a plane
  SUBCASE("PlaneAtCursorSimples"){
    const char STR1[] = "Jackdaws love my big sphinx of quartz";
    const char STR2[] = "Cwm fjord bank glyphs vext quiz";
    const char STR3[] = "Pack my box with five dozen liquor jugs";
    ncplane_set_styles(n_, NCSCALE_NONE);
    REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));
    REQUIRE(0 < ncplane_putstr(n_, STR1));
    nccell testcell = NCCELL_TRIVIAL_INITIALIZER;
    REQUIRE(0 == ncplane_at_cursor_cell(n_, &testcell)); // want nothing at the cursor
    CHECK(0 == testcell.gcluster);
    CHECK(0 == testcell.stylemask);
    CHECK(0 == testcell.channels);
    nccell_release(n_, &testcell);
    unsigned dimy, dimx;
    ncplane_dim_yx(n_, &dimy, &dimx);
    REQUIRE(0 == ncplane_cursor_move_yx(n_, 1, dimx - strlen(STR2)));
    REQUIRE(0 < ncplane_putstr(n_, STR2));
    unsigned y, x;
    ncplane_cursor_yx(n_, &y, &x);
    REQUIRE(1 == y);
    REQUIRE(dimx == x);
    // this ought not print anything, since we're at the end of the row
    REQUIRE(0 == ncplane_putstr(n_, STR3));
    REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));
    REQUIRE(0 < ncplane_at_cursor_cell(n_, &testcell)); // want first char of STR1
    CHECK(htole(STR1[0]) == testcell.gcluster);
    CHECK(0 == testcell.stylemask);
    CHECK(0 == testcell.channels);
    REQUIRE(0 == ncplane_cursor_move_yx(n_, 1, dimx - 1));
    REQUIRE(0 < ncplane_at_cursor_cell(n_, &testcell)); // want last char of STR2
    CHECK(htole(STR2[strlen(STR2) - 1]) == testcell.gcluster);
    CHECK(0 == testcell.stylemask);
    CHECK(0 == testcell.channels);
    // FIXME maybe check all cells?
    CHECK(0 == notcurses_render(nc_));
  }

  // ensure we read back what's expected for latinesque complex characters
  SUBCASE("PlaneAtCursorComplex"){
    const char STR1[] = "Σιβυλλα τι θελεις; respondebat illa:";
    const char STR2[] = "αποθανειν θελω";
    const char STR3[] = "Война и мир"; // just thrown in to complicate things
    ncplane_set_styles(n_, NCSTYLE_NONE);
    REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));
    REQUIRE(0 < ncplane_putstr(n_, STR1));
    nccell testcell = NCCELL_TRIVIAL_INITIALIZER;
    ncplane_at_cursor_cell(n_, &testcell); // should be nothing at the cursor
    CHECK(0 == testcell.gcluster);
    CHECK(0 == testcell.stylemask);
    CHECK(0 == testcell.channels);
    nccell_release(n_, &testcell);
    unsigned dimy, dimx;
    ncplane_dim_yx(n_, &dimy, &dimx);
    REQUIRE(0 == ncplane_cursor_move_yx(n_, 1, dimx - mbstowcs(nullptr, STR2, 0)));
    REQUIRE(0 < ncplane_putstr(n_, STR2));
    unsigned y, x;
    ncplane_cursor_yx(n_, &y, &x);
    REQUIRE(1 == y);
    REQUIRE(dimx == x);
    // this ought not print anything, since we're at the end of the row
    REQUIRE(0 == ncplane_putstr(n_, STR3));
    REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));
    REQUIRE(0 < ncplane_at_cursor_cell(n_, &testcell)); // want first char of STR1
    CHECK(!strcmp("Σ", nccell_extended_gcluster(n_, &testcell)));
    CHECK(0 == testcell.stylemask);
    CHECK(0 == testcell.channels);
    REQUIRE(0 == ncplane_cursor_move_yx(n_, 1, dimx - mbstowcs(nullptr, STR2, 0)));
    REQUIRE(0 < ncplane_at_cursor_cell(n_, &testcell)); // want first char of STR2
    CHECK(!strcmp("α", nccell_extended_gcluster(n_, &testcell)));
    CHECK(0 == testcell.stylemask);
    CHECK(0 == testcell.channels);
    // FIXME maybe check all cells?
    CHECK(0 == notcurses_render(nc_));
  }

  // test that we read back correct attrs/colors despite changing defaults
  SUBCASE("PlaneAtCursorAttrs"){
    const char STR1[] = "this was a world destroyer prod";
    const char STR2[] = "not to mention dank";
    const char STR3[] = "da chronic lives";
    ncplane_set_styles(n_, NCSTYLE_BOLD);
    REQUIRE(0 < ncplane_putstr_yx(n_, 0, 0, STR1));
    unsigned y, x;
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(0 == ncplane_cursor_move_yx(n_, y + 1, x - strlen(STR2)));
    ncplane_on_styles(n_, NCSTYLE_ITALIC);
    REQUIRE(0 < ncplane_putstr(n_, STR2));
    CHECK(0 == ncplane_cursor_move_yx(n_, y + 2, x - strlen(STR3)));
    ncplane_off_styles(n_, NCSTYLE_BOLD);
    REQUIRE(0 < ncplane_putstr(n_, STR3));
    ncplane_off_styles(n_, NCSTYLE_ITALIC);
    CHECK(0 == notcurses_render(nc_));
    unsigned newx;
    ncplane_cursor_yx(n_, &y, &newx);
    CHECK(newx == x);
    nccell testcell = NCCELL_TRIVIAL_INITIALIZER;
    CHECK(0 == ncplane_cursor_move_yx(n_, y - 2, x - 1));
    REQUIRE(1 == ncplane_at_cursor_cell(n_, &testcell));
    CHECK(testcell.gcluster == htole(STR1[strlen(STR1) - 1]));
    CHECK(0 == ncplane_cursor_move_yx(n_, y - 1, x - 1));
    REQUIRE(1 == ncplane_at_cursor_cell(n_, &testcell));
    CHECK(testcell.gcluster == htole(STR2[strlen(STR2) - 1]));
    CHECK(0 == ncplane_cursor_move_yx(n_, y, x - 1));
    REQUIRE(1 == ncplane_at_cursor_cell(n_, &testcell));
    CHECK(testcell.gcluster == htole(STR3[strlen(STR3) - 1]));
  }

  SUBCASE("BoxGradients") {
    const auto sidesz = 5;
    unsigned dimx, dimy;
    ncplane_dim_yx(n_, &dimy, &dimx);
    REQUIRE(20 < dimy);
    REQUIRE(40 < dimx);
    nccell ul{}, ll{}, lr{}, ur{}, hl{}, vl{};
    REQUIRE(0 == nccells_double_box(n_, 0, 0, &ul, &ur, &ll, &lr, &hl, &vl));
    CHECK(0 == ncchannels_set_fg_rgb8(&ul.channels, 255, 0, 0));
    CHECK(0 == ncchannels_set_fg_rgb8(&ur.channels, 0, 255, 0));
    CHECK(0 == ncchannels_set_fg_rgb8(&ll.channels, 0, 0, 255));
    CHECK(0 == ncchannels_set_fg_rgb8(&lr.channels, 255, 255, 255));
    CHECK(0 == ncchannels_set_bg_rgb8(&ul.channels, 0, 255, 255));
    CHECK(0 == ncchannels_set_bg_rgb8(&ur.channels, 255, 0, 255));
    CHECK(0 == ncchannels_set_bg_rgb8(&ll.channels, 255, 255, 0));
    CHECK(0 == ncchannels_set_bg_rgb8(&lr.channels, 0, 0, 0));
    // we'll try all 16 gradmasks in sideszXsidesz configs in a 4x4 map
    unsigned gradmask = 0;
    for(auto y0 = 0 ; y0 < 4 ; ++y0){
      for(auto x0 = 0 ; x0 < 4 ; ++x0){
        CHECK(0 == ncplane_cursor_move_yx(n_, y0 * sidesz, x0 * (sidesz + 1)));
        CHECK(0 == ncplane_box_sized(n_, &ul, &ur, &ll, &lr, &hl, &vl,
                                     sidesz, sidesz, gradmask << 4u));
        ++gradmask;
      }
    }
    gradmask = 0;
    for(auto y0 = 0 ; y0 < 4 ; ++y0){
      for(auto x0 = 0 ; x0 < 4 ; ++x0){
        CHECK(0 == ncplane_cursor_move_yx(n_, y0 * sidesz, x0 * (sidesz + 1) + (4 * (sidesz + 1))));
        CHECK(0 == ncplane_box_sized(n_, &ul, &ur, &ll, &lr, &hl, &vl,
                                     sidesz, sidesz, gradmask << 4u));
        ++gradmask;
      }
    }
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("BoxSideColors") {
    const auto sidesz = 5;
    unsigned dimx, dimy;
    ncplane_dim_yx(n_, &dimy, &dimx);
    REQUIRE(20 < dimy);
    REQUIRE(40 < dimx);
    nccell ul{}, ll{}, lr{}, ur{}, hl{}, vl{};
    REQUIRE(0 == nccells_rounded_box(n_, 0, 0, &ul, &ur, &ll, &lr, &hl, &vl));
    // we'll try all 16 boxmasks in sideszXsidesz configurations in a 4x4 map
    CHECK(0 == ncchannels_set_fg_rgb8(&ul.channels, 255, 0, 0));
    CHECK(0 == ncchannels_set_fg_rgb8(&ur.channels, 0, 255, 0));
    CHECK(0 == ncchannels_set_fg_rgb8(&ll.channels, 0, 0, 255));
    CHECK(0 == ncchannels_set_fg_rgb8(&lr.channels, 0, 0, 0));
    CHECK(0 == ncchannels_set_bg_rgb8(&ul.channels, 0, 255, 255));
    CHECK(0 == ncchannels_set_bg_rgb8(&ur.channels, 255, 0, 255));
    CHECK(0 == ncchannels_set_bg_rgb8(&ll.channels, 255, 255, 0));
    CHECK(0 == ncchannels_set_bg_rgb8(&lr.channels, 0, 0, 0));
    CHECK(0 == ncchannels_set_fg_rgb8(&hl.channels, 255, 0, 255));
    CHECK(0 == ncchannels_set_fg_rgb8(&vl.channels, 255, 255, 255));
    CHECK(0 == ncchannels_set_bg_rgb8(&hl.channels, 0, 255, 0));
    CHECK(0 == ncchannels_set_bg_rgb8(&vl.channels, 0, 0, 0));
    for(auto y0 = 0 ; y0 < 4 ; ++y0){
      for(auto x0 = 0 ; x0 < 4 ; ++x0){
        CHECK(0 == ncplane_cursor_move_yx(n_, y0 * sidesz, x0 * (sidesz + 1)));
        CHECK(0 == ncplane_box_sized(n_, &ul, &ur, &ll, &lr, &hl, &vl,
                                      sidesz, sidesz, 0));
      }
    }
    for(auto y0 = 0 ; y0 < 4 ; ++y0){
      for(auto x0 = 0 ; x0 < 4 ; ++x0){
        CHECK(0 == ncplane_cursor_move_yx(n_, y0 * sidesz, x0 * (sidesz + 1) + (4 * (sidesz + 1))));
        CHECK(0 == ncplane_box_sized(n_, &ul, &ur, &ll, &lr, &hl, &vl,
                                      sidesz, sidesz, 0));
      }
    }
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("RightToLeft") {
    // give us some room on both sides
    CHECK(0 == ncplane_cursor_move_yx(n_, 1, 5));
    CHECK(0 < ncplane_putegc(n_, "־", nullptr));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_cursor_move_yx(n_, 3, 5));
    CHECK(0 < ncplane_putstr(n_, "I write English + מילים בעברית together."));
    CHECK(0 == ncplane_cursor_move_yx(n_, 5, 5));
    CHECK(0 < ncplane_putstr(n_, "|🔥|I have not yet ־ begun to hack|🔥|"));
    CHECK(0 == ncplane_cursor_move_yx(n_, 7, 5));
    CHECK(0 < ncplane_putstr(n_, "㉀㉁㉂㉃㉄㉅㉆㉇㉈㉉㉊㉋㉌㉍㉎㉏㉐"));
    CHECK(0 == ncplane_cursor_move_yx(n_, 8, 5));
    CHECK(0 < ncplane_putstr(n_, "㉑㉒㉓㉔㉕㉖㉗㉘㉙㉚㉛㉜㉝㉞㉟"));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("NewPlaneOnRight") {
    unsigned ncols, nrows;
    ncplane_dim_yx(n_, &nrows, &ncols);
    nccell ul{}, ll{}, lr{}, ur{}, hl{}, vl{};
    int y, x;
    ncplane_yx(n_, &y, &x);
    struct ncplane_options nopts = {
      .y = y,
      .x = static_cast<int>(ncols) - 3,
      .rows = 2,
      .cols = 2,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* ncp = ncplane_create(n_, &nopts);
    REQUIRE(ncp);
    REQUIRE(0 == nccells_rounded_box(ncp, 0, 0, &ul, &ur, &ll, &lr, &hl, &vl));
    CHECK(0 == ncplane_box(ncp, &ul, &ur, &ll, &lr, &hl, &vl, y + 1, x + 1, 0));
    CHECK(0 == notcurses_render(nc_));
    // FIXME verify with ncplane_at_cursor_cell()
    CHECK(0 == ncplane_destroy(ncp));
  }

  SUBCASE("MoveToLowerRight") {
    unsigned ncols, nrows;
    ncplane_dim_yx(n_, &nrows, &ncols);
    nccell ul{}, ll{}, lr{}, ur{}, hl{}, vl{};
    int y, x;
    ncplane_yx(n_, &y, &x);
    struct ncplane_options nopts = {
      .y = y,
      .x = x,
      .rows = 2,
      .cols = 2,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* ncp = ncplane_create(n_, &nopts);
    REQUIRE(ncp);
    REQUIRE(0 == nccells_rounded_box(ncp, 0, 0, &ul, &ur, &ll, &lr, &hl, &vl));
    CHECK(0 == ncplane_box(ncp, &ul, &ur, &ll, &lr, &hl, &vl, y + 1, x + 1, 0));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_move_yx(ncp, nrows - 3, ncols - 3));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(ncp));
    // FIXME verify with ncplane_at_cursor_cell()
  }

  SUBCASE("Perimeter") {
    nccell c = NCCELL_CHAR_INITIALIZER('X');
    CHECK(0 == ncplane_perimeter(n_, &c, &c, &c, &c, &c, &c, 0));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("EGCStained") {
    nccell c = NCCELL_TRIVIAL_INITIALIZER;
    CHECK(0 == ncplane_set_fg_rgb(n_, 0x444444));
    CHECK(1 == ncplane_putegc(n_, "A", nullptr));
    CHECK(0 == ncplane_set_fg_rgb(n_, 0x888888));
    CHECK(1 == ncplane_putegc(n_, "B", nullptr));
    CHECK(0 == ncplane_cursor_move_yx(n_, 0, 0));
    CHECK(0 == notcurses_render(nc_));
    // EGC should change, but not the color
    CHECK(0 == ncplane_set_fg_rgb(n_, 0x222222));
    CHECK(1 == ncplane_putegc_stained(n_, "C", nullptr));
    CHECK(1 == ncplane_putegc_stained(n_, "D", nullptr));
    uint64_t channels = 0;
    CHECK(1 == ncplane_at_yx_cell(n_, 0, 0, &c));
    CHECK(cell_simple_p(&c));
    CHECK(htole('C') == c.gcluster);
    CHECK(0 == ncchannels_set_fg_rgb(&channels, 0x444444));
    CHECK(channels == c.channels);
    CHECK(1 == ncplane_at_yx_cell(n_, 0, 1, &c));
    CHECK(cell_simple_p(&c));
    CHECK(htole('D') == c.gcluster);
    CHECK(0 == ncchannels_set_fg_rgb(&channels, 0x888888));
    CHECK(channels == c.channels);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("MouseEvent") {
    unsigned dimy, dimx;
    notcurses_stddim_yx(nc_, &dimy, &dimx);
    struct ncplane_options nopts = {
      .y = 1,
      .x = 1,
      .rows = 2,
      .cols = 2,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* n = ncplane_create(n_, &nopts);
    REQUIRE(n);
    ncinput ni{};
    ni.id = NCKEY_BUTTON1;
    ni.evtype = ncpp::EvType::Release;
    int total = 0;
    for(ni.y = 0 ; ni.y < 5 ; ++ni.y){
      for(ni.x = 0 ; ni.x < 5 ; ++ni.x){
        int y = ni.y, x = ni.x;
        bool p = ncplane_translate_abs(n, &y, &x);
        if(ni.y >= 1 && ni.y <= 2 && ni.x >= 1 && ni.x <= 2){
          CHECK(p);
        }else{
          CHECK(!p);
        }
        ++total;
      }
    }
    CHECK(25 == total); // make sure x/y never got changed
    CHECK(0 == ncplane_destroy(n));
  }

  SUBCASE("BoundPlaneMoves") {
    struct ncplane_options nopts = {
      .y = 1,
      .x = 1,
      .rows = 2,
      .cols = 2,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* ndom = ncplane_create(n_, &nopts);
    REQUIRE(ndom);
    struct ncplane* nsub = ncplane_create(n_, &nopts);
    REQUIRE(nsub);
    int absy, absx;
    ncplane_yx(nsub, &absy, &absx);
    CHECK(0 == notcurses_render(nc_));
    CHECK(1 == absy); // actually at 2, 2
    CHECK(1 == absx);
    CHECK(0 == ncplane_move_yx(nsub, -1, -1));
    ncplane_yx(nsub, &absy, &absx);
    CHECK(-1 == absy); // actually at 0, 0
    CHECK(-1 == absx);
  }

  SUBCASE("BoundToPlaneMoves") { // bound plane ought move along with plane
    struct ncplane_options nopts = {
      .y = 1,
      .x = 1,
      .rows = 2,
      .cols = 2,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* ndom = ncplane_create(n_, &nopts);
    REQUIRE(ndom);
    struct ncplane* nsub = ncplane_create(n_, &nopts);
    REQUIRE(nsub);
    int absy, absx;
    ncplane_yx(nsub, &absy, &absx);
    CHECK(0 == notcurses_render(nc_));
    CHECK(1 == absy); // actually at 2, 2
    CHECK(1 == absx);
    CHECK(0 == ncplane_move_yx(ndom, 0, 0));
    ncplane_yx(nsub, &absy, &absx);
    CHECK(1 == absy);
    CHECK(1 == absx);
  }

  SUBCASE("UnboundPlaneMoves") { // unbound plane no longer gets pulled along
    struct ncplane_options nopts = {
      .y = 1,
      .x = 1,
      .rows = 2,
      .cols = 2,
      .userptr = nullptr,
      .name = "ndom",
      .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* ndom = ncplane_create(n_, &nopts);
    CHECK(ncplane_pile(ndom) == ncplane_pile(n_));
    REQUIRE(ndom);
    nopts.name = "sub";
    struct ncplane* nsub = ncplane_create(ndom, &nopts);
    CHECK(ncplane_pile(nsub) == ncplane_pile(ndom));
    REQUIRE(nsub);
    int absy, absx;
    CHECK(0 == notcurses_render(nc_));
    ncplane_yx(nsub, &absy, &absx);
    CHECK(1 == absy); // relatively at 1, 1
    CHECK(1 == absx);
    ncplane_abs_yx(nsub, &absy, &absx);
    CHECK(2 == absy); // actually at 2, 2
    CHECK(2 == absx);
    ncplane_reparent(nsub, nsub);
    CHECK(ncplane_pile(nsub) != ncplane_pile(ndom));
    ncplane_abs_yx(nsub, &absy, &absx);
    CHECK(2 == absy); // now relatively at 1, 1
    CHECK(2 == absx);
    ncplane_yx(nsub, &absy, &absx);
    CHECK(2 == absy);
    CHECK(2 == absx);
    CHECK(0 == ncplane_move_yx(ndom, 0, 0));
    ncplane_abs_yx(nsub, &absy, &absx);
    CHECK(2 == absy); // still at 1, 1
    CHECK(2 == absx);
    ncplane_yx(nsub, &absy, &absx);
    CHECK(2 == absy);
    CHECK(2 == absx);
  }

  SUBCASE("NoReparentStdPlane") {
    struct ncplane_options nopts = {
      .y = 1,
      .x = 1,
      .rows = 2,
      .cols = 2,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* ndom = ncplane_create(n_, &nopts);
    REQUIRE(ndom);
    CHECK(!ncplane_reparent(n_, ndom)); // can't reparent standard plane
    CHECK(ncplane_reparent(ndom, n_)); // *can* reparent *to* standard plane
  }

  SUBCASE("ReparentDeep") {
    struct ncplane_options nopts = {
      .y = 1,
      .x = 1,
      .rows = 2,
      .cols = 2,
      .userptr = nullptr, .name = "new1", .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto n1 = ncplane_create(n_, &nopts);
    REQUIRE(n1);
    nopts.name = "new2";
    auto n2 = ncplane_create(n1, &nopts);
    REQUIRE(n2);
    nopts.name = "new3";
    auto n3 = ncplane_create(n2, &nopts);
    REQUIRE(n3);
    CHECK(ncplane_reparent(n1, n3));
    CHECK(ncplane_reparent(n2, n3));
    ncplane_destroy(n3);
    ncplane_destroy(n2);
    ncplane_destroy(n1);
  }

  // you ought not be able to output control characters, aside from tab, and
  // newlines (when we're scrolling).
  SUBCASE("DenyControlASCII") {
    signed char c = 0;
    c = -1;
    do{
      ++c;
      if(c && !isprint(c)){
        if(c != '\t'){
          CHECK(0 > ncplane_putchar_yx(n_, 0, 0, c));
        }
      }else{
        CHECK(1 == ncplane_putchar_yx(n_, 0, 0, c));
      }
    }while(c != 127);
  }

  // relative moves
  SUBCASE("RelativePlaneMoves") {
    struct ncplane_options nopts{};
    nopts.x = 2;
    nopts.y = 2;
    nopts.rows = 4;
    nopts.cols = 4;
    auto n = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != n);
    uint64_t channels = NCCHANNELS_INITIALIZER(0, 0xff, 0, 0xff, 0, 0xff);
    int y, x;
    ncplane_yx(n, &y, &x);
    CHECK(y == 2);
    CHECK(x == 2);
    CHECK(1 == ncplane_set_base(n, " ", 0, channels));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_move_rel(n, -1, 0)); // up
    ncplane_yx(n, &y, &x);
    CHECK(y == 1);
    CHECK(x == 2);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_move_rel(n, 2, 0)); // down
    ncplane_yx(n, &y, &x);
    CHECK(y == 3);
    CHECK(x == 2);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_move_rel(n, 0, -1)); // left
    ncplane_yx(n, &y, &x);
    CHECK(y == 3);
    CHECK(x == 1);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_move_rel(n, 0, 2)); // right
    ncplane_yx(n, &y, &x);
    CHECK(y == 3);
    CHECK(x == 3);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(n));
  }

  SUBCASE("ScrollingVirtualCursor") {
    struct ncplane_options nopts{};
    nopts.x = 2;
    nopts.y = 2;
    nopts.rows = 4;
    nopts.cols = 20;
    auto n = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != n);
    CHECK(false == ncplane_set_scrolling(n, true));
    uint64_t channels = NCCHANNELS_INITIALIZER(0, 0xff, 0, 0xff, 0, 0xff);
    unsigned y, x;
    CHECK(1 == ncplane_set_base(n, " ", 0, channels));
    CHECK(0 == notcurses_render(nc_));
    for(unsigned i = 0 ; i < ncplane_dim_y(n) ; ++i){
      ncplane_cursor_yx(n, &y, &x);
      CHECK(i == y);
      CHECK(0 == x);
      CHECK(0 < ncplane_putstr(n, "here's a line\n"));
      CHECK(0 == notcurses_render(nc_));
    }
    for(unsigned i = 0 ; i < ncplane_dim_y(n) ; ++i){
      ncplane_cursor_yx(n, &y, &x);
      CHECK(ncplane_dim_y(n) - 1 == y);
      CHECK(0 == x);
      CHECK(0 < ncplane_putstr(n, "here's a line\n"));
      CHECK(0 == notcurses_render(nc_));
    }
    CHECK(0 == ncplane_destroy(n));
    CHECK(0 == notcurses_render(nc_));
  }

  CHECK(0 == notcurses_stop(nc_));

}
