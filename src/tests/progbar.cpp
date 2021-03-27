#include "main.h"
#include <cstring>
#include <iostream>

TEST_CASE("ProgressBar") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  if(!notcurses_canutf8(nc_)){
    CHECK(0 == notcurses_stop(nc_));
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);
  REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));

  // a single-cell progress bar progressing from bottom to top in 1/8 chunks
  SUBCASE("SingleCellUp") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr,
      .name = "pbar",
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    const char* egcs[] = { " ", "▁", "▂", "▃", "▄", "▅", "▆", "▇", "▀" };
    auto n = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != n);
    auto pbar = ncprogbar_create(n, nullptr);
    for(int i = 0 ; i < 9 ; ++i){
      double p = i / 8.0;
      CHECK(0 == ncprogbar_set_progress(pbar, p));
      CHECK(0 == notcurses_render(nc_));
      uint16_t smask;
      uint64_t channels;
      char* egc = notcurses_at_yx(nc_, 0, 0, &smask, &channels);
      REQUIRE(nullptr != egc);
      CHECK(0 == strcmp(egc, egcs[i]));
      free(egc);
    }
    ncprogbar_destroy(pbar);
  }
  
  SUBCASE("SingleCellDown") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr,
      .name = "pbar",
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    const char* egcs[] = { " ", "▔", "🮂", "🮃", "▀", "🮄", "🮅", "🮆", "▀"};
    auto n = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != n);
    struct ncprogbar_options popts = {
      .ulchannel = 0,
      .urchannel = 0,
      .blchannel = 0,
      .brchannel = 0,
      .flags = NCPROGBAR_OPTION_RETROGRADE,
    };
    auto pbar = ncprogbar_create(n, &popts);
    for(int i = 0 ; i < 9 ; ++i){
      double p = i / 8.0;
      CHECK(0 == ncprogbar_set_progress(pbar, p));
      CHECK(0 == notcurses_render(nc_));
      uint16_t smask;
      uint64_t channels;
      char* egc = notcurses_at_yx(nc_, 0, 0, &smask, &channels);
      REQUIRE(nullptr != egc);
      CHECK(0 == strcmp(egc, egcs[i]));
      free(egc);
    }
    ncprogbar_destroy(pbar);
  }

  SUBCASE("DualCellLeft") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 2,
      .userptr = nullptr,
      .name = "pbar",
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    const char* egcs[] = { " ", "▏", "▎", "▍", "▌", "▋", "▊", "▉" };
    auto n = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != n);
    struct ncprogbar_options popts = {
      .ulchannel = 0,
      .urchannel = 0,
      .blchannel = 0,
      .brchannel = 0,
      .flags = 0,
    };
    auto pbar = ncprogbar_create(n, &popts);
    double p = 0;
    int i = 0;
    do{
      CHECK(0 == ncprogbar_set_progress(pbar, p));
      CHECK(0 == notcurses_render(nc_));
      uint16_t smask;
      uint64_t channels;
      char* egc = notcurses_at_yx(nc_, 0, i / 8, &smask, &channels);
      REQUIRE(nullptr != egc);
      CHECK(0 == strcmp(egc, egcs[i % 8]));
      free(egc);
      p += 0.0625;
      ++i;
    }while(p < 1);
    ncprogbar_destroy(pbar);
  }

  SUBCASE("DualCellRight") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 2,
      .userptr = nullptr,
      .name = "pbar",
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    const char* egcs[] = { " ", "🮇", "🮇", "🮈", "▐", "🮉", "🮊", "🮋" };
    auto n = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != n);
    struct ncprogbar_options popts = {
      .ulchannel = 0,
      .urchannel = 0,
      .blchannel = 0,
      .brchannel = 0,
      .flags = NCPROGBAR_OPTION_RETROGRADE,
    };
    auto pbar = ncprogbar_create(n, &popts);
    double p = 0;
    int i = 0;
    do{
      CHECK(0 == ncprogbar_set_progress(pbar, p));
      CHECK(0 == notcurses_render(nc_));
      uint16_t smask;
      uint64_t channels;
      char* egc = notcurses_at_yx(nc_, 0, 1 - i / 8, &smask, &channels);
      REQUIRE(nullptr != egc);
      CHECK(0 == strcmp(egc, egcs[i % 8]));
      free(egc);
      p += 0.0625;
      ++i;
    }while(p < 1);
    ncprogbar_destroy(pbar);
  }

  CHECK(0 == notcurses_stop(nc_));
}
