#include "main.h"

// FIXME broken on big-endian, need turn WARNs back to CHECKs
// https://github.com/dankamongmen/notcurses/issues/1130
TEST_CASE("Blitting") {
  auto nc_ = testing_notcurses();
  REQUIRE(nullptr != nc_);
  ncplane* ncp_ = notcurses_stdplane(nc_);
  REQUIRE(ncp_);
  auto n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  SUBCASE("BgraToRgba") {
    const uint32_t data[8] = {
      // bgra (BE): RGBA bgra (LE): ABGR
      0xffffffff, 0xff0088ff, 0xffff8800, 0xff88ff00,
      0xffff0088, 0xff8800ff, 0xff00ff88, 0xff000000,
    };
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 2,
      .cols = 4,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
    };
    auto ncp = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != ncp);
    struct ncvisual_options vopts = {
      .n = ncp,
      .scaling = NCSCALE_NONE,
      .y = 0,
      .x = 0,
      .begy = 0,
      .begx = 0,
      .leny = 2,
      .lenx = 4,
      .blitter = NCBLIT_1x1,
      .flags = 0,
    };
    ncblit_bgrx(data, 16, &vopts);
    for(int y = 0 ; y < 2 ; ++y){
      for(int x = 0 ; x < 4 ; ++x){
        uint32_t bgra = data[(y * 4) + x];
        uint16_t stylemask;
        uint64_t channels;
        auto egc = ncplane_at_yx(ncp, y, x, &stylemask, &channels);
        WARN(0 == strcmp(" ", egc));
        free(egc);
        WARN(0 == stylemask);
        uint32_t rgb = channels_bg_rgb(channels);
        WARN(ncpixel_r(bgra) == ncpixel_r(rgb));
        WARN(ncpixel_g(bgra) == ncpixel_g(rgb));
        WARN(ncpixel_b(bgra) == ncpixel_b(rgb));
      }
    }
    ncplane_destroy(ncp);
  }

  SUBCASE("BgraToRgbaWithStride") {
    const uint32_t data[10] = {
      0xffffffff, 0xff0088ff, 0xffff8800, 0xff88ff00, 0x00000000,
      0xffff0088, 0xff8800ff, 0xff00ff88, 0xff000000, 0x00000000,
    };
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 2,
      .cols = 4,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
    };
    auto ncp = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != ncp);
    struct ncvisual_options vopts = {
      .n = ncp,
      .scaling = NCSCALE_NONE,
      .y = 0,
      .x = 0,
      .begy = 0,
      .begx = 0,
      .leny = 2,
      .lenx = 4,
      .blitter = NCBLIT_1x1,
      .flags = 0,
    };
    ncblit_bgrx(data, 20, &vopts);
    for(int y = 0 ; y < 2 ; ++y){
      for(int x = 0 ; x < 4 ; ++x){
        uint32_t bgra = data[(y * 5) + x];
        uint16_t stylemask;
        uint64_t channels;
        auto egc = ncplane_at_yx(ncp, y, x, &stylemask, &channels);
        WARN(0 == strcmp(" ", egc));
        free(egc);
        WARN(0 == stylemask);
        uint32_t rgb = channels_bg_rgb(channels);
        WARN(ncpixel_r(bgra) == ncpixel_r(rgb));
        WARN(ncpixel_g(bgra) == ncpixel_g(rgb));
        WARN(ncpixel_b(bgra) == ncpixel_b(rgb));
      }
    }
    ncplane_destroy(ncp);
  }

  CHECK(!notcurses_stop(nc_));
}
