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

  SUBCASE("SetIndexZero") {
    palette256* p = palette256_new();
    REQUIRE(nullptr != p);
    palette256_set_rgb(p, 0, 0x80, 0x90, 0xa0);
    unsigned r, g, b;
    palette256_get(p, 0, &r, &g, &b);
    CHECK(r == 0x80);
    CHECK(g == 0x90);
    CHECK(b == 0xa0);
    palette256_free(p);
  }

  SUBCASE("SetIndex255") {
    palette256* p = palette256_new();
    REQUIRE(nullptr != p);
    palette256_set_rgb(p, 255, 0xa0, 0x70, 0x50);
    unsigned r, g, b;
    palette256_get(p, 255, &r, &g, &b);
    CHECK(r == 0xa0);
    CHECK(g == 0x70);
    CHECK(b == 0x50);
    palette256_free(p);
  }

  // common teardown
  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));
}

