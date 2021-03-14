#include "main.h"

TEST_CASE("Pixel") {
  auto nc_ = testing_notcurses();
  REQUIRE(nullptr != nc_);
  ncplane* ncp_ = notcurses_stdplane(nc_);
  REQUIRE(ncp_);
  auto n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  if(!notcurses_check_pixel_support(nc_)){
    CHECK(!notcurses_stop(nc_));
    return;
  }

  CHECK(!notcurses_stop(nc_));
}
