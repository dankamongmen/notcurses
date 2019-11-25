#include <string>
#include <cstdlib>
#include <notcurses.h>
#include "main.h"

class NotcursesTest : public :: testing::Test {
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
  }

  void TearDown() override {
    if(nc_){
      EXPECT_EQ(0, notcurses_stop(nc_));
    }
  }

  struct notcurses* nc_;
};

TEST_F(NotcursesTest, BasicLifetime) {
}

TEST_F(NotcursesTest, TermDimensions) {
  int x, y;
  notcurses_term_dimyx(nc_, &y, &x);
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
}

TEST_F(NotcursesTest, ResizeSameSize) {
  int x, y;
  notcurses_term_dimyx(nc_, &y, &x);
  EXPECT_EQ(0, notcurses_resize(nc_));
  int newx, newy;
  notcurses_term_dimyx(nc_, &newy, &newx);
  EXPECT_EQ(newx, x);
  EXPECT_EQ(newy, y);
}

// we should at least have WA_BOLD everywhere, i should think?
TEST_F(NotcursesTest, CursesStyles) {
  unsigned attrs = notcurses_supported_styles(nc_);
  EXPECT_EQ(1, !!(WA_BOLD & attrs));
}
