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
    int dimy, dimx;
    ncplane_dim_yx(n_, &dimy, &dimx);
    REQUIRE(0 == ncplane_gradient_sized(n_, "M", 0, ul, ur, ll, lr, dimy, dimx));
    cell c = CELL_TRIVIAL_INITIALIZER;
    uint64_t channels = 0;
    channels_set_fg(&channels, 0x40f040);
    channels_set_bg(&channels, 0x40f040);
    // check all squares 
    for(int y = 0 ; y < dimy ; ++y){
      for(int x = 0 ; x < dimx ; ++x){
        REQUIRE(0 <= ncplane_at_yx(n_, y, x, &c));
        CHECK('M' == c.gcluster);
        CHECK(0 == c.attrword);
        CHECK(channels == c.channels);
      }
    }
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("GradientVertical") {
    uint64_t ul, ur, ll, lr;
    ul = ur = ll = lr = 0;
    channels_set_fg(&ul, 0x40f040);
    channels_set_bg(&ul, 0x40f040);
    channels_set_fg(&ll, 0xf040f0);
    channels_set_bg(&ll, 0xf040f0);
    channels_set_fg(&ur, 0x40f040);
    channels_set_bg(&ur, 0x40f040);
    channels_set_fg(&lr, 0xf040f0);
    channels_set_bg(&lr, 0xf040f0);
    int dimy, dimx;
    ncplane_dim_yx(n_, &dimy, &dimx);
    REQUIRE(0 == ncplane_gradient_sized(n_, "V", 0, ul, ur, ll, lr, dimy, dimx));
    cell c = CELL_TRIVIAL_INITIALIZER;
    uint64_t channels = 0;
    channels_set_fg(&channels, 0x40f040);
    channels_set_bg(&channels, 0x40f040);
    // check all squares. all rows ought be the same across their breadth, and
    // the components ought be going in the correct direction.
    uint64_t lastyrgb, lastxrgb;
    lastyrgb = -1;
    for(int y = 0 ; y < dimy ; ++y){
      lastxrgb = -1;
      for(int x = 0 ; x < dimx ; ++x){
        REQUIRE(0 <= ncplane_at_yx(n_, y, x, &c));
        CHECK('V' == c.gcluster);
        CHECK(0 == c.attrword);
        if(lastxrgb == (uint64_t)-1){
          if(lastyrgb == (uint64_t)-1){
            lastyrgb = c.channels;
            CHECK(ul == c.channels);
          }else if(y == dimy - 1){
/*fprintf(stderr, "HJAVE %016lx, WANT %016lx %d %d\n", c.channels, ll, y, x);
            CHECK(ll == c.channels);*/
          }
          lastxrgb = c.channels;
        }else{
          CHECK(lastxrgb == c.channels);
        }
        if(x == dimx - 1){
          if(y == 0){
            CHECK(ur == c.channels);
          }else if(y == dimy - 1){
/*fprintf(stderr, "LR %016lx, WANT %016lx %d %d\n", c.channels, lr, y, x);
            CHECK(lr == c.channels);*/
          }
        }
      }
    }
    CHECK(0 == notcurses_render(nc_));
sleep(3);
  }

  SUBCASE("GradientHorizontal") {
    uint64_t ul, ur, ll, lr;
    ul = ur = ll = lr = 0;
    channels_set_fg(&ul, 0x40f040);
    channels_set_bg(&ul, 0x40f040);
    channels_set_fg(&ur, 0xf040f0);
    channels_set_bg(&ur, 0xf040f0);
    channels_set_fg(&ll, 0x40f040);
    channels_set_bg(&ll, 0x40f040);
    channels_set_fg(&lr, 0xf040f0);
    channels_set_bg(&lr, 0xf040f0);
    int dimy, dimx;
    ncplane_dim_yx(n_, &dimy, &dimx);
    REQUIRE(0 == ncplane_gradient_sized(n_, "H", 0, ul, ur, ll, lr, dimy, dimx));
    // check corners FIXME
    CHECK(0 == notcurses_render(nc_));
sleep(3);
  }

  SUBCASE("GradientX") {
    uint64_t ul, ur, ll, lr;
    ul = ur = ll = lr = 0;
    channels_set_fg(&ul, 0x000000);
    channels_set_bg(&ul, 0xffffff);
    channels_set_fg(&ll, 0x40f040);
    channels_set_bg(&ll, 0x40f040);
    channels_set_fg(&ur, 0xf040f0);
    channels_set_bg(&ur, 0xf040f0);
    channels_set_fg(&lr, 0xffffff);
    channels_set_bg(&lr, 0x000000);
    int dimy, dimx;
    ncplane_dim_yx(n_, &dimy, &dimx);
    REQUIRE(0 == ncplane_gradient_sized(n_, "X", 0, ul, ur, ll, lr, dimy, dimx));
    // check corners FIXME
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("GradientS") {
    uint64_t ul, ur, ll, lr;
    ul = ur = ll = lr = 0;
    channels_set_fg(&ul, 0x000000);
    channels_set_bg(&ul, 0xffffff);
    channels_set_fg(&ll, 0x40f040);
    channels_set_bg(&ll, 0x40f040);
    channels_set_fg(&ur, 0xf040f0);
    channels_set_bg(&ur, 0xf040f0);
    channels_set_fg(&lr, 0xffffff);
    channels_set_bg(&lr, 0x000000);
    int dimy, dimx;
    ncplane_dim_yx(n_, &dimy, &dimx);
    REQUIRE(0 == ncplane_gradient_sized(n_, "S", 0, ul, ur, ll, lr, dimy, dimx));
    // check corners FIXME
    CHECK(0 == notcurses_render(nc_));
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));

}
