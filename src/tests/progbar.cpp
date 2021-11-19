#include "main.h"
#include <cstring>
#include <iostream>

int pbar_fill(struct notcurses* nc, struct ncprogbar* pbar){
  double p = 0;
  do{
    if(ncprogbar_set_progress(pbar, p)){
      return -1;
    }
    CHECK(0 == notcurses_render(nc));
    p += 0.1;
  }while(p <= 1.0);
  return 0;
}

struct ncprogbar* hbar_make(struct notcurses* nc, uint64_t flags){
  unsigned dimy, dimx;
  struct ncplane* std = notcurses_stddim_yx(nc, &dimy, &dimx);
  struct ncplane_options nopts = {
    .y = 1,
    .x = NCALIGN_CENTER,
    .rows = dimy - 4,
    .cols = 5,
    .userptr = nullptr,
    .name = "pbar",
    .resizecb = nullptr,
    .flags = NCPLANE_OPTION_HORALIGNED,
    .margin_b = 0,
    .margin_r = 0,
  };
  struct ncplane* pbar = ncplane_create(std, &nopts);
  if(pbar == NULL){
    return NULL;
  }
  int posy, posx;
  ncplane_yx(pbar, &posy, &posx);
  unsigned pdimy, pdimx;
  ncplane_dim_yx(pbar, &pdimy, &pdimx);
  CHECK(0 == ncplane_cursor_move_yx(std, posy - 1, posx - 1));
  uint64_t channels = 0;
  ncchannels_set_fg_rgb8(&channels, 0, 0xde, 0xde);
  if(ncplane_rounded_box(std, 0, channels, posy + pdimy, posx + pdimx, 0)){
    ncplane_destroy(pbar);
    return NULL;
  }
  struct ncprogbar_options popts{};
  popts.flags = flags;
  ncchannel_set_rgb8(&popts.ulchannel, 0x80, 0x22, 0x22);
  ncchannel_set_rgb8(&popts.urchannel, 0x22, 0x22, 0x80);
  ncchannel_set_rgb8(&popts.blchannel, 0x22, 0x80, 0x22);
  ncchannel_set_rgb8(&popts.brchannel, 0x80, 0x22, 0x22);
  struct ncprogbar* ncp = ncprogbar_create(pbar, &popts);
  if(ncp == NULL){
    return NULL;
  }
  return ncp;
}

static struct ncprogbar*
pbar_make(struct notcurses* nc, uint64_t flags){
  unsigned dimy, dimx;
  struct ncplane* std = notcurses_stddim_yx(nc, &dimy, &dimx);
  struct ncplane_options nopts = {
    .y = (int)dimy / 2,
    .x = NCALIGN_CENTER,
    .rows = 3,
    .cols = dimx - 20,
    .userptr = nullptr,
    .name = "pbar",
    .resizecb = nullptr,
    .flags = NCPLANE_OPTION_HORALIGNED,
    .margin_b = 0,
    .margin_r = 0,
  };
  struct ncplane* pbar = ncplane_create(std, &nopts);
  if(pbar == NULL){
    return NULL;
  }
  int posy, posx;
  ncplane_yx(pbar, &posy, &posx);
  unsigned pdimy, pdimx;
  ncplane_dim_yx(pbar, &pdimy, &pdimx);
  ncplane_cursor_move_yx(std, posy - 1, posx - 1);
  uint64_t channels = 0;
  ncchannels_set_fg_rgb8(&channels, 0, 0xde, 0xde);
  if(ncplane_rounded_box(std, 0, channels, posy + pdimy, posx + pdimx, 0)){
    ncplane_destroy(pbar);
    return NULL;
  }
  struct ncprogbar_options popts{};
  popts.flags = flags;
  ncchannel_set_rgb8(&popts.ulchannel, 0x80, 0xcc, 0xcc);
  ncchannel_set_rgb8(&popts.urchannel, 0xcc, 0xcc, 0x80);
  ncchannel_set_rgb8(&popts.blchannel, 0xcc, 0x80, 0xcc);
  ncchannel_set_rgb8(&popts.brchannel, 0x80, 0xcc, 0xcc);
  struct ncprogbar* ncp = ncprogbar_create(pbar, &popts);
  if(ncp == NULL){
    return NULL;
  }
  return ncp;
}

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

  SUBCASE("FullVert") {
    struct ncprogbar* ncp = pbar_make(nc_, 0);
    REQUIRE(nullptr != ncp);
    CHECK(0 == pbar_fill(nc_, ncp));
    ncprogbar_destroy(ncp);
  }

  SUBCASE("FullVertRetrograde") {
    struct ncprogbar* ncp = pbar_make(nc_, NCPROGBAR_OPTION_RETROGRADE);
    REQUIRE(nullptr != ncp);
    CHECK(0 == pbar_fill(nc_, ncp));
    ncprogbar_destroy(ncp);
  }

  SUBCASE("FullHoriz") {
    struct ncprogbar* ncp = hbar_make(nc_, 0);
    REQUIRE(nullptr != ncp);
    CHECK(0 == pbar_fill(nc_, ncp));
    ncprogbar_destroy(ncp);
  }

  SUBCASE("FullHorizRetrograde") {
    struct ncprogbar* ncp = hbar_make(nc_, NCPROGBAR_OPTION_RETROGRADE);
    REQUIRE(nullptr != ncp);
    CHECK(0 == pbar_fill(nc_, ncp));
    ncprogbar_destroy(ncp);
  }

  CHECK(0 == notcurses_stop(nc_));
}
