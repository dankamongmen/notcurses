#include <array>
#include <cstdlib>
#include <notcurses.h>
#include "internal.h"
#include "main.h"

TEST_CASE("Fills") {
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

  // trying to polyfill an invalid cell ought be an error
  SUBCASE("PolyfillOffplane") {
    int dimx, dimy;
    ncplane_dim_yx(n_, &dimy, &dimx);
    cell c = CELL_SIMPLE_INITIALIZER('+');
    CHECK(0 > ncplane_polyfill_yx(n_, dimy, 0, &c));
    CHECK(0 > ncplane_polyfill_yx(n_, 0, dimx, &c));
    CHECK(0 > ncplane_polyfill_yx(n_, 0, -1, &c));
    CHECK(0 > ncplane_polyfill_yx(n_, -1, 0, &c));
  }

  SUBCASE("PolyfillOnGlyph") {
    cell c = CELL_SIMPLE_INITIALIZER('+');
    struct ncplane* pfn = ncplane_new(nc_, 4, 4, 0, 0, nullptr);
    REQUIRE(nullptr != pfn);
    CHECK(16 == ncplane_polyfill_yx(pfn, 0, 0, &c));
    CHECK(0 < ncplane_putc_yx(pfn, 0, 0, &c));
    // Trying to fill the origin ought fill zero cells
    CHECK(0 == ncplane_polyfill_yx(pfn, 0, 0, &c));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(pfn));
  }

  SUBCASE("PolyfillEmptyPlane") {
    cell c = CELL_SIMPLE_INITIALIZER('+');
    struct ncplane* pfn = ncplane_new(nc_, 4, 4, 0, 0, nullptr);
    REQUIRE(nullptr != pfn);
    CHECK(16 == ncplane_polyfill_yx(pfn, 0, 0, &c));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(pfn));
  }

  SUBCASE("PolyfillWalledPlane") {
    cell c = CELL_SIMPLE_INITIALIZER('+');
    struct ncplane* pfn = ncplane_new(nc_, 4, 4, 0, 0, nullptr);
    REQUIRE(nullptr != pfn);
    CHECK(0 < ncplane_putc_yx(pfn, 0, 1, &c));
    CHECK(0 < ncplane_putc_yx(pfn, 1, 1, &c));
    CHECK(0 < ncplane_putc_yx(pfn, 1, 0, &c));
    // Trying to fill the origin ought fill exactly one cell
    CHECK(1 == ncplane_polyfill_yx(pfn, 0, 0, &c));
    // Beyond the origin, we ought fill 12
    CHECK(12 == ncplane_polyfill_yx(pfn, 2, 2, &c));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(pfn));
  }

  SUBCASE("GradientMonochromatic") {
    struct ncplane* pfn = ncplane_new(nc_, 4, 4, 0, 0, nullptr);
    REQUIRE(nullptr != pfn);
    uint64_t ul, ur, ll, lr;
    ul = ur = ll = lr = 0;
    channels_set_fg(&ul, 0x40f040);
    channels_set_bg(&ul, 0x40f040);
    channels_set_fg(&ur, 0x40f040);
    channels_set_bg(&ur, 0x40f040);
    channels_set_fg(&ll, 0x40f040);
    channels_set_bg(&ll, 0x40f040);
    channels_set_fg(&lr, 0x40f040);
    channels_set_bg(&lr, 0x40f040);
    CHECK(0 == ncplane_gradient(pfn, " ", 0, ul, ur, ll, lr, 3, 3));
    cell c = CELL_TRIVIAL_INITIALIZER;
    uint64_t channels = 0;
    channels_set_fg(&channels, 0x40f040);
    channels_set_bg(&channels, 0x40f040);
    // check all squares 
    for(int y = 0 ; y < 3 ; ++y){
      for(int x = 0 ; x < 3 ; ++x){
        REQUIRE(0 <= ncplane_at_yx(pfn, y, 0, &c));
        CHECK(' ' == c.gcluster);
        CHECK(0 == c.attrword);
        CHECK(channels == c.channels);
      }
    }
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(pfn));
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));

}
