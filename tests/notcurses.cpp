#include <string>
#include <cstdlib>
#include <notcurses.h>
#include "main.h"

class NotcursesTest : public :: testing::Test {
 protected:
  void SetUp() override {
    if(getenv("TERM") == nullptr){
      GTEST_SKIP();
    }
    nc_ = notcurses_init(nullptr);
    ASSERT_NE(nullptr, nc_);
  }

  struct notcurses* nc_;
};

TEST_F(NotcursesTest, BasicLifetime) {
  EXPECT_EQ(0, notcurses_stop(nc_));
}

TEST_F(NotcursesTest, TermDimensions) {
  ASSERT_NE(nullptr, nc_);
  int x, y;
  EXPECT_EQ(0, notcurses_term_dimensions(nc_, &y, &x));
  auto stry = getenv("LINES");
  if(stry){
    auto envy = std::stoi(stry, nullptr);
    EXPECT_EQ(envy, y);
  }
  auto strx = getenv("COLUMNS");
  if(stry){
    auto envx = std::stoi(strx, nullptr);
    EXPECT_EQ(envx, x);
  }
  EXPECT_EQ(0, notcurses_stop(nc_));
}
