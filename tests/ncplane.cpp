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
  }

  void TearDown() override {
    EXPECT_EQ(0, notcurses_stop(nc_));
  }

  struct notcurses* nc_;
};

TEST_F(NcplaneTest, RetrieveStdPlane) {
  EXPECT_NE(nullptr, notcurses_stdplane(nc_));
}

// Verify that we can move to all four coordinates of the standard plane
TEST_F(NcplaneTest, MoveStdPlaneDimensions) {
  auto n = notcurses_stdplane(nc_);
  ASSERT_NE(nullptr, n);
  int cols, rows;
  notcurses_term_dimensions(nc_, &rows, &cols);
  EXPECT_EQ(0, ncplane_move(n, 0, 0));
  EXPECT_EQ(0, ncplane_move(n, cols - 1, 0));
  EXPECT_EQ(0, ncplane_move(n, cols - 1, rows - 1));
  EXPECT_EQ(0, ncplane_move(n, 0, rows - 1));
}

// Verify that we can move to all four coordinates of the standard plane
TEST_F(NcplaneTest, MoveBeyondPlaneFails) {
  auto n = notcurses_stdplane(nc_);
  ASSERT_NE(nullptr, n);
  int cols, rows;
  notcurses_term_dimensions(nc_, &rows, &cols);
  EXPECT_NE(0, ncplane_move(n, -1, 0));
  EXPECT_NE(0, ncplane_move(n, -1, -1));
  EXPECT_NE(0, ncplane_move(n, 0, -1));
  EXPECT_NE(0, ncplane_move(n, cols - 1, -1));
  EXPECT_NE(0, ncplane_move(n, cols, 0));
  EXPECT_NE(0, ncplane_move(n, cols + 1, 0));
  EXPECT_NE(0, ncplane_move(n, cols, rows));
  EXPECT_NE(0, ncplane_move(n, -1, rows - 1));
  EXPECT_NE(0, ncplane_move(n, 0, rows));
  EXPECT_NE(0, ncplane_move(n, 0, rows + 1));
}
