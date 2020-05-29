#include "main.h"

TEST_CASE("NotcursesInput") {
  notcurses_options nopts{};
  nopts.suppress_banner = true;
  struct notcurses* nc_ = notcurses_init(&nopts, nullptr);
  if(!nc_){
    return;
  }

  REQUIRE(0 == notcurses_mouse_enable(nc_));
  CHECK(0 == notcurses_mouse_disable(nc_));

  CHECK(0 == notcurses_stop(nc_));
}
