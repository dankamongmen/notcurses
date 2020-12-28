#include "main.h"
#include <cstring>
#include <iostream>

TEST_CASE("ProgressBar") {
  auto nc_ = testing_notcurses();
  if(!nc_){
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
    };
    const char* egcs[] = { " ", "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█" };
    auto n = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != n);
    auto pbar = ncprogbar_create(n, NULL);
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
  
  CHECK(0 == notcurses_stop(nc_));
}
