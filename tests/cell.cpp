#include <notcurses.h>
#include "egcpool.h"
#include "main.h"

class CellTest : public :: testing::Test {
 protected:
  void SetUp() override {
    setlocale(LC_ALL, "");
    if(getenv("TERM") == nullptr){
      GTEST_SKIP();
    }
    notcurses_options nopts{};
    nopts.inhibit_alternate_screen = true;
    nopts.suppress_bannner = true;
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

TEST_F(CellTest, LoadSimple) {
  cell c = CELL_TRIVIAL_INITIALIZER;
  ASSERT_EQ(1, cell_load(n_, &c, " "));
  EXPECT_TRUE(cell_simple_p(&c));
  cell_release(n_, &c);
}

TEST_F(CellTest, SetItalic) {
  cell c = CELL_TRIVIAL_INITIALIZER;
  int dimy, dimx;
  notcurses_term_dim_yx(nc_, &dimy, &dimx);
  cell_styles_set(&c, CELL_STYLE_ITALIC);
  ASSERT_EQ(1, cell_load(n_, &c, "i"));
  cell_set_fg_rgb(&c, 255, 255, 255);
  ncplane_set_default(n_, &c);
  cell_release(n_, &c);
  EXPECT_EQ(0, notcurses_render(nc_));
  cell_styles_off(&c, CELL_STYLE_ITALIC);
}

TEST_F(CellTest, SetBold) {
  cell c = CELL_TRIVIAL_INITIALIZER;
  int dimy, dimx;
  notcurses_term_dim_yx(nc_, &dimy, &dimx);
  cell_styles_set(&c, CELL_STYLE_BOLD);
  ASSERT_EQ(1, cell_load(n_, &c, "b"));
  cell_set_fg_rgb(&c, 255, 255, 255);
  ncplane_set_default(n_, &c);
  cell_release(n_, &c);
  EXPECT_EQ(0, notcurses_render(nc_));
  cell_styles_off(&c, CELL_STYLE_BOLD);
}

TEST_F(CellTest, SetUnderline) {
  cell c = CELL_TRIVIAL_INITIALIZER;
  int dimy, dimx;
  notcurses_term_dim_yx(nc_, &dimy, &dimx);
  cell_styles_set(&c, CELL_STYLE_UNDERLINE);
  ASSERT_EQ(1, cell_load(n_, &c, "u"));
  cell_set_fg_rgb(&c, 255, 255, 255);
  ncplane_set_default(n_, &c);
  cell_release(n_, &c);
  EXPECT_EQ(0, notcurses_render(nc_));
  cell_styles_off(&c, CELL_STYLE_UNDERLINE);
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
  EXPECT_GT(0, cell_set_fg_alpha(&c, -1));
  EXPECT_GT(0, cell_set_fg_alpha(&c, 4));
  EXPECT_EQ(0, cell_set_fg_alpha(&c, 0));
  EXPECT_EQ(0, cell_get_fg_alpha(&c));
  EXPECT_EQ(0, cell_set_fg_alpha(&c, 3));
  EXPECT_EQ(3, cell_get_fg_alpha(&c));
  EXPECT_TRUE(cell_fg_default_p(&c));
  EXPECT_TRUE(cell_bg_default_p(&c));
}

TEST_F(CellTest, CellSetBGAlpha){
  cell c = CELL_TRIVIAL_INITIALIZER;
  EXPECT_GT(0, cell_set_bg_alpha(&c, -1));
  EXPECT_GT(0, cell_set_bg_alpha(&c, 4));
  EXPECT_EQ(0, cell_set_bg_alpha(&c, 0));
  EXPECT_EQ(0, cell_get_bg_alpha(&c));
  EXPECT_EQ(0, cell_set_bg_alpha(&c, 3));
  EXPECT_EQ(3, cell_get_bg_alpha(&c));
  EXPECT_TRUE(cell_fg_default_p(&c));
  EXPECT_TRUE(cell_bg_default_p(&c));
}
