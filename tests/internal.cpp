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
  for(r = 250 ; r < 256 ; ++r){
    g = b = r;
    EXPECT_EQ(15, rgb_to_ansi256(r, g, b));
  }
}

TEST_F(InternalsTest, RGBtoANSIBlack) {
  unsigned r, g, b;
  for(r = 0 ; r < 10 ; ++r){
    g = b = r;
    EXPECT_EQ(0, rgb_to_ansi256(r, g, b));
  }
}

TEST_F(InternalsTest, RGBtoANSIGrey) {
  unsigned r, g, b;
  for(r = 10 ; r < 244 ; ++r){
    g = b = r;
    EXPECT_EQ(231 + (r * 5) / 49, rgb_to_ansi256(r, g, b));
  }
}

// Pure reds are either 0 (black), or 16 plus 36 * [0..5].
TEST_F(InternalsTest, RGBtoANSIRed) {
  unsigned r, g, b;
  g = b = 0x0;
  for(r = 0 ; r < 256 ; ++r){
    int c256 = rgb_to_ansi256(r, g, b);
    if(r < 8){
      EXPECT_EQ(0, c256);
    }else{
      EXPECT_LT(15, c256);
      EXPECT_EQ(16, c256 % 36);
    }
  }
}

// Pure greens are either 0 (black), or 16 plus 6 * [0..5].
TEST_F(InternalsTest, RGBtoANSIGreen) {
  unsigned r, g, b;
  r = b = 0x0;
  for(g = 0 ; g < 256 ; ++g){
    int c256 = rgb_to_ansi256(r, g, b);
    EXPECT_GT(48, c256);
    if(g < 8){
      EXPECT_EQ(0, c256);
    }else{
      EXPECT_LT(15, c256);
      EXPECT_EQ(4, c256 % 6);
    }
  }
}

// Pure blues are either 0 (black), or one of the first 6 colors [16..22].
TEST_F(InternalsTest, RGBtoANSIBlue) {
  unsigned r, g, b;
  r = g = 0x0;
  for(b = 0 ; b < 256 ; ++b){
    int c256 = rgb_to_ansi256(r, g, b);
    EXPECT_GT(22, c256);
    if(b < 8){
      EXPECT_EQ(0, c256);
    }else{
      EXPECT_LT(15, c256);
    }
  }
}
