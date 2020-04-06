#include "main.h"
#include <array>
#include <cstdlib>
#include "internal.h"

TEST_CASE("Scrolling") {
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

  // verify that the standard plane has scrolling disabled initially, and that
  // we can enable it at runtime.
  SUBCASE("ScollingDisabledStdplane"){
    CHECK(!ncplane_set_scrolling(n_, true));
    CHECK(ncplane_set_scrolling(n_, false));
  }

  SUBCASE("ScrollingStr"){
    struct ncplane* n = ncplane_new(nc_, 2, 20, 1, 1, nullptr);
    REQUIRE(n);
    // verify that the new plane was started without scrolling
    CHECK(!ncplane_set_scrolling(n, false));
    // try to write 40 EGCs; it ought fail
    CHECK(0 > ncplane_putstr(n, "01234567890123456789012345678901234567689"));
    CHECK(0 == ncplane_cursor_move_yx(n, 0, 0));
    CHECK(!ncplane_set_scrolling(n, true)); // enable scrolling
    // try to write 40 EGCs; it ought succeed
    CHECK(40 == ncplane_putstr(n, "01234567890123456789012345678901234567689"));
    int y, x;
    ncplane_cursor_yx(n, &y, &x);
    CHECK(1 == y);
    CHECK(20 == x);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("ScrollingSplitStr"){
    struct ncplane* n = ncplane_new(nc_, 2, 20, 1, 1, nullptr);
    REQUIRE(n);
    CHECK(20 == ncplane_putstr(n, "01234567890123456789"));
    int y, x;
    ncplane_cursor_yx(n, &y, &x);
    CHECK(0 == y);
    CHECK(20 == x);
    CHECK(0 > ncplane_putstr(n, "01234567890123456789"));
    CHECK(!ncplane_set_scrolling(n, true)); // enable scrolling
    CHECK(20 > ncplane_putstr(n, "01234567890123456789"));
    ncplane_cursor_yx(n, &y, &x);
    CHECK(1 == y);
    CHECK(20 == x);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("ScrollingEGC"){
    const char* out = "01234567890123456789012345678901234567689";
    struct ncplane* n = ncplane_new(nc_, 2, 20, 1, 1, nullptr);
    REQUIRE(n);
    // verify that the new plane was started without scrolling
    CHECK(!ncplane_set_scrolling(n, false));
    // try to write 40 EGCs; the last 20 ought fail
    for(const char* c = out ; *c ; ++c){
      int ret = ncplane_putsimple(n, *c);
      if(c - out < 20){
        CHECK(1 == ret);
      }else{
        CHECK(0 > ret);
      }
    }
    int y, x;
    ncplane_cursor_yx(n, &y, &x);
    CHECK(0 == y);
    CHECK(20 == x);
    CHECK(0 == ncplane_cursor_move_yx(n, 0, 0));
    CHECK(!ncplane_set_scrolling(n, true)); // enable scrolling
    // try to write 40 EGCs; all ought succeed
    for(const char* c = out ; *c ; ++c){
      CHECK(1 == ncplane_putsimple(n, *c));
    }
    ncplane_cursor_yx(n, &y, &x);
    CHECK(1 == y);
    CHECK(20 == x);
    CHECK(0 == notcurses_render(nc_));
  }

  // FIXME add one verifying boxes don't exceed the right side
  // FIXME add one where we go past the end of the plane

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));

}
