#include "main.h"
#include <notcurses/direct.h>

TEST_CASE("Direct") {
  struct ncdirect* nc_ = ncdirect_init(NULL, stdout, 0);
  if(!nc_){
    return;
  }

  SUBCASE("SetItalic") {
    unsigned styles = ncdirect_supported_styles(nc_);
    if(styles & NCSTYLE_ITALIC){
      CHECK(0 == ncdirect_set_styles(nc_, NCSTYLE_ITALIC));
    }else{
      CHECK(0 != ncdirect_set_styles(nc_, NCSTYLE_ITALIC));
    }
    printf("DirectMode *italic*!\n");
    fflush(stdout);
    if(styles & NCSTYLE_ITALIC){
      CHECK(0 == ncdirect_off_styles(nc_, NCSTYLE_ITALIC));
    }else{
      CHECK(0 == ncdirect_off_styles(nc_, NCSTYLE_ITALIC));
    }
  }

  SUBCASE("SetBold") {
    unsigned styles = ncdirect_supported_styles(nc_);
    if(styles & NCSTYLE_BOLD){
      CHECK(0 == ncdirect_set_styles(nc_, NCSTYLE_BOLD));
      printf("DirectMode *bold*!\n");
      fflush(stdout);
      CHECK(0 == ncdirect_off_styles(nc_, NCSTYLE_BOLD));
    }
  }

  SUBCASE("SetUnderline") {
    unsigned styles = ncdirect_supported_styles(nc_);
    if(styles & NCSTYLE_UNDERLINE){
      CHECK(0 == ncdirect_set_styles(nc_, NCSTYLE_UNDERLINE));
      printf("DirectMode *underline*!\n");
      fflush(stdout);
      CHECK(0 == ncdirect_off_styles(nc_, NCSTYLE_UNDERLINE));
    }
  }

  SUBCASE("SetUndercurl") {
    unsigned styles = ncdirect_supported_styles(nc_);
    if(styles & NCSTYLE_UNDERCURL){
      CHECK(0 == ncdirect_set_styles(nc_, NCSTYLE_UNDERCURL));
      printf("DirectMode *undercurl*!\n");
      fflush(stdout);
      CHECK(0 == ncdirect_off_styles(nc_, NCSTYLE_UNDERCURL));
    }
  }

  SUBCASE("SetStruck") {
    unsigned styles = ncdirect_supported_styles(nc_);
    if(styles & NCSTYLE_STRUCK){
      CHECK(0 == ncdirect_set_styles(nc_, NCSTYLE_STRUCK));
      printf("DirectMode *struck*!\n");
      fflush(stdout);
      CHECK(0 == ncdirect_off_styles(nc_, NCSTYLE_STRUCK));
    }
  }

  SUBCASE("BoxDefault") {
    uint64_t chans = NCCHANNELS_INITIALIZER(255, 0, 255, 0, 0, 0);
    ncchannels_set_bg_default(&chans);
    ncdirect_set_bg_rgb8(nc_, 0x88, 0x88, 0x88);
    printf("test background\n");
    for(unsigned r = 2 ; r < 8 ; ++r){
      CHECK(0 == ncdirect_ascii_box(nc_, chans, chans, chans, chans, r, r, 0));
      printf("\n");
    }
  }

#ifndef NOTCURSES_USE_MULTIMEDIA
  SUBCASE("VisualDisabled"){
    CHECK(!ncdirect_canopen_images(nc_));
  }
#else
  SUBCASE("ImagesEnabled"){
    CHECK(ncdirect_canopen_images(nc_));
  }

  SUBCASE("LoadImage") {
    CHECK(0 == ncdirect_render_image(nc_, find_data("changes.jpg").get(), NCALIGN_LEFT, NCBLIT_1x1, NCSCALE_STRETCH));
    CHECK(0 == ncdirect_render_image(nc_, find_data("worldmap.png").get(), NCALIGN_RIGHT, NCBLIT_1x1, NCSCALE_SCALE));
  }

  SUBCASE("LoadSprixel") {
    if(ncdirect_check_pixel_support(nc_) > 0){
      CHECK(0 == ncdirect_render_image(nc_, find_data("changes.jpg").get(), NCALIGN_LEFT, NCBLIT_PIXEL, NCSCALE_STRETCH));
      CHECK(0 == ncdirect_render_image(nc_, find_data("worldmap.png").get(), NCALIGN_RIGHT, NCBLIT_PIXEL, NCSCALE_SCALE));
    }
  }

  SUBCASE("ImageGeom") {
    auto dirf = ncdirectf_from_file(nc_, find_data("worldmap.png").get());
    REQUIRE(nullptr != dirf);
    ncvgeom geom;
    CHECK(0 == ncdirectf_geom(nc_, dirf, nullptr, &geom));
    CHECK(475 == geom.pixy);
    CHECK(860 == geom.pixx);
    CHECK(NCBLIT_DEFAULT != geom.blitter);
    auto ncdv = ncdirectf_render(nc_, dirf, nullptr);
    CHECK(nullptr != ncdv);
    CHECK(0 == ncdirect_raster_frame(nc_, ncdv, NCALIGN_LEFT));
    ncdirectf_free(dirf);
  }

  SUBCASE("SprixelGeom") {
    if(ncdirect_check_pixel_support(nc_) > 0){
      auto dirf = ncdirectf_from_file(nc_, find_data("worldmap.png").get());
      REQUIRE(nullptr != dirf);
      struct ncvisual_options vopts{};
      vopts.blitter = NCBLIT_PIXEL;
      ncvgeom geom;
      CHECK(0 == ncdirectf_geom(nc_, dirf, &vopts, &geom));
      CHECK(475 == geom.pixy);
      CHECK(860 == geom.pixx);
      CHECK(NCBLIT_PIXEL == geom.blitter);
      CHECK(geom.cdimy == geom.scaley);
      CHECK(geom.cdimx == geom.scalex);
      auto ncdv = ncdirectf_render(nc_, dirf, &vopts);
      CHECK(nullptr != ncdv);
      CHECK(0 == ncdirect_raster_frame(nc_, ncdv, NCALIGN_LEFT));
      ncdirectf_free(dirf);
    }
  }

  SUBCASE("CursorPostCellBlit") {
    if(ncdirect_canget_cursor(nc_)){
      auto dirf = ncdirectf_from_file(nc_, find_data("worldmap.png").get());
      REQUIRE(nullptr != dirf);
      struct ncvisual_options vopts{};
      vopts.blitter = NCBLIT_1x1;
      auto ncdv = ncdirectf_render(nc_, dirf, &vopts);
      CHECK(nullptr != ncdv);
      CHECK(0 == ncdirect_raster_frame(nc_, ncdv, NCALIGN_LEFT));
      ncdirectf_free(dirf);
      unsigned y, x;
      // FIXME fails if u6 is reversed (on e.g. kmscon)
      CHECK(0 == ncdirect_cursor_yx(nc_, &y, &x));
      auto dimy = ncdirect_dim_y(nc_);
      auto dimx = ncdirect_dim_x(nc_);
      CHECK(dimy > y);
      CHECK(dimx > x);
    }
  }

  SUBCASE("CursorPostSprixel") {
    if(ncdirect_canget_cursor(nc_)){
      if(ncdirect_check_pixel_support(nc_) > 0){
        auto dirf = ncdirectf_from_file(nc_, find_data("worldmap.png").get());
        REQUIRE(nullptr != dirf);
        struct ncvisual_options vopts{};
        vopts.blitter = NCBLIT_PIXEL;
        vopts.flags = NCVISUAL_OPTION_NODEGRADE;
        auto ncdv = ncdirectf_render(nc_, dirf, &vopts);
        CHECK(nullptr != ncdv);
        CHECK(0 == ncdirect_raster_frame(nc_, ncdv, NCALIGN_LEFT));
        ncdirectf_free(dirf);
        unsigned y, x;
        CHECK(0 == ncdirect_cursor_yx(nc_, &y, &x));
        auto dimy = ncdirect_dim_y(nc_);
        auto dimx = ncdirect_dim_x(nc_);
        CHECK(dimy > y);
        CHECK(dimx > x);
      }
    }
  }
#endif

  CHECK(0 == ncdirect_stop(nc_));

  // make sure that we can pass undefined flags and still create the ncdirect.
  // we don't pass all 1s, or we turn on all logging, and run into trouble!
  SUBCASE("FutureFlags") {
    auto fnc = ncdirect_init(NULL, stdout, NCDIRECT_OPTION_VERY_VERBOSE << 1u);
    REQUIRE(nullptr != fnc);
    CHECK(0 == ncdirect_stop(fnc));
  }

}
