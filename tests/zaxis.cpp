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

// verify that moving one above another, with no other changes, is reflected at
// render time (requires explicit damage maintenance from move functionality).
TEST_F(ZAxisTest, ZAxisDamage) {
  cell cat = CELL_TRIVIAL_INITIALIZER;
  cell c = CELL_SIMPLE_INITIALIZER('x');
  ASSERT_EQ(0, cell_set_fg_rgb(&c, 0xff, 0, 0));
  ASSERT_EQ(1, ncplane_putc(n_, &c));
  EXPECT_EQ(0, notcurses_render(nc_));
  ASSERT_EQ(0, ncplane_cursor_move_yx(n_, 0, 0));
  ASSERT_EQ(1, ncplane_at_cursor(n_, &cat));
  ASSERT_TRUE(cell_simple_p(&cat));
  ASSERT_EQ('x', cat.gcluster);
  struct ncplane* n2 = notcurses_newplane(nc_, 2, 2, 0, 0, nullptr);
  ASSERT_EQ(1, cell_load(n2, &c, "y"));
  ASSERT_EQ(0, cell_set_fg_rgb(&c, 0, 0xff, 0));
  ASSERT_EQ(1, ncplane_putc(n2, &c));
  EXPECT_EQ(0, notcurses_render(nc_));
  ASSERT_EQ(0, ncplane_cursor_move_yx(n2, 0, 0));
  ASSERT_EQ(1, ncplane_at_cursor(n2, &cat));
  ASSERT_EQ('y', cat.gcluster);
  struct ncplane* n3 = notcurses_newplane(nc_, 2, 2, 0, 0, nullptr);
  ASSERT_EQ(1, cell_load(n3, &c, "z"));
  ASSERT_EQ(0, cell_set_fg_rgb(&c, 0, 0, 0xff));
  ASSERT_EQ(1, ncplane_putc(n3, &c));
  EXPECT_EQ(0, notcurses_render(nc_));
  ASSERT_EQ(0, ncplane_cursor_move_yx(n3, 0, 0));
  ASSERT_EQ(1, ncplane_at_cursor(n3, &cat));
  ASSERT_EQ('z', cat.gcluster);
  // FIXME testing damage requires notcurses keeping a copy of the screen....
  // FIXME move y atop z
  // FIXME inspect
  // FIXME move z atop y
  // FIXME inspect
}
