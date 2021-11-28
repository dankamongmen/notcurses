#include "main.h"
#include "lib/visual-details.h"
#include <vector>
#include <cmath>

// verify results for extrinsic geometries with NULL or default vopts
void default_visual_extrinsics(const notcurses* nc, const ncvgeom& g) {
  CHECK(0 == g.pixy);
  CHECK(0 == g.pixx);
  CHECK(nc->tcache.cellpxy == g.cdimy);
  CHECK(nc->tcache.cellpxx == g.cdimx);
  CHECK(1 <= g.scaley);
  CHECK(1 <= g.scalex);
  CHECK(0 == g.rpixy);
  CHECK(0 == g.rpixx);
  CHECK(0 <= g.maxpixely);
  CHECK(0 <= g.maxpixelx);
  // we never use pixel by default, and must not revolve to default
  CHECK(NCBLIT_PIXEL != g.blitter);
  CHECK(NCBLIT_DEFAULT != g.blitter);
}

TEST_CASE("Visual") {
  auto nc_ = testing_notcurses();
  REQUIRE(nullptr != nc_);
  ncplane* ncp_ = notcurses_stdplane(nc_);
  REQUIRE(ncp_);
  auto n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  // check that we properly populate RGB + A -> RGBA
  SUBCASE("VisualFromRGBPacked") {
    unsigned char rgb[] = "\x88\x77\x66\x55\x44\x33\x22\x11\x00\x99\xaa\xbb";
    unsigned char alpha = 0xff;
    auto ncv = ncvisual_from_rgb_packed(rgb, 2, 6, 2, alpha);
    REQUIRE(nullptr != ncv);
    for(int y = 0 ; y < 2 ; ++y){
      for(int x = 0 ; x < 2 ; ++x){
        uint32_t p;
        CHECK(0 == ncvisual_at_yx(ncv, y, x, &p));
        CHECK(ncpixel_r(p) == rgb[y * 6 + x * 3]);
        CHECK(ncpixel_g(p) == rgb[y * 6 + x * 3 + 1]);
        CHECK(ncpixel_b(p) == rgb[y * 6 + x * 3 + 2]);
        CHECK(ncpixel_a(p) == alpha);
      }
    }
    ncvisual_destroy(ncv);
  }

  // ncvisual_geom() with a NULL nc
  SUBCASE("VisualIntrinsicGeometry") {
    std::vector<uint32_t> v(20, 0xfffffffflu);
    auto ncv = ncvisual_from_rgba(v.data(), 2, 10 * sizeof(decltype(v)::value_type), 10);
    REQUIRE(nullptr != ncv);
    ncvgeom g{};
    CHECK(0 == ncvisual_geom(nullptr, ncv, nullptr, &g));
    ncvisual_destroy(ncv);
    CHECK(2 == g.pixy);
    CHECK(10 == g.pixx);
    CHECK(0 == g.cdimy);
    CHECK(0 == g.cdimx);
    CHECK(0 == g.rpixy);
    CHECK(0 == g.rpixx);
    CHECK(0 == g.scaley);
    CHECK(0 == g.scalex);
    CHECK(0 == g.maxpixely);
    CHECK(0 == g.maxpixelx);
    CHECK(NCBLIT_DEFAULT == g.blitter);
  }

  // ncvisual_geom() with a NULL ncvisual and NULL visual_options
  SUBCASE("VisualExtrinsicGeometryNULL") {
    ncvgeom g{};
    CHECK(0 == ncvisual_geom(nc_, nullptr, nullptr, &g));
    default_visual_extrinsics(nc_, g);
  }

  // ncvisual_geom() with a NULL ncvisual and default visual_options
  SUBCASE("VisualExtrinsicGeometryDefault") {
    ncvgeom g{};
    struct ncvisual_options vopts{};
    CHECK(0 == ncvisual_geom(nc_, nullptr, &vopts, &g));
    default_visual_extrinsics(nc_, g);
  }

  // ncvisual_geom() with a NULL ncvisual and NCBLIT_PIXEL requested
  SUBCASE("VisualExtrinsicGeometryPixel") {
    ncvgeom g{};
    struct ncvisual_options vopts{};
    vopts.blitter = NCBLIT_PIXEL;
    CHECK(0 == ncvisual_geom(nc_, nullptr, &vopts, &g));
    CHECK(0 == g.pixy);
    CHECK(0 == g.pixx);
    CHECK(nc_->tcache.cellpxy == g.cdimy);
    CHECK(nc_->tcache.cellpxx == g.cdimx);
    if(notcurses_canpixel(nc_)){
      CHECK(g.cdimy == g.scaley);
      CHECK(g.cdimx == g.scalex);
      CHECK(nc_->tcache.sixel_maxy <= g.maxpixely);
      CHECK(nc_->tcache.sixel_maxx <= g.maxpixelx);
    }else{
      CHECK(1 <= g.scaley);
      CHECK(1 <= g.scalex);
      CHECK(0 == g.maxpixely);
      CHECK(0 == g.maxpixelx);
    }
    CHECK(0 == g.rpixy);
    CHECK(0 == g.rpixx);
    CHECK(NCBLIT_DEFAULT != g.blitter); // we must not revolve to default
  }

  // build a simple ncvisual and check the calculated geometries for 1x1
  // cell blitting in the absence of scaling
  SUBCASE("VisualCellGeometryNoScaling") {
    std::vector<uint32_t> v(80, 0xfffffffflu);
    auto ncv = ncvisual_from_rgba(v.data(), 8, 10 * sizeof(decltype(v)::value_type), 10);
    REQUIRE(nullptr != ncv);
    struct ncvisual_options vopts{};
    ncvgeom g{};
    vopts.blitter = NCBLIT_1x1;
    CHECK(0 == ncvisual_geom(nc_, ncv, &vopts, &g));
    ncvisual_destroy(ncv);
    CHECK(8 == g.pixy);
    CHECK(10 == g.pixx);
    CHECK(1 == g.scaley);
    CHECK(1 == g.scalex);
    CHECK(8 == g.rpixy);
    CHECK(10 == g.rpixx);
    CHECK(8 == g.rcelly);
    CHECK(10 == g.rcellx);
    CHECK(NCBLIT_1x1 == g.blitter);
  }

  // build a square ncvisual and check the calculated geometries for 1x1
  // cell blitting with scaling
  SUBCASE("VisualCellGeometryScaling") {
    std::vector<uint32_t> v(100, 0xfffffffflu);
    auto ncv = ncvisual_from_rgba(v.data(), 10, 10 * sizeof(decltype(v)::value_type), 10);
    REQUIRE(nullptr != ncv);
    struct ncvisual_options vopts{};
    ncvgeom g{};
    vopts.blitter = NCBLIT_1x1;
    vopts.scaling = NCSCALE_SCALE;
    CHECK(0 == ncvisual_geom(nc_, ncv, &vopts, &g));
    ncvisual_destroy(ncv);
    unsigned dimy, dimx;
    ncplane_dim_yx(n_, &dimy, &dimx);
    unsigned mindim = dimy < dimx ? dimy : dimx;
    CHECK(10 == g.pixy);
    CHECK(10 == g.pixx);
    CHECK(1 == g.scaley);
    CHECK(1 == g.scalex);
    CHECK(mindim == g.rpixy);
    CHECK(mindim == g.rpixx);
    CHECK(mindim == g.rcelly);
    CHECK(mindim == g.rcellx);
    CHECK(NCBLIT_1x1 == g.blitter);
  }

  // build a square ncvisual and check the calculated geometries for 1x1
  // cell blitting with stretching
  SUBCASE("VisualCellGeometryStretching") {
    std::vector<uint32_t> v(100, 0xfffffffflu);
    auto ncv = ncvisual_from_rgba(v.data(), 10, 10 * sizeof(decltype(v)::value_type), 10);
    REQUIRE(nullptr != ncv);
    struct ncvisual_options vopts{};
    ncvgeom g{};
    vopts.blitter = NCBLIT_1x1;
    vopts.scaling = NCSCALE_STRETCH;
    CHECK(0 == ncvisual_geom(nc_, ncv, &vopts, &g));
    ncvisual_destroy(ncv);
    unsigned dimy, dimx;
    ncplane_dim_yx(n_, &dimy, &dimx);
    CHECK(10 == g.pixy);
    CHECK(10 == g.pixx);
    CHECK(1 == g.scaley);
    CHECK(1 == g.scalex);
    CHECK(dimy == g.rpixy);
    CHECK(dimx == g.rpixx);
    CHECK(dimy == g.rcelly);
    CHECK(dimx == g.rcellx);
    CHECK(NCBLIT_1x1 == g.blitter);
  }

  // check that we properly populate RGB + A -> RGBA from 35x4 (see #1806)
  SUBCASE("VisualFromRGBPacked35x4") {
    unsigned char rgb[4 * 35 * 3] = "";
    unsigned char alpha = 0xff;
    auto ncv = ncvisual_from_rgb_packed(rgb, 4, 35 * 3, 35, alpha);
    REQUIRE(nullptr != ncv);
    for(int y = 0 ; y < 4 ; ++y){
      for(int x = 0 ; x < 35 ; ++x){
        uint32_t p;
        CHECK(0 == ncvisual_at_yx(ncv, y, x, &p));
        CHECK(ncpixel_r(p) == rgb[y * 6 + x * 3]);
        CHECK(ncpixel_g(p) == rgb[y * 6 + x * 3 + 1]);
        CHECK(ncpixel_b(p) == rgb[y * 6 + x * 3 + 2]);
        CHECK(ncpixel_a(p) == alpha);
      }
    }
    ncvisual_destroy(ncv);
  }

  // check that we properly populate RGBx + A -> RGBA
  SUBCASE("VisualFromRGBxPacked") {
    unsigned char rgb[] = "\x88\x77\x66\x12\x55\x44\x33\x10\x22\x11\x00\xdd\x99\xaa\xbb\xcc";
    unsigned char alpha = 0xff;
    auto ncv = ncvisual_from_rgb_loose(rgb, 2, 8, 2, alpha);
    REQUIRE(nullptr != ncv);
    for(int y = 0 ; y < 2 ; ++y){
      for(int x = 0 ; x < 2 ; ++x){
        uint32_t p;
        CHECK(0 == ncvisual_at_yx(ncv, y, x, &p));
        CHECK(ncpixel_r(p) == rgb[y * 8 + x * 4]);
        CHECK(ncpixel_g(p) == rgb[y * 8 + x * 4 + 1]);
        CHECK(ncpixel_b(p) == rgb[y * 8 + x * 4 + 2]);
        CHECK(ncpixel_a(p) == alpha);
      }
    }
    ncvisual_destroy(ncv);
  }

  // resize followed by rotate, see #1800
  SUBCASE("ResizeThenRotateFromMemory") {
    unsigned char rgb[90];
    memset(rgb, 0, sizeof(rgb));
    auto ncv = ncvisual_from_rgb_packed(rgb, 3, 10 * 3, 10, 0xff);
    REQUIRE(nullptr != ncv);
    struct ncvisual_options vopts{};
    vopts.x = NCALIGN_CENTER;
    vopts.y = NCALIGN_CENTER;
    vopts.n = n_;
    vopts.flags |= NCVISUAL_OPTION_HORALIGNED | NCVISUAL_OPTION_VERALIGNED
                    | NCVISUAL_OPTION_CHILDPLANE;
    auto p = ncvisual_blit(nc_, ncv, &vopts);
    REQUIRE(nullptr != p);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(p));
    CHECK(0 == ncvisual_resize(ncv, 20, 20));
    p = ncvisual_blit(nc_, ncv, &vopts);
    REQUIRE(nullptr != p);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(p));
    CHECK(0 == ncvisual_rotate(ncv, M_PI / 2));
    p = ncvisual_blit(nc_, ncv, &vopts);
    REQUIRE(nullptr != p);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(p));
    ncvisual_destroy(ncv);
  }

  // check that NCVISUAL_OPTION_HORALIGNED works in all three cases
  SUBCASE("VisualAligned") {
    const uint32_t pixels[4] = { htole(0xffff0000), htole(0xff00ff00), htole(0xff0000ff), htole(0xffffffff) };
    ncvisual_options vopts = {
      .n = n_,
      .scaling = NCSCALE_NONE,
      .y = 0,
      .x = NCALIGN_LEFT,
      .begy = 0,
      .begx = 0,
      .leny = 2,
      .lenx = 2,
      .blitter = NCBLIT_1x1,
      .flags = NCVISUAL_OPTION_HORALIGNED | NCVISUAL_OPTION_CHILDPLANE,
      .transcolor = 0,
      .pxoffy = 0, .pxoffx = 0,
    };
    auto ncv = ncvisual_from_rgba(pixels, 2, 2 * sizeof(*pixels), 2);
    REQUIRE(nullptr != ncv);
    auto n = ncvisual_blit(nc_, ncv, &vopts);
    REQUIRE(nullptr != n);
    CHECK(0 == notcurses_render(nc_));
    ncvisual_destroy(ncv);
    CHECK(0 == ncplane_destroy(n));
  }

  // check that leny/lenx properly limit the output, new plane
  SUBCASE("Partial") {
    auto y = 10;
    auto x = 10;
    std::vector<uint32_t> v(x * y, htole(0xe61c28ff));
    auto ncv = ncvisual_from_rgba(v.data(), y, sizeof(decltype(v)::value_type) * x, x);
    REQUIRE(nullptr != ncv);
    struct ncvisual_options vopts = {
      .n = n_,
      .scaling = NCSCALE_NONE,
      .y = 0, .x = 0,
      .begy = 0, .begx = 0,
      .leny = 5, .lenx = 8,
      .blitter = NCBLIT_1x1,
      .flags = NCVISUAL_OPTION_CHILDPLANE,
      .transcolor = 0,
      .pxoffy = 0, .pxoffx = 0,
    };
    auto n = ncvisual_blit(nc_, ncv, &vopts);
    REQUIRE(nullptr != n);
    CHECK(5 == ncplane_dim_y(n));
    CHECK(8 == ncplane_dim_x(n));
    ncvisual_destroy(ncv);
    CHECK(0 == ncplane_destroy(n));
  }

  // ensure that NCSCALE_STRETCH gives us a full plane, and that we write
  // everywhere within that plane
  SUBCASE("Stretch") {
    std::vector<uint32_t> v(1, htole(0xff1c28ff));
    unsigned dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    auto ncv = ncvisual_from_rgba(v.data(), 1, sizeof(decltype(v)::value_type), 1);
    REQUIRE(nullptr != ncv);
    struct ncvisual_options vopts{};
    vopts.n = n_;
    vopts.scaling = NCSCALE_STRETCH;
    vopts.blitter = NCBLIT_1x1;
    vopts.flags = NCVISUAL_OPTION_CHILDPLANE;
    auto n = ncvisual_blit(nc_, ncv, &vopts);
    CHECK(0 == notcurses_render(nc_));
    REQUIRE(nullptr != n);
    CHECK(dimy == ncplane_dim_y(n));
    CHECK(dimx == ncplane_dim_x(n));
    ncvisual_destroy(ncv);
    for(unsigned y = 0 ; y < dimy ; ++y){
      for(unsigned x = 0 ; x < dimx ; ++x){
        uint16_t stylemask;
        uint64_t channels;
        auto c = ncplane_at_yx(n, y, x, &stylemask, &channels);
        CHECK(0 == strcmp(c, " "));
        free(c);
        CHECK(ncchannels_bg_rgb(channels) == 0xff281c);
        CHECK(stylemask == 0);
      }
    }
    CHECK(0 == ncplane_destroy(n));
  }

  // partial limit via len, offset via y/x, new plane
  SUBCASE("PartialOffset") {
    auto y = 10;
    auto x = 10;
    std::vector<uint32_t> v(x * y, htole(0xe61c28ff));
    auto ncv = ncvisual_from_rgba(v.data(), y, sizeof(decltype(v)::value_type) * x, x);
    REQUIRE(nullptr != ncv);
    struct ncvisual_options vopts = {
      .n = nullptr,
      .scaling = NCSCALE_NONE,
      .y = 2, .x = 4,
      .begy = 0, .begx = 0,
      .leny = 5, .lenx = 8,
      .blitter = NCBLIT_1x1,
      .flags = 0, .transcolor = 0,
      .pxoffy = 0, .pxoffx = 0,
    };
    auto n = ncvisual_blit(nc_, ncv, &vopts);
    REQUIRE(nullptr != n);
    CHECK(5 == ncplane_dim_y(n));
    CHECK(8 == ncplane_dim_x(n));
    CHECK(2 == ncplane_y(n));
    CHECK(4 == ncplane_x(n));
    ncvisual_destroy(ncv);
    CHECK(0 == ncplane_destroy(n));
  }

  SUBCASE("InflateBitmap") {
    const uint32_t pixels[4] = { htole(0xffff00ff), htole(0xff00ffff), htole(0xff0000ff), htole(0xffffffff) };
    auto ncv = ncvisual_from_rgba(pixels, 2, 8, 2);
    REQUIRE(ncv);
    ncvisual_options vopts = {
      .n = nullptr,
      .scaling = NCSCALE_NONE,
      .y = 0, .x = 0,
      .begy = 0, .begx = 0,
      .leny = 0, .lenx = 0,
      .blitter = NCBLIT_1x1,
      .flags = 0, .transcolor = 0,
      .pxoffy = 0, .pxoffx = 0,
    };
    auto newn = ncvisual_blit(nc_, ncv, &vopts);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncvisual_resize_noninterpolative(ncv, ncv->pixy * 3, ncv->pixx * 3));
    CHECK(6 == ncv->pixy);
    CHECK(6 == ncv->pixx);
    for(int y = 0 ; y < 3 ; ++y){
      for(int x = 0 ; x < 3 ; ++x){
        CHECK(pixels[0] == ncv->data[y * ncv->rowstride / 4 + x]);
      }
      for(int x = 3 ; x < 6 ; ++x){
        CHECK(pixels[1] == ncv->data[y * ncv->rowstride / 4 + x]);
      }
    }
    for(int y = 3 ; y < 6 ; ++y){
      for(int x = 0 ; x < 3 ; ++x){
        CHECK(pixels[2] == ncv->data[y * ncv->rowstride / 4 + x]);
      }
      for(int x = 3 ; x < 6 ; ++x){
        CHECK(pixels[3] == ncv->data[y * ncv->rowstride / 4 + x]);
      }
    }
    REQUIRE(newn);
    auto enewn = ncvisual_blit(nc_, ncv, &vopts);
    unsigned newy, newx;
    ncplane_dim_yx(enewn, &newy, &newx);
    CHECK(6 == newy);
    CHECK(6 == newx);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(newn));
    CHECK(0 == ncplane_destroy(enewn));
    ncvisual_destroy(ncv);
  }

  SUBCASE("LoadRGBAFromMemory") {
    unsigned dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    // alpha, then b, g, r
    std::vector<uint32_t> rgba(dimx * dimy * 2, htole(0xff88bbccull));
    auto ncv = ncvisual_from_rgba(rgba.data(), dimy * 2, dimx * 4, dimx);
    REQUIRE(ncv);
    struct ncvisual_options opts{};
    opts.blitter = NCBLIT_1x1;
    opts.n = ncp_;
    CHECK(ncp_ == ncvisual_blit(nc_, ncv, &opts));
    CHECK(0 == notcurses_render(nc_));
    for(unsigned y = 0 ; y < dimy ; ++y){
      for(unsigned x = 0 ; x < dimx ; ++x){
        uint16_t stylemask;
        uint64_t channels;
        auto c = ncplane_at_yx(ncp_, y, x, &stylemask, &channels);
        CHECK(0 == strcmp(c, " "));
        free(c);
        CHECK(htole(ncchannels_bg_rgb(channels)) == htole(0xccbb88));
        CHECK(stylemask == 0);
      }
    }
    ncvisual_destroy(ncv);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("LoadBGRAFromMemory") {
    unsigned dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    // A should be at the highest memory address, which would be the most
    // significant byte on little-endian. then r, g, b.
    std::vector<uint32_t> rgba(dimx * dimy * 2, htole(0xff88bbcc));
    auto ncv = ncvisual_from_bgra(rgba.data(), dimy * 2, dimx * 4, dimx);
    REQUIRE(ncv);
    struct ncvisual_options opts{};
    opts.blitter = NCBLIT_1x1;
    opts.n = ncp_;
    CHECK(nullptr != ncvisual_blit(nc_, ncv, &opts));
    CHECK(0 == notcurses_render(nc_));
    for(unsigned y = 0 ; y < dimy ; ++y){
      for(unsigned x = 0 ; x < dimx ; ++x){
        uint16_t stylemask;
        uint64_t channels;
        auto c = ncplane_at_yx(ncp_, y, x, &stylemask, &channels);
        CHECK(0 == strcmp(c, " "));
        free(c);
        CHECK(ncchannels_bg_rgb(channels) == 0x88bbcc);
        CHECK(stylemask == 0);
      }
    }
    ncvisual_destroy(ncv);
    CHECK(0 == notcurses_render(nc_));
  }

  // write a checkerboard pattern and verify the NCBLIT_2x1 output
  SUBCASE("Dualblitter") {
    if(notcurses_canutf8(nc_)){
      constexpr int DIMY = 10;
      constexpr int DIMX = 11; // odd number to get checkerboard effect
      auto rgba = new uint32_t[DIMY * DIMX];
      for(int i = 0 ; i < DIMY * DIMX ; ++i){
        CHECK(0 == ncpixel_set_a(&rgba[i], 0xff));
        if(i % 2){
          CHECK(0 == ncpixel_set_b(&rgba[i], 0xff));
          CHECK(0 == ncpixel_set_r(&rgba[i], 0));
        }else{
          CHECK(0 == ncpixel_set_r(&rgba[i], 0xff));
          CHECK(0 == ncpixel_set_b(&rgba[i], 0));
        }
        CHECK(0 == ncpixel_set_g(&rgba[i], 0));
      }
      auto ncv = ncvisual_from_rgba(rgba, DIMY, DIMX * sizeof(uint32_t), DIMX);
      REQUIRE(nullptr != ncv);
      struct ncvisual_options vopts{};
      vopts.n = n_;
      vopts.blitter = NCBLIT_2x1;
      vopts.flags = NCVISUAL_OPTION_NODEGRADE;
      CHECK(n_ == ncvisual_blit(nc_, ncv, &vopts));
      CHECK(0 == notcurses_render(nc_));
      for(int y = 0 ; y < DIMY / 2 ; ++y){
        for(int x = 0 ; x < DIMX ; ++x){
          uint16_t stylemask;
          uint64_t channels;
          char* egc = notcurses_at_yx(nc_, y, x, &stylemask, &channels);
          REQUIRE(nullptr != egc);
          CHECK((htole(rgba[y * 2 * DIMX + x]) & 0xffffff) == ncchannels_bg_rgb(channels));
          CHECK((htole(rgba[(y * 2 + 1) * DIMX + x]) & 0xffffff) == ncchannels_fg_rgb(channels));
          free(egc);
        }
      }
      delete[] rgba;
      ncvisual_destroy(ncv);
    }
  }

  // write a checkerboard pattern and verify the NCBLIT_2x2 output
  SUBCASE("Quadblitter") {
    if(notcurses_canquadrant(nc_)){
      constexpr int DIMY = 10;
      constexpr int DIMX = 11; // odd number to get checkerboard effect
      auto rgba = new uint32_t[DIMY * DIMX];
      for(int i = 0 ; i < DIMY * DIMX ; ++i){
        CHECK(0 == ncpixel_set_a(&rgba[i], 0xff));
        if(i % 2){
          CHECK(0 == ncpixel_set_b(&rgba[i], 0xff));
          CHECK(0 == ncpixel_set_g(&rgba[i], 0));
        }else{
          CHECK(0 == ncpixel_set_g(&rgba[i], 0xff));
          CHECK(0 == ncpixel_set_b(&rgba[i], 0));
        }
        CHECK(0 == ncpixel_set_r(&rgba[i], 0));
      }
      auto ncv = ncvisual_from_rgba(rgba, DIMY, DIMX * sizeof(uint32_t), DIMX);
      REQUIRE(nullptr != ncv);
      struct ncvisual_options vopts{};
      vopts.n = n_;
      vopts.blitter = NCBLIT_2x2;
      vopts.flags = NCVISUAL_OPTION_NODEGRADE;
      CHECK(n_ == ncvisual_blit(nc_, ncv, &vopts));
      CHECK(0 == notcurses_render(nc_));
      for(int y = 0 ; y < DIMY / 2 ; ++y){
        for(int x = 0 ; x < DIMX / 2 ; ++x){
          uint16_t stylemask;
          uint64_t channels;
          char* egc = notcurses_at_yx(nc_, y, x, &stylemask, &channels);
          REQUIRE(nullptr != egc);
          CHECK((htole(rgba[(y * 2 * DIMX) + (x * 2)]) & 0xffffff) == ncchannels_fg_rgb(channels));
          CHECK((htole(rgba[(y * 2 + 1) * DIMX + (x * 2) + 1]) & 0xffffff) == ncchannels_fg_rgb(channels));
          free(egc);
        }
      }
      delete[] rgba;
      ncvisual_destroy(ncv);
    }
  }

  // close-in verification of each quadblitter output EGC 
  SUBCASE("QuadblitterEGCs") {
    if(notcurses_canquadrant(nc_)){
      // there are 16 configurations, each mapping four (2x2) pixels
      int DIMX = 32;
      int DIMY = 2;
      auto rgba = new uint32_t[DIMY * DIMX];
      memset(rgba, 0, sizeof(*rgba) * DIMY * DIMX);
      // the top has 4 configurations of 4 each, each being 2 columns
      for(int top = 0 ; top < 4 ; ++top){
        for(int idx = 0 ; idx < 4 ; ++idx){
          const int itop = (top * 4 + idx) * 2; // index of first column
          CHECK(0 == ncpixel_set_a(&rgba[itop], 0xff));
          CHECK(0 == ncpixel_set_a(&rgba[itop + 1], 0xff));
          if(top == 1 || top == 3){
            CHECK(0 == ncpixel_set_r(&rgba[itop], 0xff));
          }
          if(top == 2 || top == 3){
            CHECK(0 == ncpixel_set_r(&rgba[itop + 1], 0xff));
          }
        }
      }
      for(int bot = 0 ; bot < 4 ; ++bot){
        for(int idx = 0 ; idx < 4 ; ++idx){
          const int ibot = (bot * 4 + idx) * 2 + DIMX;
          CHECK(0 == ncpixel_set_a(&rgba[ibot], 0xff));
          CHECK(0 == ncpixel_set_a(&rgba[ibot + 1], 0xff));
          if(idx == 1 || idx == 3){
            CHECK(0 == ncpixel_set_r(&rgba[ibot], 0xff));
          }
          if(idx == 2 || idx == 3){
            CHECK(0 == ncpixel_set_r(&rgba[ibot + 1], 0xff));
          }
        }
      }
      auto ncv = ncvisual_from_rgba(rgba, DIMY, DIMX * sizeof(uint32_t), DIMX);
      REQUIRE(nullptr != ncv);
      struct ncvisual_options vopts{};
      vopts.n = n_;
      vopts.blitter = NCBLIT_2x2;
      vopts.flags = NCVISUAL_OPTION_NODEGRADE;
      CHECK(n_ == ncvisual_blit(nc_, ncv, &vopts));
      CHECK(0 == notcurses_render(nc_));
      for(int y = 0 ; y < DIMY / 2 ; ++y){
        for(int x = 0 ; x < DIMX / 2 ; ++x){
          uint16_t stylemask;
          uint64_t channels;
          char* egc = notcurses_at_yx(nc_, y, x, &stylemask, &channels);
          REQUIRE(nullptr != egc);
  /* FIXME need to match
  [▀] 00000000 00000000
  [▜] 00000000 00ff0000
  [▛] 00000000 00ff0000
  [▀] 00000000 00ff0000
  [▟] 00000000 00ff0000
  [▋] 00ff0000 00000000
  [▚] 00ff0000 00000000
  [▙] 00ff0000 00000000
  [▙] 00000000 00ff0000
  [▚] 00000000 00ff0000
  [▋] 00000000 00ff0000
  [▟] 00ff0000 00000000
  [▀] 00ff0000 00000000
  [▛] 00ff0000 00000000
  [▜] 00ff0000 00000000
  [▀] 00ff0000 00ff0000
  */
          free(egc);
        }
      }
      delete[] rgba;
      ncvisual_destroy(ncv);
    }
  }

  // quadblitter with all 4 colors equal ought generate space
  SUBCASE("Quadblitter4Same") {
    if(notcurses_canquadrant(nc_)){
      const uint32_t pixels[4] = { htole(0xff605040), htole(0xff605040), htole(0xff605040), htole(0xff605040) };
      auto ncv = ncvisual_from_rgba(pixels, 2, 2 * sizeof(*pixels), 2);
      REQUIRE(nullptr != ncv);
      struct ncvisual_options vopts = {
        .n = n_,
        .scaling = NCSCALE_NONE,
        .y = 0,
        .x = 0,
        .begy = 0,
        .begx = 0,
        .leny = 0,
        .lenx = 0,
        .blitter = NCBLIT_2x2,
        .flags = NCVISUAL_OPTION_CHILDPLANE,
        .transcolor = 0,
        .pxoffy = 0, .pxoffx = 0,
      };
      auto ncvp = ncvisual_blit(nc_, ncv, &vopts);
      REQUIRE(nullptr != ncvp);
      unsigned dimy, dimx;
      ncplane_dim_yx(ncvp, &dimy, &dimx);
      CHECK(1 == dimy);
      CHECK(1 == dimx);
      uint16_t stylemask;
      uint64_t channels;
      auto egc = ncplane_at_yx(ncvp, 0, 0, &stylemask, &channels);
      CHECK(0 == strcmp(" ", egc));
      CHECK(0 == stylemask);
      CHECK(0x405060 == ncchannels_fg_rgb(channels));
      CHECK(0x405060 == ncchannels_bg_rgb(channels));
      free(egc);
      ncvisual_destroy(ncv);
      CHECK(0 == notcurses_render(nc_));
    }
  }

  // quadblitter with three pixels equal ought generate three-quarter block
  SUBCASE("Quadblitter3Same") {
    if(notcurses_canquadrant(nc_)){
      const uint32_t pixels[4][4] = {
        { htole(0xffcccccc), htole(0xff605040), htole(0xff605040), htole(0xff605040) },
        { htole(0xff605040), htole(0xffcccccc), htole(0xff605040), htole(0xff605040) },
        { htole(0xff605040), htole(0xff605040), htole(0xffcccccc), htole(0xff605040) },
        { htole(0xff605040), htole(0xff605040), htole(0xff605040), htole(0xffcccccc) } };
      const char* egcs[] = { "▟", "▙", "▜", "▛" };
      for(int i = 0 ; i < 4 ; ++i){
        auto ncv = ncvisual_from_rgba(pixels[i], 2, 2 * sizeof(**pixels), 2);
        REQUIRE(nullptr != ncv);
        struct ncvisual_options vopts = {
          .n = n_,
          .scaling = NCSCALE_NONE,
          .y = 0,
          .x = 0,
          .begy = 0,
          .begx = 0,
          .leny = 0,
          .lenx = 0,
          .blitter = NCBLIT_2x2,
          .flags = NCVISUAL_OPTION_CHILDPLANE,
          .transcolor = 0,
          .pxoffy = 0, .pxoffx = 0,
        };
        auto ncvp = ncvisual_blit(nc_, ncv, &vopts);
        REQUIRE(nullptr != ncvp);
        unsigned dimy, dimx;
        ncplane_dim_yx(ncvp, &dimy, &dimx);
        CHECK(1 == dimy);
        CHECK(1 == dimx);
        uint16_t stylemask;
        uint64_t channels;
        auto egc = ncplane_at_yx(ncvp, 0, 0, &stylemask, &channels);
        CHECK(0 == strcmp(egcs[i], egc));
        CHECK(0 == stylemask);
        CHECK(0x405060 == ncchannels_fg_rgb(channels));
        CHECK(0xcccccc == ncchannels_bg_rgb(channels));
        free(egc);
        ncvisual_destroy(ncv);
        CHECK(0 == notcurses_render(nc_));
      }
    }
  }

  // quadblitter with two sets of two equal pixels
  SUBCASE("Quadblitter2Pairs") {
    if(notcurses_canquadrant(nc_)){
      const uint32_t pixels[6][4] = {
        { htole(0xffcccccc), htole(0xffcccccc), htole(0xff605040), htole(0xff605040) },
        { htole(0xffcccccc), htole(0xff605040), htole(0xffcccccc), htole(0xff605040) },
        { htole(0xffcccccc), htole(0xff605040), htole(0xff605040), htole(0xffcccccc) },
        { htole(0xff605040), htole(0xffcccccc), htole(0xffcccccc), htole(0xff605040) },
        { htole(0xff605040), htole(0xffcccccc), htole(0xff605040), htole(0xffcccccc) },
        { htole(0xff605040), htole(0xff605040), htole(0xffcccccc), htole(0xffcccccc) } };
      const char* egcs[] = { "▀", "▌", "▚", "▚", "▌", "▀" };
      for(size_t i = 0 ; i < sizeof(egcs) / sizeof(*egcs) ; ++i){
        auto ncv = ncvisual_from_rgba(pixels[i], 2, 2 * sizeof(**pixels), 2);
        REQUIRE(nullptr != ncv);
        struct ncvisual_options vopts = {
          .n = n_,
          .scaling = NCSCALE_NONE,
          .y = 0,
          .x = 0,
          .begy = 0,
          .begx = 0,
          .leny = 0,
          .lenx = 0,
          .blitter = NCBLIT_2x2,
          .flags = NCVISUAL_OPTION_CHILDPLANE,
          .transcolor = 0,
          .pxoffy = 0, .pxoffx = 0,
        };
        auto ncvp = ncvisual_blit(nc_, ncv, &vopts);
        REQUIRE(nullptr != ncvp);
        unsigned dimy, dimx;
        ncplane_dim_yx(ncvp, &dimy, &dimx);
        CHECK(1 == dimy);
        CHECK(1 == dimx);
        uint16_t stylemask;
        uint64_t channels;
        auto egc = ncplane_at_yx(ncvp, 0, 0, &stylemask, &channels);
        CHECK(0 == strcmp(egcs[i], egc));
        CHECK(0 == stylemask);
        if(i >= 3){
          CHECK(0x405060 == ncchannels_fg_rgb(channels));
          CHECK(0xcccccc == ncchannels_bg_rgb(channels));
        }else{
          CHECK(0x405060 == ncchannels_bg_rgb(channels));
          CHECK(0xcccccc == ncchannels_fg_rgb(channels));
        }
        free(egc);
        ncvisual_destroy(ncv);
        CHECK(0 == notcurses_render(nc_));
      }
    }
  }

  // quadblitter with one pair plus two split
  SUBCASE("Quadblitter1Pair") {
    if(notcurses_canquadrant(nc_)){
      const uint32_t pixels[6][4] = {
        { htole(0xffcccccc), htole(0xff444444), htole(0xff605040), htole(0xff605040) },
        { htole(0xff444444), htole(0xff605040), htole(0xffcccccc), htole(0xff605040) },
        { htole(0xffcccccc), htole(0xff605040), htole(0xff605040), htole(0xff444444) },
        { htole(0xff605040), htole(0xffcccccc), htole(0xff444444), htole(0xff605040) },
        { htole(0xff605040), htole(0xffeeeeee), htole(0xff605040), htole(0xffcccccc) },
        { htole(0xff605040), htole(0xff605040), htole(0xffeeeeee), htole(0xffcccccc) } };
      const char* egcs[] = { "▟", "▜", "▟", "▙", "▌", "▀" };
      for(size_t i = 0 ; i < sizeof(egcs) / sizeof(*egcs) ; ++i){
        auto ncv = ncvisual_from_rgba(pixels[i], 2, 2 * sizeof(**pixels), 2);
        REQUIRE(nullptr != ncv);
        struct ncvisual_options vopts = {
          .n = n_,
          .scaling = NCSCALE_NONE,
          .y = 0,
          .x = 0,
          .begy = 0,
          .begx = 0,
          .leny = 0,
          .lenx = 0,
          .blitter = NCBLIT_2x2,
          .flags = NCVISUAL_OPTION_CHILDPLANE,
          .transcolor = 0,
          .pxoffy = 0, .pxoffx = 0,
        };
        auto ncvp = ncvisual_blit(nc_, ncv, &vopts);
        REQUIRE(nullptr != ncvp);
        unsigned dimy, dimx;
        ncplane_dim_yx(ncvp, &dimy, &dimx);
        CHECK(1 == dimy);
        CHECK(1 == dimx);
        uint16_t stylemask;
        uint64_t channels;
        auto egc = ncplane_at_yx(ncvp, 0, 0, &stylemask, &channels);
        CHECK(0 == strcmp(egcs[i], egc));
        CHECK(0 == stylemask);
        if(i > 3){
          CHECK(0x405060 == ncchannels_fg_rgb(channels));
          CHECK(0xdddddd == ncchannels_bg_rgb(channels));
        }else{
          CHECK(0x424c57 == ncchannels_fg_rgb(channels));
          CHECK(0xcccccc == ncchannels_bg_rgb(channels));
        }
        free(egc);
        ncvisual_destroy(ncv);
        CHECK(0 == notcurses_render(nc_));
      }
    }
  }

  // quadblitter with one pair plus two split
  SUBCASE("QuadblitterAllDifferent") {
    if(notcurses_canquadrant(nc_)){
      const uint32_t pixels[6][4] = {
        { htole(0xffdddddd), htole(0xff000000), htole(0xff111111), htole(0xff222222) },
        { htole(0xff000000), htole(0xff111111), htole(0xffdddddd), htole(0xff222222) },
        { htole(0xff111111), htole(0xffdddddd), htole(0xff000000), htole(0xff222222) },
        { htole(0xff000000), htole(0xffcccccc), htole(0xff222222), htole(0xffeeeeee) },
        { htole(0xff222222), htole(0xff000000), htole(0xffeeeeee), htole(0xffcccccc), } };
      const char* egcs[] = { "▟", "▜", "▙", "▌", "▀" };
      for(size_t i = 0 ; i < sizeof(egcs) / sizeof(*egcs) ; ++i){
        auto ncv = ncvisual_from_rgba(pixels[i], 2, 2 * sizeof(**pixels), 2);
        REQUIRE(nullptr != ncv);
        struct ncvisual_options vopts = {
          .n = n_,
          .scaling = NCSCALE_NONE,
          .y = 0,
          .x = 0,
          .begy = 0,
          .begx = 0,
          .leny = 0,
          .lenx = 0,
          .blitter = NCBLIT_2x2,
          .flags = NCVISUAL_OPTION_CHILDPLANE,
          .transcolor = 0,
          .pxoffy = 0, .pxoffx = 0,
        };
        auto ncvp = ncvisual_blit(nc_, ncv, &vopts);
        REQUIRE(nullptr != ncvp);
        unsigned dimy, dimx;
        ncplane_dim_yx(ncvp, &dimy, &dimx);
        CHECK(1 == dimy);
        CHECK(1 == dimx);
        uint16_t stylemask;
        uint64_t channels;
        auto egc = ncplane_at_yx(ncvp, 0, 0, &stylemask, &channels);
        CHECK(0 == strcmp(egcs[i], egc));
        CHECK(0 == stylemask);
        CHECK(0x111111 == ncchannels_fg_rgb(channels));
        CHECK(0xdddddd == ncchannels_bg_rgb(channels));
        free(egc);
        ncvisual_destroy(ncv);
        CHECK(0 == notcurses_render(nc_));
      }
    }
  }

  // test NCVISUAL_OPTION_CHILDPLANE + stretch + null alignment
  SUBCASE("ImageChildScaling") {
    struct ncplane_options opts = {
      .y = 0, .x = 0,
      .rows = 20, .cols = 20,
      .userptr = nullptr,
      .name = "parent",
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0,
      .margin_r = 0,
    };
    auto parent = ncplane_create(n_, &opts);
    REQUIRE(parent);
    struct ncvisual_options vopts = {
      .n = n_,
      .scaling = NCSCALE_NONE,
      .y = 0,
      .x = 0,
      .begy = 0, .begx = 0,
      .leny = 0, .lenx = 0,
      .blitter = NCBLIT_1x1,
      .flags = NCVISUAL_OPTION_CHILDPLANE,
      .transcolor = 0,
      .pxoffy = 0, .pxoffx = 0,
    };
    const uint32_t pixels[16] = {
      htole(0xffffffff), htole(0xffffffff), htole(0xffc0ffff), htole(0xffffc0ff),
      htole(0xffc0c0ff), htole(0xffc0c0ff), htole(0xff80c0ff), htole(0xffc080ff),
      htole(0xff8080ff), htole(0xff8080ff), htole(0xff4080ff), htole(0xff8040ff),
      htole(0xff4040ff), htole(0xff4040ff), htole(0xffff40ff), htole(0xff40ffff),
    };
    auto ncv = ncvisual_from_rgba(pixels, 4, 16, 4);
    REQUIRE(ncv);
    auto child = ncvisual_blit(nc_, ncv, &vopts);
    REQUIRE(child);
    CHECK(4 == ncplane_dim_y(child));
    CHECK(4 == ncplane_dim_x(child));
    CHECK(0 == ncplane_y(child));
    CHECK(0 == ncplane_x(child));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(child));
    vopts.n = parent,
    vopts.scaling = NCSCALE_STRETCH,
    vopts.flags = NCVISUAL_OPTION_CHILDPLANE;
    child = ncvisual_blit(nc_, ncv, &vopts);
    REQUIRE(child);
    CHECK(20 == ncplane_dim_y(child));
    CHECK(20 == ncplane_dim_x(child));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(parent));
    CHECK(0 == ncplane_destroy(child));
    ncvisual_destroy(ncv);
  }

  SUBCASE("ImageChildAlignment") {
    struct ncplane_options opts = {
      .y = 0, .x = 0,
      .rows = 5, .cols = 5,
      .userptr = nullptr,
      .name = "parent",
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0,
      .margin_r = 0,
    };
    auto parent = ncplane_create(n_, &opts);
    REQUIRE(parent);
    struct ncvisual_options vopts = {
      .n = parent,
      .scaling = NCSCALE_NONE,
      .y = NCALIGN_CENTER,
      .x = NCALIGN_CENTER,
      .begy = 0, .begx = 0,
      .leny = 0, .lenx = 0,
      .blitter = NCBLIT_1x1,
      .flags = NCVISUAL_OPTION_CHILDPLANE |
               NCVISUAL_OPTION_HORALIGNED |
               NCVISUAL_OPTION_VERALIGNED,
      .transcolor = 0,
      .pxoffy = 0, .pxoffx = 0,
    };
    const uint32_t pixels[1] = { htole(0xffffffff) };
    auto ncv = ncvisual_from_rgba(pixels, 1, 4, 1);
    REQUIRE(ncv);
    auto child = ncvisual_blit(nc_, ncv, &vopts);
    REQUIRE(child);
    CHECK(1 == ncplane_dim_y(child));
    CHECK(1 == ncplane_dim_x(child));
    CHECK(2 == ncplane_y(child));
    CHECK(2 == ncplane_x(child));
    ncvisual_destroy(ncv);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(parent));
    CHECK(0 == ncplane_destroy(child));
  }

  CHECK(!notcurses_stop(nc_));
}
