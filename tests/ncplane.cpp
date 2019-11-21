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
  ncplane_posyx(n_, &y, &x);
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
  EXPECT_EQ(0, ncplane_movyx(n_, 0, 0));
  int x, y;
  ncplane_posyx(n_, &y, &x);
  EXPECT_EQ(y, 0);
  EXPECT_EQ(x, 0);
  EXPECT_EQ(0, ncplane_movyx(n_, rows - 1, 0));
  ncplane_posyx(n_, &y, &x);
  EXPECT_EQ(y, rows - 1);
  EXPECT_EQ(x, 0);
  EXPECT_EQ(0, ncplane_movyx(n_, rows - 1, cols - 1));
  ncplane_posyx(n_, &y, &x);
  EXPECT_EQ(y, rows - 1);
  EXPECT_EQ(x, cols - 1);
  EXPECT_EQ(0, ncplane_movyx(n_, 0, cols - 1));
  ncplane_posyx(n_, &y, &x);
  EXPECT_EQ(y, 0);
  EXPECT_EQ(x, cols - 1);
}

// Verify that we can move to all four coordinates of the standard plane
TEST_F(NcplaneTest, MoveBeyondPlaneFails) {
  int cols, rows;
  notcurses_term_dimyx(nc_, &rows, &cols);
  EXPECT_NE(0, ncplane_movyx(n_, -1, 0));
  EXPECT_NE(0, ncplane_movyx(n_, -1, -1));
  EXPECT_NE(0, ncplane_movyx(n_, 0, -1));
  EXPECT_NE(0, ncplane_movyx(n_, rows - 1, -1));
  EXPECT_NE(0, ncplane_movyx(n_, rows, 0));
  EXPECT_NE(0, ncplane_movyx(n_, rows + 1, 0));
  EXPECT_NE(0, ncplane_movyx(n_, rows, cols));
  EXPECT_NE(0, ncplane_movyx(n_, -1, cols - 1));
  EXPECT_NE(0, ncplane_movyx(n_, 0, cols));
  EXPECT_NE(0, ncplane_movyx(n_, 0, cols + 1));
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
  wchar_t cchar[] = L"✔";
  EXPECT_EQ(0, ncplane_putwc(n_, cchar));
  int x, y;
  ncplane_posyx(n_, &y, &x);
  EXPECT_EQ(y, 0);
  EXPECT_EQ(x, 1);
}
