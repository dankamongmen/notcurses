#include "main.h"

TEST_CASE("Input") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  unsigned dimy, dimx;
  struct ncplane* n_ = notcurses_stddim_yx(nc_, &dimy, &dimx);
  REQUIRE(n_);


  CHECK(0 == notcurses_render(nc_));
  CHECK(0 == notcurses_stop(nc_));

}
