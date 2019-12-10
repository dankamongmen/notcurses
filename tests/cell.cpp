#include <notcurses.h>
#include "egcpool.h"
#include "main.h"

class CellTest : public :: testing::Test {
 protected:
  void SetUp() override {
    setlocale(LC_ALL, nullptr);
    if(getenv("TERM") == nullptr){
      GTEST_SKIP();
    }
    notcurses_options nopts{};
    nopts.inhibit_alternate_screen = true;
    nopts.outfp = fopen("/dev/tty", "wb");
    nc_ = notcurses_init(&nopts);
    ASSERT_NE(nullptr, nc_);
    n_ = notcurses_stdplane(nc_);
    ASSERT_NE(nullptr, n_);
  }

  void TearDown() override {
    if(nc_){
      EXPECT_EQ(0, notcurses_stop(nc_));
    }
  }

  struct notcurses* nc_{};
  struct ncplane* n_{};
};

TEST_F(CellTest, SetStyles) {
  cell c;
  int dimy, dimx;
  memset(&c, 0, sizeof(c));
  notcurses_term_dim_yx(nc_, &dimy, &dimx);
  cell_styles_set(&c, CELL_STYLE_ITALIC);
  ASSERT_EQ(1, cell_load(n_, &c, "s"));
  EXPECT_EQ(0, ncplane_set_fg_rgb(n_, 255, 255, 255));
  EXPECT_EQ(1, ncplane_putc(n_, &c));
  int x, y;
  ncplane_cursor_yx(n_, &y, &x);
  EXPECT_EQ(1, x);
  EXPECT_EQ(0, y);
  for(y = 0 ; y < dimy ; ++y){
    ncplane_cursor_move_yx(n_, y, 0);
    for(x = 0 ; x < dimx ; ++x){
      EXPECT_EQ(1, ncplane_putc(n_, &c));
    }
  }
  EXPECT_EQ(0, notcurses_render(nc_));
  cell_styles_off(&c, CELL_STYLE_ITALIC);
  for(y = 0 ; y < dimy ; ++y){
    ncplane_cursor_move_yx(n_, y, 0);
    for(x = 0 ; x < dimx ; ++x){
      EXPECT_EQ(1, ncplane_putc(n_, &c));
    }
  }
  ncplane_cursor_yx(n_, &y, &x);
  EXPECT_EQ(dimy, y);
  EXPECT_EQ(0, x); // FIXME shouldn't this be dimx?!?!
  EXPECT_EQ(0, notcurses_render(nc_));
}

/*TEST_F(CellTest, CellLoadTamil) {
  const char zerodeg[] = "\u0bb8\u0bc0\u0bb0\u0bc7\u0bb3\u0b95\u0bbf\u0b95\u0bbf\u0bb0\u0bbf";
  cell c = CELL_TRIVIAL_INITIALIZER;
  size_t ulen = cell_load(n_, &c, zerodeg);
  // First have U+0BB8 TAMIL LETTER SA U+0BC0 TAMIL VOWEL SIGN II
  // // e0 ae b8 e0 af 80
  ASSERT_EQ(6, ulen);
  ulen = cell_load(n_, &c, zerodeg + ulen);
  // U+0BB0 TAMIL LETTER RA U+0BCB TAMIL VOWEL SIGN OO
  // e0 ae b0 e0 af 8b
  ASSERT_EQ(6, ulen);
  // FIXME
}*/

TEST_F(CellTest, CellSetFGAlpha){
  cell c = CELL_TRIVIAL_INITIALIZER;
  EXPECT_GT(0, cell_fg_set_alpha(&c, -1));
  EXPECT_GT(0, cell_fg_set_alpha(&c, 4));
  EXPECT_EQ(0, cell_fg_set_alpha(&c, 0));
  EXPECT_EQ(0, cell_fg_alpha(&c));
  EXPECT_EQ(0, cell_fg_set_alpha(&c, 3));
  EXPECT_EQ(3, cell_fg_alpha(&c));
  EXPECT_TRUE(cell_fg_default_p(&c));
  EXPECT_TRUE(cell_bg_default_p(&c));
}

TEST_F(CellTest, CellSetBGAlpha){
  cell c = CELL_TRIVIAL_INITIALIZER;
  EXPECT_GT(0, cell_bg_set_alpha(&c, -1));
  EXPECT_GT(0, cell_bg_set_alpha(&c, 4));
  EXPECT_EQ(0, cell_bg_set_alpha(&c, 0));
  EXPECT_EQ(0, cell_bg_alpha(&c));
  EXPECT_EQ(0, cell_bg_set_alpha(&c, 3));
  EXPECT_EQ(3, cell_bg_alpha(&c));
  EXPECT_TRUE(cell_fg_default_p(&c));
  EXPECT_TRUE(cell_bg_default_p(&c));
}
