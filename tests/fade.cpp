#include "main.h"
#include <cstdlib>
#include <iostream>

TEST_CASE("Fade") {
  if(getenv("TERM") == nullptr){
    return;
  }
  FILE* outfp_ = fopen("/dev/tty", "wb");
  REQUIRE(outfp_);
  notcurses_options nopts{};
  nopts.suppress_banner = true;
  nopts.inhibit_alternate_screen = true;
  struct notcurses* nc_ = notcurses_init(&nopts, outfp_);
  REQUIRE(nc_);
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);
  if(!notcurses_canfade(nc_)){
    return;
  }
  REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));
  int dimy, dimx;
  ncplane_dim_yx(n_, &dimy, &dimx);
  cell c = CELL_TRIVIAL_INITIALIZER;
  c.gcluster = '*';
  cell_set_fg_rgb(&c, 0xff, 0xff, 0xff);
  unsigned rgb = 0xffffffu;
  for(int y = 0 ; y < dimy ; ++y){
    for(int x = 0 ; x < dimx ; ++x){
      rgb -= 32;
      if(rgb < 32){
        rgb = 0xffffffu;
      }
      cell_set_fg_rgb(&c, (rgb >> 16u) & 0xff, (rgb >> 8u) & 0xff, rgb & 0xff);
      cell_set_bg_rgb(&c, rgb & 0xff, (rgb >> 16u) & 0xff, (rgb >> 8u) & 0xff);
      CHECK(0 < ncplane_putc(n_, &c));
    }
  }

  SUBCASE("FadeOut") {
    CHECK(0 == notcurses_render(nc_));
    struct timespec ts;
    ts.tv_sec = 1;
    ts.tv_nsec = 0;
    REQUIRE(0 == ncplane_fadeout(n_, &ts, nullptr));
  }

  SUBCASE("FadeIn") {
    struct timespec ts;
    ts.tv_sec = 1;
    ts.tv_nsec = 0;
    REQUIRE(0 == ncplane_fadein(n_, &ts, nullptr));
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));

}
