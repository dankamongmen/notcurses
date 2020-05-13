#include "main.h"
#include <vector>

TEST_CASE("Visual") {
  if(getenv("TERM") == nullptr){
    return;
  }
  notcurses_options nopts{};
  nopts.inhibit_alternate_screen = true;
  nopts.suppress_banner = true;
  FILE* outfp_ = fopen("/dev/tty", "wb");
  REQUIRE(outfp_);
  notcurses* nc_ = notcurses_init(&nopts, outfp_);
  REQUIRE(nc_);
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
    auto ncv = ncvisual_from_file(nc_, find_data("changes.jpg"), &ncerr, 0, 0, NCSCALE_STRETCH);
    REQUIRE(ncv);
    REQUIRE(NCERR_SUCCESS == ncerr);
    ncerr = ncvisual_decode(ncv);
    REQUIRE(NCERR_SUCCESS == ncerr);
    /*CHECK(dimy * 2 == frame->height);
    CHECK(dimx == frame->width); FIXME */
    CHECK(0 < ncvisual_render(ncv, 0, 0, -1, -1));
    CHECK(0 == notcurses_render(nc_));
    ncerr = ncvisual_decode(ncv);
    CHECK(NCERR_EOF == ncerr);
    ncvisual_destroy(ncv);
  }

  SUBCASE("LoadImage") {
    nc_err_e ncerr = NCERR_SUCCESS;
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    auto ncv = ncplane_visual_open(ncp_, find_data("changes.jpg"), &ncerr);
    REQUIRE(ncv);
    REQUIRE(0 == ncerr);
    ncerr = ncvisual_decode(ncv);
    REQUIRE(NCERR_SUCCESS == ncerr);
    /*CHECK(dimy * 2 == frame->height);
    CHECK(dimx == frame->width); FIXME */
    CHECK(0 < ncvisual_render(ncv, 0, 0, -1, -1));
    CHECK(0 == notcurses_render(nc_));
    ncerr = ncvisual_decode(ncv);
    CHECK(NCERR_EOF == ncerr);
    ncvisual_destroy(ncv);
  }

  SUBCASE("PlaneDuplicate") {
    nc_err_e ncerr = NCERR_SUCCESS;
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    auto ncv = ncplane_visual_open(ncp_, find_data("changes.jpg"), &ncerr);
    REQUIRE(ncv);
    REQUIRE(NCERR_SUCCESS == ncerr);
    ncerr = ncvisual_decode(ncv);
    REQUIRE(NCERR_SUCCESS == ncerr);
    /*CHECK(dimy * 2 == frame->height);
    CHECK(dimx == frame->width); FIXME */
    CHECK(0 < ncvisual_render(ncv, 0, 0, -1, -1));
    void* needle = malloc(1);
    REQUIRE(nullptr != needle);
    struct ncplane* newn = ncplane_dup(ncvisual_plane(ncv), needle);
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
      auto ncv = ncplane_visual_open(ncp_, find_data("notcursesI.avi"), &ncerr);
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
        CHECK(0 < ncvisual_render(ncv, 0, 0, -1, -1));
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
      auto ncv = ncvisual_from_file(nc_, find_data("notcursesI.avi"), &ncerr, 0, 0, NCSCALE_STRETCH);
      REQUIRE(ncv);
      CHECK(NCERR_SUCCESS == ncerr);
      ncerr = ncvisual_decode(ncv);
      CHECK(NCERR_SUCCESS == ncerr);
      /*CHECK(dimy * 2 == frame->height);
      CHECK(dimx == frame->width); FIXME */
      CHECK(0 < ncvisual_render(ncv, 0, 0, -1, -1));
      CHECK(0 == notcurses_render(nc_));
      ncvisual_destroy(ncv);
    }
  }
#endif

  SUBCASE("LoadRGBAFromMemory") {
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    std::vector<uint32_t> rgba(dimx * dimy * 2, 0x88bbccff);
    auto ncv = ncvisual_from_rgba(nc_, rgba.data(), dimy * 2, dimx * 4, dimx);
    REQUIRE(ncv);
    CHECK(0 < ncvisual_render(ncv, 0, 0, -1, -1));
    CHECK(0 == notcurses_render(nc_));
    ncvisual_destroy(ncv);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("LoadBGRAFromMemory") {
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    std::vector<uint32_t> rgba(dimx * dimy * 2, 0x88bbccff);
    auto ncv = ncvisual_from_bgra(nc_, rgba.data(), dimy * 2, dimx * 4, dimx);
    REQUIRE(ncv);
    CHECK(0 < ncvisual_render(ncv, 0, 0, -1, -1));
    CHECK(0 == notcurses_render(nc_));
    ncvisual_destroy(ncv);
    CHECK(0 == notcurses_render(nc_));
  }

  CHECK(!notcurses_stop(nc_));
  CHECK(!fclose(outfp_));
}

