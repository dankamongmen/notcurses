#include <array>
#include <cstdlib>
#include "main.h"

TEST_CASE("Fills") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  if(!notcurses_canutf8(nc_)){
    CHECK(0 == notcurses_stop(nc_));
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  // can't polyfill with a null glyph
  SUBCASE("PolyfillNullGlyph") {
    int dimx, dimy;
    ncplane_dim_yx(n_, &dimy, &dimx);
    nccell c = CELL_TRIVIAL_INITIALIZER;
    CHECK(0 > ncplane_polyfill_yx(n_, dimy, dimx, &c));
  }

  // trying to polyfill an invalid cell ought be an error
  SUBCASE("PolyfillOffplane") {
    int dimx, dimy;
    ncplane_dim_yx(n_, &dimy, &dimx);
    nccell c = CELL_CHAR_INITIALIZER('+');
    CHECK(0 > ncplane_polyfill_yx(n_, dimy, 0, &c));
    CHECK(0 > ncplane_polyfill_yx(n_, 0, dimx, &c));
    CHECK(0 > ncplane_polyfill_yx(n_, 0, -1, &c));
    CHECK(0 > ncplane_polyfill_yx(n_, -1, 0, &c));
  }

  SUBCASE("PolyfillOnGlyph") {
    nccell c = CELL_CHAR_INITIALIZER('+');
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 4,
      .cols = 4,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* pfn = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != pfn);
    CHECK(16 == ncplane_polyfill_yx(pfn, 0, 0, &c));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 < ncplane_putc_yx(pfn, 0, 0, &c));
    CHECK(0 < cell_load(pfn, &c, "/"));
    CHECK(0 < ncplane_polyfill_yx(pfn, 0, 0, &c));
    char* ncpc = ncplane_at_yx(pfn, 0, 0, NULL, NULL);
    CHECK(0 == strcmp(ncpc, "/"));
    free(ncpc);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(pfn));
  }

  SUBCASE("PolyfillStandardPlane") {
    nccell c = CELL_CHAR_INITIALIZER('-');
    CHECK(0 < ncplane_polyfill_yx(n_, 0, 0, &c));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("PolyfillEmptyPlane") {
    nccell c = CELL_CHAR_INITIALIZER('+');
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 20,
      .cols = 20,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* pfn = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != pfn);
    CHECK(400 == ncplane_polyfill_yx(pfn, 0, 0, &c));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(pfn));
  }

  SUBCASE("PolyfillWalledPlane") {
    nccell c = CELL_CHAR_INITIALIZER('+');
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 4,
      .cols = 4,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* pfn = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != pfn);
    CHECK(0 < ncplane_putc_yx(pfn, 0, 1, &c));
    CHECK(0 < ncplane_putc_yx(pfn, 1, 1, &c));
    CHECK(0 < ncplane_putc_yx(pfn, 1, 0, &c));
    // Trying to fill the origin ought fill exactly one cell
    CHECK(1 == ncplane_polyfill_yx(pfn, 0, 0, &c));
    // Beyond the origin, we ought fill 12
    CHECK(12 == ncplane_polyfill_yx(pfn, 2, 2, &c));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(pfn));
  }

  SUBCASE("GradientMonochromatic") {
    uint64_t c = 0;
    channels_set_fg_rgb(&c, 0x40f040);
    channels_set_bg_rgb(&c, 0x40f040);
    int dimy, dimx;
    ncplane_dim_yx(n_, &dimy, &dimx);
    REQUIRE(0 < ncplane_gradient_sized(n_, "M", 0, c, c, c, c, dimy, dimx));
    nccell cl = CELL_TRIVIAL_INITIALIZER;
    uint64_t channels = 0;
    channels_set_fg_rgb(&channels, 0x40f040);
    channels_set_bg_rgb(&channels, 0x40f040);
    // check all squares 
    for(int y = 0 ; y < dimy ; ++y){
      for(int x = 0 ; x < dimx ; ++x){
        REQUIRE(0 <= ncplane_at_yx_cell(n_, y, x, &cl));
        CHECK(htole('M') == cl.gcluster);
        CHECK(0 == cl.stylemask);
        CHECK(channels == cl.channels);
      }
    }
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("GradientVertical") {
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
    int dimy, dimx;
    ncplane_dim_yx(n_, &dimy, &dimx);
    REQUIRE(0 < ncplane_gradient_sized(n_, "V", 0, ul, ur, ll, lr, dimy, dimx));
    nccell c = CELL_TRIVIAL_INITIALIZER;
    uint64_t channels = 0;
    channels_set_fg_rgb(&channels, 0x40f040);
    channels_set_bg_rgb(&channels, 0x40f040);
    // check all squares. all rows ought be the same across their breadth, and
    // the components ought be going in the correct direction.
    uint64_t lastyrgb, lastxrgb;
    lastyrgb = -1;
    for(int y = 0 ; y < dimy ; ++y){
      lastxrgb = -1;
      for(int x = 0 ; x < dimx ; ++x){
        REQUIRE(0 <= ncplane_at_yx_cell(n_, y, x, &c));
        CHECK(htole('V') == c.gcluster);
        CHECK(0 == c.stylemask);
        if(lastxrgb == (uint64_t)-1){
          if(lastyrgb == (uint64_t)-1){
            lastyrgb = c.channels;
            CHECK(ul == c.channels);
          }else if(y == dimy - 1){
            CHECK(ll == c.channels);
          }
          lastxrgb = c.channels;
        }else{
          CHECK(lastxrgb == c.channels);
        }
        if(x == dimx - 1){
          if(y == 0){
            CHECK(ur == c.channels);
          }else if(y == dimy - 1){
            CHECK(lr == c.channels);
          }
        }
      }
    }
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("GradientHorizontal") {
    uint64_t ul, ur, ll, lr;
    ul = ur = ll = lr = 0;
    channels_set_fg_rgb(&ul, 0x40f040);
    channels_set_bg_rgb(&ul, 0x40f040);
    channels_set_fg_rgb(&ur, 0xf040f0);
    channels_set_bg_rgb(&ur, 0xf040f0);
    channels_set_fg_rgb(&ll, 0x40f040);
    channels_set_bg_rgb(&ll, 0x40f040);
    channels_set_fg_rgb(&lr, 0xf040f0);
    channels_set_bg_rgb(&lr, 0xf040f0);
    int dimy, dimx;
    ncplane_dim_yx(n_, &dimy, &dimx);
    REQUIRE(0 < ncplane_gradient_sized(n_, "H", 0, ul, ur, ll, lr, dimy, dimx));
    // check corners FIXME
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("GradientX") {
    uint64_t ul, ur, ll, lr;
    ul = ur = ll = lr = 0;
    channels_set_fg_rgb(&ul, 0x000000);
    channels_set_bg_rgb(&ul, 0xffffff);
    channels_set_fg_rgb(&ll, 0x40f040);
    channels_set_bg_rgb(&ll, 0x40f040);
    channels_set_fg_rgb(&ur, 0xf040f0);
    channels_set_bg_rgb(&ur, 0xf040f0);
    channels_set_fg_rgb(&lr, 0xffffff);
    channels_set_bg_rgb(&lr, 0x000000);
    int dimy, dimx;
    ncplane_dim_yx(n_, &dimy, &dimx);
    REQUIRE(0 < ncplane_gradient_sized(n_, "X", 0, ul, ur, ll, lr, dimy, dimx));
    // check corners FIXME
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("GradientS") {
    uint64_t ul, ur, ll, lr;
    ul = ur = ll = lr = 0;
    channels_set_fg_rgb(&ul, 0xffffff);
    channels_set_bg_rgb(&ul, 0xffffff);
    channels_set_fg_rgb(&lr, 0x000000);
    channels_set_bg_rgb(&lr, 0x000000);
    channels_set_fg_rgb(&ll, 0x00ffff);
    channels_set_bg_rgb(&ll, 0xff0000);
    channels_set_fg_rgb(&ur, 0xff00ff);
    channels_set_bg_rgb(&ur, 0x00ff00);
    int dimy, dimx;
    ncplane_dim_yx(n_, &dimy, &dimx);
    REQUIRE(0 < ncplane_gradient_sized(n_, "S", 0, ul, ur, ll, lr, dimy, dimx));
    // check corners FIXME
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("Ncplane_Format") {
    int sbytes;
    CHECK(0 == ncplane_set_fg_rgb(n_, 0x444444));
    CHECK(1 == ncplane_putegc(n_, "A", &sbytes));
    CHECK(0 == ncplane_set_fg_rgb(n_, 0x888888));
    CHECK(1 == ncplane_putegc(n_, "B", &sbytes));
    CHECK(0 == notcurses_render(nc_));
    // attr should change, but not the EGC/color
    CHECK(0 == ncplane_cursor_move_yx(n_, 0, 0));
    nccell c = CELL_TRIVIAL_INITIALIZER;
    cell_on_styles(&c, NCSTYLE_BOLD);
    CHECK(0 < ncplane_format(n_, 0, 0, c.stylemask));
    nccell d = CELL_TRIVIAL_INITIALIZER;
    CHECK(1 == ncplane_at_yx_cell(n_, 0, 0, &d));
    CHECK(d.stylemask == c.stylemask);
    CHECK(0x444444 == cell_fg_rgb(&d));
  }

  SUBCASE("Ncplane_Stain") {
    int sbytes;
    CHECK(0 == ncplane_set_fg_rgb(n_, 0x444444));
    for(int y = 0 ; y < 8 ; ++y){
      for(int x = 0 ; x < 8 ; ++x){
        CHECK(1 == ncplane_putegc_yx(n_, y, x, "A", &sbytes));
      }
    }
    CHECK(0 == notcurses_render(nc_));
    // EGC/color should change, but nothing else
    CHECK(0 == ncplane_cursor_move_yx(n_, 0, 0));
    uint64_t channels = 0;
    channels_set_fg_rgb8(&channels, 0x88, 0x99, 0x77);
    channels_set_bg_rgb(&channels, 0);
    REQUIRE(0 < ncplane_stain(n_, 7, 7, channels, channels, channels, channels));
    CHECK(0 == notcurses_render(nc_));
    nccell d = CELL_TRIVIAL_INITIALIZER;
    for(int y = 0 ; y < 8 ; ++y){
      for(int x = 0 ; x < 8 ; ++x){
        CHECK(1 == ncplane_at_yx_cell(n_, y, x, &d));
        CHECK(channels == d.channels);
        REQUIRE(cell_simple_p(&d));
        CHECK(htole('A') == d.gcluster);
      }
    }
  }

  // test the single-cell (1x1) special case
  SUBCASE("GradientSingleCell") {
    int sbytes;
    CHECK(0 == ncplane_set_fg_rgb(n_, 0x444444));
    CHECK(1 == ncplane_putegc_yx(n_, 0, 0, "A", &sbytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_cursor_move_yx(n_, 0, 0));
    uint64_t channels = 0;
    channels_set_fg_rgb8(&channels, 0x88, 0x99, 0x77);
    channels_set_bg_rgb(&channels, 0);
    REQUIRE(0 < ncplane_gradient(n_, "A", 0, channels, channels, channels, channels, 0, 0));
    CHECK(0 == notcurses_render(nc_));
    nccell d = CELL_TRIVIAL_INITIALIZER;
    CHECK(1 == ncplane_at_yx_cell(n_, 0, 0, &d));
    CHECK(channels == d.channels);
    REQUIRE(cell_simple_p(&d));
    CHECK(htole('A') == d.gcluster);
  }

  // 1d gradients over multiple cells
  SUBCASE("Gradient1D") {
    int sbytes;
    CHECK(0 == ncplane_set_fg_rgb(n_, 0x444444));
    CHECK(1 == ncplane_putegc_yx(n_, 0, 0, "A", &sbytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_cursor_move_yx(n_, 0, 0));
    uint64_t chan1 = 0, chan2 = 0;
    channels_set_fg_rgb8(&chan1, 0x88, 0x99, 0x77);
    channels_set_fg_rgb8(&chan2, 0x77, 0x99, 0x88);
    channels_set_bg_rgb(&chan1, 0);
    channels_set_bg_rgb(&chan2, 0);
    REQUIRE(0 < ncplane_gradient(n_, "A", 0, chan1, chan2, chan1, chan2, 0, 3));
    CHECK(0 == notcurses_render(nc_));
    nccell d = CELL_TRIVIAL_INITIALIZER;
    CHECK(1 == ncplane_at_yx_cell(n_, 0, 0, &d));
    CHECK(chan1 == d.channels);
    REQUIRE(cell_simple_p(&d));
    CHECK(htole('A') == d.gcluster);
    CHECK(0 == ncplane_cursor_move_yx(n_, 0, 0));
    REQUIRE(0 < ncplane_gradient(n_, "A", 0, chan2, chan1, chan2, chan1, 0, 3));
    CHECK(1 == ncplane_at_yx_cell(n_, 0, 0, &d));
    REQUIRE(cell_simple_p(&d));
    CHECK(htole('A') == d.gcluster);
    CHECK(chan2 == d.channels);
  }

  // Unlike a typical gradient, a high gradient ought be able to do a vertical
  // change in a single row.
  SUBCASE("HighGradient2Colors1Row") {
    uint32_t ul, ur, ll, lr;
    ul = ur = ll = lr = 0;
    channel_set(&ul, 0xffffff);
    channel_set(&lr, 0x000000);
    channel_set(&ll, 0x00ffff);
    channel_set(&ur, 0xff00ff);
    int dimy, dimx;
    ncplane_dim_yx(n_, &dimy, &dimx);
    REQUIRE(0 < ncplane_highgradient_sized(n_, ul, ur, ll, lr, dimy, dimx));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("HighGradient") {
    uint32_t ul, ur, ll, lr;
    ul = ur = ll = lr = 0;
    channel_set(&ul, 0xffffff);
    channel_set(&lr, 0x000000);
    channel_set(&ll, 0x00ffff);
    channel_set(&ur, 0xff00ff);
    int dimy, dimx;
    ncplane_dim_yx(n_, &dimy, &dimx);
    REQUIRE(0 < ncplane_highgradient_sized(n_, ul, ur, ll, lr, dimy, dimx));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("MergeDownASCII") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 10,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* p1 = ncplane_create(n_, &nopts);
    REQUIRE(p1);
    // make sure glyphs replace nulls
    CHECK(0 < ncplane_putstr(p1, "0123456789"));
    CHECK(0 == ncplane_mergedown_simple(p1, n_));
    nccell cbase = CELL_TRIVIAL_INITIALIZER;
    nccell cp = CELL_TRIVIAL_INITIALIZER;
    for(int i = 0 ; i < 10 ; ++i){
      CHECK(0 < ncplane_at_yx_cell(n_, 0, i, &cbase));
      CHECK(0 < ncplane_at_yx_cell(p1, 0, i, &cp));
      CHECK(0 == cellcmp(n_, &cbase, p1, &cp));
    }
    CHECK(0 == ncplane_cursor_move_yx(p1, 0, 0));
    // make sure glyphs replace glyps
    CHECK(0 < ncplane_putstr(p1, "9876543210"));
    CHECK(0 == ncplane_mergedown_simple(p1, n_));
    for(int i = 0 ; i < 10 ; ++i){
      CHECK(0 < ncplane_at_yx_cell(n_, 0, i, &cbase));
      CHECK(0 < ncplane_at_yx_cell(p1, 0, i, &cp));
      CHECK(0 == cellcmp(n_, &cbase, p1, &cp));
    }
    // make sure nulls do not replace glyphs
    auto p2 = ncplane_create(n_, &nopts);
    CHECK(0 == ncplane_mergedown_simple(p2, n_));
    ncplane_destroy(p2);
    for(int i = 0 ; i < 10 ; ++i){
      CHECK(0 < ncplane_at_yx_cell(n_, 0, i, &cbase));
      CHECK(0 < ncplane_at_yx_cell(p1, 0, i, &cp));
      CHECK(0 == cellcmp(n_, &cbase, p1, &cp));
    }
    ncplane_destroy(p1);
  }

  SUBCASE("MergeDownUni") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 10,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto p1 = ncplane_create(n_, &nopts);
    REQUIRE(p1);
    // make sure glyphs replace nulls
    CHECK(0 < ncplane_putstr(p1, "█▀▄▌▐🞵🞶🞷🞸🞹"));
    CHECK(0 == ncplane_mergedown_simple(p1, n_));
    nccell cbase = CELL_TRIVIAL_INITIALIZER;
    nccell cp = CELL_TRIVIAL_INITIALIZER;
    for(int i = 0 ; i < 10 ; ++i){
      CHECK(0 < ncplane_at_yx_cell(n_, 0, i, &cbase));
      CHECK(0 < ncplane_at_yx_cell(p1, 0, i, &cp));
      CHECK(0 == cellcmp(n_, &cbase, p1, &cp));
    }
    ncplane_destroy(p1);
    CHECK(0 == notcurses_render(nc_));
    auto p3 = ncplane_create(n_, &nopts);
    CHECK(0 == ncplane_cursor_move_yx(p3, 0, 0));
    // make sure glyphs replace glyps
    CHECK(0 < ncplane_putstr(p3, "🞵🞶🞷🞸🞹█▀▄▌▐"));
    CHECK(0 == ncplane_mergedown_simple(p3, nullptr));
    nccell c3 = CELL_TRIVIAL_INITIALIZER;
    for(int i = 0 ; i < 10 ; ++i){
      CHECK(0 < ncplane_at_yx_cell(n_, 0, i, &cbase));
      CHECK(0 < ncplane_at_yx_cell(p3, 0, i, &c3));
      CHECK(0 == cellcmp(n_, &cbase, p3, &c3));
    }
    CHECK(0 == notcurses_render(nc_));
    // make sure nulls do not replace glyphs
    auto p2 = ncplane_create(n_, &nopts);
    CHECK(0 == ncplane_mergedown_simple(p2, nullptr));
    ncplane_destroy(p2);
    for(int i = 0 ; i < 10 ; ++i){
      CHECK(0 < ncplane_at_yx_cell(n_, 0, i, &cbase));
      CHECK(0 < ncplane_at_yx_cell(p3, 0, i, &c3));
      CHECK(0 == cellcmp(n_, &cbase, p3, &c3));
    }
    ncplane_destroy(p3);
    CHECK(0 == notcurses_render(nc_));
  }

  // test merging down one plane to another plane which is smaller than the
  // standard plane
  SUBCASE("MergeDownSmallPlane") {
    constexpr int DIMX = 10;
    constexpr int DIMY = 10;
    struct ncplane_options nopts = {
      .y = 2,
      .x = 2,
      .rows = DIMY,
      .cols = DIMX,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* p1 = ncplane_create(n_, &nopts);
    REQUIRE(p1);
    nccell c1 = CELL_TRIVIAL_INITIALIZER;
    CHECK(0 < cell_load(p1, &c1, "█"));
    CHECK(0 == cell_set_bg_rgb(&c1, 0x00ff00));
    CHECK(0 == cell_set_fg_rgb(&c1, 0x0000ff));
    CHECK(0 < ncplane_polyfill_yx(p1, 0, 0, &c1));
    CHECK(0 == notcurses_render(nc_));
    struct ncplane_options n2opts = {
      .y = 3,
      .x = 3,
      .rows = DIMY / 2,
      .cols = DIMX / 2,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto p2 = ncplane_create(n_, &n2opts);
    REQUIRE(p2);
    nccell c2 = CELL_TRIVIAL_INITIALIZER;
    CHECK(0 < cell_load(p2, &c2, "🞶"));
    CHECK(0 == cell_set_bg_rgb(&c2, 0x00ffff));
    CHECK(0 == cell_set_fg_rgb(&c2, 0xff00ff));
    CHECK(0 < ncplane_polyfill_yx(p2, 0, 0, &c2));
    CHECK(0 == ncplane_mergedown_simple(p2, p1));
    CHECK(0 == notcurses_render(nc_));
    for(int y = 0 ; y < DIMY ; ++y){
      for(int x = 0 ; x < DIMX ; ++x){
        CHECK(0 < ncplane_at_yx_cell(p1, y, x, &c1));
        if(y < 1 || y > 5 || x < 1 || x > 5){
          auto cstr = cell_strdup(p1, &c1);
          CHECK(0 == strcmp(cstr, "█"));
          free(cstr);
        }else{
          CHECK(0 < ncplane_at_yx_cell(p2, y - 1, x - 1, &c2));
          CHECK(0 == cellcmp(p1, &c1, p2, &c2));
        }
      }
    }
    ncplane_destroy(p1);
    ncplane_destroy(p2);
  }

  // test merging down one plane to another plane which is smaller than the
  // standard plane when using unicode
  SUBCASE("MergeDownSmallPlaneUni") {
    constexpr int DIMX = 10;
    constexpr int DIMY = 10;
    struct ncplane_options nopts = {
      .y = 2,
      .x = 2,
      .rows = DIMY,
      .cols = DIMX,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* p1 = ncplane_create(n_, &nopts);
    REQUIRE(p1);
    uint64_t ul = 0, ur = 0, bl = 0, br = 0;
    channels_set_fg_rgb(&ur, 0xff0000);
    channels_set_fg_rgb(&bl, 0x00ff00);
    channels_set_fg_rgb(&br, 0x0000ff);
    ncplane_highgradient_sized(p1, ul, ur, bl, br, DIMY, DIMX);
    CHECK(0 == notcurses_render(nc_));
    struct ncplane_options n2opts = {
      .y = 3,
      .x = 3,
      .rows = DIMY / 2,
      .cols = DIMX / 2,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto p2 = ncplane_create(n_, &n2opts);
    REQUIRE(p2);
    ncplane_highgradient_sized(p2, br, bl, ur, ul, DIMY / 2, DIMX / 2);
    CHECK(0 == ncplane_mergedown_simple(p2, p1));
    CHECK(0 == notcurses_render(nc_));
    for(int y = 0 ; y < DIMY ; ++y){
      for(int x = 0 ; x < DIMX ; ++x){
        nccell c1 = CELL_TRIVIAL_INITIALIZER;
        CHECK(0 < ncplane_at_yx_cell(p1, y, x, &c1));
        if(y < 1 || y > 5 || x < 1 || x > 5){
          auto cstr = cell_strdup(p1, &c1);
          CHECK(0 == strcmp(cstr, "▀"));
          free(cstr);
        }else{
          nccell c2 = CELL_TRIVIAL_INITIALIZER;
          CHECK(0 < ncplane_at_yx_cell(p2, y - 1, x - 1, &c2));
          CHECK(0 == cellcmp(p1, &c1, p2, &c2));
          cell_release(p2, &c2);
        }
        cell_release(p1, &c1);
      }
    }
    ncplane_destroy(p1);
    ncplane_destroy(p2);
  }

#ifdef USE_QRCODEGEN
  SUBCASE("QRCodes") {
    const char* qr = "a very simple qr code";
    int dimy, dimx;
    ncplane_dim_yx(n_, &dimy, &dimx);
    CHECK(0 < ncplane_qrcode(n_, &dimy, &dimx, qr, strlen(qr)));
    CHECK(0 == notcurses_render(nc_));
  }
#endif

  CHECK(0 == notcurses_stop(nc_));

}
