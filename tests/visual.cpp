#include "main.h"
#include <vector>

TEST_CASE("Visual") {
  notcurses_options nopts{};
  nopts.suppress_banner = true;
  notcurses* nc_ = notcurses_init(&nopts, nullptr);
  if(!nc_){
    return;
  }
  ncplane* ncp_ = notcurses_stdplane(nc_);
  REQUIRE(ncp_);

#ifndef USE_MULTIMEDIA
  SUBCASE("VisualDisabled"){
    REQUIRE(!notcurses_canopen_images(nc_));
    REQUIRE(!notcurses_canopen_videos(nc_));
  }
#else
  SUBCASE("ImagesEnabled"){
    REQUIRE(notcurses_canopen_images(nc_));
  }

  SUBCASE("LoadImageCreatePlane") {
    nc_err_e ncerr = NCERR_SUCCESS;
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    auto ncv = ncvisual_from_file(find_data("changes.jpg"), &ncerr);
    REQUIRE(ncv);
    REQUIRE(NCERR_SUCCESS == ncerr);
    /*CHECK(dimy * 2 == frame->height);
    CHECK(dimx == frame->width); FIXME */
    struct ncvisual_options opts{};
    opts.scaling = NCSCALE_STRETCH;
    auto newn = ncvisual_render(nc_, ncv, &opts);
    CHECK(newn);
    CHECK(0 == notcurses_render(nc_));
    ncerr = ncvisual_decode(ncv);
    CHECK(NCERR_EOF == ncerr);
    ncplane_destroy(newn);
    ncvisual_destroy(ncv);
  }

  SUBCASE("LoadImage") {
    nc_err_e ncerr = NCERR_SUCCESS;
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    auto ncv = ncvisual_from_file(find_data("changes.jpg"), &ncerr);
    REQUIRE(ncv);
    REQUIRE(NCERR_SUCCESS == ncerr);
    /*CHECK(dimy * 2 == frame->height);
    CHECK(dimx == frame->width); FIXME */
    struct ncvisual_options opts{};
    opts.scaling = NCSCALE_STRETCH;
    opts.n = ncp_;
    CHECK(ncvisual_render(nc_, ncv, &opts));
    CHECK(0 == notcurses_render(nc_));
    ncerr = ncvisual_decode(ncv);
    CHECK(NCERR_EOF == ncerr);
    ncvisual_destroy(ncv);
  }

  SUBCASE("PlaneDuplicate") {
    nc_err_e ncerr = NCERR_SUCCESS;
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    auto ncv = ncvisual_from_file(find_data("changes.jpg"), &ncerr);
    REQUIRE(ncv);
    REQUIRE(NCERR_SUCCESS == ncerr);
    /*CHECK(dimy * 2 == frame->height);
    CHECK(dimx == frame->width); FIXME */
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

  SUBCASE("LoadVideo") {
    if(notcurses_canopen_videos(nc_)){
      nc_err_e ncerr = NCERR_SUCCESS;
      int dimy, dimx;
      ncplane_dim_yx(ncp_, &dimy, &dimx);
      auto ncv = ncvisual_from_file(find_data("notcursesI.avi"), &ncerr);
      REQUIRE(ncv);
      CHECK(NCERR_SUCCESS == ncerr);
      for(;;){ // run at the highest speed we can
        ncerr = ncvisual_decode(ncv);
        if(NCERR_EOF == ncerr){
          break;
        }
        CHECK(NCERR_SUCCESS == ncerr);
        /*CHECK(dimy * 2 == frame->height);
        CHECK(dimx == frame->width); FIXME */
        struct ncvisual_options opts{};
        opts.scaling = NCSCALE_STRETCH;
        opts.n = ncp_;
        CHECK(ncvisual_render(nc_, ncv, &opts));
        CHECK(0 == notcurses_render(nc_));
      }
      ncvisual_destroy(ncv);
    }
  }

  SUBCASE("LoadVideoCreatePlane") {
    if(notcurses_canopen_videos(nc_)){
      nc_err_e ncerr = NCERR_SUCCESS;
      int dimy, dimx;
      ncplane_dim_yx(ncp_, &dimy, &dimx);
      auto ncv = ncvisual_from_file(find_data("notcursesI.avi"), &ncerr);
      REQUIRE(ncv);
      CHECK(NCERR_SUCCESS == ncerr);
      ncerr = ncvisual_decode(ncv);
      CHECK(NCERR_SUCCESS == ncerr);
      /*CHECK(dimy * 2 == frame->height);
      CHECK(dimx == frame->width); FIXME */
      struct ncvisual_options opts{};
      opts.scaling = NCSCALE_STRETCH;
      auto newn = ncvisual_render(nc_, ncv, &opts);
      CHECK(newn);
      CHECK(0 == notcurses_render(nc_));
      ncplane_destroy(newn);
      ncvisual_destroy(ncv);
    }
  }
#endif

  SUBCASE("LoadRGBAFromMemory") {
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    std::vector<uint32_t> rgba(dimx * dimy * 2, 0x88bbccff);
    auto ncv = ncvisual_from_rgba(rgba.data(), dimy * 2, dimx * 4, dimx);
    REQUIRE(ncv);
    struct ncvisual_options opts{};
    opts.n = ncp_;
    CHECK(ncvisual_render(nc_, ncv, &opts));
    CHECK(0 == notcurses_render(nc_));
    ncvisual_destroy(ncv);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("LoadBGRAFromMemory") {
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    std::vector<uint32_t> rgba(dimx * dimy * 2, 0x88bbccff);
    auto ncv = ncvisual_from_bgra(rgba.data(), dimy * 2, dimx * 4, dimx);
    REQUIRE(ncv);
    struct ncvisual_options opts{};
    opts.n = ncp_;
    CHECK(ncvisual_render(nc_, ncv, &opts));
    CHECK(0 == notcurses_render(nc_));
    ncvisual_destroy(ncv);
    CHECK(0 == notcurses_render(nc_));
  }

  CHECK(!notcurses_stop(nc_));
}
