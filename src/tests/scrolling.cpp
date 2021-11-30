#include "main.h"
#include <array>
#include <cstdlib>

TEST_CASE("Scrolling") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  // verify that the standard plane has scrolling disabled initially, and that
  // we can enable it at runtime.
  SUBCASE("ScollingDisabledStdplane") {
    CHECK(!ncplane_set_scrolling(n_, true));
    CHECK(ncplane_set_scrolling(n_, false));
  }

  SUBCASE("ScrollingStr") {
    struct ncplane_options nopts = {
      .y = 1,
      .x = 1,
      .rows = 2,
      .cols = 20,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* n = ncplane_create(n_, &nopts);
    REQUIRE(n);
    // verify that the new plane was started without scrolling
    CHECK(!ncplane_set_scrolling(n, false));
    // try to write 40 EGCs; it ought fail after 20
    CHECK(-20 == ncplane_putstr(n, "0123456789012345678901234567890123456789"));
    unsigned y, x;
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
    struct ncplane_options nopts = {
      .y = 1,
      .x = 1,
      .rows = 2,
      .cols = 20,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* n = ncplane_create(n_, &nopts);
    REQUIRE(n);
    CHECK(!ncplane_set_scrolling(n, true)); // enable scrolling
    CHECK(0 > ncplane_cursor_move_yx(n, 0, 20));
    CHECK(0 > ncplane_cursor_move_yx(n, 1, 20));
    CHECK(0 > ncplane_cursor_move_yx(n, 2, 2));
    CHECK(0 > ncplane_putchar_yx(n, 0, 20, 'c'));
    CHECK(0 > ncplane_putchar_yx(n, 1, 20, 'c'));
    CHECK(0 > ncplane_putchar_yx(n, 2, 0, 'c'));
  }

  // verify that two strings, each the length of the plane, can be output when
  // scrolling is enabled (the second ought get an error without scrolling)
  SUBCASE("ScrollingSplitStr") {
    struct ncplane_options nopts = {
      .y = 1,
      .x = 1,
      .rows = 2,
      .cols = 20,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* n = ncplane_create(n_, &nopts);
    REQUIRE(n);
    CHECK(20 == ncplane_putstr(n, "01234567890123456789"));
    unsigned y, x;
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
    struct ncplane_options nopts = {
      .y = 1,
      .x = 1,
      .rows = 2,
      .cols = 20,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* n = ncplane_create(n_, &nopts);
    REQUIRE(n);
    // verify that the new plane was started without scrolling
    CHECK(!ncplane_set_scrolling(n, false));
    // try to write 40 EGCs; the last 20 ought fail
    for(const char* c = out ; *c ; ++c){
      int ret = ncplane_putchar(n, *c);
      if(c - out < 20){
        CHECK(1 == ret);
      }else{
        CHECK(0 > ret);
      }
    }
    unsigned y, x;
    ncplane_cursor_yx(n, &y, &x);
    CHECK(0 == y);
    CHECK(20 == x);
    CHECK(0 == ncplane_cursor_move_yx(n, 0, 0));
    CHECK(!ncplane_set_scrolling(n, true)); // enable scrolling
    // try to write 40 EGCs; all ought succeed
    for(const char* c = out ; *c ; ++c){
      CHECK(1 == ncplane_putchar(n, *c));
    }
    ncplane_cursor_yx(n, &y, &x);
    CHECK(1 == y);
    CHECK(20 == x);
    CHECK(0 == notcurses_render(nc_));
  }

  // ensure that if we draw a box on a scrolling plane, it stops at the right
  // side, as opposed to scrolling and making a horrible mess
  SUBCASE("ScrollingBoxen") {
    struct ncplane_options nopts = {
      .y = 1,
      .x = 1,
      .rows = 4,
      .cols = 20,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* n = ncplane_create(n_, &nopts);
    REQUIRE(n);
    // verify that the new plane was started without scrolling
    CHECK(!ncplane_set_scrolling(n, true));
    nccell ul = NCCELL_TRIVIAL_INITIALIZER, ur = NCCELL_TRIVIAL_INITIALIZER;
    nccell dl = NCCELL_TRIVIAL_INITIALIZER, dr = NCCELL_TRIVIAL_INITIALIZER;
    nccell hl = NCCELL_TRIVIAL_INITIALIZER, vl = NCCELL_TRIVIAL_INITIALIZER;
    CHECK(0 == nccells_double_box(n, 0, 0, &ul, &ur, &dl, &dr, &hl, &vl));
    CHECK(0 > ncplane_box_sized(n, &ul, &ur, &dl, &dr, &hl, &vl, 2, 25, 0));
    CHECK(0 > ncplane_box_sized(n, &ul, &ur, &dl, &dr, &hl, &vl, 2, 21, 0));
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("ScrollingOffBottom") {
    const char* out = "0123456789012345678901234567890123456789";
    const char* onext = "ABCDEFGHIJKLMNOPQRST";
    const char* next2 = "UVWXYZabcd";
    struct ncplane_options nopts = {
      .y = 1,
      .x = 1,
      .rows = 2,
      .cols = 20,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* n = ncplane_create(n_, &nopts);
    REQUIRE(n);
    // verify that the new plane was started without scrolling
    CHECK(!ncplane_set_scrolling(n, true));
    CHECK(40 == ncplane_putstr(n, out)); // fill up the plane w/ numbers
    CHECK(0 == notcurses_render(nc_));
    unsigned y, x;
    ncplane_cursor_yx(n, &y, &x);
    CHECK(1 == y);
    CHECK(20 == x);
    CHECK(20 == ncplane_putstr(n, onext)); // scroll off one line, fill new one
    ncplane_cursor_yx(n, &y, &x);
    CHECK(1 == y);
    CHECK(20 == x);
    CHECK(0 == notcurses_render(nc_));
    for(int i = 1 ; i < 21 ; ++i){
      uint16_t styles;
      uint64_t channels;
      char* egc = notcurses_at_yx(nc_, 2, i, &styles, &channels);
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
      uint16_t styles;
      uint64_t channels;
      char* egc = notcurses_at_yx(nc_, 2, i, &styles, &channels);
      REQUIRE(egc);
      if(i < 11){
        CHECK(next2[i - 1] == *egc);
      }else{
        CHECK('\0' == *egc);
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
    //const char* next2 = "KLMNOPQRST";
    //const char* next3 = "UVWXYZ";
    CHECK(0 == notcurses_render(nc_));
    struct ncplane_options nopts = {
      .y = 1,
      .x = 1,
      .rows = 2,
      .cols = 20,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* n = ncplane_create(n_, &nopts);
    REQUIRE(n);
    // verify that the new plane was started without scrolling
    CHECK(!ncplane_set_scrolling(n, true));
    CHECK(40 == ncplane_putstr(n, out));
    CHECK(0 == notcurses_render(nc_));
    CHECK(10 == ncplane_putstr(n, onext));
    CHECK(0 == notcurses_render(nc_));
    for(int i = 1 ; i < 21 ; ++i){
      uint16_t styles;
      uint64_t channels;
      char* egc = notcurses_at_yx(nc_, 2, i, &styles, &channels);
      REQUIRE(egc);
      if(i < 11){
        CHECK(onext[i - 1] == *egc);
      }else{
        CHECK('\0' == *egc);
      }
      free(egc);
    }
    // FIXME
  }

  // ensure that bound planes are scrolled along with us
  SUBCASE("BoundScroll") {
    int starty = 4;
    struct ncplane_options nopts = {
      .y = starty,
      .x = 4,
      .rows = 2,
      .cols = 20,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    ncplane_set_scrolling(n_, true);
    auto np = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != np);
    nccell c = NCCELL_INITIALIZER('a', 0, NCCHANNELS_INITIALIZER(0xbb, 0, 0xbb, 0, 0, 0));
    CHECK(0 < ncplane_polyfill_yx(np, 0, 0, &c));
    CHECK(0 == ncplane_cursor_move_yx(n_, ncplane_dim_y(n_) - 1, 0));
    CHECK(0 == notcurses_render(nc_));
    for(unsigned i = 0 ; i < ncplane_dim_y(np) + starty ; ++i){
      CHECK(starty - i == ncplane_y(np));
      CHECK(1 == ncplane_putchar(n_, '\n'));
      CHECK(0 == notcurses_render(nc_));
    }
    CHECK(0 == ncplane_destroy(np));
  }

  // ensure that bound planes are *not* scrolled along with us when the
  // NCPLANE_OPTION_FIXED flag is used
  SUBCASE("BoundScrollFixed") {
    int starty = 4;
    struct ncplane_options nopts = {
      .y = starty,
      .x = 4,
      .rows = 2,
      .cols = 20,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr,
      .flags = NCPLANE_OPTION_FIXED,
      .margin_b = 0, .margin_r = 0,
    };
    ncplane_set_scrolling(n_, true);
    auto np = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != np);
    nccell c = NCCELL_INITIALIZER('a', 0, NCCHANNELS_INITIALIZER(0xbb, 0, 0xbb, 0, 0, 0));
    CHECK(0 < ncplane_polyfill_yx(np, 0, 0, &c));
    CHECK(0 == ncplane_cursor_move_yx(n_, ncplane_dim_y(n_) - 1, 0));
    CHECK(0 == notcurses_render(nc_));
    for(unsigned i = 0 ; i < ncplane_dim_y(np) + starty ; ++i){
      CHECK(starty == ncplane_y(np));
      CHECK(1 == ncplane_putchar(n_, '\n'));
      CHECK(0 == notcurses_render(nc_));
    }
    CHECK(0 == ncplane_destroy(np));
  }

  CHECK(0 == notcurses_stop(nc_));

}
