#include <array>
#include <cstdlib>
#include "main.h"

TEST_CASE("Erase") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  unsigned dimy, dimx;
  struct ncplane* n_ = notcurses_stddim_yx(nc_, &dimy, &dimx);
  REQUIRE(n_);

  // fill the standard plane with 'x's
  nccell nc = NCCELL_CHAR_INITIALIZER('x');
  CHECK(0 < ncplane_polyfill_yx(n_, 0, 0, &nc));
  nccell_release(n_, &nc);
  CHECK(0 == notcurses_render(nc_));
  CHECK(0 == ncplane_cursor_move_yx(n_, dimy / 2, dimx / 2));

  // clear all columns to the left of cursor, inclusive
  SUBCASE("EraseColumnsLeft") {
    CHECK(0 == ncplane_erase_region(n_, -1, -1, 0, -INT_MAX));
    for(unsigned y = 0 ; y < dimy ; ++y){
      for(unsigned x = 0 ; x < dimx ; ++x){
        char* c = ncplane_at_yx(n_, y, x, nullptr, nullptr);
        REQUIRE(nullptr != c);
        if(x <= dimx / 2){
          CHECK(0 == strcmp(c, ""));
        }else{
          CHECK(0 == strcmp(c, "x"));
        }
        free(c);
      }
    }
  }

  // clear all columns to the right of cursor, inclusive
  SUBCASE("EraseColumnsRight") {
    CHECK(0 == ncplane_erase_region(n_, -1, -1, 0, INT_MAX));
    for(unsigned y = 0 ; y < dimy ; ++y){
      for(unsigned x = 0 ; x < dimx ; ++x){
        char* c = ncplane_at_yx(n_, y, x, nullptr, nullptr);
        REQUIRE(nullptr != c);
        if(x >= dimx / 2){
          CHECK(0 == strcmp(c, ""));
        }else{
          CHECK(0 == strcmp(c, "x"));
        }
        free(c);
      }
    }
  }

  // clear all rows above cursor, inclusive
  SUBCASE("EraseRowsAbove") {
    CHECK(0 == ncplane_erase_region(n_, -1, -1, -INT_MAX, 0));
    for(unsigned y = 0 ; y < dimy ; ++y){
      for(unsigned x = 0 ; x < dimx ; ++x){
        char* c = ncplane_at_yx(n_, y, x, nullptr, nullptr);
        REQUIRE(nullptr != c);
        if(y <= dimy / 2){
          CHECK(0 == strcmp(c, ""));
        }else{
          CHECK(0 == strcmp(c, "x"));
        }
        free(c);
      }
    }
  }

  // clear all rows below cursor, inclusive
  SUBCASE("EraseRowsBelow") {
    CHECK(0 == ncplane_erase_region(n_, -1, -1, INT_MAX, 0));
    for(unsigned y = 0 ; y < dimy ; ++y){
      for(unsigned x = 0 ; x < dimx ; ++x){
        char* c = ncplane_at_yx(n_, y, x, nullptr, nullptr);
        REQUIRE(nullptr != c);
        if(y >= dimy / 2){
          CHECK(0 == strcmp(c, ""));
        }else{
          CHECK(0 == strcmp(c, "x"));
        }
        free(c);
      }
    }
  }

  // current cursor to the end of the line
  SUBCASE("EraseToEOL") {
    CHECK(0 == ncplane_erase_region(n_, -1, -1, 1, INT_MAX));
    for(unsigned y = 0 ; y < dimy ; ++y){
      for(unsigned x = 0 ; x < dimx ; ++x){
        char* c = ncplane_at_yx(n_, y, x, nullptr, nullptr);
        REQUIRE(nullptr != c);
        if(y == dimy / 2 && x >= dimx / 2){
          CHECK(0 == strcmp(c, ""));
        }else{
          CHECK(0 == strcmp(c, "x"));
        }
        free(c);
      }
    }
  }

  // current cursor to the start of the line
  SUBCASE("EraseToSOL") {
    CHECK(0 == ncplane_erase_region(n_, -1, -1, 1, -INT_MAX));
    for(unsigned y = 0 ; y < dimy ; ++y){
      for(unsigned x = 0 ; x < dimx ; ++x){
        char* c = ncplane_at_yx(n_, y, x, nullptr, nullptr);
        REQUIRE(nullptr != c);
        if(y == dimy / 2 && x <= dimx / 2){
          CHECK(0 == strcmp(c, ""));
        }else{
          CHECK(0 == strcmp(c, "x"));
        }
        free(c);
      }
    }
  }

  // current cursor to the end of the line using -1 len
  SUBCASE("EraseToEOLNeg") {
    CHECK(0 == ncplane_erase_region(n_, -1, -1, -1, INT_MAX));
    for(unsigned y = 0 ; y < dimy ; ++y){
      for(unsigned x = 0 ; x < dimx ; ++x){
        char* c = ncplane_at_yx(n_, y, x, nullptr, nullptr);
        REQUIRE(nullptr != c);
        if(y == dimy / 2 && x >= dimx / 2){
          CHECK(0 == strcmp(c, ""));
        }else{
          CHECK(0 == strcmp(c, "x"));
        }
        free(c);
      }
    }
  }

  // current cursor to the start of the line using -1 len
  SUBCASE("EraseToSOLNeg") {
    CHECK(0 == ncplane_erase_region(n_, -1, -1, -1, -INT_MAX));
    for(unsigned y = 0 ; y < dimy ; ++y){
      for(unsigned x = 0 ; x < dimx ; ++x){
        char* c = ncplane_at_yx(n_, y, x, nullptr, nullptr);
        REQUIRE(nullptr != c);
        if(y == dimy / 2 && x <= dimx / 2){
          CHECK(0 == strcmp(c, ""));
        }else{
          CHECK(0 == strcmp(c, "x"));
        }
        free(c);
      }
    }
  }

  // current cursor only
  SUBCASE("EraseCurrentCursor") {
    CHECK(0 == ncplane_erase_region(n_, -1, -1, 1, 1));
    for(unsigned y = 0 ; y < dimy ; ++y){
      for(unsigned x = 0 ; x < dimx ; ++x){
        char* c = ncplane_at_yx(n_, y, x, nullptr, nullptr);
        REQUIRE(nullptr != c);
        if(y == dimy / 2 && x == dimx / 2){
          CHECK(0 == strcmp(c, ""));
        }else{
          CHECK(0 == strcmp(c, "x"));
        }
        free(c);
      }
    }
  }

  CHECK(0 == notcurses_render(nc_));
  CHECK(0 == notcurses_stop(nc_));

}
