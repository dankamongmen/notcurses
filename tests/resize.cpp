#include "main.h"

TEST_CASE("Resize") {
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

  // start at full size, and shrink to a nothing
  SUBCASE("ResizeShrink") {
    int y = dimy;
    int x = dimx;
    struct ncplane* testn = ncplane_new(nc_, y, x, 0, 0, nullptr);
    REQUIRE(nullptr != testn);
    REQUIRE(0 < ncplane_gradient_sized(testn, "V", 0, ul, ur, ll, lr, y, x));
    CHECK(0 == notcurses_render(nc_));
    while(y > 1 || x > 1){
      y = y > 1 ? y - 1 : y;
      x = x > 1 ? x - 1 : x;
      CHECK(0 == ncplane_resize(testn, 0, 0, y, x, 0, 0, y, x));
      CHECK(0 == notcurses_render(nc_));
    }
    CHECK(0 == ncplane_destroy(testn));
  }

  // start at 1x1, and enlarge to fill the screen
  SUBCASE("ResizeEnlarge") {
    int y = 2;
    int x = 2;
    struct ncplane* testn = ncplane_new(nc_, y, x, 0, 0, nullptr);
    REQUIRE(nullptr != testn);
    REQUIRE(0 < ncplane_gradient_sized(testn, "V", 0, ul, ur, ll, lr, y, x));
    CHECK(0 == notcurses_render(nc_));
    while(y < dimy || x < dimx){
      y = y < dimy ? y + 1 : y;
      x = x < dimx ? x + 1 : x;
      CHECK(0 == ncplane_resize(testn, 0, 0, 0, 0, 0, 0, y, x));
      REQUIRE(0 < ncplane_gradient_sized(testn, "V", 0, ul, ur, ll, lr, y, x));
      CHECK(0 == notcurses_render(nc_));
    }
    CHECK(0 == ncplane_destroy(testn));
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));

}
