#include "main.h"
#include "internal.h"

TEST_CASE("TextLayout") {
  auto nc_ = testing_notcurses();
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
    char* line = ncplane_contents(sp, 0, 0, 2, 20);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "this is going to bebroken up"));
    free(line);
    ncplane_destroy(sp);
  }

  SUBCASE("LayoutRight") {
    auto sp = ncplane_new(nc_, 2, 20, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_RIGHT, str, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(str));
    char* line = ncplane_contents(sp, 0, 0, 2, 20);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "this is going to bebroken up"));
    free(line);
    ncplane_destroy(sp);
  }

  SUBCASE("LayoutCenter") {
    auto sp = ncplane_new(nc_, 2, 20, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_CENTER, str, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(str));
    char* line = ncplane_contents(sp, 0, 0, 2, 20);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "this is going to bebroken up"));
    free(line);
    ncplane_destroy(sp);
  }

  // lay out text where a word ends on the boundary
  SUBCASE("LayoutOnBoundary") {
    auto sp = ncplane_new(nc_, 2, 10, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "my nuclear arms";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_CENTER, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, -1, -1);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "my nucleararms"));
    free(line);
    ncplane_destroy(sp);
  }

  // lay out text where a word crosses the boundary
  SUBCASE("LayoutCrossBoundary") {
    auto sp = ncplane_new(nc_, 3, 10, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "my grasping arms";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_CENTER, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, -1, -1);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "mygraspingarms"));
    free(line);
    ncplane_destroy(sp);
  }

  // lay out text where a word is longer than the plane
  SUBCASE("LayoutCrossBoundary") {
    auto sp = ncplane_new(nc_, 3, 10, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "my thermonuclear arms";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_CENTER, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, -1, -1);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "mythermonuclear arms"));
    free(line);
    ncplane_destroy(sp);
  }

  CHECK(0 == notcurses_stop(nc_));

}
