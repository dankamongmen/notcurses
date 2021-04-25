#include "main.h"
#include "visual-details.h"
#include <vector>

TEST_CASE("Bitmaps") {
  auto nc_ = testing_notcurses();
  REQUIRE(nullptr != nc_);
  ncplane* ncp_ = notcurses_stdplane(nc_);
  REQUIRE(ncp_);
  auto n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  if(notcurses_check_pixel_support(nc_) <= 0){
    CHECK(0 == nc_->tcache.bitmap_supported);
    CHECK(!notcurses_stop(nc_));
    return;
  }

  SUBCASE("SprixelTermValues") {
    CHECK(0 < nc_->tcache.cellpixy);
    CHECK(0 < nc_->tcache.cellpixx);
    CHECK(nc_->tcache.bitmap_supported);
  }

  SUBCASE("SprixelResize") {
    auto y = 10;
    auto x = 10;
    std::vector<uint32_t> v(x * y, htole(0xe61c28ff));
    auto ncv = ncvisual_from_rgba(v.data(), y, sizeof(decltype(v)::value_type) * x, x);
    REQUIRE(nullptr != ncv);
    struct ncvisual_options vopts = {
      .n = nullptr,
      .scaling = NCSCALE_NONE,
      .y = 0, .x = 0,
      .begy = 0, .begx = 0,
      .leny = 0, .lenx = 0,
      .blitter = NCBLIT_PIXEL,
      .flags = NCVISUAL_OPTION_NODEGRADE,
      .transcolor = 0,
    };
    CHECK(0 == ncvisual_resize(ncv, 6, 1)); // FIXME get down to 1, 1 (sixel needs handle)
    auto n = ncvisual_render(nc_, ncv, &vopts);
    REQUIRE(nullptr != n);
    auto s = n->sprite;
    REQUIRE(nullptr != s);
    ncvisual_destroy(ncv);
  }

  // a sprixel requires a plane large enough to hold it
  SUBCASE("SprixelTooTall") {
    auto y = nc_->tcache.cellpixy + 6;
    auto x = nc_->tcache.cellpixx;
    std::vector<uint32_t> v(x * y, htole(0xe61c28ff));
    auto ncv = ncvisual_from_rgba(v.data(), y, sizeof(decltype(v)::value_type) * x, x);
    REQUIRE(nullptr != ncv);
    struct ncplane_options nopts = {
      .y = 0, .x = 0,
      .rows = 1, .cols = 1,
      .userptr = nullptr, .name = "small", .resizecb = nullptr,
      .flags = 0, .margin_b = 0, .margin_r = 0,
    };
    auto n = ncplane_create(n_, &nopts);
    struct ncvisual_options vopts = {
      .n = n,
      .scaling = NCSCALE_NONE,
      .y = 0,
      .x = 0,
      .begy = 0, .begx = 0,
      .leny = 0, .lenx = 0,
      .blitter = NCBLIT_PIXEL,
      .flags = NCVISUAL_OPTION_NODEGRADE,
      .transcolor = 0,
    };
    CHECK(nullptr == ncvisual_render(nc_, ncv, &vopts));
    CHECK(0 == notcurses_render(nc_));
    ncvisual_destroy(ncv);
    CHECK(0 == ncplane_destroy(n));
  }

  SUBCASE("SprixelTooWide") {
    auto y = nc_->tcache.cellpixy;
    auto x = nc_->tcache.cellpixx + 1;
    std::vector<uint32_t> v(x * y, htole(0xe61c28ff));
    auto ncv = ncvisual_from_rgba(v.data(), y, sizeof(decltype(v)::value_type) * x, x);
    REQUIRE(nullptr != ncv);
    struct ncplane_options nopts = {
      .y = 0, .x = 0,
      .rows = 1, .cols = 1,
      .userptr = nullptr, .name = "small", .resizecb = nullptr,
      .flags = 0, .margin_b = 0, .margin_r = 0,
    };
    auto n = ncplane_create(n_, &nopts);
    struct ncvisual_options vopts = {
      .n = n,
      .scaling = NCSCALE_NONE,
      .y = 0,
      .x = 0,
      .begy = 0, .begx = 0,
      .leny = 0, .lenx = 0,
      .blitter = NCBLIT_PIXEL,
      .flags = NCVISUAL_OPTION_NODEGRADE,
      .transcolor = 0,
    };
    CHECK(nullptr == ncvisual_render(nc_, ncv, &vopts));
    CHECK(0 == notcurses_render(nc_));
    ncvisual_destroy(ncv);
    CHECK(0 == ncplane_destroy(n));
    CHECK(0 == notcurses_render(nc_));
  }

  // should not be able to emit glyphs to a sprixelated plane
  SUBCASE("SprixelNoGlyphs") {
    auto y = nc_->tcache.cellpixy;
    auto x = nc_->tcache.cellpixx;
    std::vector<uint32_t> v(x * y, htole(0xe61c28ff));
    auto ncv = ncvisual_from_rgba(v.data(), y, sizeof(decltype(v)::value_type) * x, x);
    REQUIRE(nullptr != ncv);
    struct ncplane_options nopts = {
      .y = 0, .x = 0,
      .rows = 1, .cols = 1,
      .userptr = nullptr, .name = "small", .resizecb = nullptr,
      .flags = 0, .margin_b = 0, .margin_r = 0,
    };
    auto n = ncplane_create(n_, &nopts);
    struct ncvisual_options vopts = {
      .n = n,
      .scaling = NCSCALE_NONE,
      .y = 0,
      .x = 0,
      .begy = 0, .begx = 0,
      .leny = 0, .lenx = 0,
      .blitter = NCBLIT_PIXEL,
      .flags = NCVISUAL_OPTION_NODEGRADE,
      .transcolor = 0,
    };
    CHECK(nullptr != ncvisual_render(nc_, ncv, &vopts));
    CHECK(0 > ncplane_putchar_yx(n, ' ', 0, 0));
    CHECK(0 == notcurses_render(nc_));
    ncvisual_destroy(ncv);
    CHECK(0 == ncplane_destroy(n));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("BitmapStack") {
    auto y = nc_->tcache.cellpixy * 10;
    auto x = nc_->tcache.cellpixx * 10;
    std::vector<uint32_t> v(x * y, htole(0xe61c28ff));
    auto ncv = ncvisual_from_rgba(v.data(), y, sizeof(decltype(v)::value_type) * x, x);
    REQUIRE(nullptr != ncv);
    struct ncvisual_options vopts = {
      .n = nullptr, .scaling = NCSCALE_NONE,
      .y = 0,
      .x = 0,
      .begy = 0, .begx = 0,
      .leny = 0, .lenx = 0,
      .blitter = NCBLIT_PIXEL,
      .flags = NCVISUAL_OPTION_NODEGRADE,
      .transcolor = 0,
    };
    auto botn = ncvisual_render(nc_, ncv, &vopts);
    REQUIRE(nullptr != botn);
    // should just have a red plane
    CHECK(0 == notcurses_render(nc_));
    y = nc_->tcache.cellpixy * 5;
    x = nc_->tcache.cellpixx * 5;
    std::vector<uint32_t> v2(x * y, htole(0x8142f1ff));
    auto ncv2 = ncvisual_from_rgba(v2.data(), y, sizeof(decltype(v2)::value_type) * x, x);
    REQUIRE(nullptr != ncv2);
    auto topn = ncvisual_render(nc_, ncv2, &vopts);
    REQUIRE(nullptr != topn);
    // should have a yellow plane partially obscuring a red one
    CHECK(0 == notcurses_render(nc_));
    ncplane_move_yx(topn, 5, 5);
    // yellow bitmap ought move to lower right, but remain visible
    CHECK(0 == notcurses_render(nc_));
    ncplane_move_top(botn);
    // should see only the red one now
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(botn));
    ncvisual_destroy(ncv);
    // now we see only yellow
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(topn));
    ncvisual_destroy(ncv2);
    // and now we see none
    CHECK(0 == notcurses_render(nc_));
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
    CHECK(0 == ncplane_destroy(newn));
    CHECK(0 == notcurses_render(nc_));
    ncvisual_destroy(ncv);
    CHECK(0 == notcurses_render(nc_));
  }
#endif

  SUBCASE("BitmapStretch") {
    // first, assemble a visual equivalent to 1 cell
    auto y = nc_->tcache.cellpixy;
    auto x = nc_->tcache.cellpixx;
    std::vector<uint32_t> v(x * y, htole(0xffffff00));
    auto ncv = ncvisual_from_rgba(v.data(), y, sizeof(decltype(v)::value_type) * x, x);
    REQUIRE(nullptr != ncv);
    struct ncvisual_options vopts = {
      .n = nullptr,
      .scaling = NCSCALE_NONE,
      .y = 0, .x = 0,
      .begy = 0, .begx = 0,
      .leny = 0, .lenx = 0,
      .blitter = NCBLIT_PIXEL,
      .flags = NCVISUAL_OPTION_NODEGRADE,
      .transcolor = 0,
    };
    auto n = ncvisual_render(nc_, ncv, &vopts);
    REQUIRE(nullptr != n);
    auto s = n->sprite;
    REQUIRE(nullptr != s);
    CHECK(nc_->tcache.cellpixy == ncv->rows);
    CHECK(nc_->tcache.cellpixx == ncv->cols);
    CHECK(0 == notcurses_render(nc_));
    struct ncplane_options nopts = {
      .y = 1,
      .x = 1,
      .rows = 4,
      .cols = 4,
      .userptr = nullptr, .name = "bigp", .resizecb = nullptr,
      .flags = 0, .margin_b = 0, .margin_r = 0,
    };
    vopts.scaling = NCSCALE_SCALE;
    auto bigp = ncplane_create(n_, &nopts);
    REQUIRE(bigp);
    vopts.n = bigp;
    uint64_t white = CHANNELS_RGB_INITIALIZER(0xff, 0xff, 0xff, 0xff, 0xff, 0xff);
    ncplane_set_base(bigp, "x", 0, white);
    CHECK(vopts.n == ncvisual_render(nc_, ncv, &vopts));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncvisual_inflate(ncv, 4));
    CHECK(4 * nc_->tcache.cellpixy == ncv->rows);
    CHECK(4 * nc_->tcache.cellpixx == ncv->cols);
    vopts.y = 1;
    vopts.x = 6;
    vopts.n = nullptr;
    vopts.scaling = NCSCALE_NONE;
    auto infn = ncvisual_render(nc_, ncv, &vopts);
    REQUIRE(infn);
    CHECK(4 == ncplane_dim_y(infn));
    CHECK(4 == ncplane_dim_x(infn));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncvisual_resize(ncv, 8, 8));
    CHECK(ncv->rows == 8);
    CHECK(ncv->cols == 8);
    vopts.x = 11;
    auto resizen = ncvisual_render(nc_, ncv, &vopts);
    REQUIRE(resizen);
    CHECK((8 + nc_->tcache.cellpixy - 1) / nc_->tcache.cellpixy == ncplane_dim_y(resizen));
    CHECK((8 + nc_->tcache.cellpixx - 1) / nc_->tcache.cellpixx == ncplane_dim_x(resizen));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(bigp));
    CHECK(0 == ncplane_destroy(resizen));
    CHECK(0 == ncplane_destroy(infn));
    CHECK(0 == ncplane_destroy(n));
    ncvisual_destroy(ncv);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("PixelCellWipe") {
    // first, assemble a visual equivalent to 4 cells
    auto y = 2 * nc_->tcache.cellpixy;
    auto x = 2 * nc_->tcache.cellpixx;
    std::vector<uint32_t> v(x * y, htole(0xffffffff));
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
      .transcolor = 0,
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
    CHECK(0 == ncplane_destroy(n));
    ncvisual_destroy(ncv);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("PixelCellWipePolychromatic") {
    // first, assemble a visual equivalent to 4 cells
    auto y = 2 * nc_->tcache.cellpixy;
    auto x = 2 * nc_->tcache.cellpixx;
    std::vector<uint32_t> v(x * y, htole(0xffffffff));
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
      .transcolor = 0,
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
    CHECK(0 == ncplane_destroy(n));
    ncvisual_destroy(ncv);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("PixelBigCellWipePolychromatic") {
    // first, assemble a visual equivalent to 100 cells
    auto y = 10 * nc_->tcache.cellpixy;
    auto x = 10 * nc_->tcache.cellpixx;
    std::vector<uint32_t> v(x * y, htole(0xffffffff));
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
      .transcolor = 0,
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
    CHECK(0 == ncplane_destroy(n));
    ncvisual_destroy(ncv);
    CHECK(0 == notcurses_render(nc_));
  }

  // verify that the sprixel's TAM is properly initialized
  SUBCASE("PixelTAMSetup") {
    // first, assemble a visual equivalent to 54 cells
    auto dimy = 6;
    auto dimx = 9;
    auto y = dimy * nc_->tcache.cellpixy;
    auto x = dimx * nc_->tcache.cellpixx;
    std::vector<uint32_t> v(x * y, htole(0xffffffff));
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
      .y = 2, .x = 2,
      .begy = 0, .begx = 0,
      .leny = y, .lenx = x,
      .blitter = NCBLIT_PIXEL,
      .flags = NCVISUAL_OPTION_NODEGRADE,
      .transcolor = 0,
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
      // cells with a transparent pixel ought be SPRIXCELL_MIXED;
      // cells without one ought be SPRIXCELL_OPAQUE.
      sprixcell_e state = tam[(i / dimx) + (i % dimx)];
      if(i % 2){
        if(state == SPRIXCELL_MIXED_SIXEL){
          state = SPRIXCELL_MIXED_KITTY;
        }
        CHECK(SPRIXCELL_MIXED_KITTY == state);
      }else{
        if(state == SPRIXCELL_OPAQUE_SIXEL){
          state = SPRIXCELL_OPAQUE_KITTY;
        }
        CHECK(SPRIXCELL_OPAQUE_KITTY == state);
      }
      ncpixel_set_a(&v[py * x + px], 0);
    }
    for(int yy = vopts.y ; yy < vopts.y + dimy ; ++yy){
      for(int xx = vopts.x ; xx < vopts.x + dimx ; ++xx){
        sprixcell_e state = sprixel_state(s, yy, xx);
        if((yy * dimx + xx) % 2){
          if(state == SPRIXCELL_MIXED_SIXEL){
            state = SPRIXCELL_MIXED_KITTY;
          }
          CHECK(SPRIXCELL_MIXED_KITTY == state);
        }else{
          if(state == SPRIXCELL_OPAQUE_SIXEL){
            state = SPRIXCELL_OPAQUE_KITTY;
          }
          CHECK(SPRIXCELL_OPAQUE_KITTY == state);
        }
      }
    }
    CHECK(0 == ncplane_destroy(n));
    CHECK(0 == notcurses_render(nc_));
  }

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
        // FIXME generates too much output, OOMing ctest
        // CHECK(0 == notcurses_render(nc_));
      }
    }
    CHECK(0 == ncplane_destroy(newn));
    CHECK(0 == notcurses_render(nc_));
    ncvisual_destroy(ncv);
    CHECK(0 == notcurses_render(nc_));
  }
#endif

  CHECK(!notcurses_stop(nc_));
}
