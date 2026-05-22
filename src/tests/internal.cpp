#include <string>
#include <cstdlib>
#include <clocale>
#include <cstring>
#include "main.h"

TEST_CASE("Internals") {

  SUBCASE("RGBtoANSIWhiteGreyBlack") {
    unsigned r;
    for(r = 0 ; r < 256 ; ++r){
      int g = rgb_quantize_256(r, r, r);
      if(r < 8){
        CHECK(0 == g);
      }else if(r > 238){
        CHECK(15 == g);
      }else{
        CHECK(g == (r - 8) / 10 + 232);
      }
    }
  }

  // Pure reds are either 0 (black), or 16 plus 36 * [0..5].
  SUBCASE("RGBtoANSIRed") {
    unsigned r, g, b;
    g = b = 0x0;
    for(r = 0 ; r < 256 ; ++r){
      int c256 = rgb_quantize_256(r, g, b);
      if(r < 8){
        CHECK(0 == c256);
      }else{
        CHECK(15 < c256);
        CHECK(16 == c256 % 36);
      }
    }
  }

  // Pure greens are either 0 (black), or 16 plus 6 * [0..5].
  SUBCASE("RGBtoANSIGreen") {
    unsigned r, g, b;
    r = b = 0x0;
    for(g = 0 ; g < 256 ; ++g){
      int c256 = rgb_quantize_256(r, g, b);
      CHECK(48 > c256);
      if(g < 8){
        CHECK(0 == c256);
      }else{
        CHECK(15 < c256);
        CHECK(4 == c256 % 6);
      }
    }
  }

  // Pure blues are either 0 (black), or one of the first 6 colors [16..22].
  SUBCASE("RGBtoANSIBlue") {
    unsigned r, g, b;
    r = g = 0x0;
    for(b = 0 ; b < 256 ; ++b){
      int c256 = rgb_quantize_256(r, g, b);
      CHECK(22 > c256);
      if(b < 8){
        CHECK(0 == c256);
      }else{
        CHECK(15 < c256);
      }
    }
  }

  SUBCASE("UCS32toUTF8RejectsInvalidCodepoints") {
    const uint32_t invalid[] = { 0x110000 };
    unsigned char utf8[5] = { 0 };

    CHECK(-1 == notcurses_ucs32_to_utf8(invalid, 1, utf8, 4));
  }

  SUBCASE("UCS32toUTF8AllowsExactFitBuffers") {
    const uint32_t unicode[] = { 0x1f600 };
    const unsigned char expected[] = { 0xf0, 0x9f, 0x98, 0x80 };
    unsigned char utf8[4] = { 0 };

    CHECK(4 == notcurses_ucs32_to_utf8(unicode, 1, utf8, sizeof(utf8)));
    CHECK(0 == std::memcmp(utf8, expected, sizeof(expected)));
  }

  SUBCASE("UCS32toUTF8HandlesMultipleCodepoints") {
    const uint32_t unicode[] = { 'A', 0x1f600 };
    const unsigned char expected[] = { 'A', 0xf0, 0x9f, 0x98, 0x80 };
    unsigned char utf8[8] = { 0 };

    CHECK(5 == notcurses_ucs32_to_utf8(unicode, 2, utf8, sizeof(utf8)));
    CHECK(0 == std::memcmp(utf8, expected, sizeof(expected)));
    CHECK(0 == utf8[5]);
  }

}
