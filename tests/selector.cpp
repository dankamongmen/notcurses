#include "main.h"
#include <iostream>

TEST_CASE("SelectorTest") {
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

  SUBCASE("LifecycleEmptySelector") {
    struct selector_options opts{};
    opts.ylen = 1;
    opts.xlen = 1;
    struct ncselector* ncs = ncselector_create(notcurses_stdplane(nc_), 0, 0, &opts);
    REQUIRE(nullptr != ncs);
    CHECK(0 == notcurses_render(nc_));
    ncselector_destroy(ncs, nullptr);
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));
}
