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

TEST_F(ZAxisTest, NewPlaneOnTop) {
  struct ncplane* np = notcurses_newplane(nc_, 2, 2, 0, 0, nullptr);
  ASSERT_NE(nullptr, np);
  struct ncplane* top = notcurses_top(nc_);
  EXPECT_EQ(np, top);
  EXPECT_EQ(n_, ncplane_below(top));
  EXPECT_EQ(nullptr, ncplane_below(n_));
}
