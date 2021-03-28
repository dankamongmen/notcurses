#include "main.h"
#include <vector>

TEST_CASE("Pixel") {
  auto nc_ = testing_notcurses();
  REQUIRE(nullptr != nc_);
  ncplane* ncp_ = notcurses_stdplane(nc_);
  REQUIRE(ncp_);
  auto n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  if(notcurses_check_pixel_support(nc_) <= 0){
    CHECK(0 == nc_->tcache.sixel_supported);
    CHECK(!notcurses_stop(nc_));
    return;
  }

  SUBCASE("SprixelTermValues") {
    CHECK(0 < nc_->tcache.cellpixy);
    CHECK(0 < nc_->tcache.cellpixx);
    CHECK(nc_->tcache.sixel_supported);
    CHECK(nullptr != nc_->tcache.pixel_cell_wipe);
  }

#ifdef NOTCURSES_USE_MULTIMEDIA
  SUBCASE("PixelRender") {
    auto ncv = ncvisual_from_file(find_data("worldmap.png"));
    REQUIRE(ncv);
    struct ncvisual_options vopts{};
    vopts.blitter = NCBLIT_PIXEL;
    vopts.flags = NCVISUAL_OPTION_NODEGRADE;
    auto newn = ncvisual_render(nc_, ncv, &vopts);
    CHECK(newn);
    CHECK(0 == notcurses_render(nc_));
    ncplane_destroy(newn);
    CHECK(0 == notcurses_render(nc_));
    ncvisual_destroy(ncv);
  }
#endif

  SUBCASE("PixelCellWipe") {
    // first, assemble a visual equivalent to 4 cells
    auto y = 2 * nc_->tcache.cellpixy;
    auto x = 2 * nc_->tcache.cellpixx;
    std::vector<uint32_t> v(x * y, 0xffffffff);
    auto ncv = ncvisual_from_rgba(v.data(), y, sizeof(decltype(v)::value_type) * x, x);
    REQUIRE(nullptr != ncv);
    struct ncvisual_options vopts = {
      .n = nullptr,
      .scaling = NCSCALE_NONE,
      .y = 0, .x = 0,
      .begy = 0, .begx = 0,
      .leny = y, .lenx = x,
      .blitter = NCBLIT_PIXEL,
      .flags = NCVISUAL_OPTION_NODEGRADE,
    };
    auto n = ncvisual_render(nc_, ncv, &vopts);
    REQUIRE(nullptr != n);
    auto s = n->sprite;
    REQUIRE(nullptr != s);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_putchar_yx(n_, 0, 0, 'x'));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_putchar_yx(n_, 1, 1, 'x'));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_putchar_yx(n_, 1, 0, 'x'));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_putchar_yx(n_, 0, 1, 'x'));
    CHECK(0 == notcurses_render(nc_));
    ncplane_destroy(n);
    ncvisual_destroy(ncv);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("PixelCellWipePolychromatic") {
    // first, assemble a visual equivalent to 4 cells
    auto y = 2 * nc_->tcache.cellpixy;
    auto x = 2 * nc_->tcache.cellpixx;
    std::vector<uint32_t> v(x * y, 0xffffffff);
    for(auto& e : v){
      e -= random() % 0x1000000;
    }
    auto ncv = ncvisual_from_rgba(v.data(), y, sizeof(decltype(v)::value_type) * x, x);
    REQUIRE(nullptr != ncv);
    struct ncvisual_options vopts = {
      .n = nullptr,
      .scaling = NCSCALE_NONE,
      .y = 0, .x = 0,
      .begy = 0, .begx = 0,
      .leny = y, .lenx = x,
      .blitter = NCBLIT_PIXEL,
      .flags = NCVISUAL_OPTION_NODEGRADE,
    };
    auto n = ncvisual_render(nc_, ncv, &vopts);
    REQUIRE(nullptr != n);
    auto s = n->sprite;
    REQUIRE(nullptr != s);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_putchar_yx(n_, 0, 0, 'x'));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_putchar_yx(n_, 1, 1, 'x'));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_putchar_yx(n_, 1, 0, 'x'));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_putchar_yx(n_, 0, 1, 'x'));
    CHECK(0 == notcurses_render(nc_));
    ncplane_destroy(n);
    ncvisual_destroy(ncv);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("PixelBigCellWipePolychromatic") {
    // first, assemble a visual equivalent to 4 cells
    auto y = 10 * nc_->tcache.cellpixy;
    auto x = 10 * nc_->tcache.cellpixx;
    std::vector<uint32_t> v(x * y, 0xffffffff);
    for(auto& e : v){
      e -= random() % 0x1000000;
    }
    auto ncv = ncvisual_from_rgba(v.data(), y, sizeof(decltype(v)::value_type) * x, x);
    REQUIRE(nullptr != ncv);
    struct ncvisual_options vopts = {
      .n = nullptr,
      .scaling = NCSCALE_NONE,
      .y = 0, .x = 0,
      .begy = 0, .begx = 0,
      .leny = y, .lenx = x,
      .blitter = NCBLIT_PIXEL,
      .flags = NCVISUAL_OPTION_NODEGRADE,
    };
    auto n = ncvisual_render(nc_, ncv, &vopts);
    REQUIRE(nullptr != n);
    auto s = n->sprite;
    REQUIRE(nullptr != s);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_putchar_yx(n_, 5, 5, 'x'));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_putchar_yx(n_, 3, 3, 'x'));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_putchar_yx(n_, 8, 8, 'x'));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_putchar_yx(n_, 8, 3, 'x'));
    CHECK(0 == notcurses_render(nc_));
    ncplane_destroy(n);
    ncvisual_destroy(ncv);
    CHECK(0 == notcurses_render(nc_));
  }

#ifdef NOTCURSES_USE_MULTIMEDIA
  SUBCASE("PixelWipeImage") {
    auto ncv = ncvisual_from_file(find_data("worldmap.png"));
    REQUIRE(ncv);
    struct ncvisual_options vopts{};
    vopts.blitter = NCBLIT_PIXEL;
    vopts.flags = NCVISUAL_OPTION_NODEGRADE;
    auto newn = ncvisual_render(nc_, ncv, &vopts);
    CHECK(newn);
    CHECK(0 == notcurses_render(nc_));
    const auto s = newn->sprite;
    for(int y = 0 ; y < s->dimy ; ++y){
      for(int x = 0 ; x < s->dimx ; ++x){
        CHECK(0 == ncplane_putchar_yx(n_, y, x, 'x'));
        CHECK(0 == notcurses_render(nc_));
      }
    }
    ncplane_destroy(newn);
    CHECK(0 == notcurses_render(nc_));
    ncvisual_destroy(ncv);
  }
#endif

  CHECK(!notcurses_stop(nc_));
}
