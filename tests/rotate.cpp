#include "main.h"
#include <cmath>
#include <vector>

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
  if(!enforce_utf8()){
    return;
  }
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  int dimy, dimx;
  struct ncplane* n_ = notcurses_stddim_yx(nc_, &dimy, &dimx);
  REQUIRE(n_);

  uint64_t ul, ur, ll, lr;
  ul = ur = ll = lr = 0;
  channels_set_fg_rgb(&ul, 0x40f040);
  channels_set_bg_rgb(&ul, 0x40f040);
  channels_set_fg_rgb(&ll, 0xf040f0);
  channels_set_bg_rgb(&ll, 0xf040f0);
  channels_set_fg_rgb(&ur, 0x40f040);
  channels_set_bg_rgb(&ur, 0x40f040);
  channels_set_fg_rgb(&lr, 0xf040f0);
  channels_set_bg_rgb(&lr, 0xf040f0);

  SUBCASE("RotateTransparentCW") {
    struct ncplane_options nopts = {
      .y = dimy / 2,
      .x = dimx / 2,
      .rows = 8,
      .cols = 16,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .nc = nullptr,
    };
    struct ncplane* testn = ncplane_create(n_, &nopts);
    uint64_t channels = 0;
    CHECK(0 == channels_set_fg_alpha(&channels, CELL_ALPHA_TRANSPARENT));
    CHECK(0 == channels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT));
    REQUIRE(0 >= ncplane_set_base(testn, "", 0, channels));
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
    struct ncplane_options nopts = {
      .y = dimy / 2,
      .x = dimx / 2,
      .rows = 8,
      .cols = 16,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .nc = nullptr,
    };
    struct ncplane* testn = ncplane_create(n_, &nopts);
    REQUIRE(0 < ncplane_gradient_sized(testn, " ", 0, ul, ur, ll, lr, 8, 16));
    RotateCW(nc_, testn);
    CHECK(0 == ncplane_destroy(testn));
  }

  SUBCASE("RotateRectangleCW") {
    // should be a square, and should remain a square through rotations
    struct ncplane_options nopts = {
      .y = dimy / 2,
      .x = dimx / 2,
      .rows = 8,
      .cols = 32,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .nc = nullptr,
    };
    struct ncplane* testn = ncplane_create(n_, &nopts);
    REQUIRE(0 < ncplane_gradient_sized(testn, " ", 0, ul, ur, ll, lr, 8, 32));
    RotateCW(nc_, testn);
    CHECK(0 == ncplane_destroy(testn));
  }

  SUBCASE("RotateGradientCCW") {
    // should be a square, and should remain a square through rotations
    struct ncplane_options nopts = {
      .y = dimy / 2,
      .x = dimx / 2,
      .rows = 8,
      .cols = 16,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .nc = nullptr,
    };
    struct ncplane* testn = ncplane_create(n_, &nopts);
    REQUIRE(0 < ncplane_gradient_sized(testn, " ", 0, ul, ur, ll, lr, 8, 16));
    RotateCCW(nc_, testn);
    CHECK(0 == ncplane_destroy(testn));
  }

  SUBCASE("RotateRectangleCCW") {
    // should be a square, and should remain a square through rotations
    struct ncplane_options nopts = {
      .y = dimy / 2,
      .x = dimx / 2,
      .rows = 8,
      .cols = 32,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .nc = nullptr,
    };
    struct ncplane* testn = ncplane_create(n_, &nopts);
    REQUIRE(0 < ncplane_gradient_sized(testn, " ", 0, ul, ur, ll, lr, 8, 32));
    RotateCCW(nc_, testn);
    CHECK(0 == ncplane_destroy(testn));
  }

  // use half of each dimension
  SUBCASE("RotateRGBACW") {
    int height = dimy / 2;
    int width = dimx / 2;
    std::vector<uint32_t> rgba(width * height, 0xffbbccff);
    for(int i = 0 ; i < height * width / 2 ; ++i){
      CHECK(0xffbbccff == rgba[i]);
    }
    auto ncv = ncvisual_from_rgba(rgba.data(), height, width * 4, width);
    REQUIRE(ncv);
    ncvisual_options opts{};
    auto rendered = ncvisual_render(nc_, ncv, &opts);
    REQUIRE(rendered);
    uint32_t* rgbaret = ncplane_rgba(rendered, NCBLIT_DEFAULT, 0, 0, -1, -1);
    REQUIRE(rgbaret);
    for(int i = 0 ; i < height * width / 2 ; ++i){
      if(rgbaret[i] & CELL_BG_RGB_MASK){
        CHECK(rgbaret[i] == rgba[i]);
      }
    }
    free(rgbaret);
    CHECK(0 == notcurses_render(nc_));
    for(int x = 0 ; x < width ; ++x){
      uint16_t stylemask;
      uint64_t channels;
      char* c = notcurses_at_yx(nc_, 0, x, &stylemask, &channels);
      REQUIRE(c);
      CHECK(0 == strcmp(c, " "));
      if(channels_fg_rgb(channels) & CELL_BG_RGB_MASK){
        CHECK(0xffccbb == channels_fg_rgb(channels));
      }
      if(channels_bg_rgb(channels) & CELL_BG_RGB_MASK){
        CHECK(0xffccbb == channels_bg_rgb(channels));
      }
      free(c);
    }
    opts.n = rendered;
    // FIXME check pixels after all rotations
    CHECK(0 == ncvisual_rotate(ncv, M_PI / 2));
    CHECK(ncvisual_render(nc_, ncv, &opts));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncvisual_rotate(ncv, M_PI / 2));
    CHECK(ncvisual_render(nc_, ncv, &opts));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncvisual_rotate(ncv, M_PI / 2));
    CHECK(ncvisual_render(nc_, ncv, &opts));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncvisual_rotate(ncv, M_PI / 2));
    CHECK(ncvisual_render(nc_, ncv, &opts));
    CHECK(0 == notcurses_render(nc_));
    ncvisual_destroy(ncv);
    ncplane_destroy(rendered);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("RotateRGBACCW") {
    int height = dimy / 2;
    int width = dimx / 2;
    std::vector<uint32_t> rgba(width * height, 0xffbbccff);
    auto ncv = ncvisual_from_rgba(rgba.data(), height, width * 4, width);
    REQUIRE(ncv);
    ncvisual_options opts{};
    auto rendered = ncvisual_render(nc_, ncv, &opts);
    REQUIRE(rendered);
    uint32_t* rgbaret = ncplane_rgba(rendered, NCBLIT_DEFAULT, 0, 0, -1, -1);
    REQUIRE(rgbaret);
    for(int i = 0 ; i < height * width / 2 ; ++i){
      if(rgbaret[i] & CELL_BG_RGB_MASK){
        CHECK(rgbaret[i] == rgba[i]);
      }
    }
    free(rgbaret);
    CHECK(0 == notcurses_render(nc_));
    for(int x = 0 ; x < width ; ++x){
      uint16_t stylemask;
      uint64_t channels;
      char* c = notcurses_at_yx(nc_, 0, x, &stylemask, &channels);
      REQUIRE(c);
      CHECK(0 == strcmp(c, " "));
      if(channels_fg_rgb(channels) & CELL_BG_RGB_MASK){
        CHECK(0xffccbb == channels_fg_rgb(channels));
      }
      if(channels_bg_rgb(channels) & CELL_BG_RGB_MASK){
        CHECK(0xffccbb == channels_bg_rgb(channels));
      }
      free(c);
    }
    // FIXME check pixels after all rotations
    opts.n = rendered;
    CHECK(0 == ncvisual_rotate(ncv, -M_PI / 2));
    CHECK(ncvisual_render(nc_, ncv, &opts));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncvisual_rotate(ncv, -M_PI / 2));
    CHECK(ncvisual_render(nc_, ncv, &opts));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncvisual_rotate(ncv, -M_PI / 2));
    CHECK(ncvisual_render(nc_, ncv, &opts));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncvisual_rotate(ncv, -M_PI / 2));
    CHECK(ncvisual_render(nc_, ncv, &opts));
    CHECK(0 == notcurses_render(nc_));
    ncvisual_destroy(ncv);
    ncplane_destroy(rendered);
    CHECK(0 == notcurses_render(nc_));
  }

  CHECK(0 == notcurses_stop(nc_));

}
