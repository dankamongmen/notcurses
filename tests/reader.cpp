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

  SUBCASE("ReaderBadOptions") {
    ncreader_options opts{};
    auto nr = ncreader_create(nc_, 0, 0, &opts);
    CHECK(!nr);
    opts.physrows = 1;
    nr = ncreader_create(nc_, 0, 0, &opts);
    CHECK(!nr);
    opts.physcols = 1;
    opts.physrows = 0;
    nr = ncreader_create(nc_, 0, 0, &opts);
    CHECK(!nr);
  }

  SUBCASE("ReaderRender") {
    ncreader_options opts{};
    opts.physrows = 1;
    opts.physcols = dimx / 2;
    opts.egc = strdup("▒");
    auto nr = ncreader_create(nc_, 0, 0, &opts);
    REQUIRE(nullptr != nr);
    CHECK(0 == notcurses_render(nc_));
CHECK(0 < ncplane_set_base(n_, opts.egc, opts.eattrword, opts.echannels));
sleep(5);
    ncreader_destroy(nr);
    CHECK(0 == notcurses_render(nc_));
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));
}
