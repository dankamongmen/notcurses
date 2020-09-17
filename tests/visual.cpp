#include "main.h"
#include <vector>

TEST_CASE("Visual") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  ncplane* ncp_ = notcurses_stdplane(nc_);
  REQUIRE(ncp_);
  auto n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

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
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    auto ncv = ncvisual_from_file(find_data("changes.jpg"));
    REQUIRE(ncv);
    /*CHECK(dimy * 2 == frame->height);
    CHECK(dimx == frame->width); FIXME */
    struct ncvisual_options opts{};
    opts.scaling = NCSCALE_STRETCH;
    auto newn = ncvisual_render(nc_, ncv, &opts);
    CHECK(newn);
    CHECK(0 == notcurses_render(nc_));
    CHECK(1 == ncvisual_decode(ncv));
    ncplane_destroy(newn);
    ncvisual_destroy(ncv);
  }

  SUBCASE("LoadImage") {
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    auto ncv = ncvisual_from_file(find_data("changes.jpg"));
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

  SUBCASE("PlaneDuplicate") {
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    auto ncv = ncvisual_from_file(find_data("changes.jpg"));
    REQUIRE(ncv);
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
      int dimy, dimx;
      ncplane_dim_yx(ncp_, &dimy, &dimx);
      auto ncv = ncvisual_from_file(find_data("notcursesI.avi"));
      REQUIRE(ncv);
      for(;;){ // run at the highest speed we can
        int ret = ncvisual_decode(ncv);
        if(1 == ret){
          break;
        }
        CHECK(0 == ret);
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
      int dimy, dimx;
      ncplane_dim_yx(ncp_, &dimy, &dimx);
      auto ncv = ncvisual_from_file(find_data("notcursesI.avi"));
      REQUIRE(ncv);
      CHECK(0 == ncvisual_decode(ncv));
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

  // write a checkerboard pattern and verify the NCBLIT_2x1 output
  SUBCASE("Dualblitter") {
    if(enforce_utf8()){
      constexpr int DIMY = 10;
      constexpr int DIMX = 11; // odd number to get checkerboard effect
      auto rgba = new uint32_t[DIMY * DIMX];
      for(int i = 0 ; i < DIMY * DIMX ; ++i){
        CHECK(0 == ncpixel_set_a(&rgba[i], 0xff));
        if(i % 2){
          CHECK(0 == ncpixel_set_g(&rgba[i], 0xff));
          CHECK(0 == ncpixel_set_r(&rgba[i], 0));
        }else{
          CHECK(0 == ncpixel_set_r(&rgba[i], 0xff));
          CHECK(0 == ncpixel_set_g(&rgba[i], 0));
        }
        CHECK(0 == ncpixel_set_b(&rgba[i], 0));
      }
      auto ncv = ncvisual_from_rgba(rgba, DIMY, DIMX * sizeof(uint32_t), DIMX);
      REQUIRE(nullptr != ncv);
      struct ncvisual_options vopts{};
      vopts.n = n_;
      vopts.blitter = NCBLIT_2x1;
      vopts.flags = NCVISUAL_OPTION_NODEGRADE;
      CHECK(n_ == ncvisual_render(nc_, ncv, &vopts));
      CHECK(0 == notcurses_render(nc_));
      for(int y = 0 ; y < DIMY / 2 ; ++y){
        for(int x = 0 ; x < DIMX ; ++x){
          uint16_t stylemask;
          uint64_t channels;
          char* egc = notcurses_at_yx(nc_, y, x, &stylemask, &channels);
          REQUIRE(nullptr != egc);
          CHECK((rgba[y * 2 * DIMX + x] & 0xffffff) == channels_bg_rgb(channels));
          CHECK((rgba[(y * 2 + 1) * DIMX + x] & 0xffffff) == channels_fg_rgb(channels));
          free(egc);
        }
      }
      delete[] rgba;
    }
  }

  // write a checkerboard pattern and verify the NCBLIT_2x2 output
  SUBCASE("Quadblitter") {
    if(enforce_utf8()){
      constexpr int DIMY = 10;
      constexpr int DIMX = 11; // odd number to get checkerboard effect
      auto rgba = new uint32_t[DIMY * DIMX];
      for(int i = 0 ; i < DIMY * DIMX ; ++i){
        CHECK(0 == ncpixel_set_a(&rgba[i], 0xff));
        if(i % 2){
          CHECK(0 == ncpixel_set_g(&rgba[i], 0xff));
          CHECK(0 == ncpixel_set_b(&rgba[i], 0));
        }else{
          CHECK(0 == ncpixel_set_b(&rgba[i], 0xff));
          CHECK(0 == ncpixel_set_g(&rgba[i], 0));
        }
        CHECK(0 == ncpixel_set_r(&rgba[i], 0));
      }
      auto ncv = ncvisual_from_rgba(rgba, DIMY, DIMX * sizeof(uint32_t), DIMX);
      REQUIRE(nullptr != ncv);
      struct ncvisual_options vopts{};
      vopts.n = n_;
      vopts.blitter = NCBLIT_2x2;
      vopts.flags = NCVISUAL_OPTION_NODEGRADE;
      CHECK(n_ == ncvisual_render(nc_, ncv, &vopts));
      CHECK(0 == notcurses_render(nc_));
      for(int y = 0 ; y < DIMY / 2 ; ++y){
        for(int x = 0 ; x < DIMX / 2 ; ++x){
          uint16_t stylemask;
          uint64_t channels;
          char* egc = notcurses_at_yx(nc_, y, x, &stylemask, &channels);
          REQUIRE(nullptr != egc);
          CHECK((rgba[(y * 2 * DIMX) + (x * 2)] & 0xffffff) == channels_fg_rgb(channels));
          CHECK((rgba[(y * 2 + 1) * DIMX + (x * 2) + 1] & 0xffffff) == channels_fg_rgb(channels));
          free(egc);
        }
      }
      delete[] rgba;
    }
  }

  // close-in verification of each quadblitter output EGC 
  SUBCASE("QuadblitterEGCs") {
    if(enforce_utf8()){
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
      CHECK(n_ == ncvisual_render(nc_, ncv, &vopts));
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
    }
  }

  CHECK(!notcurses_stop(nc_));
}
