#include "main.h"
#include <cstring>
#include <iostream>

TEST_CASE("ProgressBar") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);
  REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));

  // FIXME add tests
  
  CHECK(0 == notcurses_stop(nc_));
}
