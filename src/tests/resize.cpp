#include "main.h"

TEST_CASE("Resize") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  unsigned dimy, dimx;
  struct ncplane* n_ = notcurses_stddim_yx(nc_, &dimy, &dimx);
  REQUIRE(n_);
  uint64_t ul, ur, ll, lr;
  ul = ur = ll = lr = 0;
  ncchannels_set_fg_rgb(&ul, 0x40f040);
  ncchannels_set_bg_rgb(&ul, 0x40f040);
  ncchannels_set_fg_rgb(&ll, 0xf040f0);
  ncchannels_set_bg_rgb(&ll, 0xf040f0);
  ncchannels_set_fg_rgb(&ur, 0x40f040);
  ncchannels_set_bg_rgb(&ur, 0x40f040);
  ncchannels_set_fg_rgb(&lr, 0xf040f0);
  ncchannels_set_bg_rgb(&lr, 0xf040f0);

  // start at full size, and shrink to a nothing
  SUBCASE("ResizeShrink") {
    unsigned y = dimy;
    unsigned x = dimx;
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = y,
      .cols = x,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* testn = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != testn);
    REQUIRE(0 < ncplane_gradient(testn, -1, -1, y, x, "V", 0, ul, ur, ll, lr));
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
    unsigned y = 2;
    unsigned x = 2;
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = y,
      .cols = x,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* testn = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != testn);
    REQUIRE(0 < ncplane_gradient(testn, -1, -1, y, x, "V", 0, ul, ur, ll, lr));
    CHECK(0 == notcurses_render(nc_));
    while(y < dimy || x < dimx){
      y = y < dimy ? y + 1 : y;
      x = x < dimx ? x + 1 : x;
      CHECK(0 == ncplane_resize(testn, 0, 0, 0, 0, 0, 0, y, x));
      REQUIRE(0 < ncplane_gradient(testn, -1, -1, y, x, "V", 0, ul, ur, ll, lr));
      CHECK(0 == notcurses_render(nc_));
    }
    CHECK(0 == ncplane_destroy(testn));
  }

  CHECK(0 == notcurses_stop(nc_));

}
