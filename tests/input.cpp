#include "main.h"

TEST_CASE("NotcursesInput") {
  if(getenv("TERM") == nullptr){
    return;
  }
  notcurses_options nopts{};
  nopts.inhibit_alternate_screen = true;
  nopts.suppress_bannner = true;
  FILE* outfp_ = fopen("/dev/tty", "wb");
  REQUIRE(outfp_);
  struct notcurses* nc_ = notcurses_init(&nopts, outfp_);
  REQUIRE(nc_);

  REQUIRE(0 == notcurses_mouse_enable(nc_));
  CHECK(0 == notcurses_mouse_disable(nc_));

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));
}
