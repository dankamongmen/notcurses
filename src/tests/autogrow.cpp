#include "main.h"

TEST_CASE("Autogrow") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  // verify that the standard plane has scrolling disabled initially, and that
  // we cannot enable it at runtime.
  SUBCASE("AutogrowDisabledStdplane") {
    CHECK(!ncplane_set_autogrow(n_, true));  // disabled at start?
    CHECK(!ncplane_set_autogrow(n_, false)); // attempt to enable failed?
  }

  CHECK(0 == notcurses_stop(nc_));

}
