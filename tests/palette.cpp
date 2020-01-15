#include <notcurses.h>
#include "main.h"

TEST_CASE("Palette256") {
  // common initialization
  if(getenv("TERM") == nullptr){
    return;
  }
  FILE* outfp_{};
  outfp_ = fopen("/dev/tty", "wb");
  REQUIRE(nullptr != outfp_);
  notcurses_options nopts{};
  nopts.inhibit_alternate_screen = true;
  nopts.suppress_banner = true;
  struct notcurses* nc_ = notcurses_init(&nopts, outfp_);
  REQUIRE(nullptr != nc_);
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(nullptr != n_);

  SUBCASE("AllocAndFree") {
    palette256* p = palette256_new();
    REQUIRE(nullptr != p);
    palette256_free(p);
  }

  // common teardown
  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));
}

