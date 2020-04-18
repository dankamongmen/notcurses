#include "main.h"
#include "internal.h"
#include <cstring>
#include <iostream>

TEST_CASE("Plot") {
  if(!enforce_utf8()){
    return;
  }
  if(getenv("TERM") == nullptr){
    return;
  }
  notcurses_options nopts{};
  nopts.inhibit_alternate_screen = true;
  nopts.suppress_banner = true;
  FILE* outfp_ = fopen("/dev/tty", "wb");
  REQUIRE(outfp_);
  struct notcurses* nc_ = notcurses_init(&nopts, outfp_);
  REQUIRE(nc_);
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);
  REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));

  // setting miny == maxy with non-zero domain limits is invalid
  SUBCASE("DetectRangeBadY"){
    ncplot_options popts{};
    popts.maxy = popts.miny = -1;
    ncplot* p = ncplot_create(n_, &popts);
    CHECK(nullptr == p);
    popts.miny = 1;
    popts.maxy = 1;
    p = ncplot_create(n_, &popts);
    CHECK(nullptr == p);
    popts.miny = 0;
    popts.maxy = 0;
    p = ncplot_create(n_, &popts);
    REQUIRE(nullptr != p);
    ncplot_destroy(p);
  }

  // maxy < miny is invalid
  SUBCASE("RejectMaxyLessMiny"){
    ncplot_options popts{};
    popts.miny = 2;
    popts.maxy = 1;
    ncplot* p = ncplot_create(n_, &popts);
    CHECK(nullptr == p);
  }

  SUBCASE("SimplePlot"){
    ncplot_options popts{};
    ncplot* p = ncplot_create(n_, &popts);
    REQUIRE(p);
    CHECK(n_ == ncplot_plane(p));
    ncplot_destroy(p);
  }

  // 5-ary slot space without any window movement
  SUBCASE("AugmentSamples5"){
    ncplot_options popts{};
    popts.rangex = 5;
    popts.maxy = 10;
    popts.miny = 0;
    ncplot* p = ncplot_create(n_, &popts);
    REQUIRE(p);
    CHECK(0 == p->slots[0]);
    CHECK(0 == ncplot_add_sample(p, 0, 1));
    CHECK(1 == p->slots[0]);
    CHECK(0 == ncplot_add_sample(p, 0, 1));
    CHECK(2 == p->slots[0]);
    CHECK(0 == p->slots[1]);
    CHECK(0 == p->slots[2]);
    CHECK(0 == ncplot_add_sample(p, 2, 3));
    CHECK(3 == p->slots[2]);
    CHECK(0 == ncplot_set_sample(p, 2, 3));
    CHECK(3 == p->slots[2]);
    CHECK(0 == p->slots[3]);
    CHECK(0 == p->slots[4]);
    CHECK(0 == ncplot_add_sample(p, 4, 6));
    CHECK(6 == p->slots[4]);
    CHECK(2 == p->slots[0]);
    CHECK(4 == p->slotx);
    ncplot_destroy(p);
  }

  // 2-ary slot space with window movement
  SUBCASE("AugmentCycle2"){
    ncplot_options popts{};
    popts.rangex = 2;
    popts.maxy = 10;
    popts.miny = 0;
    ncplot* p = ncplot_create(n_, &popts);
    REQUIRE(p);
    CHECK(0 == p->slots[0]);
    CHECK(0 == ncplot_add_sample(p, 0, 1));
    CHECK(1 == p->slots[0]);
    CHECK(0 == ncplot_add_sample(p, 0, 1));
    CHECK(2 == p->slots[0]);
    CHECK(0 == ncplot_set_sample(p, 1, 5));
    CHECK(5 == p->slots[1]);
    CHECK(0 == ncplot_set_sample(p, 2, 9));
    CHECK(5 == p->slots[1]);
    CHECK(9 == p->slots[0]);
    CHECK(0 == ncplot_add_sample(p, 3, 4));
    CHECK(9 == p->slots[0]);
    CHECK(4 == p->slots[1]);
    CHECK(3 == p->slotx);
    CHECK(0 == ncplot_add_sample(p, 5, 1));
    CHECK(1 == p->slots[0]);
    CHECK(0 == p->slots[1]);
    CHECK(5 == p->slotx);
    ncplot_destroy(p);
  }

  // augment past the window, ensuring everything gets zeroed
  SUBCASE("AugmentLong"){
    ncplot_options popts{};
    popts.rangex = 5;
    popts.maxy = 10;
    popts.miny = 0;
    ncplot* p = ncplot_create(n_, &popts);
    REQUIRE(p);
    for(int x = 0 ; x < 5 ; ++x){
      CHECK(0 == p->slots[x]);
    }
    CHECK(0 == ncplot_add_sample(p, 4, 4));
    for(int x = 0 ; x < 4 ; ++x){
      CHECK(0 == p->slots[x]);
    }
    CHECK(4 == p->slots[4]);
    CHECK(0 == ncplot_add_sample(p, 10, 5));
    CHECK(5 == p->slots[0]);
    for(int x = 1 ; x < 4 ; ++x){
      CHECK(0 == p->slots[x]);
    }
    CHECK(0 == ncplot_add_sample(p, 24, 7));
    CHECK(7 == p->slots[0]);
    for(int x = 1 ; x < 5 ; ++x){
      CHECK(0 == p->slots[x]);
    }
    CHECK(0 == ncplot_add_sample(p, 100, 0));
    for(int x = 0 ; x < 5 ; ++x){
      CHECK(0 == p->slots[x]);
    }
    ncplot_destroy(p);
  }

  //  FIXME need some rendering tests, one for each geometry

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));
}
