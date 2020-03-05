#include "main.h"

TEST_CASE("Rotate") {
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
  int dimy, dimx;
  struct ncplane* n_ = notcurses_stddim_yx(nc_, &dimy, &dimx);
  REQUIRE(n_);

  uint64_t ul, ur, ll, lr;
  ul = ur = ll = lr = 0;
  channels_set_fg(&ul, 0x40f040);
  channels_set_bg(&ul, 0x40f040);
  channels_set_fg(&ll, 0xf040f0);
  channels_set_bg(&ll, 0xf040f0);
  channels_set_fg(&ur, 0x40f040);
  channels_set_bg(&ur, 0x40f040);
  channels_set_fg(&lr, 0xf040f0);
  channels_set_bg(&lr, 0xf040f0);

  // should be a square, and should remain a square through rotations
  struct ncplane* testn_ = ncplane_new(nc_, 4, 8, dimy / 2, dimx / 2, nullptr);
  REQUIRE(0 == ncplane_gradient_sized(testn_, "V", 0, ul, ur, ll, lr, 4, 8));
  CHECK(0 == notcurses_render(nc_));

  SUBCASE("RotateClockwise") {
    CHECK(0 == ncplane_rotate_cw(testn_));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_rotate_cw(testn_));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_rotate_cw(testn_));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_rotate_cw(testn_));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("RotateCounterClockwise") {
    CHECK(0 == ncplane_rotate_ccw(testn_));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_rotate_ccw(testn_));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_rotate_ccw(testn_));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_rotate_ccw(testn_));
    CHECK(0 == notcurses_render(nc_));
  }

  CHECK(0 == ncplane_destroy(testn_));
  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));

}
