#include <string>
#include <cstdlib>
#include <notcurses.h>
#include "main.h"

TEST(Notcurses, BasicLifetime) {
  struct notcurses* nc = notcurses_init();
  ASSERT_NE(nullptr, nc);
  EXPECT_EQ(0, notcurses_stop(nc));
}

TEST(Notcurses, TermDimensions) {
  struct notcurses* nc = notcurses_init();
  int x, y;
  ASSERT_NE(nullptr, nc);
  EXPECT_EQ(0, notcurses_term_dimensions(nc, &y, &x));
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
  EXPECT_EQ(0, notcurses_stop(nc));
}
