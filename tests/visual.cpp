#include "main.h"
#include <vector>

TEST_CASE("Visual") {
  auto nc_ = testing_notcurses();
  REQUIRE(nullptr != nc_);
  ncplane* ncp_ = notcurses_stdplane(nc_);
  REQUIRE(ncp_);
  auto n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

#ifndef NOTCURSES_USE_MULTIMEDIA
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

  SUBCASE("LoadVideoASCII") {
    if(notcurses_canopen_videos(nc_)){
      int dimy, dimx;
      ncplane_dim_yx(ncp_, &dimy, &dimx);
      auto ncv = ncvisual_from_file(find_data("notcursesII.mkv"));
      REQUIRE(ncv);
      for(;;){ // run at the highest speed we can
        int ret = ncvisual_decode(ncv);
        if(1 == ret){
          break;
        }
        CHECK(0 == ret);
        struct ncvisual_options opts{};
        opts.scaling = NCSCALE_STRETCH;
        opts.n = ncp_;
        opts.blitter = NCBLIT_1x1;
        CHECK(ncvisual_render(nc_, ncv, &opts));
        CHECK(0 == notcurses_render(nc_));
      }
      ncvisual_destroy(ncv);
    }
  }

  SUBCASE("LoadVideoHalfblocks") {
    if(notcurses_canopen_videos(nc_)){
      int dimy, dimx;
      ncplane_dim_yx(ncp_, &dimy, &dimx);
      auto ncv = ncvisual_from_file(find_data("notcursesII.mkv"));
      REQUIRE(ncv);
      for(;;){ // run at the highest speed we can
        int ret = ncvisual_decode(ncv);
        if(1 == ret){
          break;
        }
        CHECK(0 == ret);
        struct ncvisual_options opts{};
        opts.scaling = NCSCALE_STRETCH;
        opts.n = ncp_;
        opts.blitter = NCBLIT_2x1;
        CHECK(ncvisual_render(nc_, ncv, &opts));
        CHECK(0 == notcurses_render(nc_));
      }
      ncvisual_destroy(ncv);
    }
  }

  // quadblitter is default for NCSCALE_STRETCH
  SUBCASE("LoadVideoQuadblitter") {
    if(notcurses_canopen_videos(nc_)){
      int dimy, dimx;
      ncplane_dim_yx(ncp_, &dimy, &dimx);
      auto ncv = ncvisual_from_file(find_data("notcursesII.mkv"));
      REQUIRE(ncv);
      for(;;){ // run at the highest speed we can
        int ret = ncvisual_decode(ncv);
        if(1 == ret){
          break;
        }
        CHECK(0 == ret);
        struct ncvisual_options opts{};
        opts.scaling = NCSCALE_STRETCH;
        opts.n = ncp_;
        opts.blitter = NCBLIT_2x2;
        CHECK(ncvisual_render(nc_, ncv, &opts));
        CHECK(0 == notcurses_render(nc_));
      }
      ncvisual_destroy(ncv);
    }
  }

  SUBCASE("LoadVideoSexblitter") {
    if(notcurses_canopen_videos(nc_)){
      int dimy, dimx;
      ncplane_dim_yx(ncp_, &dimy, &dimx);
      auto ncv = ncvisual_from_file(find_data("notcursesII.mkv"));
      REQUIRE(ncv);
      for(;;){ // run at the highest speed we can
        int ret = ncvisual_decode(ncv);
        if(1 == ret){
          break;
        }
        CHECK(0 == ret);
        struct ncvisual_options opts{};
        opts.scaling = NCSCALE_STRETCH;
        opts.n = ncp_;
        opts.blitter = NCBLIT_3x2;
        CHECK(ncvisual_render(nc_, ncv, &opts));
        CHECK(0 == notcurses_render(nc_));
      }
      ncvisual_destroy(ncv);
    }
  }

  SUBCASE("LoadVideoBraille") {
    if(notcurses_canopen_videos(nc_)){
      int dimy, dimx;
      ncplane_dim_yx(ncp_, &dimy, &dimx);
      auto ncv = ncvisual_from_file(find_data("notcursesII.mkv"));
      REQUIRE(ncv);
      for(;;){ // run at the highest speed we can
        int ret = ncvisual_decode(ncv);
        if(1 == ret){
          break;
        }
        CHECK(0 == ret);
        struct ncvisual_options opts{};
        opts.scaling = NCSCALE_STRETCH;
        opts.n = ncp_;
        opts.blitter = NCBLIT_BRAILLE;
        CHECK(ncvisual_render(nc_, ncv, &opts));
        CHECK(0 == notcurses_render(nc_));
      }
      ncvisual_destroy(ncv);
    }
  }

  SUBCASE("LoopVideo") {
    if(notcurses_canopen_videos(nc_)){
      int dimy, dimx;
      ncplane_dim_yx(ncp_, &dimy, &dimx);
      auto ncv = ncvisual_from_file(find_data("notcursesII.mkv"));
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
      ncplane_destroy(ncp);
      // FIXME verify that it is first frame, not last?
      ret = ncvisual_decode_loop(ncv);
      CHECK(0 == ret);
      ncp = ncvisual_render(nc_, ncv, nullptr);
      CHECK(nullptr != ncp);
      ncplane_destroy(ncp);
      ncvisual_destroy(ncv);
    }
  }

  SUBCASE("LoadVideoCreatePlane") {
    if(notcurses_canopen_videos(nc_)){
      int dimy, dimx;
      ncplane_dim_yx(ncp_, &dimy, &dimx);
      auto ncv = ncvisual_from_file(find_data("notcursesII.mkv"));
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
    std::vector<uint32_t> rgba(dimx * dimy * 2, 0xff88bbcc);
    auto ncv = ncvisual_from_rgba(rgba.data(), dimy * 2, dimx * 4, dimx);
    REQUIRE(ncv);
    struct ncvisual_options opts{};
    opts.n = ncp_;
    CHECK(nullptr != ncvisual_render(nc_, ncv, &opts));
    CHECK(0 == notcurses_render(nc_));
    // FIXME check cell for color -- want ccbb88
    ncvisual_destroy(ncv);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("LoadBGRAFromMemory") {
    int dimy, dimx;
    ncplane_dim_yx(ncp_, &dimy, &dimx);
    std::vector<uint32_t> rgba(dimx * dimy * 2, 0xff88bbcc);
    auto ncv = ncvisual_from_bgra(rgba.data(), dimy * 2, dimx * 4, dimx);
    REQUIRE(ncv);
    struct ncvisual_options opts{};
    opts.n = ncp_;
    CHECK(nullptr != ncvisual_render(nc_, ncv, &opts));
    CHECK(0 == notcurses_render(nc_));
    // FIXME check cell for color -- want 88bbcc
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

  // quadblitter with all 4 colors equal ought generate space
  SUBCASE("Quadblitter4Same") {
    const uint32_t pixels[4] = { 0xff605040, 0xff605040, 0xff605040, 0xff605040 };
    auto ncv = ncvisual_from_rgba(pixels, 2, 2 * sizeof(*pixels), 2);
    REQUIRE(nullptr != ncv);
    struct ncvisual_options vopts = {
      .n = nullptr,
      .scaling = NCSCALE_NONE,
      .y = 0,
      .x = 0,
      .begy = 0,
      .begx = 0,
      .leny = 0,
      .lenx = 0,
      .blitter = NCBLIT_2x2,
      .flags = 0,
    };
    auto ncvp = ncvisual_render(nc_, ncv, &vopts);
    REQUIRE(nullptr != ncvp);
    int dimy, dimx;
    ncplane_dim_yx(ncvp, &dimy, &dimx);
    CHECK(1 == dimy);
    CHECK(1 == dimx);
    uint16_t stylemask;
    uint64_t channels;
    auto egc = ncplane_at_yx(ncvp, 0, 0, &stylemask, &channels);
    CHECK(0 == strcmp(" ", egc));
    CHECK(0 == stylemask);
    CHECK(0x405060 == channels_fg_rgb(channels));
    CHECK(0x405060 == channels_bg_rgb(channels));
    free(egc);
    ncvisual_destroy(ncv);
  }

  // quadblitter with three pixels equal ought generate three-quarter block
  SUBCASE("Quadblitter3Same") {
    const uint32_t pixels[4][4] = {
      { 0xffcccccc, 0xff605040, 0xff605040, 0xff605040 },
      { 0xff605040, 0xffcccccc, 0xff605040, 0xff605040 },
      { 0xff605040, 0xff605040, 0xffcccccc, 0xff605040 },
      { 0xff605040, 0xff605040, 0xff605040, 0xffcccccc } };
    const char* egcs[4] = { "▟", "▙", "▜", "▛" };
    for(int i = 0 ; i < 4 ; ++i){
      auto ncv = ncvisual_from_rgba(pixels[i], 2, 2 * sizeof(**pixels), 2);
      REQUIRE(nullptr != ncv);
      struct ncvisual_options vopts = {
        .n = nullptr,
        .scaling = NCSCALE_NONE,
        .y = 0,
        .x = 0,
        .begy = 0,
        .begx = 0,
        .leny = 0,
        .lenx = 0,
        .blitter = NCBLIT_2x2,
        .flags = 0,
      };
      auto ncvp = ncvisual_render(nc_, ncv, &vopts);
      REQUIRE(nullptr != ncvp);
      int dimy, dimx;
      ncplane_dim_yx(ncvp, &dimy, &dimx);
      CHECK(1 == dimy);
      CHECK(1 == dimx);
      uint16_t stylemask;
      uint64_t channels;
      auto egc = ncplane_at_yx(ncvp, 0, 0, &stylemask, &channels);
      CHECK(0 == strcmp(egcs[i], egc));
      CHECK(0 == stylemask);
      CHECK(0x405060 == channels_fg_rgb(channels));
      CHECK(0xcccccc == channels_bg_rgb(channels));
      free(egc);
      ncvisual_destroy(ncv);
    }
  }

  // quadblitter with two sets of two equal pixels
  SUBCASE("Quadblitter2Pairs") {
    const uint32_t pixels[6][4] = {
      { 0xffcccccc, 0xffcccccc, 0xff605040, 0xff605040 },
      { 0xffcccccc, 0xff605040, 0xffcccccc, 0xff605040 },
      { 0xffcccccc, 0xff605040, 0xff605040, 0xffcccccc },
      { 0xff605040, 0xffcccccc, 0xffcccccc, 0xff605040 },
      { 0xff605040, 0xffcccccc, 0xff605040, 0xffcccccc },
      { 0xff605040, 0xff605040, 0xffcccccc, 0xffcccccc } };
    const char* egcs[6] = { "▀", "▌", "▚", "▚", "▌", "▀" };
    for(size_t i = 0 ; i < sizeof(egcs) / sizeof(*egcs) ; ++i){
      auto ncv = ncvisual_from_rgba(pixels[i], 2, 2 * sizeof(**pixels), 2);
      REQUIRE(nullptr != ncv);
      struct ncvisual_options vopts = {
        .n = nullptr,
        .scaling = NCSCALE_NONE,
        .y = 0,
        .x = 0,
        .begy = 0,
        .begx = 0,
        .leny = 0,
        .lenx = 0,
        .blitter = NCBLIT_2x2,
        .flags = 0,
      };
      auto ncvp = ncvisual_render(nc_, ncv, &vopts);
      REQUIRE(nullptr != ncvp);
      int dimy, dimx;
      ncplane_dim_yx(ncvp, &dimy, &dimx);
      CHECK(1 == dimy);
      CHECK(1 == dimx);
      uint16_t stylemask;
      uint64_t channels;
      auto egc = ncplane_at_yx(ncvp, 0, 0, &stylemask, &channels);
      CHECK(0 == strcmp(egcs[i], egc));
      CHECK(0 == stylemask);
      if(i >= 3){
        CHECK(0x405060 == channels_fg_rgb(channels));
        CHECK(0xcccccc == channels_bg_rgb(channels));
      }else{
        CHECK(0x405060 == channels_bg_rgb(channels));
        CHECK(0xcccccc == channels_fg_rgb(channels));
      }
      free(egc);
      ncvisual_destroy(ncv);
    }
  }

  // quadblitter with one pair plus two split
  SUBCASE("Quadblitter1Pair") {
    const uint32_t pixels[6][4] = {
      { 0xffcccccc, 0xff444444, 0xff605040, 0xff605040 },
      { 0xff444444, 0xff605040, 0xffcccccc, 0xff605040 },
      { 0xffcccccc, 0xff605040, 0xff605040, 0xff444444 },
      { 0xff605040, 0xffcccccc, 0xff444444, 0xff605040 },
      { 0xff605040, 0xffeeeeee, 0xff605040, 0xffcccccc },
      { 0xff605040, 0xff605040, 0xffeeeeee, 0xffcccccc } };
    const char* egcs[6] = { "▟", "▜", "▟", "▙", "▌", "▀" };
    for(size_t i = 0 ; i < sizeof(egcs) / sizeof(*egcs) ; ++i){
      auto ncv = ncvisual_from_rgba(pixels[i], 2, 2 * sizeof(**pixels), 2);
      REQUIRE(nullptr != ncv);
      struct ncvisual_options vopts = {
        .n = nullptr,
        .scaling = NCSCALE_NONE,
        .y = 0,
        .x = 0,
        .begy = 0,
        .begx = 0,
        .leny = 0,
        .lenx = 0,
        .blitter = NCBLIT_2x2,
        .flags = 0,
      };
      auto ncvp = ncvisual_render(nc_, ncv, &vopts);
      REQUIRE(nullptr != ncvp);
      int dimy, dimx;
      ncplane_dim_yx(ncvp, &dimy, &dimx);
      CHECK(1 == dimy);
      CHECK(1 == dimx);
      uint16_t stylemask;
      uint64_t channels;
      auto egc = ncplane_at_yx(ncvp, 0, 0, &stylemask, &channels);
      CHECK(0 == strcmp(egcs[i], egc));
      CHECK(0 == stylemask);
      if(i > 3){
        CHECK(0x405060 == channels_fg_rgb(channels));
        CHECK(0xdddddd == channels_bg_rgb(channels));
      }else{
        CHECK(0x424c57 == channels_fg_rgb(channels));
        CHECK(0xcccccc == channels_bg_rgb(channels));
      }
      free(egc);
      ncvisual_destroy(ncv);
    }
  }
  CHECK(!notcurses_stop(nc_));
}
