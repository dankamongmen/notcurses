#include "main.h"
#include "lib/visual-details.h"
#include <vector>
#include <cmath>

TEST_CASE("Media") {
  auto nc_ = testing_notcurses();
  REQUIRE(nullptr != nc_);
  ncplane* ncp_ = notcurses_stdplane(nc_);
  REQUIRE(ncp_);
  auto n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

#ifndef NOTCURSES_USE_MULTIMEDIA
  SUBCASE("VisualDisabled") {
    CHECK(!notcurses_canopen_images(nc_));
    CHECK(!notcurses_canopen_videos(nc_));
  }
#else
  SUBCASE("ImagesEnabled") {
    CHECK(notcurses_canopen_images(nc_));
  }

  // resize followed by rotate, see #1800
  SUBCASE("ResizeThenRotateFromFile") {
    auto ncv = ncvisual_from_file(find_data("changes.jpg").get());
    REQUIRE(nullptr != ncv);
    struct ncvisual_options vopts{};
    vopts.x = NCALIGN_CENTER;
    vopts.y = NCALIGN_CENTER;
    vopts.flags |= NCVISUAL_OPTION_HORALIGNED | NCVISUAL_OPTION_VERALIGNED;
    auto p = ncvisual_render(nc_, ncv, &vopts);
    REQUIRE(nullptr != p);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(p));
    CHECK(0 == ncvisual_resize(ncv, 20, 20));
    p = ncvisual_render(nc_, ncv, &vopts);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(p));
    p = ncvisual_render(nc_, ncv, &vopts);
    REQUIRE(nullptr != p);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(p));
    CHECK(0 == ncvisual_rotate(ncv, M_PI / 2));
    p = ncvisual_render(nc_, ncv, &vopts);
    REQUIRE(nullptr != p);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(p));
    ncvisual_destroy(ncv);
  }

  SUBCASE("LoadImageCreatePlane") {
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    auto ncv = ncvisual_from_file(find_data("changes.jpg").get());
    REQUIRE(ncv);
    /*CHECK(dimy * 2 == frame->height);
    CHECK(dimx == frame->width); FIXME */
    struct ncvisual_options opts{};
    opts.scaling = NCSCALE_STRETCH;
    auto newn = ncvisual_render(nc_, ncv, &opts);
    CHECK(newn);
    CHECK(0 == notcurses_render(nc_));
    CHECK(1 == ncvisual_decode(ncv));
    CHECK(0 == ncplane_destroy(newn));
    ncvisual_destroy(ncv);
  }

  SUBCASE("LoadImage") {
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    auto ncv = ncvisual_from_file(find_data("changes.jpg").get());
    REQUIRE(ncv);
    /*CHECK(dimy * 2 == frame->height);
    CHECK(dimx == frame->width); FIXME */
    struct ncvisual_options opts{};
    opts.scaling = NCSCALE_STRETCH;
    opts.n = ncp_;
    CHECK(ncvisual_render(nc_, ncv, &opts));
    CHECK(0 == notcurses_render(nc_));
    CHECK(1 == ncvisual_decode(ncv));
    ncvisual_destroy(ncv);
  }

  SUBCASE("InflateImage") {
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    auto ncv = ncvisual_from_file(find_data("changes.jpg").get());
    REQUIRE(ncv);
    int odimy, odimx, ndimy, ndimx;
    struct ncvisual_options opts{};
    opts.n = ncp_;
    CHECK(0 == ncvisual_blitter_geom(nc_, ncv, &opts, &odimy, &odimx, nullptr, nullptr, nullptr));
    CHECK(ncvisual_render(nc_, ncv, &opts));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncvisual_resize_noninterpolative(ncv, ncv->pixy * 2, ncv->pixx * 2));
    CHECK(0 == ncvisual_blitter_geom(nc_, ncv, &opts, &ndimy, &ndimx, nullptr, nullptr, nullptr));
    CHECK(ndimy == odimy * 2);
    CHECK(ndimx == odimx * 2);
    CHECK(ncvisual_render(nc_, ncv, &opts));
    CHECK(0 == notcurses_render(nc_));
    ncvisual_destroy(ncv);
  }

  SUBCASE("PlaneDuplicate") {
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    auto ncv = ncvisual_from_file(find_data("changes.jpg").get());
    REQUIRE(ncv);
    struct ncvisual_options opts{};
    opts.n = ncp_;
    opts.scaling = NCSCALE_STRETCH;
    CHECK(ncvisual_render(nc_, ncv, &opts));
    void* needle = malloc(1);
    REQUIRE(nullptr != needle);
    struct ncplane* newn = ncplane_dup(ncp_, needle);
    int ndimx, ndimy;
    REQUIRE(nullptr != newn);
    ncvisual_destroy(ncv);
    ncplane_erase(ncp_);
    // should still have the image
    CHECK(0 == notcurses_render(nc_));
    ncplane_dim_yx(newn, &ndimy, &ndimx);
    CHECK(ndimy == dimy);
    CHECK(ndimx == dimx);
  }

  SUBCASE("LoadVideoASCIIScale") {
    if(notcurses_canopen_videos(nc_)){
      int dimy, dimx;
      ncplane_dim_yx(ncp_, &dimy, &dimx);
      auto ncv = ncvisual_from_file(find_data("notcursesIII.mkv").get());
      REQUIRE(ncv);
      // FIXME can't we use use ncvisual_stream() here?
      for(;;){ // run at the highest speed we can
        int ret = ncvisual_decode(ncv);
        if(1 == ret){
          break;
        }
        CHECK(0 == ret);
        struct ncvisual_options opts{};
        opts.scaling = NCSCALE_SCALE_HIRES;
        opts.n = ncp_;
        opts.blitter = NCBLIT_1x1;
        CHECK(ncvisual_render(nc_, ncv, &opts));
        CHECK(0 == notcurses_render(nc_));
      }
      ncvisual_destroy(ncv);
    }
  }

  SUBCASE("LoadVideoHalfScale") {
    if(notcurses_canopen_videos(nc_)){
      int dimy, dimx;
      ncplane_dim_yx(ncp_, &dimy, &dimx);
      auto ncv = ncvisual_from_file(find_data("notcursesIII.mkv").get());
      REQUIRE(ncv);
      for(;;){ // run at the highest speed we can
        int ret = ncvisual_decode(ncv);
        if(1 == ret){
          break;
        }
        CHECK(0 == ret);
        struct ncvisual_options opts{};
        opts.scaling = NCSCALE_SCALE_HIRES;
        opts.n = ncp_;
        opts.blitter = NCBLIT_2x1;
        CHECK(ncvisual_render(nc_, ncv, &opts));
        CHECK(0 == notcurses_render(nc_));
      }
      ncvisual_destroy(ncv);
    }
  }

  // quadblitter is default for NCSCALE_SCALE_HIRES
  SUBCASE("LoadVideoQuadScale") {
    if(notcurses_canopen_videos(nc_)){
      int dimy, dimx;
      ncplane_dim_yx(ncp_, &dimy, &dimx);
      auto ncv = ncvisual_from_file(find_data("notcursesIII.mkv").get());
      REQUIRE(ncv);
      for(;;){ // run at the highest speed we can
        int ret = ncvisual_decode(ncv);
        if(1 == ret){
          break;
        }
        CHECK(0 == ret);
        struct ncvisual_options opts{};
        opts.scaling = NCSCALE_SCALE_HIRES;
        opts.n = ncp_;
        opts.blitter = NCBLIT_2x2;
        CHECK(ncvisual_render(nc_, ncv, &opts));
        CHECK(0 == notcurses_render(nc_));
      }
      ncvisual_destroy(ncv);
    }
  }

  SUBCASE("LoadVideoSexScale") {
    if(notcurses_canopen_videos(nc_)){
      int dimy, dimx;
      ncplane_dim_yx(ncp_, &dimy, &dimx);
      auto ncv = ncvisual_from_file(find_data("notcursesIII.mkv").get());
      REQUIRE(ncv);
      for(;;){ // run at the highest speed we can
        int ret = ncvisual_decode(ncv);
        if(1 == ret){
          break;
        }
        CHECK(0 == ret);
        struct ncvisual_options opts{};
        opts.scaling = NCSCALE_SCALE_HIRES;
        opts.n = ncp_;
        opts.blitter = NCBLIT_3x2;
        CHECK(ncvisual_render(nc_, ncv, &opts));
        CHECK(0 == notcurses_render(nc_));
      }
      ncvisual_destroy(ncv);
    }
  }

  SUBCASE("LoadVideoBrailleScale") {
    if(notcurses_canopen_videos(nc_)){
      int dimy, dimx;
      ncplane_dim_yx(ncp_, &dimy, &dimx);
      auto ncv = ncvisual_from_file(find_data("notcursesIII.mkv").get());
      REQUIRE(ncv);
      for(;;){ // run at the highest speed we can
        int ret = ncvisual_decode(ncv);
        if(1 == ret){
          break;
        }
        CHECK(0 == ret);
        struct ncvisual_options opts{};
        opts.scaling = NCSCALE_SCALE_HIRES;
        opts.n = ncp_;
        opts.blitter = NCBLIT_BRAILLE;
        CHECK(ncvisual_render(nc_, ncv, &opts));
        CHECK(0 == notcurses_render(nc_));
      }
      ncvisual_destroy(ncv);
    }
  }

  // do a pixel video, reusing the same plane over and over
  SUBCASE("LoadVideoPixelScaleOnePlane") {
    if(notcurses_check_pixel_support(nc_) > 0){
      if(notcurses_canopen_videos(nc_)){
        int dimy, dimx;
        ncplane_dim_yx(ncp_, &dimy, &dimx);
        auto ncv = ncvisual_from_file(find_data("notcursesIII.mkv").get());
        REQUIRE(ncv);
        struct ncplane* n = NULL;
        for(;;){ // run at the highest speed we can
          int ret = ncvisual_decode(ncv);
          if(1 == ret){
            break;
          }
          CHECK(0 == ret);
          struct ncvisual_options opts{};
          opts.scaling = NCSCALE_SCALE;
          opts.blitter = NCBLIT_PIXEL;
          opts.n = n;
          n = ncvisual_render(nc_, ncv, &opts);
          REQUIRE(nullptr != n);
          CHECK(0 == notcurses_render(nc_));
        }
        CHECK(0 == ncplane_destroy(n));
        ncvisual_destroy(ncv);
      }
    }
  }

  // do a pixel video with a different plane for each frame
  SUBCASE("LoadVideoPixelScaleDifferentPlanes") {
    if(notcurses_check_pixel_support(nc_) > 0){
      if(notcurses_canopen_videos(nc_)){
        int dimy, dimx;
        ncplane_dim_yx(ncp_, &dimy, &dimx);
        auto ncv = ncvisual_from_file(find_data("notcursesIII.mkv").get());
        REQUIRE(ncv);
        for(;;){ // run at the highest speed we can
          int ret = ncvisual_decode(ncv);
          if(1 == ret){
            break;
          }
          CHECK(0 == ret);
          struct ncvisual_options opts{};
          opts.scaling = NCSCALE_SCALE;
          opts.blitter = NCBLIT_PIXEL;
          REQUIRE(ncvisual_render(nc_, ncv, &opts));
          CHECK(0 == notcurses_render(nc_));
        }
        ncvisual_destroy(ncv);
      }
    }
  }

  SUBCASE("LoopVideo") {
    if(notcurses_canopen_videos(nc_)){
      int dimy, dimx;
      ncplane_dim_yx(ncp_, &dimy, &dimx);
      auto ncv = ncvisual_from_file(find_data("notcursesIII.mkv").get());
      REQUIRE(ncv);
      int ret;
      while((ret = ncvisual_decode(ncv)) == 0){
        ;
      }
      // FIXME verify that it is last frame?
      CHECK(1 == ret);
      ret = ncvisual_decode_loop(ncv);
      CHECK(1 == ret);
      struct ncplane* ncp = ncvisual_render(nc_, ncv, nullptr);
      CHECK(nullptr != ncp);
      CHECK(0 == ncplane_destroy(ncp));
      // FIXME verify that it is first frame, not last?
      ret = ncvisual_decode_loop(ncv);
      CHECK(0 == ret);
      ncp = ncvisual_render(nc_, ncv, nullptr);
      CHECK(nullptr != ncp);
      CHECK(0 == ncplane_destroy(ncp));
      ncvisual_destroy(ncv);
    }
  }

  SUBCASE("LoadVideoCreatePlane") {
    if(notcurses_canopen_videos(nc_)){
      int dimy, dimx;
      ncplane_dim_yx(ncp_, &dimy, &dimx);
      auto ncv = ncvisual_from_file(find_data("notcursesIII.mkv").get());
      REQUIRE(ncv);
      CHECK(0 == ncvisual_decode(ncv));
      /*CHECK(dimy * 2 == frame->height);
      CHECK(dimx == frame->width); FIXME */
      struct ncvisual_options opts{};
      opts.scaling = NCSCALE_STRETCH;
      auto newn = ncvisual_render(nc_, ncv, &opts);
      CHECK(newn);
      CHECK(0 == notcurses_render(nc_));
      CHECK(0 == ncplane_destroy(newn));
      ncvisual_destroy(ncv);
    }
  }

  // test NCVISUAL_OPTIONS_CHILDPLANE + stretch + null alignment, using a file
  SUBCASE("ImageFileChildScaling") {
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
      .scaling = NCSCALE_STRETCH,
      .y = 0, .x = 0,
      .begy = 0, .begx = 0,
      .leny = 0, .lenx = 0,
      .blitter = NCBLIT_1x1,
      .flags = NCVISUAL_OPTION_CHILDPLANE,
      .transcolor = 0,
    };
    auto ncv = ncvisual_from_file(find_data("onedot.png").get());
    REQUIRE(ncv);
    auto child = ncvisual_render(nc_, ncv, &vopts);
    REQUIRE(child);
    CHECK(5 == ncplane_dim_y(child));
    CHECK(5 == ncplane_dim_x(child));
    CHECK(0 == ncplane_y(child));
    CHECK(0 == ncplane_x(child));
    ncvisual_destroy(ncv);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(parent));
    CHECK(0 == ncplane_destroy(child));
  }

#endif

  CHECK(!notcurses_stop(nc_));
}
