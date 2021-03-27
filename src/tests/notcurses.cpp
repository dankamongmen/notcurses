#include <string>
#include <cstdlib>
#include <iostream>
#include "main.h"
#include "internal.h"

TEST_CASE("NotcursesBase") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }

  SUBCASE("NotcursesVersionString") {
    const char* ver = notcurses_version();
    REQUIRE(ver);
    REQUIRE(0 < strlen(ver));
    std::cout << "notcurses version " << ver << std::endl;
  }

  SUBCASE("TermDimensions") {
    int x, y;
    notcurses_term_dim_yx(nc_, &y, &x);
    auto stry = getenv("LINES");
    if(stry){
      auto envy = std::stoi(stry, nullptr);
      CHECK(envy == y);
    }
    auto strx = getenv("COLUMNS");
    if(stry){
      auto envx = std::stoi(strx, nullptr);
      CHECK(envx == x);
    }
  }

  SUBCASE("RefreshSameSize") {
    int x, y;
    notcurses_term_dim_yx(nc_, &y, &x);
    int newx, newy;
    CHECK(0 == notcurses_refresh(nc_, &newy, &newx));
    CHECK(newx == x);
    CHECK(newy == y);
  }

  // we should at least have NCSTYLE_BOLD everywhere, i should think?
  SUBCASE("CursesStyles") {
    unsigned attrs = notcurses_supported_styles(nc_);
    CHECK(1 == !!(NCSTYLE_BOLD & attrs));
  }

  // it is an error to attempt to destroy the standard plane
  SUBCASE("RejectDestroyStdPlane") {
    ncplane* ncp = notcurses_stdplane(nc_);
    REQUIRE(ncp);
    REQUIRE(0 > ncplane_destroy(ncp));
  }

  // it is an error to attempt to move the standard plane
  SUBCASE("RejectMoveStdPlane") {
    ncplane* ncp = notcurses_stdplane(nc_);
    REQUIRE(ncp);
    REQUIRE(0 > ncplane_move_yx(ncp, 1, 1));
  }

  // create planes partitioning the entirety of the screen, one at each coordinate
  SUBCASE("TileScreenWithPlanes") {
    int maxx, maxy;
    notcurses_term_dim_yx(nc_, &maxy, &maxx);
    auto total = maxx * maxy;
    auto planes = new struct ncplane*[total];
    int* planesecrets = new int[total];
    for(int y = 0 ; y < maxy ; ++y){
      for(int x = 0 ; x < maxx ; ++x){
        const auto idx = y * maxx + x;
        struct ncplane_options nopts = {
          .y = y,
          .x = x,
          .rows = 1,
          .cols = 1,
          .userptr = &planesecrets[idx],
          .name = nullptr,
          .resizecb = nullptr,
          .flags = 0,
          .margin_b = 0, .margin_r = 0,
        };
        planes[idx] = ncplane_create(notcurses_stdplane(nc_), &nopts);
        REQUIRE(planes[idx]);
      }
    }
    REQUIRE(0 == notcurses_render(nc_));
    for(int y = 0 ; y < maxy ; ++y){
      for(int x = 0 ; x < maxx ; ++x){
        const auto idx = y * maxx + x;
        auto userptr = ncplane_userptr(planes[idx]);
        REQUIRE(userptr);
        CHECK(userptr == &planesecrets[idx]);
        REQUIRE(0 == ncplane_destroy(planes[idx]));
      }
    }
    delete[] planesecrets;
    delete[] planes;
    REQUIRE(0 == notcurses_render(nc_));
  }

  SUBCASE("ChannelSetFGAlpha"){
    uint64_t channel = 0;
    CHECK(0 > channels_set_fg_alpha(&channel, -1));
    CHECK(0 > channels_set_fg_alpha(&channel, 4));
    CHECK(0 == channels_set_fg_alpha(&channel, CELL_ALPHA_OPAQUE));
    CHECK(CELL_ALPHA_OPAQUE == channels_fg_alpha(channel));
    CHECK(channels_fg_default_p(channel));
    CHECK(channels_bg_default_p(channel));
    CHECK(0 == channels_set_fg_alpha(&channel, CELL_ALPHA_HIGHCONTRAST));
    CHECK(CELL_ALPHA_HIGHCONTRAST == channels_fg_alpha(channel));
    CHECK(!channels_fg_default_p(channel));
    CHECK(channels_bg_default_p(channel));
  }

  SUBCASE("ChannelSetBGAlpha"){
    uint64_t channel = 0;
    CHECK(0 > channels_set_bg_alpha(&channel, -1));
    CHECK(0 > channels_set_bg_alpha(&channel, 4));
    CHECK(0 == channels_set_bg_alpha(&channel, CELL_ALPHA_OPAQUE));
    CHECK(CELL_ALPHA_OPAQUE == channels_bg_alpha(channel));
    CHECK(0 == channels_set_bg_alpha(&channel, CELL_ALPHA_TRANSPARENT));
    CHECK(0 > channels_set_bg_alpha(&channel, CELL_ALPHA_HIGHCONTRAST));
    CHECK(CELL_ALPHA_TRANSPARENT == channels_bg_alpha(channel));
    CHECK(channels_fg_default_p(channel));
    CHECK(!channels_bg_default_p(channel));
  }

  SUBCASE("Stats"){
    struct ncstats stats;
    notcurses_stats(nc_, &stats);
    CHECK(0 == stats.renders);
    CHECK(0 == notcurses_render(nc_));
    notcurses_stats(nc_, &stats);
    CHECK(1 == stats.renders);
    notcurses_stats_reset(nc_, &stats);
    notcurses_stats(nc_, &stats);
    CHECK(0 == stats.renders);
  }

  CHECK(0 == notcurses_stop(nc_));

}
