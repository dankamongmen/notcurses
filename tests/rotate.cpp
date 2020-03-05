#include "main.h"

void RotateCW(struct notcurses* nc, struct ncplane* n) {
    CHECK(0 == notcurses_render(nc));
    CHECK(0 == ncplane_rotate_cw(n));
    CHECK(0 == notcurses_render(nc));
    CHECK(0 == ncplane_rotate_cw(n));
    CHECK(0 == notcurses_render(nc));
    CHECK(0 == ncplane_rotate_cw(n));
    CHECK(0 == notcurses_render(nc));
    CHECK(0 == ncplane_rotate_cw(n));
    CHECK(0 == notcurses_render(nc));
}

void RotateCCW(struct notcurses* nc, struct ncplane* n) {
    CHECK(0 == notcurses_render(nc));
    CHECK(0 == ncplane_rotate_ccw(n));
    CHECK(0 == notcurses_render(nc));
    CHECK(0 == ncplane_rotate_ccw(n));
    CHECK(0 == notcurses_render(nc));
    CHECK(0 == ncplane_rotate_ccw(n));
    CHECK(0 == notcurses_render(nc));
    CHECK(0 == ncplane_rotate_ccw(n));
    CHECK(0 == notcurses_render(nc));
}

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

  SUBCASE("RotateTransparentCW") {
    struct ncplane* testn = ncplane_new(nc_, 8, 16, dimy / 2, dimx / 2, nullptr);
    uint64_t channels = 0;
    CHECK(0 == channels_set_fg_alpha(&channels, CELL_ALPHA_TRANSPARENT));
    CHECK(0 == channels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT));
    REQUIRE(0 >= ncplane_set_base(testn, channels, 0, ""));
    cell tl = CELL_TRIVIAL_INITIALIZER; cell tr = CELL_TRIVIAL_INITIALIZER;
    cell bl = CELL_TRIVIAL_INITIALIZER; cell br = CELL_TRIVIAL_INITIALIZER;
    cell hl = CELL_TRIVIAL_INITIALIZER; cell vl = CELL_TRIVIAL_INITIALIZER;
    CHECK(-1 < cell_prime(testn, &tl, "█", 0, ul));
    CHECK(-1 < cell_prime(testn, &tr, "█", 0, ur));
    CHECK(-1 < cell_prime(testn, &bl, "█", 0, ll));
    CHECK(-1 < cell_prime(testn, &br, "█", 0, lr));
    CHECK(-1 < cell_prime(testn, &hl, "█", 0, ll));
    CHECK(-1 < cell_prime(testn, &vl, "█", 0, lr));
    CHECK(0 == ncplane_perimeter(testn, &tl, &tr, &bl, &br, &hl, &vl, 0));
    RotateCW(nc_, testn);
    cell_release(testn, &tl); cell_release(testn, &tr);
    cell_release(testn, &bl); cell_release(testn, &br);
    cell_release(testn, &hl); cell_release(testn, &vl);
    CHECK(0 == ncplane_destroy(testn));
  }

  SUBCASE("RotateGradientCW") {
    // should be a square, and should remain a square through rotations
    struct ncplane* testn = ncplane_new(nc_, 8, 16, dimy / 2, dimx / 2, nullptr);
    REQUIRE(0 == ncplane_gradient_sized(testn, " ", 0, ul, ur, ll, lr, 8, 16));
    RotateCW(nc_, testn);
    CHECK(0 == ncplane_destroy(testn));
  }

  SUBCASE("RotateRectangleCW") {
    // should be a square, and should remain a square through rotations
    struct ncplane* testn = ncplane_new(nc_, 8, 32, dimy / 2, dimx / 2, nullptr);
    REQUIRE(0 == ncplane_gradient_sized(testn, " ", 0, ul, ur, ll, lr, 8, 32));
    RotateCW(nc_, testn);
    CHECK(0 == ncplane_destroy(testn));
  }

  SUBCASE("RotateGradientCCW") {
    // should be a square, and should remain a square through rotations
    struct ncplane* testn = ncplane_new(nc_, 8, 16, dimy / 2, dimx / 2, nullptr);
    REQUIRE(0 == ncplane_gradient_sized(testn, " ", 0, ul, ur, ll, lr, 8, 16));
    RotateCCW(nc_, testn);
    CHECK(0 == ncplane_destroy(testn));
  }

  SUBCASE("RotateRectangleCCW") {
    // should be a square, and should remain a square through rotations
    struct ncplane* testn = ncplane_new(nc_, 8, 32, 0, 0, nullptr);
    REQUIRE(0 == ncplane_gradient_sized(testn, " ", 0, ul, ur, ll, lr, 8, 32));
    RotateCCW(nc_, testn);
    CHECK(0 == ncplane_destroy(testn));
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));

}
