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
  SUBCASE("ScollingDisabledStdplane") {
    CHECK(!ncplane_set_scrolling(n_, true));
    CHECK(ncplane_set_scrolling(n_, false));
  }

  SUBCASE("ScrollingStr") {
    struct ncplane* n = ncplane_new(nc_, 2, 20, 1, 1, nullptr);
    REQUIRE(n);
    // verify that the new plane was started without scrolling
    CHECK(!ncplane_set_scrolling(n, false));
    // try to write 40 EGCs; it ought fail after 20
    CHECK(-20 == ncplane_putstr(n, "0123456789012345678901234567890123456789"));
    int y, x;
    ncplane_cursor_yx(n, &y, &x);
    CHECK(0 == y);
    CHECK(20 == x);
    CHECK(!ncplane_set_scrolling(n, true)); // enable scrolling
    // try to write 40 EGCs from origin; it ought succeed
    CHECK(40 == ncplane_putstr_yx(n, 0, 0, "0123456789012345678901234567890123456789"));
    ncplane_cursor_yx(n, &y, &x);
    CHECK(1 == y);
    CHECK(20 == x);
    CHECK(0 == notcurses_render(nc_));
  }

  // even when scrolling is enabled, you aren't allowed to move the cursor
  // off-plane, or initiate output there
  SUBCASE("NoScrollingManually") {
    struct ncplane* n = ncplane_new(nc_, 2, 20, 1, 1, nullptr);
    REQUIRE(n);
    CHECK(!ncplane_set_scrolling(n, true)); // enable scrolling
    CHECK(0 > ncplane_cursor_move_yx(n, 0, 20));
    CHECK(0 > ncplane_cursor_move_yx(n, 1, 20));
    CHECK(0 > ncplane_cursor_move_yx(n, 2, 2));
    CHECK(0 > ncplane_putsimple_yx(n, 0, 20, 'c'));
    CHECK(0 > ncplane_putsimple_yx(n, 1, 20, 'c'));
    CHECK(0 > ncplane_putsimple_yx(n, 2, 0, 'c'));
  }

  // verify that two strings, each the length of the plane, can be output when
  // scrolling is enabled (the second ought get an error without scrolling)
  SUBCASE("ScrollingSplitStr") {
    struct ncplane* n = ncplane_new(nc_, 2, 20, 1, 1, nullptr);
    REQUIRE(n);
    CHECK(20 == ncplane_putstr(n, "01234567890123456789"));
    int y, x;
    ncplane_cursor_yx(n, &y, &x);
    CHECK(0 == y);
    CHECK(20 == x);
    CHECK(0 == ncplane_putstr(n, "01234567890123456789"));
    CHECK(!ncplane_set_scrolling(n, true)); // enable scrolling
    CHECK(20 == ncplane_putstr(n, "01234567890123456789"));
    ncplane_cursor_yx(n, &y, &x);
    CHECK(1 == y);
    CHECK(20 == x);
    CHECK(0 == notcurses_render(nc_));
  }

  // verify that a single string the size of the plane can be output when
  // scrolling is enabled (it ought be an error without scrolling)
  SUBCASE("ScrollingEGC") {
    const char* out = "0123456789012345678901234567890123456789";
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

  // ensure that if we draw a box on a scrolling plane, it stops at the right
  // side, as opposed to scrolling and making a horrible mess
  SUBCASE("ScrollingBoxen") {
    struct ncplane* n = ncplane_new(nc_, 4, 20, 1, 1, nullptr);
    REQUIRE(n);
    // verify that the new plane was started without scrolling
    CHECK(!ncplane_set_scrolling(n, true));
    cell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
    cell dl = CELL_TRIVIAL_INITIALIZER, dr = CELL_TRIVIAL_INITIALIZER;
    cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
    CHECK(0 == cells_double_box(n, 0, 0, &ul, &ur, &dl, &dr, &hl, &vl));
    CHECK(0 > ncplane_box_sized(n, &ul, &ur, &dl, &dr, &hl, &vl, 2, 25, 0));
    CHECK(0 > ncplane_box_sized(n, &ul, &ur, &dl, &dr, &hl, &vl, 2, 21, 0));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("ScrollingOffBottom") {
    const char* out = "0123456789012345678901234567890123456789";
    const char* onext = "ABCDEFGHIJKLMNOPQRST";
    const char* next2 = "UVWXYZabcd";
    struct ncplane* n = ncplane_new(nc_, 2, 20, 1, 1, nullptr);
    REQUIRE(n);
    // verify that the new plane was started without scrolling
    CHECK(!ncplane_set_scrolling(n, true));
    CHECK(40 == ncplane_putstr(n, out)); // fill up the plane w/ numbers
    CHECK(0 == notcurses_render(nc_));
    int y, x;
    ncplane_cursor_yx(n, &y, &x);
    CHECK(1 == y);
    CHECK(20 == x);
    CHECK(20 == ncplane_putstr(n, onext)); // scroll off one line, fill new one
    ncplane_cursor_yx(n, &y, &x);
    CHECK(1 == y);
    CHECK(20 == x);
    CHECK(0 == notcurses_render(nc_));
    for(int i = 1 ; i < 21 ; ++i){
      uint32_t attr;
      uint64_t channels;
      char* egc = notcurses_at_yx(nc_, 2, i, &attr, &channels);
      REQUIRE(egc);
      CHECK(onext[i - 1] == *egc);
      free(egc);
    }
    ncplane_cursor_yx(n, &y, &x);
    CHECK(1 == y);
    CHECK(20 == x);
    CHECK(10 == ncplane_putstr(n, next2));
    CHECK(0 == notcurses_render(nc_));
    for(int i = 1 ; i < 21 ; ++i){
      uint32_t attr;
      uint64_t channels;
      char* egc = notcurses_at_yx(nc_, 2, i, &attr, &channels);
      REQUIRE(egc);
      if(i < 11){
        CHECK(next2[i - 1] == *egc);
      }else{
        CHECK(' ' == *egc);
      }
      free(egc);
    }
    ncplane_cursor_yx(n, &y, &x);
    CHECK(1 == y);
    CHECK(10 == x);
  }

  // make sure that, after scrolling a line up, our y specifications are
  // correctly adjusted for scrolling.
  SUBCASE("XYPostScroll") {
    const char* out = "0123456789012345678901234567890123456789";
    const char* onext = "ABCDEFGHIJ";
    const char* next2 = "KLMNOPQRST";
    const char* next3 = "UVWXYZ";
    CHECK(0 == notcurses_render(nc_));
    struct ncplane* n = ncplane_new(nc_, 2, 20, 1, 1, nullptr);
    REQUIRE(n);
    // verify that the new plane was started without scrolling
    CHECK(!ncplane_set_scrolling(n, true));
    CHECK(40 == ncplane_putstr(n, out));
    CHECK(0 == notcurses_render(nc_));
    CHECK(10 == ncplane_putstr(n, onext));
    CHECK(0 == notcurses_render(nc_));
    for(int i = 1 ; i < 21 ; ++i){
      uint32_t attr;
      uint64_t channels;
      char* egc = notcurses_at_yx(nc_, 2, i, &attr, &channels);
      REQUIRE(egc);
      if(i < 11){
        CHECK(onext[i - 1] == *egc);
      }else{
        CHECK(' ' == *egc);
      }
      free(egc);
    }
    // FIXME
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));

}
