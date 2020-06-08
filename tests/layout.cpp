#include "main.h"
#include "internal.h"

TEST_CASE("TextLayout") {
  notcurses_options nopts{};
  notcurses* nc_ = notcurses_init(&nopts, nullptr);
  if(!nc_){
    return;
  }
  ncplane* ncp_ = notcurses_stdplane(nc_);
  REQUIRE(ncp_);

  const char str[] = "this is going to be broken up";

  SUBCASE("LayoutLeft") {
    auto sp = ncplane_new(nc_, 2, 20, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, str, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(str));
    // FIXME inspect layout
    ncplane_destroy(sp);
  }

  SUBCASE("LayoutRight") {
    auto sp = ncplane_new(nc_, 2, 20, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_RIGHT, str, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(str));
    // FIXME inspect layout
    ncplane_destroy(sp);
  }

  SUBCASE("LayoutCenter") {
    auto sp = ncplane_new(nc_, 2, 20, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_CENTER, str, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(str));
    // FIXME inspect layout
    ncplane_destroy(sp);
  }

  CHECK(0 == notcurses_stop(nc_));

}
