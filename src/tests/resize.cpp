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

  SUBCASE("Marginalized") {
    ncplane_options parentopts{};
    parentopts.y = 3;
    parentopts.x = 5;
    parentopts.rows = 20;
    parentopts.cols = 40;
    auto parent = ncplane_create(n_, &parentopts);
    REQUIRE(parent);
    ncplane_options opts{};
    opts.y = 1;
    opts.x = 2;
    opts.margin_b = 3;
    opts.margin_r = 4;
    opts.flags = NCPLANE_OPTION_MARGINALIZED;
    opts.resizecb = ncplane_resize_marginalized;
    auto child = ncplane_create(parent, &opts);
    REQUIRE(child);
    CHECK(ncplane_dim_y(child) == 16);
    CHECK(ncplane_dim_x(child) == 34);
    REQUIRE(ncplane_putchar_yx(child, 0, 0, 'A') == 1);
    for(auto [rows, cols] : {std::pair{10u, 20u}, {24u, 50u}, {2u, 3u}, {20u, 40u}}){
      REQUIRE(ncplane_resize_simple(parent, rows, cols) == 0);
      CHECK(ncplane_y(child) == 1);
      CHECK(ncplane_x(child) == 2);
      CHECK(ncplane_dim_y(child) == (rows > 4 ? rows - 4 : 1));
      CHECK(ncplane_dim_x(child) == (cols > 6 ? cols - 6 : 1));
      uint16_t style;
      uint64_t channels;
      char* egc = ncplane_at_yx(child, 0, 0, &style, &channels);
      REQUIRE(egc);
      CHECK(std::string(egc) == "A");
      free(egc);
    }
    CHECK(ncplane_destroy(child) == 0);
    opts.y = opts.x = 0;
    opts.margin_b = opts.margin_r = 0;
    child = ncplane_create(parent, &opts);
    REQUIRE(child);
    CHECK(ncplane_dim_y(child) == 20);
    CHECK(ncplane_dim_x(child) == 40);
    REQUIRE(ncplane_resize_simple(parent, 10, 20) == 0);
    CHECK(ncplane_dim_y(child) == 10);
    CHECK(ncplane_dim_x(child) == 20);
    CHECK(ncplane_y(child) == 0);
    CHECK(ncplane_x(child) == 0);
    CHECK(ncplane_destroy(child) == 0);
    opts.y = 1;
    opts.x = 2;
    opts.margin_b = 100;
    opts.margin_r = 100;
    child = ncplane_create(parent, &opts);
    REQUIRE(child);
    CHECK(ncplane_dim_y(child) == 1);
    CHECK(ncplane_dim_x(child) == 1);
    CHECK(ncplane_destroy(child) == 0);
    CHECK(ncplane_destroy(parent) == 0);
  }

  SUBCASE("MarginalizedRoot") {
    ncplane_options opts{};
    opts.y = 1;
    opts.x = 2;
    opts.margin_b = 3;
    opts.margin_r = 4;
    opts.flags = NCPLANE_OPTION_MARGINALIZED;
    auto root = ncpile_create(nc_, &opts);
    REQUIRE(root);
    CHECK(ncplane_dim_y(root) == (dimy > 4 ? dimy - 4 : 1));
    CHECK(ncplane_dim_x(root) == (dimx > 6 ? dimx - 6 : 1));
    for(int repeat = 0 ; repeat < 2 ; ++repeat){
      REQUIRE(ncplane_resize_marginalized(root) == 0);
      CHECK(ncplane_y(root) == 1);
      CHECK(ncplane_x(root) == 2);
      CHECK(ncplane_dim_y(root) == (dimy > 4 ? dimy - 4 : 1));
      CHECK(ncplane_dim_x(root) == (dimx > 6 ? dimx - 6 : 1));
    }
    CHECK(ncplane_destroy(root) == 0);
  }

  CHECK(0 == notcurses_stop(nc_));

}
