#include <notcurses.h>
#include "main.h"

class NcplaneTest : public :: testing::Test {
 protected:
  void SetUp() override {
    if(getenv("TERM") == nullptr){
      GTEST_SKIP();
    }
    notcurses_options nopts{};
    nopts.outfd = STDIN_FILENO;
    nc_ = notcurses_init(&nopts);
    ASSERT_NE(nullptr, nc_);
    n_ = notcurses_stdplane(nc_);
    ASSERT_NE(nullptr, n_);
  }

  void TearDown() override {
    if(nc_){
      EXPECT_EQ(0, ncplane_fg_rgb8(n_, 255, 255, 255));
      EXPECT_EQ(0, notcurses_render(nc_));
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
  notcurses_term_dimyx(nc_, &rows, &cols);
  int ncols, nrows;
  ncplane_dimyx(n_, &nrows, &ncols);
  EXPECT_EQ(rows, nrows);
  EXPECT_EQ(cols, ncols);
}

// Verify that we can move to all four coordinates of the standard plane
TEST_F(NcplaneTest, MoveStdPlaneDimensions) {
  int cols, rows;
  notcurses_term_dimyx(nc_, &rows, &cols);
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
  notcurses_term_dimyx(nc_, &rows, &cols);
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
  cell_load(n_, &c, cchar);
  EXPECT_EQ(strlen(cchar), ncplane_putc(n_, &c));
  int x, y;
  ncplane_cursor_yx(n_, &y, &x);
  EXPECT_EQ(0, y);
  EXPECT_EQ(1, x);
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
}

TEST_F(NcplaneTest, HorizontalLines) {
  int x, y;
  ncplane_dimyx(n_, &y, &x);
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
}

TEST_F(NcplaneTest, VerticalLines) {
  int x, y;
  ncplane_dimyx(n_, &y, &x);
  ASSERT_LT(0, y);
  ASSERT_LT(0, x);
  cell c{};
  cell_load(n_, &c, "-");
  for(int xidx = 0 ; xidx < x - 1 ; ++xidx){
    EXPECT_EQ(0, ncplane_cursor_move_yx(n_, 1, xidx));
    EXPECT_EQ(y - 2, ncplane_vline(n_, &c, y - 2));
    int posx, posy;
    ncplane_cursor_yx(n_, &posy, &posx);
    EXPECT_EQ(y - 2, posy);
    EXPECT_EQ(xidx, posx - 1);
  }
  cell_release(n_, &c);
}

TEST_F(NcplaneTest, ConcentricBoxen) {
  int x, y;
  ncplane_dimyx(n_, &y, &x);
  ASSERT_LT(0, y);
  ASSERT_LT(0, x);
  cell ul{}, ll{}, lr{}, ur{}, hl{}, vl{};
  cell_load(n_, &ul, "╭");
  cell_load(n_, &ur, "╮");
  cell_load(n_, &ll, "╰");
  cell_load(n_, &lr, "╯");
  cell_load(n_, &vl, "│");
  cell_load(n_, &hl, "─");
  EXPECT_EQ(0, ncplane_box(n_, &ul, &ur, &ll, &lr, &hl, &vl, y, x));
  cell_release(n_, &vl);
  cell_release(n_, &hl);
  cell_release(n_, &ul);
  cell_release(n_, &ll);
  cell_release(n_, &ur);
  cell_release(n_, &lr);
}

TEST_F(NcplaneTest, EraseScreen) {
  ncplane_erase(n_);
}
