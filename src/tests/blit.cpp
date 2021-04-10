#include "main.h"

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
      .margin_b = 0, .margin_r = 0,
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
      .transcolor = 0,
    };
    ncblit_bgrx(data, 16, &vopts);
    for(int y = 0 ; y < 2 ; ++y){
      for(int x = 0 ; x < 4 ; ++x){
        uint32_t bgra = data[(y * 4) + x];
        uint16_t stylemask;
        uint64_t channels;
        auto egc = ncplane_at_yx(ncp, y, x, &stylemask, &channels);
        CHECK(0 == strcmp(" ", egc));
        free(egc);
        CHECK(0 == stylemask);
        uint32_t rgb = htole(channels_bg_rgb(channels));
        CHECK(ncpixel_r(bgra) == ncpixel_r(rgb));
        CHECK(ncpixel_g(bgra) == ncpixel_g(rgb));
        CHECK(ncpixel_b(bgra) == ncpixel_b(rgb));
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
      .margin_b = 0, .margin_r = 0,
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
      .transcolor = 0,
    };
    ncblit_bgrx(data, 20, &vopts);
    for(int y = 0 ; y < 2 ; ++y){
      for(int x = 0 ; x < 4 ; ++x){
        uint32_t bgra = data[(y * 5) + x];
        uint16_t stylemask;
        uint64_t channels;
        auto egc = ncplane_at_yx(ncp, y, x, &stylemask, &channels);
        CHECK(0 == strcmp(" ", egc));
        free(egc);
        CHECK(0 == stylemask);
        uint32_t rgb = htole(channels_bg_rgb(channels));
        CHECK(ncpixel_r(bgra) == ncpixel_r(rgb));
        CHECK(ncpixel_g(bgra) == ncpixel_g(rgb));
        CHECK(ncpixel_b(bgra) == ncpixel_b(rgb));
      }
    }
    ncplane_destroy(ncp);
  }

  // addresses a case with quadblitter that was done incorrectly with the
  // original version https://github.com/dankamongmen/notcurses/issues/1354
  SUBCASE("QuadblitterMax") {
    if(notcurses_canutf8(nc_)){
      if(nc_->tcache.quadrants){
        uint32_t p2x2[4];
        uint32_t* ptl = &p2x2[0];
        ncpixel_set_a(ptl, 0xff);
        ncpixel_set_r(ptl, 0);
        ncpixel_set_g(ptl, 0);
        ncpixel_set_b(ptl, 0);
        uint32_t* ptr = &p2x2[1];
        ncpixel_set_a(ptr, 0xff);
        ncpixel_set_r(ptr, 0x43);
        ncpixel_set_g(ptr, 0x46);
        ncpixel_set_b(ptr, 0x43);
        uint32_t* pbl = &p2x2[2];
        ncpixel_set_a(pbl, 0xff);
        ncpixel_set_r(pbl, 0x4c);
        ncpixel_set_g(pbl, 0x50);
        ncpixel_set_b(pbl, 0x51);
        uint32_t* pbr = &p2x2[3];
        ncpixel_set_a(pbr, 0xff);
        ncpixel_set_r(pbr, 0x90);
        ncpixel_set_g(pbr, 0x94);
        ncpixel_set_b(pbr, 0x95);
        auto ncv = ncvisual_from_rgba(p2x2, 2, 8, 2);
        REQUIRE(nullptr != ncv);
        struct ncvisual_options vopts = {
          .n = nullptr, .scaling = NCSCALE_NONE,
          .y = 0, .x = 0, .begy = 0, .begx = 0, .leny = 0, .lenx = 0,
          .blitter = NCBLIT_2x2, .flags = 0,
          .transcolor = 0,
        };
        auto ncp = ncvisual_render(nc_, ncv, &vopts);
        ncvisual_destroy(ncv);
        REQUIRE(nullptr != ncp);
        CHECK(0 == notcurses_render(nc_));
        ncplane_destroy(ncp);
        uint64_t channels;
        uint16_t stylemask;
        auto egc = notcurses_at_yx(nc_, 0, 0, &stylemask, &channels);
        REQUIRE(nullptr != egc);
        CHECK(0 == strcmp(egc, "▟"));
        CHECK(0 == stylemask);
        CHECK(0x4060646340000000 == channels);
        free(egc);
      }
    }
  }

  CHECK(!notcurses_stop(nc_));
}
