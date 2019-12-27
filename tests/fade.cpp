#include "main.h"
#include <cstdlib>
#include <iostream>

class FadeTest : public :: testing::Test {
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
    if(!notcurses_canfade(nc_)){
      GTEST_SKIP();
    }
    n_ = notcurses_stdplane(nc_);
    ASSERT_NE(nullptr, n_);
    ASSERT_EQ(0, ncplane_cursor_move_yx(n_, 0, 0));
    int dimy, dimx;
    ncplane_dim_yx(n_, &dimy, &dimx);
    cell c = CELL_TRIVIAL_INITIALIZER;
    c.gcluster = '*';
    cell_set_fg_rgb(&c, 0xff, 0xff, 0xff);
    unsigned rgb = 0xffffffu;
    for(int y = 0 ; y < dimy ; ++y){
      for(int x = 0 ; x < dimx ; ++x){
        rgb -= 32;
        if(rgb < 32){
          rgb = 0xffffffu;
        }
        cell_set_fg_rgb(&c, (rgb >> 16u) & 0xff, (rgb >> 8u) & 0xff, rgb & 0xff);
        cell_set_bg_rgb(&c, rgb & 0xff, (rgb >> 16u) & 0xff, (rgb >> 8u) & 0xff);
        EXPECT_LT(0, ncplane_putc(n_, &c));
      }
    }
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

TEST_F(FadeTest, FadeOut) {
  EXPECT_EQ(0, notcurses_render(nc_));
  struct timespec ts;
  ts.tv_sec = 1;
  ts.tv_nsec = 0;
  ASSERT_EQ(0, ncplane_fadeout(n_, &ts, nullptr));
}

TEST_F(FadeTest, FadeIn) {
  struct timespec ts;
  ts.tv_sec = 1;
  ts.tv_nsec = 0;
  ASSERT_EQ(0, ncplane_fadein(n_, &ts, nullptr));
}
