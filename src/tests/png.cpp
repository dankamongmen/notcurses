#include "main.h"
#include "png.h"
#include "visual-details.h"
#include <vector>
#include <cmath>

TEST_CASE("PNG") {
  auto nc_ = testing_notcurses();
  REQUIRE(nullptr != nc_);
  ncplane* ncp_ = notcurses_stdplane(nc_);
  REQUIRE(ncp_);
  auto n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

#ifndef NOTCURSES_USE_MULTIMEDIA
#else
  // write a 10x10 opaque PNG out, and ensure we can read it back
  SUBCASE("ReadWrittenOpaquePNG") {
    std::array<uint32_t, 100> pixels;
    pixels.fill(htole(0x00ff0000ull)); // green, opaque set later
    auto ncv = ncvisual_from_rgb_loose(pixels.data(), 10, 40, 10, 0xff);
    REQUIRE(nullptr != ncv);
    // FIXME write out ncvisual to PNG, read it back, render it
    CHECK(0 == notcurses_render(nc_));
    ncvisual_destroy(ncv);
  }
#endif

  CHECK(!notcurses_stop(nc_));
}
