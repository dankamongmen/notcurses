#include "main.h"
#include "cpp.h"
#include <cstring>
#include <iostream>

TEST_CASE("Plot") {
  if(getenv("TERM") == nullptr){
    return;
  }
  notcurses_options nopts{};
  nopts.inhibit_alternate_screen = true;
  nopts.suppress_banner = true;
  auto outfp_ = fopen("/dev/tty", "wb");
  REQUIRE(outfp_);
  auto nc_ = notcurses_init(&nopts, outfp_);
  REQUIRE(nc_);
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
    ncppplot<uint64_t> p;
    ncppplot<uint64_t>::create(&p, n_, &popts, 0, 10);
    CHECK(0 == p.slots[0]);
    CHECK(0 == p.add_sample((uint64_t)0, (uint64_t)1));
    CHECK(1 == p.slots[0]);
    CHECK(0 == p.add_sample((uint64_t)0, (uint64_t)1));
    CHECK(2 == p.slots[0]);
    CHECK(0 == p.slots[1]);
    CHECK(0 == p.slots[2]);
    CHECK(0 == p.add_sample((uint64_t)2, (uint64_t)3));
    CHECK(3 == p.slots[2]);
    CHECK(0 == p.set_sample((uint64_t)2, (uint64_t)3));
    CHECK(3 == p.slots[2]);
    CHECK(0 == p.slots[3]);
    CHECK(0 == p.slots[4]);
    CHECK(0 == p.add_sample((uint64_t)4, (uint64_t)6));
    CHECK(6 == p.slots[4]);
    CHECK(2 == p.slots[0]);
    CHECK(4 == p.slotx);
    p.destroy();
  }

  // 2-ary slot space with window movement
  SUBCASE("AugmentCycle2"){
    ncplot_options popts{};
    popts.rangex = 2;
    ncppplot<uint64_t> p;
    ncppplot<uint64_t>::create(&p, n_, &popts, 0, 10);
    CHECK(0 == p.slots[0]);
    CHECK(0 == p.add_sample((uint64_t)0, (uint64_t)1));
    CHECK(1 == p.slots[0]);
    CHECK(0 == p.add_sample((uint64_t)0, (uint64_t)1));
    CHECK(2 == p.slots[0]);
    CHECK(0 == p.set_sample((uint64_t)1, (uint64_t)5));
    CHECK(5 == p.slots[1]);
    CHECK(0 == p.set_sample((uint64_t)2, (uint64_t)9));
    CHECK(5 == p.slots[1]);
    CHECK(9 == p.slots[0]);
    CHECK(0 == p.add_sample((uint64_t)3, (uint64_t)4));
    CHECK(9 == p.slots[0]);
    CHECK(4 == p.slots[1]);
    CHECK(3 == p.slotx);
    CHECK(0 == p.add_sample((uint64_t)5, (uint64_t)1));
    CHECK(1 == p.slots[0]);
    CHECK(0 == p.slots[1]);
    CHECK(5 == p.slotx);
    p.destroy();
  }

  // augment past the window, ensuring everything gets zeroed
  SUBCASE("AugmentLong"){
    ncplot_options popts{};
    popts.rangex = 5;
    ncppplot<uint64_t> p;
    ncppplot<uint64_t>::create(&p, n_, &popts, 0, 10);
    for(int x = 0 ; x < 5 ; ++x){
      CHECK(0 == p.slots[x]);
    }
    CHECK(0 == p.add_sample((uint64_t)4, (uint64_t)4));
    for(int x = 0 ; x < 4 ; ++x){
      CHECK(0 == p.slots[x]);
    }
    CHECK(4 == p.slots[4]);
    CHECK(0 == p.add_sample((uint64_t)10, (uint64_t)5));
    CHECK(5 == p.slots[0]);
    for(int x = 1 ; x < 4 ; ++x){
      CHECK(0 == p.slots[x]);
    }
    CHECK(0 == p.add_sample((uint64_t)24, (uint64_t)7));
    CHECK(7 == p.slots[0]);
    for(int x = 1 ; x < 5 ; ++x){
      CHECK(0 == p.slots[x]);
    }
    CHECK(0 == p.add_sample((uint64_t)100, (uint64_t)0));
    for(int x = 0 ; x < 5 ; ++x){
      CHECK(0 == p.slots[x]);
    }
    p.destroy();
  }

  //  FIXME need some high-level rendering tests, one for each geometry

  SUBCASE("SimpleFloatPlot"){
    ncplot_options popts{};
    auto p = ncdplot_create(n_, &popts, 0, 0);
    REQUIRE(p);
    CHECK(n_ == ncdplot_plane(p));
    ncdplot_destroy(p);
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));
}
