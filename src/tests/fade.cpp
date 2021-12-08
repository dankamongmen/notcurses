#include "main.h"
#include <cstdlib>
#include <iostream>

auto pulser(struct notcurses* nc, struct ncplane* ncp __attribute__ ((unused)),
            const struct timespec* ts __attribute__ ((unused)), void* curry) -> int {
  auto pulsestart = static_cast<struct timespec*>(curry);
  if(notcurses_render(nc)){
    return -1;
  }
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC, &now);
  auto delta = timespec_to_ns(&now) - timespec_to_ns(pulsestart);
  if(delta > 250000000){
    return 1;
  }
  return 0;
}

auto fadeaborter(struct notcurses* nc, struct ncplane* ncp,
                 const struct timespec* ts, void* curry) -> int {
  (void)nc;
  (void)ncp;
  (void)ts;
  (void)curry;
  return 1;
}

TEST_CASE("Fade") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);
  if(!notcurses_canfade(nc_)){
    CHECK(0 == notcurses_stop(nc_));
    return;
  }
  REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));
  unsigned dimy, dimx;
  ncplane_dim_yx(n_, &dimy, &dimx);
  nccell c = NCCELL_CHAR_INITIALIZER('*');
  unsigned rgb = 0xffffffu;
  CHECK(!ncplane_set_scrolling(n_, true));
  for(unsigned y = 0 ; y < dimy ; ++y){
    for(unsigned x = 0 ; x < dimx ; ++x){
      rgb -= 32;
      if(rgb < 32){
        rgb = 0xffffffu;
      }
      nccell_set_fg_rgb(&c, rgb);
      nccell_set_bg_rgb8(&c, rgb & 0xff, (rgb >> 16u) & 0xff, (rgb >> 8u) & 0xff);
      CHECK(0 < ncplane_putc(n_, &c));
    }
  }

  SUBCASE("FadeOut") {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 250000000;
    CHECK(0 == ncplane_fadeout(n_, &ts, nullptr, nullptr));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("FadeIn") {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 250000000;
    CHECK(0 == ncplane_fadein(n_, &ts, nullptr, nullptr));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("FadeOutAbort") {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 250000000;
    CHECK(0 < ncplane_fadeout(n_, &ts, fadeaborter, nullptr));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("FadeInAbort") {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 250000000;
    CHECK(0 < ncplane_fadein(n_, &ts, fadeaborter, nullptr));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("Pulse") {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 75000000;
    ncplane_erase(n_);
    ncplane_set_fg_rgb(n_, 0xffd700);
    CHECK(0 < ncplane_printf_aligned(n_, dimy - 1, NCALIGN_LEFT, "pulllllllse"));
    CHECK(0 < ncplane_printf_aligned(n_, dimy - 1, NCALIGN_CENTER, "pulllllllse"));
    CHECK(0 < ncplane_printf_aligned(n_, dimy - 1, NCALIGN_RIGHT, "pulllllllse"));
    struct timespec pulsestart;
    clock_gettime(CLOCK_MONOTONIC, &pulsestart);
    CHECK(0 < ncplane_pulse(n_, &ts, pulser, &pulsestart));
    CHECK(0 == notcurses_render(nc_));
  }

  // drive fadeout with the more flexible api
  SUBCASE("FadeOutFlexible") {
    auto nctx = ncfadectx_setup(n_);
    REQUIRE(nctx);
    auto maxiter = ncfadectx_iterations(nctx);
    CHECK(0 < maxiter);
    for(int i = 0 ; i < maxiter ; ++i){
      CHECK(0 == ncplane_fadeout_iteration(n_, nctx, i, nullptr, nullptr));
    }
    ncfadectx_free(nctx);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("FadeOutFlexibleAbort") {
    auto nctx = ncfadectx_setup(n_);
    REQUIRE(nctx);
    auto maxiter = ncfadectx_iterations(nctx);
    CHECK(0 < maxiter);
    for(int i = 0 ; i < maxiter ; ++i){
      CHECK(0 < ncplane_fadeout_iteration(n_, nctx, i, fadeaborter, nullptr));
    }
    ncfadectx_free(nctx);
    CHECK(0 == notcurses_render(nc_));
  }

  // drive fadein with the more flexible api
  SUBCASE("FadeInFlexible") {
    auto nctx = ncfadectx_setup(n_);
    REQUIRE(nctx);
    auto maxiter = ncfadectx_iterations(nctx);
    CHECK(0 < maxiter);
    for(int i = 0 ; i < maxiter ; ++i){
      CHECK(0 == ncplane_fadein_iteration(n_, nctx, i, nullptr, nullptr));
    }
    ncfadectx_free(nctx);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("FadeInFlexibleAbort") {
    auto nctx = ncfadectx_setup(n_);
    REQUIRE(nctx);
    auto maxiter = ncfadectx_iterations(nctx);
    CHECK(0 < maxiter);
    for(int i = 0 ; i < maxiter ; ++i){
      CHECK(0 < ncplane_fadein_iteration(n_, nctx, i, fadeaborter, nullptr));
    }
    ncfadectx_free(nctx);
    CHECK(0 == notcurses_render(nc_));
  }

  CHECK(0 == notcurses_stop(nc_));

}
