#include "main.h"
#include <cstring>
#include <iostream>

TEST_CASE("Readers") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  int dimx, dimy;
  struct ncplane* n_ = notcurses_stddim_yx(nc_, &dimy, &dimx);
  REQUIRE(n_);
  REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));

  SUBCASE("ReaderRender") {
    ncreader_options opts{};
    struct ncplane_options nopts = {
      .y = 0,
      .horiz = { .x = 0, },
      .rows = dimy / 2,
      .cols = dimx / 2,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
    };
    auto ncp = ncplane_create(notcurses_stdplane(nc_), &nopts);
    uint64_t echannels = CHANNELS_RGB_INITIALIZER(0xff, 0x44, 0xff, 0, 0, 0);
    ncplane_set_base(ncp, enforce_utf8() ? strdup("▒") : strdup("x"), 0, echannels);
    auto nr = ncreader_create(ncp, &opts);
    REQUIRE(nullptr != nr);
    CHECK(0 == notcurses_render(nc_));
    char* contents = nullptr;
    ncreader_destroy(nr, &contents);
    REQUIRE(contents);
    CHECK(0 == notcurses_render(nc_));
  }

  CHECK(0 == notcurses_stop(nc_));
}
