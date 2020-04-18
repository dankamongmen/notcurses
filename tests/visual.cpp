#include "main.h"
#ifdef USE_FFMPEG
#include <libavutil/pixdesc.h>
#include <libavutil/avconfig.h>
#include <libavcodec/avcodec.h>
#endif

TEST_CASE("Multimedia") {
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

#ifndef USE_FFMPEG
  SUBCASE("LibavDisabled"){
    REQUIRE(!notcurses_canopen(nc_));
  }
#else
  SUBCASE("LibavEnabled"){
    REQUIRE(notcurses_canopen(nc_));
  }

  SUBCASE("LoadImageCreatePlane") {
    int averr;
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    auto ncv = ncvisual_open_plane(nc_, find_data("changes.jpg"), &averr, 0, 0, NCSCALE_STRETCH);
    REQUIRE(ncv);
    REQUIRE(0 == averr);
    auto frame = ncvisual_decode(ncv, &averr);
    REQUIRE(frame);
    REQUIRE(0 == averr);
    CHECK(dimy * 2 == frame->height);
    CHECK(dimx == frame->width);
    CHECK(0 < ncvisual_render(ncv, 0, 0, -1, -1));
    CHECK(0 == notcurses_render(nc_));
    frame = ncvisual_decode(ncv, &averr);
    REQUIRE_EQ(nullptr, frame);
    CHECK(AVERROR_EOF == averr);
    ncvisual_destroy(ncv);
  }

  SUBCASE("LoadImage") {
    int averr;
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    auto ncv = ncplane_visual_open(ncp_, find_data("changes.jpg"), &averr);
    REQUIRE(ncv);
    REQUIRE(0 == averr);
    auto frame = ncvisual_decode(ncv, &averr);
    REQUIRE(frame);
    REQUIRE(0 == averr);
    CHECK(dimy * 2 == frame->height);
    CHECK(dimx == frame->width);
    CHECK(0 < ncvisual_render(ncv, 0, 0, -1, -1));
    CHECK(0 == notcurses_render(nc_));
    frame = ncvisual_decode(ncv, &averr);
    REQUIRE_EQ(nullptr, frame);
    CHECK(AVERROR_EOF == averr);
    ncvisual_destroy(ncv);
  }

  SUBCASE("PlaneDuplicate") {
    int averr;
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    auto ncv = ncplane_visual_open(ncp_, find_data("changes.jpg"), &averr);
    REQUIRE(ncv);
    REQUIRE(0 == averr);
    auto frame = ncvisual_decode(ncv, &averr);
    REQUIRE(frame);
    REQUIRE(0 == averr);
    CHECK(dimy * 2 == frame->height);
    CHECK(dimx == frame->width);
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
    int averr;
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    auto ncv = ncplane_visual_open(ncp_, find_data("samoa.avi"), &averr);
    REQUIRE(ncv);
    CHECK(0 == averr);
    for(;;){ // run at the highest speed we can
      auto frame = ncvisual_decode(ncv, &averr);
      if(!frame){
        CHECK(AVERROR_EOF == averr);
        break;
      }
      CHECK(0 == averr);
      CHECK(dimy * 2 == frame->height);
      CHECK(dimx == frame->width);
      CHECK(0 < ncvisual_render(ncv, 0, 0, -1, -1));
      CHECK(0 == notcurses_render(nc_));
    }
    ncvisual_destroy(ncv);
  }

  SUBCASE("LoadVideoCreatePlane") {
    int averr;
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    auto ncv = ncvisual_open_plane(nc_, find_data("notcursesI.avi"), &averr, 0, 0, NCSCALE_STRETCH);
    REQUIRE(ncv);
    CHECK(0 == averr);
    auto frame = ncvisual_decode(ncv, &averr);
    REQUIRE_NE(nullptr, frame);
    CHECK(0 == averr);
    CHECK(dimy * 2 == frame->height);
    CHECK(dimx == frame->width);
    CHECK(0 < ncvisual_render(ncv, 0, 0, -1, -1));
    CHECK(0 == notcurses_render(nc_));
    ncvisual_destroy(ncv);
  }
#endif

  CHECK(!notcurses_stop(nc_));
  CHECK(!fclose(outfp_));
}

