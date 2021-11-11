#include "main.h"
#include <cstring>
#include <iostream>

TEST_CASE("Plot") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  if(!notcurses_canutf8(nc_)){
    CHECK(0 == notcurses_stop(nc_));
    return;
  }
  auto n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);
  REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));

  // setting miny == maxy with non-zero domain limits is invalid
  SUBCASE("DetectRangeBadY"){
    ncplot_options popts{};
    auto p = ncuplot_create(n_, &popts, -1, -1);
    CHECK(nullptr == p);
    p = ncuplot_create(n_, &popts, 1, 1);
    CHECK(nullptr == p);
    p = ncuplot_create(n_, &popts, 0, 0);
    REQUIRE(nullptr != p);
    ncuplot_destroy(p);
  }

  // maxy < miny is invalid
  SUBCASE("RejectMaxyLessMiny"){
    ncplot_options popts{};
    auto p = ncuplot_create(n_, &popts, 2, 1);
    CHECK(nullptr == p);
  }

  SUBCASE("SimplePlot"){
    ncplot_options popts{};
    auto p = ncuplot_create(n_, &popts, 0, 0);
    REQUIRE(p);
    CHECK(n_ == ncuplot_plane(p));
    ncuplot_destroy(p);
  }

  // 5-ary slot space without any window movement
  SUBCASE("AugmentSamples5"){
    ncplot_options popts{};
    popts.rangex = 5;
    struct ncuplot* p = ncuplot_create(n_, &popts, 0, 10);
    uint64_t y;
    CHECK(0 == ncuplot_sample(p, 0, &y));
    CHECK(0 == y);
    CHECK(0 == ncuplot_add_sample(p, (uint64_t)0, (uint64_t)1));
    CHECK(0 == ncuplot_sample(p, 0, &y));
    CHECK(1 == y);
    CHECK(0 == ncuplot_add_sample(p, (uint64_t)0, (uint64_t)1));
    CHECK(0 == ncuplot_sample(p, 0, &y));
    CHECK(2 == y);
    CHECK(-1 == ncuplot_sample(p, 1, &y));
    CHECK(-1 == ncuplot_sample(p, 2, &y));
    CHECK(0 == ncuplot_add_sample(p, (uint64_t)2, (uint64_t)3));
    CHECK(0 == ncuplot_sample(p, 2, &y));
    CHECK(3 == y);
    CHECK(0 == ncuplot_set_sample(p, (uint64_t)2, (uint64_t)3));
    CHECK(0 == ncuplot_sample(p, 2, &y));
    CHECK(3 == y);
    CHECK(-1 == ncuplot_sample(p, 3, &y));
    CHECK(-1 == ncuplot_sample(p, 4, &y));
    CHECK(0 == ncuplot_add_sample(p, (uint64_t)4, (uint64_t)6));
    CHECK(0 == ncuplot_sample(p, 4, &y));
    CHECK(6 == y);
    CHECK(0 == ncuplot_sample(p, 0, &y));
    CHECK(2 == y);
    //CHECK(4 == p->slotx);
    ncuplot_destroy(p);
  }

  // 2-ary slot space with window movement
  SUBCASE("AugmentCycle2"){
    ncplot_options popts{};
    popts.rangex = 2;
    struct ncuplot* p = ncuplot_create(n_, &popts, 0, 10);
    uint64_t y;
    CHECK(0 == ncuplot_sample(p, 0, &y));
    CHECK(0 == ncuplot_add_sample(p, (uint64_t)0, (uint64_t)1));
    CHECK(0 == ncuplot_sample(p, 0, &y));
    CHECK(1 == y);
    CHECK(0 == ncuplot_add_sample(p, (uint64_t)0, (uint64_t)1));
    CHECK(0 == ncuplot_sample(p, 0, &y));
    CHECK(2 == y);
    CHECK(0 == ncuplot_set_sample(p, (uint64_t)1, (uint64_t)5));
    CHECK(0 == ncuplot_sample(p, 1, &y));
    CHECK(5 == y);
    CHECK(0 == ncuplot_set_sample(p, (uint64_t)2, (uint64_t)9));
    CHECK(0 == ncuplot_sample(p, 1, &y));
    CHECK(5 == y);
    CHECK(-1 == ncuplot_sample(p, 0, &y));
    CHECK(0 == ncuplot_add_sample(p, (uint64_t)3, (uint64_t)4));
    CHECK(-1 == ncuplot_sample(p, 0, &y));
    CHECK(-1 == ncuplot_sample(p, 1, &y));
    //CHECK(3 == p->slotx);
    CHECK(0 == ncuplot_add_sample(p, (uint64_t)5, (uint64_t)1));
    CHECK(-1 == ncuplot_sample(p, 0, &y));
    CHECK(-1 == ncuplot_sample(p, 1, &y));
    //CHECK(5 == p.slotx);
    ncuplot_destroy(p);
  }

  // augment past the window, ensuring everything gets zeroed
  SUBCASE("AugmentLong"){
    ncplot_options popts{};
    popts.rangex = 5;
    struct ncuplot* p = ncuplot_create(n_, &popts, 0, 10);
    uint64_t y;
    CHECK(0 == ncuplot_sample(p, 0, &y));
    for(int x = 1 ; x < 5 ; ++x){
      CHECK(-1 == ncuplot_sample(p, x, &y));
    }
    CHECK(0 == ncuplot_add_sample(p, (uint64_t)4, (uint64_t)4));
    for(int x = 1 ; x < 4 ; ++x){
      CHECK(0 == ncuplot_sample(p, x, &y));
    }
    CHECK(0 == ncuplot_sample(p, 4, &y));
    CHECK(4 == y);
    CHECK(0 == ncuplot_add_sample(p, (uint64_t)10, (uint64_t)5));
    for(int x = 0 ; x < 4 ; ++x){
      CHECK(-1 == ncuplot_sample(p, x, &y));
    }
    CHECK(0 == ncuplot_add_sample(p, (uint64_t)24, (uint64_t)7));
    for(int x = 0 ; x < 5 ; ++x){
      CHECK(-1 == ncuplot_sample(p, x, &y));
    }
    CHECK(0 == ncuplot_add_sample(p, (uint64_t)100, (uint64_t)0));
    for(int x = 0 ; x < 5 ; ++x){
      CHECK(-1 == ncuplot_sample(p, x, &y));
    }
    ncuplot_destroy(p);
  }

  SUBCASE("SimpleFloatPlot"){
    ncplot_options popts{};
    auto p = ncdplot_create(n_, &popts, 0, 0);
    REQUIRE(p);
    CHECK(n_ == ncdplot_plane(p));
    ncdplot_destroy(p);
  }

  SUBCASE("BraillePlot") {
    ncplane_options nopts = {
      .y = 1, .x = 1, .rows = 6, .cols = 50,
      .userptr = nullptr, .name = "plot", .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto ncp = ncplane_create(n_, &nopts);
    REQUIRE(ncp);
    ncplane_set_base(ncp, " ", 0, NCCHANNELS_INITIALIZER(0x80, 0, 0, 0, 0, 0x80));
    ncplot_options popts;
    memset(&popts, 0, sizeof(popts));
    popts.maxchannels = NCCHANNELS_INITIALIZER(0xff, 0xff, 0xff, 0, 0, 0);
    popts.minchannels = NCCHANNELS_INITIALIZER(0, 0xff, 0, 0, 0, 0);
    ncchannels_set_bg_alpha(&popts.minchannels, NCALPHA_BLEND);
    ncchannels_set_fg_alpha(&popts.minchannels, NCALPHA_BLEND);
    popts.gridtype = NCBLIT_BRAILLE;
    auto p = ncuplot_create(ncp, &popts, 0, 0);
    REQUIRE(p);
    for(auto i = 0 ; i < 100 ; ++i){
      CHECK(0 == ncuplot_add_sample(p, i, i));
    }
    CHECK(0 == notcurses_render(nc_));
    uint64_t channels;
    uint16_t smask;
    if(notcurses_canbraille(nc_)){
      // FIXME loop throughout plane, check all cells
      auto egc = ncplane_at_yx(ncp, 5, 49, &smask, &channels);
      CHECK(0 == strcmp(egc, "⣿"));
      free(egc);
    }
    ncuplot_destroy(p);
  }

  SUBCASE("QuadPlot1Row") {
    ncplane_options nopts = {
      .y = 1, .x = 1, .rows = 1, .cols = 9,
      .userptr = nullptr, .name = "plot", .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto ncp = ncplane_create(n_, &nopts);
    REQUIRE(ncp);
    ncplot_options popts;
    memset(&popts, 0, sizeof(popts));
    popts.maxchannels = NCCHANNELS_INITIALIZER(0xff, 0xff, 0xff, 0, 0, 0);
    popts.minchannels = NCCHANNELS_INITIALIZER(0, 0xff, 0, 0, 0, 0);
    ncchannels_set_bg_alpha(&popts.minchannels, NCALPHA_BLEND);
    ncchannels_set_fg_alpha(&popts.minchannels, NCALPHA_BLEND);
    popts.gridtype = NCBLIT_2x2;
    auto p = ncuplot_create(ncp, &popts, 0, 0);
    REQUIRE(p);
    for(auto i = 0 ; i < 3 ; ++i){
      for(auto j = 0 ; j < 3 ; ++j){
        CHECK(0 == ncuplot_add_sample(p, i * 6 + j * 2, i));
        CHECK(0 == ncuplot_add_sample(p, i * 6 + j * 2 + 1, j));
      }
    }
    CHECK(0 == notcurses_render(nc_));
    ncuplot_destroy(p);
  }

  SUBCASE("EighthsPlot1Row") {
    ncplane_options nopts = {
      .y = 1, .x = 1, .rows = 1, .cols = 9,
      .userptr = nullptr, .name = "plot", .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto ncp = ncplane_create(n_, &nopts);
    REQUIRE(ncp);
    ncplot_options popts;
    memset(&popts, 0, sizeof(popts));
    popts.maxchannels = NCCHANNELS_INITIALIZER(0xff, 0xff, 0xff, 0, 0, 0);
    popts.minchannels = NCCHANNELS_INITIALIZER(0, 0xff, 0, 0, 0, 0);
    ncchannels_set_bg_alpha(&popts.minchannels, NCALPHA_BLEND);
    ncchannels_set_fg_alpha(&popts.minchannels, NCALPHA_BLEND);
    popts.gridtype = NCBLIT_8x1;
    auto p = ncuplot_create(ncp, &popts, 0, 0);
    REQUIRE(p);
    for(auto i = 0 ; i < 9 ; ++i){
      CHECK(0 == ncuplot_add_sample(p, i, i));
    }
    CHECK(0 == notcurses_render(nc_));
    ncuplot_destroy(p);
  }

  SUBCASE("SextantPlot1Row") {
    ncplane_options nopts = {
      .y = 1, .x = 1, .rows = 1, .cols = 16,
      .userptr = nullptr, .name = "plot", .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto ncp = ncplane_create(n_, &nopts);
    REQUIRE(ncp);
    ncplot_options popts;
    memset(&popts, 0, sizeof(popts));
    popts.maxchannels = NCCHANNELS_INITIALIZER(0xff, 0xff, 0xff, 0, 0, 0);
    popts.minchannels = NCCHANNELS_INITIALIZER(0, 0xff, 0, 0, 0, 0);
    ncchannels_set_bg_alpha(&popts.minchannels, NCALPHA_BLEND);
    ncchannels_set_fg_alpha(&popts.minchannels, NCALPHA_BLEND);
    popts.gridtype = NCBLIT_3x2;
    auto p = ncuplot_create(ncp, &popts, 0, 0);
    REQUIRE(p);
    for(auto i = 0 ; i < 4 ; ++i){
      for(auto j = 0 ; j < 4 ; ++j){
        CHECK(0 == ncuplot_add_sample(p, i * 8 + j * 2, i));
        CHECK(0 == ncuplot_add_sample(p, i * 8 + j * 2 + 1, j));
      }
    }
    CHECK(0 == notcurses_render(nc_));
    ncuplot_destroy(p);
  }

  SUBCASE("BraillePlot1Row") {
    ncplane_options nopts = {
      .y = 1, .x = 1, .rows = 1, .cols = 25,
      .userptr = nullptr, .name = "plot", .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto ncp = ncplane_create(n_, &nopts);
    REQUIRE(ncp);
    ncplot_options popts;
    memset(&popts, 0, sizeof(popts));
    popts.maxchannels = NCCHANNELS_INITIALIZER(0xff, 0xff, 0xff, 0, 0, 0);
    popts.minchannels = NCCHANNELS_INITIALIZER(0, 0xff, 0, 0, 0, 0);
    ncchannels_set_bg_alpha(&popts.minchannels, NCALPHA_BLEND);
    ncchannels_set_fg_alpha(&popts.minchannels, NCALPHA_BLEND);
    popts.gridtype = NCBLIT_BRAILLE;
    auto p = ncuplot_create(ncp, &popts, 0, 0);
    REQUIRE(p);
    for(auto i = 0 ; i < 5 ; ++i){
      for(auto j = 0 ; j < 5 ; ++j){
        CHECK(0 == ncuplot_add_sample(p, i * 10 + j * 2, i));
        CHECK(0 == ncuplot_add_sample(p, i * 10 + j * 2 + 1, j));
      }
    }
    CHECK(0 == notcurses_render(nc_));
    ncuplot_destroy(p);
  }

  CHECK(0 == notcurses_stop(nc_));
}
