#include <array>
#include <cstdlib>
#include "main.h"

TEST_CASE("Erase") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  // clear all columns to the left of cursor, inclusive
  SUBCASE("EraseColumnsLeft") {
  }

  // clear all columns to the right of cursor, inclusive
  SUBCASE("EraseColumnsRight") {
  }

  // clear all rows above cursor, inclusive
  SUBCASE("EraseRowsLeft") {
  }

  // clear all rows below cursor, inclusive
  SUBCASE("EraseRowsBelow") {
  }

  CHECK(0 == notcurses_stop(nc_));

}
