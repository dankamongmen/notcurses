#include "main.h"
#include <cstdlib>
#include <iostream>

class ZAxisTest : public :: testing::Test {
 protected:
  void SetUp() override {
    setlocale(LC_ALL, "");
    if(getenv("TERM") == nullptr){
      GTEST_SKIP();
    }
    notcurses_options nopts{};
    nopts.inhibit_alternate_screen = true;
    outfp_ = fopen("/dev/tty", "wb");
    ASSERT_NE(nullptr, outfp_);
    nc_ = notcurses_init(&nopts, outfp_);
    ASSERT_NE(nullptr, nc_);
    n_ = notcurses_stdplane(nc_);
    ASSERT_NE(nullptr, n_);
  }

  void TearDown() override {
    if(nc_){
      EXPECT_EQ(0, notcurses_stop(nc_));
    }
    if(outfp_){
      fclose(outfp_);
    }
  }

  struct notcurses* nc_{};
  struct ncplane* n_{};
  FILE* outfp_{};
};

TEST_F(ZAxisTest, StdPlaneOnly) {
  struct ncplane* top = notcurses_top(nc_);
  EXPECT_EQ(n_, top);
  EXPECT_EQ(nullptr, ncplane_below(top));
}

// if you want to move the plane which is already top+bottom to either, go ahead
TEST_F(ZAxisTest, StdPlaneOnanism) {
  EXPECT_EQ(0, ncplane_move_top(n_));
  struct ncplane* top = notcurses_top(nc_);
  EXPECT_EQ(n_, top);
  EXPECT_EQ(nullptr, ncplane_below(top));
  EXPECT_EQ(0, ncplane_move_bottom(n_));
  EXPECT_EQ(nullptr, ncplane_below(n_));
}

// you can't place a plane above or below itself, stdplane or otherwise
TEST_F(ZAxisTest, NoMoveSelf) {
  struct ncplane* np = notcurses_newplane(nc_, 2, 2, 0, 0, nullptr);
  ASSERT_NE(nullptr, np);
  EXPECT_NE(0, ncplane_move_below(n_, n_));
  EXPECT_NE(0, ncplane_move_above(n_, n_));
  EXPECT_NE(0, ncplane_move_below(np, np));
  EXPECT_NE(0, ncplane_move_above(np, np));
}

// new planes ought be on the top
TEST_F(ZAxisTest, NewPlaneOnTop) {
  struct ncplane* np = notcurses_newplane(nc_, 2, 2, 0, 0, nullptr);
  ASSERT_NE(nullptr, np);
  struct ncplane* top = notcurses_top(nc_);
  EXPECT_EQ(np, top);
  EXPECT_EQ(n_, ncplane_below(top));
  EXPECT_EQ(nullptr, ncplane_below(n_));
}

// "move" top plane to top. everything ought remain the same.
TEST_F(ZAxisTest, TopToTop) {
  struct ncplane* np = notcurses_newplane(nc_, 2, 2, 0, 0, nullptr);
  ASSERT_NE(nullptr, np);
  struct ncplane* top = notcurses_top(nc_);
  EXPECT_EQ(np, top);
  EXPECT_EQ(n_, ncplane_below(top));
  EXPECT_EQ(nullptr, ncplane_below(n_));
  EXPECT_EQ(0, ncplane_move_top(np));
  // verify it
  top = notcurses_top(nc_);
  EXPECT_EQ(np, top);
  EXPECT_EQ(n_, ncplane_below(top));
  EXPECT_EQ(nullptr, ncplane_below(n_));
}

// move top plane to bottom, and verify enumeration
TEST_F(ZAxisTest, TopToBottom) {
  struct ncplane* np = notcurses_newplane(nc_, 2, 2, 0, 0, nullptr);
  ASSERT_NE(nullptr, np);
  struct ncplane* top = notcurses_top(nc_);
  EXPECT_EQ(np, top);
  EXPECT_EQ(n_, ncplane_below(top));
  EXPECT_EQ(nullptr, ncplane_below(n_));
  EXPECT_EQ(0, ncplane_move_bottom(np));
  top = notcurses_top(nc_);
  EXPECT_EQ(n_, top);
  EXPECT_EQ(np, ncplane_below(top));
  EXPECT_EQ(nullptr, ncplane_below(np));
}
