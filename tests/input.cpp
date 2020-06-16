#include "main.h"

TEST_CASE("NotcursesInput") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }

  REQUIRE(0 == notcurses_mouse_enable(nc_));
  CHECK(0 == notcurses_mouse_disable(nc_));

  CHECK(0 == notcurses_stop(nc_));
}
