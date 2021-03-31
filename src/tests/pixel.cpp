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
    CHECK(1 == ncplane_putchar_yx(n_, 0, 0, 'x'));
    CHECK(0 == notcurses_render(nc_));
    CHECK(1 == ncplane_putchar_yx(n_, 1, 1, 'x'));
    CHECK(0 == notcurses_render(nc_));
    CHECK(1 == ncplane_putchar_yx(n_, 1, 0, 'x'));
    CHECK(0 == notcurses_render(nc_));
    CHECK(1 == ncplane_putchar_yx(n_, 0, 1, 'x'));
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
    CHECK(1 == ncplane_putchar_yx(n_, 0, 0, 'x'));
    CHECK(0 == notcurses_render(nc_));
    CHECK(1 == ncplane_putchar_yx(n_, 1, 1, 'x'));
    CHECK(0 == notcurses_render(nc_));
    CHECK(1 == ncplane_putchar_yx(n_, 1, 0, 'x'));
    CHECK(0 == notcurses_render(nc_));
    CHECK(1 == ncplane_putchar_yx(n_, 0, 1, 'x'));
    CHECK(0 == notcurses_render(nc_));
    ncplane_destroy(n);
    ncvisual_destroy(ncv);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("PixelBigCellWipePolychromatic") {
    // first, assemble a visual equivalent to 100 cells
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
    CHECK(1 == ncplane_putchar_yx(n_, 5, 5, 'x'));
    CHECK(0 == notcurses_render(nc_));
    CHECK(1 == ncplane_putchar_yx(n_, 3, 3, 'x'));
    CHECK(0 == notcurses_render(nc_));
    CHECK(1 == ncplane_putchar_yx(n_, 8, 8, 'x'));
    CHECK(0 == notcurses_render(nc_));
    CHECK(1 == ncplane_putchar_yx(n_, 8, 3, 'x'));
    CHECK(0 == notcurses_render(nc_));
    ncplane_destroy(n);
    ncvisual_destroy(ncv);
    CHECK(0 == notcurses_render(nc_));
  }

  // verify that the sprixel's TAM is properly initialized
  SUBCASE("PixelTAMSetup") {
    // first, assemble a visual equivalent to 81 cells
    auto dimy = 9;
    auto dimx = 9;
    auto y = dimy * nc_->tcache.cellpixy;
    auto x = dimx * nc_->tcache.cellpixx;
    std::vector<uint32_t> v(x * y, 0xffffffff);
    // every other cell, set some pixels transparent
    for(int i = 0 ; i < dimy * dimx ; ++i){
      if(i % 2){
        int py = (i / dimx) * nc_->tcache.cellpixy;
        int px = (i % dimx) * nc_->tcache.cellpixx;
        ncpixel_set_a(&v[py * x + px], 0);
      }
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
    ncvisual_destroy(ncv);
    CHECK(0 == notcurses_render(nc_));
    const auto s = n->sprite;
    REQUIRE(s);
    CHECK(s->dimy == dimy);
    CHECK(s->dimx == dimx);
    const auto tam = n->tacache;
    for(int i = 0 ; i < s->dimy * s->dimx ; ++i){
      int py = (i / dimx) * nc_->tcache.cellpixy;
      int px = (i % dimx) * nc_->tcache.cellpixx;
      // cells with a transparent pixel ought be SPRIXCELL_CONTAINS_TRANS;
      // cells without one ought be SPRIXCELL_OPAQUE.
      CHECK((i % 2) == tam[(i / dimx) + (i % dimx)]);
      ncpixel_set_a(&v[py * x + px], 0);
    }
    ncplane_destroy(n);
  }

  // too much output -- OOMs ctest FIXME
  /*
#ifdef NOTCURSES_USE_MULTIMEDIA
  SUBCASE("PixelWipeImage") {
    uint64_t channels = 0;
    channels_set_fg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
    channels_set_bg_alpha(&channels, CELL_ALPHA_TRANSPARENT);
    CHECK(0 == ncplane_set_base(n_, "", 0, channels));
    auto ncv = ncvisual_from_file(find_data("worldmap.png"));
    REQUIRE(ncv);
    struct ncvisual_options vopts{};
    vopts.blitter = NCBLIT_PIXEL;
    vopts.flags = NCVISUAL_OPTION_NODEGRADE;
    auto newn = ncvisual_render(nc_, ncv, &vopts);
    CHECK(newn);
    ncplane_move_bottom(newn);
    CHECK(0 == notcurses_render(nc_));
    const auto s = newn->sprite;
    for(int y = 0 ; y < s->dimy ; ++y){
      for(int x = 0 ; x < s->dimx ; ++x){
        CHECK(1 == ncplane_putchar_yx(n_, y, x, 'x'));
        CHECK(0 == notcurses_render(nc_));
      }
    }
    ncplane_destroy(newn);
    CHECK(0 == notcurses_render(nc_));
    ncvisual_destroy(ncv);
  }
#endif
*/

  CHECK(!notcurses_stop(nc_));
}
