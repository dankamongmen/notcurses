#include "main.h"

// These tests address cases where box characters on two overlapping planes
// interact in non-trivial ways. A simple example is a U2580 UPPER HALF BLOCK
// (▀) with a white foreground and transparent background, above a U2584 LOWER
// HALF BLOCK (▄) with a white foreground and transparent background. One might
// expect the result to be an entirely white cell, but by typical Notcurses
// rendering rules, we would instead get a white upper half and transparent
// lower half:
//
// - after first cell, glyph is locked U2584, fg is locked white, bg transparent
// - second cell can't override glyph nor fg, and background remains transparent
//
// we will instead special-case block-drawing characters.
// see https://github.com/dankamongmen/notcurses/issues/1068
TEST_CASE("Stacking") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  if(!notcurses_canutf8(nc_)){
    CHECK(0 == notcurses_stop(nc_));
    return;
  }
  int dimy, dimx;
  struct ncplane* n_ = notcurses_stddim_yx(nc_, &dimy, &dimx);
  REQUIRE(nullptr != n_);


  SUBCASE("UpperAtopLowerWhite") {
    struct ncplane_options opts = {
      0, 0, 1, 1, nullptr, "top", nullptr, 0,
    };
    auto top = ncplane_create(n_, &opts);
    REQUIRE(nullptr != top);
    CHECK(0 == ncplane_set_fg_rgb(top, 0xffffff));
    CHECK(0 == ncplane_set_fg_rgb(n_, 0xffffff));
    CHECK(1 == ncplane_putwc(top, L'\u2580')); // upper half block
    CHECK(1 == ncplane_putwc(n_, L'\u2584')); // lower half block
    CHECK(0 == notcurses_render(nc_));
    uint64_t channels;
    auto egc = notcurses_at_yx(nc_, 0, 0, nullptr, &channels);
    REQUIRE(nullptr != egc);
    // ought yield space with white background
    CHECK(0 == strcmp(" ", egc));
    CHECK(0xffffff == channels_fg_rgb(channels));
    CHECK(0xffffff == channels_bg_rgb(channels));
    ncplane_destroy(top);
  }

  // common teardown
  CHECK(0 == notcurses_stop(nc_));
}
