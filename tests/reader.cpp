#include "main.h"
#include <cstring>
#include <iostream>

TEST_CASE("Readers") {
  if(getenv("TERM") == nullptr){
    return;
  }
  notcurses_options nopts{};
  nopts.inhibit_alternate_screen = true;
  nopts.suppress_banner = true;
  FILE* outfp_ = fopen("/dev/tty", "wb");
  REQUIRE(outfp_);
  struct notcurses* nc_ = notcurses_init(&nopts, outfp_);
  REQUIRE(nc_);
  int dimx, dimy;
  struct ncplane* n_ = notcurses_stddim_yx(nc_, &dimy, &dimx);
  REQUIRE(n_);
  REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));

  SUBCASE("ReaderLifecycle") {
    auto nr = ncreader_create(nc_, 1, 0, 0, 0);
    REQUIRE(nullptr != nr);
    ncreader_destroy(nr);
    CHECK(0 == notcurses_render(nc_));
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));
}
