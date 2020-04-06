#include "main.h"
#include <cstdlib>
#include <iostream>
#include "internal.h"

int pulser(struct notcurses* nc, struct ncplane* ncp __attribute__ ((unused)), void* curry){
  struct timespec* pulsestart = static_cast<struct timespec*>(curry);
  if(notcurses_render(nc)){
    return -1;
  }
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC_RAW, &now);
  auto delta = timespec_to_ns(&now) - timespec_to_ns(pulsestart);
  if(delta > 500000000){
    return 1;
  }
  return 0;
}

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
  CHECK(!ncplane_set_scrolling(n_, true));
  for(int y = 0 ; y < dimy ; ++y){
    for(int x = 0 ; x < dimx ; ++x){
      rgb -= 32;
      if(rgb < 32){
        rgb = 0xffffffu;
      }
      cell_set_fg(&c, rgb);
      cell_set_bg_rgb(&c, rgb & 0xff, (rgb >> 16u) & 0xff, (rgb >> 8u) & 0xff);
      CHECK(0 < ncplane_putc(n_, &c));
    }
  }

  SUBCASE("FadeOut") {
    CHECK(0 == notcurses_render(nc_));
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 500000000;
    CHECK(0 == ncplane_fadeout(n_, &ts, nullptr, nullptr));
  }

  SUBCASE("FadeIn") {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 500000000;
    CHECK(0 == ncplane_fadein(n_, &ts, nullptr, nullptr));
  }

  SUBCASE("Pulse") {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 150000000;
    ncplane_erase(n_);
    ncplane_set_fg(n_, 0xffd700);
    CHECK(0 < ncplane_printf_aligned(n_, dimy - 1, NCALIGN_CENTER, "pulllllllse"));
    struct timespec pulsestart;
    clock_gettime(CLOCK_MONOTONIC_RAW, &pulsestart);
    CHECK(0 < ncplane_pulse(n_, &ts, pulser, &pulsestart));
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));

}
