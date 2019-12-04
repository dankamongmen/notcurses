#include <cstdlib>
#include <notcurses.h>
#include "main.h"

class NcplaneTest : public :: testing::Test {
 protected:
  void SetUp() override {
    setlocale(LC_ALL, nullptr);
    if(getenv("TERM") == nullptr){
      GTEST_SKIP();
    }
    notcurses_options nopts{};
    nopts.inhibit_alternate_screen = true;
    nopts.pass_through_esc = true;
    nopts.retain_cursor = true;
    nopts.outfp = stdin;
    nc_ = notcurses_init(&nopts);
    ASSERT_NE(nullptr, nc_);
    n_ = notcurses_stdplane(nc_);
    ASSERT_NE(nullptr, n_);
    ASSERT_EQ(0, ncplane_cursor_move_yx(n_, 0, 0));
  }

  void TearDown() override {
    if(nc_){
      EXPECT_EQ(0, notcurses_stop(nc_));
    }
  }

  struct notcurses* nc_{};
  struct ncplane* n_{};
};

// Starting position ought be 0, 0 (the origin)
TEST_F(NcplaneTest, StdPlanePosition) {
  int x, y;
  ncplane_cursor_yx(n_, &y, &x);
  EXPECT_EQ(0, x);
  EXPECT_EQ(0, y);
}

// Dimensions of the standard plane ought be the same as those of the context
TEST_F(NcplaneTest, StdPlaneDimensions) {
  int cols, rows;
  notcurses_term_dim_yx(nc_, &rows, &cols);
  int ncols, nrows;
  ncplane_dim_yx(n_, &nrows, &ncols);
  EXPECT_EQ(rows, nrows);
  EXPECT_EQ(cols, ncols);
}

// Verify that we can move to all four coordinates of the standard plane
TEST_F(NcplaneTest, MoveStdPlaneDimensions) {
  int cols, rows;
  notcurses_term_dim_yx(nc_, &rows, &cols);
  EXPECT_EQ(0, ncplane_cursor_move_yx(n_, 0, 0));
  int x, y;
  ncplane_cursor_yx(n_, &y, &x);
  EXPECT_EQ(y, 0);
  EXPECT_EQ(x, 0);
  EXPECT_EQ(0, ncplane_cursor_move_yx(n_, rows - 1, 0));
  ncplane_cursor_yx(n_, &y, &x);
  EXPECT_EQ(y, rows - 1);
  EXPECT_EQ(x, 0);
  EXPECT_EQ(0, ncplane_cursor_move_yx(n_, rows - 1, cols - 1));
  ncplane_cursor_yx(n_, &y, &x);
  EXPECT_EQ(y, rows - 1);
  EXPECT_EQ(x, cols - 1);
  EXPECT_EQ(0, ncplane_cursor_move_yx(n_, 0, cols - 1));
  ncplane_cursor_yx(n_, &y, &x);
  EXPECT_EQ(y, 0);
  EXPECT_EQ(x, cols - 1);
}

// Verify that we can move to all four coordinates of the standard plane
TEST_F(NcplaneTest, MoveBeyondPlaneFails) {
  int cols, rows;
  notcurses_term_dim_yx(nc_, &rows, &cols);
  EXPECT_NE(0, ncplane_cursor_move_yx(n_, -1, 0));
  EXPECT_NE(0, ncplane_cursor_move_yx(n_, -1, -1));
  EXPECT_NE(0, ncplane_cursor_move_yx(n_, 0, -1));
  EXPECT_NE(0, ncplane_cursor_move_yx(n_, rows - 1, -1));
  EXPECT_NE(0, ncplane_cursor_move_yx(n_, rows, 0));
  EXPECT_NE(0, ncplane_cursor_move_yx(n_, rows + 1, 0));
  EXPECT_NE(0, ncplane_cursor_move_yx(n_, rows, cols));
  EXPECT_NE(0, ncplane_cursor_move_yx(n_, -1, cols - 1));
  EXPECT_NE(0, ncplane_cursor_move_yx(n_, 0, cols));
  EXPECT_NE(0, ncplane_cursor_move_yx(n_, 0, cols + 1));
}

TEST_F(NcplaneTest, SetPlaneRGB) {
  EXPECT_EQ(0, ncplane_fg_rgb8(n_, 0, 0, 0));
  EXPECT_EQ(0, ncplane_fg_rgb8(n_, 255, 255, 255));
  EXPECT_EQ(0, notcurses_render(nc_));
}

TEST_F(NcplaneTest, RejectBadRGB) {
  EXPECT_NE(0, ncplane_fg_rgb8(n_, -1, 0, 0));
  EXPECT_NE(0, ncplane_fg_rgb8(n_, 0, -1, 0));
  EXPECT_NE(0, ncplane_fg_rgb8(n_, 0, 0, -1));
  EXPECT_NE(0, ncplane_fg_rgb8(n_, -1, -1, -1));
  EXPECT_NE(0, ncplane_fg_rgb8(n_, 256, 255, 255));
  EXPECT_NE(0, ncplane_fg_rgb8(n_, 255, 256, 255));
  EXPECT_NE(0, ncplane_fg_rgb8(n_, 255, 255, 256));
  EXPECT_NE(0, ncplane_fg_rgb8(n_, 256, 256, 256));
}

// Verify we can emit a wide character, and it advances the cursor
TEST_F(NcplaneTest, EmitWchar) {
  const char cchar[] = "✔";
  cell c{};
  EXPECT_EQ(strlen(cchar), cell_load(n_, &c, cchar));
  EXPECT_EQ(strlen(cchar), ncplane_putc(n_, &c));
  int x, y;
  ncplane_cursor_yx(n_, &y, &x);
  EXPECT_EQ(0, y);
  EXPECT_EQ(1, x);
  EXPECT_EQ(0, notcurses_render(nc_));
}

// Verify we can emit a wide string, and it advances the cursor
TEST_F(NcplaneTest, EmitStr) {
  const char s[] = "Σιβυλλα τι θελεις; respondebat illa: αποθανειν θελω.";
  int wrote = ncplane_putstr(n_, s);
  EXPECT_EQ(strlen(s), wrote);
  int x, y;
  ncplane_cursor_yx(n_, &y, &x);
  EXPECT_EQ(0, y);
  EXPECT_NE(1, x); // FIXME tighten in on this
  EXPECT_EQ(0, notcurses_render(nc_));
}

TEST_F(NcplaneTest, HorizontalLines) {
  int x, y;
  ncplane_dim_yx(n_, &y, &x);
  ASSERT_LT(0, y);
  ASSERT_LT(0, x);
  cell c{};
  cell_load(n_, &c, "-");
  for(int yidx = 0 ; yidx < y ; ++yidx){
    EXPECT_EQ(0, ncplane_cursor_move_yx(n_, yidx, 1));
    EXPECT_EQ(x - 2, ncplane_hline(n_, &c, x - 2));
    int posx, posy;
    ncplane_cursor_yx(n_, &posy, &posx);
    EXPECT_EQ(yidx, posy);
    EXPECT_EQ(x - 1, posx);
  }
  cell_release(n_, &c);
  EXPECT_EQ(0, notcurses_render(nc_));
}

TEST_F(NcplaneTest, VerticalLines) {
  int x, y;
  ncplane_dim_yx(n_, &y, &x);
  ASSERT_LT(0, y);
  ASSERT_LT(0, x);
  cell c{};
  cell_load(n_, &c, "|");
  for(int xidx = 1 ; xidx < x - 1 ; ++xidx){
    EXPECT_EQ(0, ncplane_cursor_move_yx(n_, 1, xidx));
    EXPECT_EQ(y - 2, ncplane_vline(n_, &c, y - 2));
    int posx, posy;
    ncplane_cursor_yx(n_, &posy, &posx);
    EXPECT_EQ(y - 2, posy);
    EXPECT_EQ(xidx, posx - 1);
  }
  cell_release(n_, &c);
  EXPECT_EQ(0, notcurses_render(nc_));
}

// reject attempts to draw boxes beyond the boundaries of the ncplane
TEST_F(NcplaneTest, BadlyPlacedBoxen) {
  int x, y;
  ncplane_dim_yx(n_, &y, &x);
  ASSERT_LT(2, y);
  ASSERT_LT(2, x);
  cell ul{}, ll{}, lr{}, ur{}, hl{}, vl{};
  ASSERT_EQ(0, cells_rounded_box(n_, 0, 0, &ul, &ur, &ll, &lr, &hl, &vl));
  EXPECT_GT(0, ncplane_box(n_, &ul, &ur, &ll, &lr, &hl, &vl, y + 1, x + 1, 0));
  EXPECT_EQ(0, ncplane_cursor_move_yx(n_, 1, 0));
  EXPECT_GT(0, ncplane_box(n_, &ul, &ur, &ll, &lr, &hl, &vl, y, x, 0));
  EXPECT_EQ(0, ncplane_cursor_move_yx(n_, 0, 1));
  EXPECT_GT(0, ncplane_box(n_, &ul, &ur, &ll, &lr, &hl, &vl, y, x, 0));
  EXPECT_EQ(0, ncplane_cursor_move_yx(n_, y - 1, x - 1));
  EXPECT_GT(0, ncplane_box(n_, &ul, &ur, &ll, &lr, &hl, &vl, 2, 2, 0));
  EXPECT_EQ(0, ncplane_cursor_move_yx(n_, y - 2, x - 1));
  EXPECT_GT(0, ncplane_box(n_, &ul, &ur, &ll, &lr, &hl, &vl, 2, 2, 0));
  EXPECT_EQ(0, ncplane_cursor_move_yx(n_, y - 1, x - 2));
  EXPECT_GT(0, ncplane_box(n_, &ul, &ur, &ll, &lr, &hl, &vl, 2, 2, 0));
  EXPECT_EQ(0, notcurses_render(nc_));
}

TEST_F(NcplaneTest, BoxPermutations) {
  int dimx, dimy;
  ncplane_dim_yx(n_, &dimy, &dimx);
  ASSERT_LT(12, dimy);
  ASSERT_LT(24, dimx);
  // we'll try all 16 boxmasks in 3x3 configurations in a 4x4 map
  unsigned boxmask = 0;
  for(auto y0 = 0 ; y0 < 4 ; ++y0){
    for(auto x0 = 0 ; x0 < 4 ; ++x0){
      EXPECT_EQ(0, ncplane_cursor_move_yx(n_, y0 * 3, x0 * 3));
      EXPECT_EQ(0, ncplane_rounded_box_sized(n_, 0, 0, 3, 3, boxmask));
      ++boxmask;
    }
  }
  boxmask = 0;
  for(auto y0 = 0 ; y0 < 4 ; ++y0){
    for(auto x0 = 0 ; x0 < 4 ; ++x0){
      EXPECT_EQ(0, ncplane_cursor_move_yx(n_, y0 * 3, x0 * 3 + 12));
      EXPECT_EQ(0, ncplane_double_box_sized(n_, 0, 0, 3, 3, boxmask));
      ++boxmask;
    }
  }
  EXPECT_EQ(0, notcurses_render(nc_));
}

TEST_F(NcplaneTest, PerimeterRoundedBox) {
  int x, y;
  ncplane_dim_yx(n_, &y, &x);
  ASSERT_LT(2, y);
  ASSERT_LT(2, x);
  ASSERT_EQ(0, ncplane_cursor_move_yx(n_, 0, 0));
  EXPECT_EQ(0, ncplane_rounded_box(n_, 0, 0, y - 1, x - 1, 0));
  EXPECT_EQ(0, notcurses_render(nc_));
}

TEST_F(NcplaneTest, PerimeterRoundedBoxSized) {
  int x, y;
  ncplane_dim_yx(n_, &y, &x);
  ASSERT_LT(2, y);
  ASSERT_LT(2, x);
  ASSERT_EQ(0, ncplane_cursor_move_yx(n_, 0, 0));
  EXPECT_EQ(0, ncplane_rounded_box_sized(n_, 0, 0, y, x, 0));
  EXPECT_EQ(0, notcurses_render(nc_));
}

TEST_F(NcplaneTest, PerimeterDoubleBox) {
  int x, y;
  ncplane_dim_yx(n_, &y, &x);
  ASSERT_LT(2, y);
  ASSERT_LT(2, x);
  ASSERT_EQ(0, ncplane_cursor_move_yx(n_, 0, 0));
  EXPECT_EQ(0, ncplane_double_box(n_, 0, 0, y - 1, x - 1, 0));
  EXPECT_EQ(0, notcurses_render(nc_));
}

TEST_F(NcplaneTest, PerimeterDoubleBoxSized) {
  int x, y;
  ncplane_dim_yx(n_, &y, &x);
  ASSERT_LT(2, y);
  ASSERT_LT(2, x);
  ASSERT_EQ(0, ncplane_cursor_move_yx(n_, 0, 0));
  EXPECT_EQ(0, ncplane_double_box_sized(n_, 0, 0, y, x, 0));
  EXPECT_EQ(0, notcurses_render(nc_));
}

TEST_F(NcplaneTest, EraseScreen) {
  ncplane_erase(n_);
  EXPECT_EQ(0, notcurses_render(nc_));
}

// we're gonna run both a composed latin a with grave, and then a latin a with
// a combining nonspacing grave
TEST_F(NcplaneTest, CellLoadCombining) {
  const char* w1 = "à"; // U+00E0, U+0000         (c3 a0)
  const char* w2 = "à"; // U+0061, U+0300, U+0000 (61 cc 80)
  const char* w3 = "a"; // U+0061, U+0000         (61)
  cell cell1 = CELL_TRIVIAL_INITIALIZER;
  cell cell2 = CELL_TRIVIAL_INITIALIZER;
  cell cell3 = CELL_TRIVIAL_INITIALIZER;
  auto u1 = cell_load(n_, &cell1, w1);
  auto u2 = cell_load(n_, &cell2, w2);
  auto u3 = cell_load(n_, &cell3, w3);
  ASSERT_EQ(2, u1);
  ASSERT_EQ(3, u2);
  ASSERT_EQ(1, u3);
  cell_release(n_, &cell1);
  cell_release(n_, &cell2);
  cell_release(n_, &cell3);
}

TEST_F(NcplaneTest, CellDuplicateCombining) {
  const char* w1 = "à"; // U+00E0, U+0000         (c3 a0)
  const char* w2 = "à"; // U+0061, U+0300, U+0000 (61 cc 80)
  const char* w3 = "a"; // U+0061, U+0000         (61)
  cell cell1 = CELL_TRIVIAL_INITIALIZER;
  cell cell2 = CELL_TRIVIAL_INITIALIZER;
  cell cell3 = CELL_TRIVIAL_INITIALIZER;
  auto u1 = cell_load(n_, &cell1, w1);
  auto u2 = cell_load(n_, &cell2, w2);
  auto u3 = cell_load(n_, &cell3, w3);
  ASSERT_EQ(2, u1);
  ASSERT_EQ(3, u2);
  ASSERT_EQ(1, u3);
  cell cell4 = CELL_TRIVIAL_INITIALIZER;
  cell cell5 = CELL_TRIVIAL_INITIALIZER;
  cell cell6 = CELL_TRIVIAL_INITIALIZER;
  EXPECT_EQ(2, cell_duplicate(n_, &cell4, &cell1));
  EXPECT_EQ(3, cell_duplicate(n_, &cell5, &cell2));
  EXPECT_EQ(1, cell_duplicate(n_, &cell6, &cell3));
  cell_release(n_, &cell1);
  cell_release(n_, &cell2);
  cell_release(n_, &cell3);
  cell_release(n_, &cell4);
  cell_release(n_, &cell5);
  cell_release(n_, &cell6);
}

TEST_F(NcplaneTest, CellMultiColumn) {
  const char* w1 = "👨";
  const char* w2 = "N";
  cell c1 = CELL_TRIVIAL_INITIALIZER;
  cell c2 = CELL_TRIVIAL_INITIALIZER;
  auto u1 = cell_load(n_, &c1, w1);
  auto u2 = cell_load(n_, &c2, w2);
  ASSERT_EQ(strlen(w1), u1);
  ASSERT_EQ(strlen(w2), u2);
  EXPECT_TRUE(cell_double_wide_p(&c1));
  EXPECT_FALSE(cell_double_wide_p(&c2));
  cell_release(n_, &c1);
  cell_release(n_, &c2);
}

// verifies that the initial userptr is what we provided, that it is a nullptr
// for the standard plane, and that we can change it.
TEST_F(NcplaneTest, UserPtr) {
  EXPECT_EQ(nullptr, ncplane_userptr(n_));
  int x, y;
  void* sentinel = &x;
  notcurses_term_dim_yx(nc_, &y, &x);
  struct ncplane* ncp = notcurses_newplane(nc_, y, x, 0, 0, sentinel);
  ASSERT_NE(ncp, nullptr);
  EXPECT_EQ(&x, ncplane_userptr(ncp));
  EXPECT_EQ(sentinel, ncplane_set_userptr(ncp, nullptr));
  EXPECT_EQ(nullptr, ncplane_userptr(ncp));
  sentinel = &y;
  EXPECT_EQ(nullptr, ncplane_set_userptr(ncp, sentinel));
  EXPECT_EQ(&y, ncplane_userptr(ncp));
  EXPECT_EQ(0, ncplane_destroy(ncp));
}

// create a new plane, the same size as the terminal, and verify that it
// occupies the same dimensions as the standard plane.
TEST_F(NcplaneTest, NewPlaneSameSize) {
  int x, y;
  notcurses_term_dim_yx(nc_, &y, &x);
  struct ncplane* ncp = notcurses_newplane(nc_, y, x, 0, 0, nullptr);
  ASSERT_NE(nullptr, ncp);
  int px, py;
  ncplane_dim_yx(ncp, &py, &px);
  EXPECT_EQ(y, py);
  EXPECT_EQ(x, px);
  int sx, sy;
  ncplane_dim_yx(n_, &sy, &sx);
  EXPECT_EQ(sy, py);
  EXPECT_EQ(sx, px);
  EXPECT_EQ(0, ncplane_destroy(ncp));
}

TEST_F(NcplaneTest, ShrinkPlane) {
  int maxx, maxy;
  int x = 0, y = 0;
  notcurses_term_dim_yx(nc_, &maxy, &maxx);
  struct ncplane* newp = notcurses_newplane(nc_, maxy, maxx, y, x, nullptr);
  ASSERT_NE(nullptr, newp);
  while(y > 4 && x > 4){
    maxx -= 2;
    maxy -= 2;
    ++x;
    ++y;
    ASSERT_EQ(0, ncplane_resize(newp, 1, 1, maxy, maxx, 1, 1, maxy, maxx));
    // FIXME check dims, pos
  }
  while(y > 4){
    maxy -= 2;
    ++y;
    ASSERT_EQ(0, ncplane_resize(newp, 1, 0, maxy, maxx, 1, 0, maxy, maxx));
    // FIXME check dims, pos
  }
  while(x > 4){
    maxx -= 2;
    ++x;
    ASSERT_EQ(0, ncplane_resize(newp, 0, 1, maxy, maxx, 0, 1, maxy, maxx));
    // FIXME check dims, pos
  }
  ASSERT_EQ(0, ncplane_resize(newp, 0, 0, 0, 0, 0, 0, 2, 2));
  // FIXME check dims, pos
  ASSERT_EQ(0, ncplane_destroy(newp));
}

TEST_F(NcplaneTest, GrowPlane) {
  int maxx = 2, maxy = 2;
  int x = 0, y = 0;
  int dimy, dimx;
  notcurses_term_dim_yx(nc_, &dimy, &dimx);
  x = dimx / 2 - 1;
  y = dimy / 2 - 1;
  struct ncplane* newp = notcurses_newplane(nc_, maxy, maxx, y, x, nullptr);
  ASSERT_NE(nullptr, newp);
  while(dimx - maxx > 4 && dimy - maxy > 4){
    maxx += 2;
    maxy += 2;
    --x;
    --y;
    // ASSERT_EQ(0, ncplane_resize(newp, 1, 1, maxy, maxx, 1, 1, maxy, maxx));
    // FIXME check dims, pos
  }
  while(y < dimy){
    ++maxy;
    if(y){
      ++y;
    }
    // ASSERT_EQ(0, ncplane_resize(newp, 1, 0, maxy, maxx, 1, 0, maxy, maxx));
    // FIXME check dims, pos
  }
  while(x < dimx){
    ++maxx;
    if(x){
      ++x;
    }
    // ASSERT_EQ(0, ncplane_resize(newp, 0, 1, maxy, maxx, 0, 1, maxy, maxx));
    // FIXME check dims, pos
  }
  ASSERT_EQ(0, ncplane_resize(newp, 0, 0, 0, 0, 0, 0, dimy, dimx));
  // FIXME check dims, pos
  ASSERT_EQ(0, ncplane_destroy(newp));
}

// we ought be able to see what we're about to render, or have just rendered, or
// in any case whatever's in the virtual framebuffer for a plane
TEST_F(NcplaneTest, PlaneAtCursorSimples){
  const char STR1[] = "Jackdaws love my big sphinx of quartz";
  const char STR2[] = "Cwm fjord bank glyphs vext quiz";
  const char STR3[] = "Pack my box with five dozen liquor jugs";
  ncplane_styles_set(n_, 0);
  ASSERT_EQ(0, ncplane_cursor_move_yx(n_, 0, 0));
  ASSERT_LT(0, ncplane_putstr(n_, STR1));
  int dimy, dimx;
  ncplane_dim_yx(n_, &dimy, &dimx);
  ASSERT_EQ(0, ncplane_cursor_move_yx(n_, 1, dimx - strlen(STR2)));
  ASSERT_LT(0, ncplane_putstr(n_, STR2));
  int y, x;
  ncplane_cursor_yx(n_, &y, &x);
  ASSERT_EQ(2, y);
  ASSERT_EQ(0, x);
  ASSERT_LT(0, ncplane_putstr(n_, STR3));
  cell testcell = CELL_TRIVIAL_INITIALIZER;
  ASSERT_EQ(0, ncplane_at_cursor(n_, &testcell)); // want nothing at the cursor
  EXPECT_EQ(0, testcell.gcluster);
  EXPECT_EQ(0, testcell.attrword);
  EXPECT_EQ(0, testcell.channels);
  ASSERT_EQ(0, ncplane_cursor_move_yx(n_, 0, 0));
  ASSERT_LT(0, ncplane_at_cursor(n_, &testcell)); // want first char of STR1
  EXPECT_EQ(STR1[0], testcell.gcluster);
  EXPECT_EQ(0, testcell.attrword);
  EXPECT_EQ(0, testcell.channels);
  ASSERT_EQ(0, ncplane_cursor_move_yx(n_, 1, dimx - 1));
  ASSERT_LT(0, ncplane_at_cursor(n_, &testcell)); // want last char of STR2
  EXPECT_EQ(STR2[strlen(STR2) - 1], testcell.gcluster);
  EXPECT_EQ(0, testcell.attrword);
  EXPECT_EQ(0, testcell.channels);
  // FIXME maybe check all cells?
  EXPECT_EQ(0, notcurses_render(nc_));
}

// ensure we read back what's expected for latinesque complex characters
TEST_F(NcplaneTest, PlaneAtCursorComplex){
  const char STR1[] = "Σιβυλλα τι θελεις; respondebat illa:";
  const char STR2[] = "αποθανειν θελω";
  const char STR3[] = "Война и мир"; // just thrown in to complicate things
  ncplane_styles_set(n_, 0);
  ASSERT_EQ(0, ncplane_cursor_move_yx(n_, 0, 0));
  ASSERT_LT(0, ncplane_putstr(n_, STR1));
  int dimy, dimx;
  ncplane_dim_yx(n_, &dimy, &dimx);
  ASSERT_EQ(0, ncplane_cursor_move_yx(n_, 1, dimx - mbstowcs(NULL, STR2, 0)));
  ASSERT_LT(0, ncplane_putstr(n_, STR2));
  int y, x;
  ncplane_cursor_yx(n_, &y, &x);
  ASSERT_EQ(2, y);
  ASSERT_EQ(0, x);
  ASSERT_LT(0, ncplane_putstr(n_, STR3));
  cell testcell = CELL_TRIVIAL_INITIALIZER;
  ncplane_at_cursor(n_, &testcell); // should be nothing at the cursor
  EXPECT_EQ(0, testcell.gcluster);
  EXPECT_EQ(0, testcell.attrword);
  EXPECT_EQ(0, testcell.channels);
  ASSERT_EQ(0, ncplane_cursor_move_yx(n_, 0, 0));
  ASSERT_LT(0, ncplane_at_cursor(n_, &testcell)); // want first char of STR1
  EXPECT_STREQ("Σ", cell_extended_gcluster(n_, &testcell));
  EXPECT_EQ(0, testcell.attrword);
  EXPECT_EQ(0, testcell.channels);
  ASSERT_EQ(0, ncplane_cursor_move_yx(n_, 1, dimx - mbstowcs(NULL, STR2, 0)));
  ASSERT_LT(0, ncplane_at_cursor(n_, &testcell)); // want first char of STR2
  EXPECT_STREQ("α", cell_extended_gcluster(n_, &testcell));
  EXPECT_EQ(0, testcell.attrword);
  EXPECT_EQ(0, testcell.channels);
  // FIXME maybe check all cells?
  EXPECT_EQ(0, notcurses_render(nc_));
}

// half-width, double-width, huge-yet-narrow, all that crap
TEST_F(NcplaneTest, PlaneAtCursorInsane){
  const char EGC0[] = "\uffe0"; // fullwidth cent sign ￠
  const char EGC1[] = "\u00c5"; // neutral A with ring above Å
  const char EGC2[] = "\u20a9"; // half-width won ₩
  const char EGC3[] = "\u212b"; // ambiguous angstrom Å
  const char EGC4[] = "\ufdfd"; // neutral yet huge bismillah ﷽
  std::array<cell, 5> tcells;
  for(auto i = 0u ; i < tcells.size() ; ++i){
    cell_init(&tcells[i]);
  }
  ASSERT_LT(1, cell_load(n_, &tcells[0], EGC0));
  ASSERT_LT(1, cell_load(n_, &tcells[1], EGC1));
  ASSERT_LT(1, cell_load(n_, &tcells[2], EGC2));
  ASSERT_LT(1, cell_load(n_, &tcells[3], EGC3));
  ASSERT_LT(1, cell_load(n_, &tcells[4], EGC4));
  for(auto i = 0u ; i < tcells.size() ; ++i){
    ASSERT_LT(1, ncplane_putc(n_, &tcells[i]));
  }
  EXPECT_EQ(0, notcurses_render(nc_));
  int x = 0;
  for(auto i = 0u ; i < tcells.size() ; ++i){
    EXPECT_EQ(0, ncplane_cursor_move_yx(n_, 0, x));
    cell testcell = CELL_TRIVIAL_INITIALIZER;
    ASSERT_LT(0, ncplane_at_cursor(n_, &testcell));
    EXPECT_STREQ(cell_extended_gcluster(n_, &tcells[i]),
                 cell_extended_gcluster(n_, &testcell));
    EXPECT_EQ(0, testcell.attrword);
    wchar_t w;
    ASSERT_LT(0, mbtowc(&w, cell_extended_gcluster(n_, &tcells[i]), MB_CUR_MAX));
    if(wcwidth(w) == 2){
      EXPECT_EQ(CELL_WIDEASIAN_MASK, testcell.channels);
      ++x;
    }else{
      EXPECT_EQ(0, testcell.channels);
    }
    ++x;
  }
}

// test that we read back correct attrs/colors despite changing defaults
TEST_F(NcplaneTest, PlaneAtCursorAttrs){
  const char STR1[] = "this has been a world destroyer production";
  const char STR2[] = "not to mention dank";
  const char STR3[] = "da chronic lives";
  ncplane_styles_set(n_, CELL_STYLE_BOLD);
  ASSERT_LT(0, ncplane_putstr(n_, STR1));
  int y, x;
  ncplane_cursor_yx(n_, &y, &x);
  EXPECT_EQ(0, ncplane_cursor_move_yx(n_, y + 1, x - strlen(STR2)));
  ncplane_styles_on(n_, CELL_STYLE_ITALIC);
  ASSERT_LT(0, ncplane_putstr(n_, STR2));
  EXPECT_EQ(0, ncplane_cursor_move_yx(n_, y + 2, x - strlen(STR3)));
  ncplane_styles_off(n_, CELL_STYLE_BOLD);
  ASSERT_LT(0, ncplane_putstr(n_, STR3));
  ncplane_styles_off(n_, CELL_STYLE_ITALIC);
  EXPECT_EQ(0, notcurses_render(nc_));
  int newx;
  ncplane_cursor_yx(n_, &y, &newx);
  EXPECT_EQ(newx, x);
  cell testcell = CELL_TRIVIAL_INITIALIZER;
  EXPECT_EQ(0, ncplane_cursor_move_yx(n_, y - 2, x - 1));
  ASSERT_EQ(1, ncplane_at_cursor(n_, &testcell));
  EXPECT_EQ(testcell.gcluster, STR1[strlen(STR1) - 1]);
  EXPECT_EQ(0, ncplane_cursor_move_yx(n_, y - 1, x - 1));
  ASSERT_EQ(1, ncplane_at_cursor(n_, &testcell));
  EXPECT_EQ(testcell.gcluster, STR2[strlen(STR2) - 1]);
  EXPECT_EQ(0, ncplane_cursor_move_yx(n_, y, x - 1));
  ASSERT_EQ(1, ncplane_at_cursor(n_, &testcell));
  EXPECT_EQ(testcell.gcluster, STR3[strlen(STR3) - 1]);
}

TEST_F(NcplaneTest, BoxSideColors) {
  int dimx, dimy;
  ncplane_dim_yx(n_, &dimy, &dimx);
  ASSERT_LT(12, dimy);
  ASSERT_LT(24, dimx);
  cell ul{}, ll{}, lr{}, ur{}, hl{}, vl{};
  ASSERT_EQ(0, cells_rounded_box(n_, 0, 0, &ul, &ur, &ll, &lr, &hl, &vl));
  // we'll try all 16 boxmasks in 4x4 configurations in a 4x4 map
  EXPECT_EQ(0, notcurses_fg_prep(&ul.channels, 255, 0, 0));
  EXPECT_EQ(0, notcurses_fg_prep(&ur.channels, 0, 255, 0));
  EXPECT_EQ(0, notcurses_fg_prep(&ll.channels, 0, 0, 255));
  EXPECT_EQ(0, notcurses_fg_prep(&lr.channels, 0, 0, 0));
  EXPECT_EQ(0, notcurses_fg_prep(&hl.channels, 255, 0, 255));
  EXPECT_EQ(0, notcurses_fg_prep(&vl.channels, 255, 255, 255));
  unsigned boxmask = 0;
  for(auto y0 = 0 ; y0 < 4 ; ++y0){
    for(auto x0 = 0 ; x0 < 4 ; ++x0){
      EXPECT_EQ(0, ncplane_cursor_move_yx(n_, y0 * 4, x0 * 4));
      EXPECT_EQ(0, ncplane_box_sized(n_, &ul, &ur, &ll, &lr, &hl, &vl,
                                     4, 4, boxmask));
      ++boxmask;
    }
  }
  boxmask = 0;
  for(auto y0 = 0 ; y0 < 4 ; ++y0){
    for(auto x0 = 0 ; x0 < 4 ; ++x0){
      EXPECT_EQ(0, ncplane_cursor_move_yx(n_, y0 * 4, x0 * 4 + 12));
      EXPECT_EQ(0, ncplane_box_sized(n_, &ul, &ur, &ll, &lr, &hl, &vl,
                                     4, 4, boxmask));
      ++boxmask;
    }
  }
  EXPECT_EQ(0, notcurses_render(nc_));
}

TEST_F(NcplaneTest, BoxGradients) {
  int dimx, dimy;
  ncplane_dim_yx(n_, &dimy, &dimx);
  ASSERT_LT(16, dimy);
  ASSERT_LT(32, dimx);
  cell ul{}, ll{}, lr{}, ur{}, hl{}, vl{};
  ASSERT_EQ(0, cells_double_box(n_, 0, 0, &ul, &ur, &ll, &lr, &hl, &vl));
  EXPECT_EQ(0, notcurses_fg_prep(&ul.channels, 255, 0, 0));
  EXPECT_EQ(0, notcurses_fg_prep(&ur.channels, 0, 255, 0));
  EXPECT_EQ(0, notcurses_fg_prep(&ll.channels, 0, 0, 255));
  EXPECT_EQ(0, notcurses_fg_prep(&lr.channels, 255, 255, 255));
  // we'll try all 16 boxmasks in 4x4 configurations in a 4x4 map
  unsigned gradmask = 0;
  for(auto y0 = 0 ; y0 < 4 ; ++y0){
    for(auto x0 = 0 ; x0 < 4 ; ++x0){
      EXPECT_EQ(0, ncplane_cursor_move_yx(n_, y0 * 4, x0 * 4));
      EXPECT_EQ(0, ncplane_box_sized(n_, &ul, &ur, &ll, &lr, &hl, &vl,
                                     4, 4, gradmask));
      ++gradmask;
    }
  }
  gradmask = 0;
  for(auto y0 = 0 ; y0 < 4 ; ++y0){
    for(auto x0 = 0 ; x0 < 4 ; ++x0){
      EXPECT_EQ(0, ncplane_cursor_move_yx(n_, y0 * 4, x0 * 4 + 16));
      EXPECT_EQ(0, ncplane_box_sized(n_, &ul, &ur, &ll, &lr, &hl, &vl,
                                     4, 4, gradmask));
      ++gradmask;
    }
  }
  EXPECT_EQ(0, notcurses_render(nc_));
}
