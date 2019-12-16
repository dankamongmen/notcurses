#include <string>
#include <cstdlib>
#include <clocale>
#include "main.h"

class InternalsTest : public :: testing::Test {
 protected:
  void SetUp() override {
    setlocale(LC_ALL, "");
  }

};

TEST_F(InternalsTest, RGBtoANSIWhite) {
  unsigned r, g, b;
  r = g = b = 0xff;
  EXPECT_EQ(255, rgb_to_ansi256(r, g, b));
}

TEST_F(InternalsTest, RGBtoANSIBlack) {
  unsigned r, g, b;
  r = g = b = 0x0;
  EXPECT_EQ(232, rgb_to_ansi256(r, g, b));
}
