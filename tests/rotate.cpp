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

  // should be a square, and should remain a square through rotations
  struct ncplane* testn_ = ncplane_new(nc_, 4, 8, dimy / 2, dimx / 2, nullptr);

  SUBCASE("RotateClockwise") {
    CHECK(0 == ncplane_rotate_cw(testn_));
    CHECK(0 == ncplane_rotate_cw(testn_));
    CHECK(0 == ncplane_rotate_cw(testn_));
    CHECK(0 == ncplane_rotate_cw(testn_));
  }

  SUBCASE("RotateCounterClockwise") {
    CHECK(0 == ncplane_rotate_ccw(testn_));
    CHECK(0 == ncplane_rotate_ccw(testn_));
    CHECK(0 == ncplane_rotate_ccw(testn_));
    CHECK(0 == ncplane_rotate_ccw(testn_));
  }

  CHECK(0 == ncplane_destroy(testn_));
  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));

}
