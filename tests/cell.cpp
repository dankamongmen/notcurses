#include <notcurses.h>
#include "main.h"

class CellTest : public :: testing::Test {
 protected:
  void SetUp() override {
    setlocale(LC_ALL, nullptr);
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
      EXPECT_EQ(0, notcurses_stop(nc_));
    }
  }

  struct notcurses* nc_{};
  struct ncplane* n_{};
};

TEST_F(CellTest, SetStyles) {
  cell c;
  memset(&c, 0, sizeof(c));
  cell_set_style(&c, WA_ITALIC);
  ASSERT_EQ(1, cell_load(n_, &c, "s"));
  EXPECT_EQ(0, ncplane_fg_rgb8(n_, 255, 255, 255));
  EXPECT_EQ(1, ncplane_putc(n_, &c));
  int x, y;
  ncplane_cursor_yx(n_, &y, &x);
  EXPECT_EQ(1, x);
  EXPECT_EQ(0, y);
  EXPECT_EQ(0, notcurses_render(nc_));
}
