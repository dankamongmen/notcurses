#include "main.h"
#include <cstring>
#include <iostream>

TEST_CASE("Plot") {
  if(!enforce_utf8()){
    return;
  }
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
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);
  REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));

  // setting detectrange with non-zero domain limits is invalid
  SUBCASE("DetectRangeBadY"){
    ncplot_options popts{};
    popts.detectrange = true;
    popts.miny = -1;
    ncplot* p = ncplot_create(n_, &popts);
    CHECK(nullptr == p);
    popts.miny = 0;
    popts.maxy = 1;
    p = ncplot_create(n_, &popts);
    CHECK(nullptr == p);
  }

  // maxy < miny is invalid
  SUBCASE("RejectMaxyLessMiny"){
    ncplot_options popts{};
    popts.miny = 2;
    popts.maxy = 1;
    ncplot* p = ncplot_create(n_, &popts);
    CHECK(nullptr == p);
  }

  SUBCASE("SimplePlot"){
    ncplot_options popts{};
    ncplot* p = ncplot_create(n_, &popts);
    REQUIRE(p);
    CHECK(n_ == ncplot_plane(p));
    ncplot_destroy(p);
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));
}
