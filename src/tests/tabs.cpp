#include <cstdlib>
#include "main.h"

TEST_CASE("TaBs") { // refreshing and delicious
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  SUBCASE("PutcTaB") {
    unsigned y, x;
    CHECK(1 == ncplane_putchar(n_, '\n'));
    ncplane_cursor_yx(n_, &y, &x);
    CHECK(y == 8);
    CHECK(x == 0);
  }

  CHECK(0 == notcurses_stop(nc_));

}
