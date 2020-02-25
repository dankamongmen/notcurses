#include <array>
#include <cstdlib>
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
    uint64_t c = 0;
    channels_set_fg(&c, 0x40f040);
    channels_set_bg(&c, 0x40f040);
    int dimy, dimx;
    ncplane_dim_yx(n_, &dimy, &dimx);
    REQUIRE(0 == ncplane_gradient_sized(n_, "M", 0, c, c, c, c, dimy, dimx));
    cell cl = CELL_TRIVIAL_INITIALIZER;
    uint64_t channels = 0;
    channels_set_fg(&channels, 0x40f040);
    channels_set_bg(&channels, 0x40f040);
    // check all squares 
    for(int y = 0 ; y < dimy ; ++y){
      for(int x = 0 ; x < dimx ; ++x){
        REQUIRE(0 <= ncplane_at_yx(n_, y, x, &cl));
        CHECK('M' == cl.gcluster);
        CHECK(0 == cl.attrword);
        CHECK(channels == cl.channels);
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
            CHECK(ll == c.channels);
          }
          lastxrgb = c.channels;
        }else{
          CHECK(lastxrgb == c.channels);
        }
        if(x == dimx - 1){
          if(y == 0){
            CHECK(ur == c.channels);
          }else if(y == dimy - 1){
            CHECK(lr == c.channels);
          }
        }
      }
    }
    CHECK(0 == notcurses_render(nc_));
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
    channels_set_fg(&ul, 0xffffff);
    channels_set_bg(&ul, 0xffffff);
    channels_set_fg(&lr, 0x000000);
    channels_set_bg(&lr, 0x000000);
    channels_set_fg(&ll, 0x00ffff);
    channels_set_bg(&ll, 0xff0000);
    channels_set_fg(&ur, 0xff00ff);
    channels_set_bg(&ur, 0x00ff00);
    int dimy, dimx;
    ncplane_dim_yx(n_, &dimy, &dimx);
    REQUIRE(0 == ncplane_gradient_sized(n_, "S", 0, ul, ur, ll, lr, dimy, dimx));
    // check corners FIXME
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("Ncplane_Format") {
    int sbytes;
    CHECK(0 == ncplane_set_fg(n_, 0x444444));
    CHECK(1 == ncplane_putegc(n_, "A", &sbytes));
    CHECK(0 == ncplane_set_fg(n_, 0x888888));
    CHECK(1 == ncplane_putegc(n_, "B", &sbytes));
    CHECK(0 == notcurses_render(nc_));
    // attr should change, but not the EGC/color
    CHECK(0 == ncplane_cursor_move_yx(n_, 0, 0));
    cell c = CELL_TRIVIAL_INITIALIZER;
    cell_styles_on(&c, NCSTYLE_BOLD);
    CHECK(0 == ncplane_format(n_, 0, 0, c.attrword));
    cell d = CELL_TRIVIAL_INITIALIZER;
    CHECK(1 == ncplane_at_yx(n_, 0, 0, &d));
    CHECK(d.attrword == c.attrword);
    CHECK(0x444444 == cell_fg(&d));
  }

  SUBCASE("Ncplane_Stain") {
    int sbytes;
    CHECK(0 == ncplane_set_fg(n_, 0x444444));
    for(int y = 0 ; y < 8 ; ++y){
      for(int x = 0 ; x < 8 ; ++x){
        CHECK(1 == ncplane_putegc_yx(n_, y, x, "A", &sbytes));
      }
    }
    CHECK(0 == notcurses_render(nc_));
    // EGC/color should change, but nothing else
    CHECK(0 == ncplane_cursor_move_yx(n_, 0, 0));
    uint64_t channels = 0;
    channels_set_fg_rgb(&channels, 0x88, 0x99, 0x77);
    channels_set_bg(&channels, 0);
    REQUIRE(0 == ncplane_stain(n_, 7, 7, channels, channels, channels, channels));
    CHECK(0 == notcurses_render(nc_));
    cell d = CELL_TRIVIAL_INITIALIZER;
    for(int y = 0 ; y < 8 ; ++y){
      for(int x = 0 ; x < 8 ; ++x){
        CHECK(1 == ncplane_at_yx(n_, y, x, &d));
        CHECK(channels == d.channels);
        REQUIRE(cell_simple_p(&d));
        CHECK('A' == d.gcluster);
      }
    }
  }

  // test the single-cell (1x1) special case
  SUBCASE("GradientSingleCell") {
    int sbytes;
    CHECK(0 == ncplane_set_fg(n_, 0x444444));
    CHECK(1 == ncplane_putegc_yx(n_, 0, 0, "A", &sbytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_cursor_move_yx(n_, 0, 0));
    uint64_t channels = 0;
    channels_set_fg_rgb(&channels, 0x88, 0x99, 0x77);
    channels_set_bg(&channels, 0);
    REQUIRE(0 == ncplane_gradient(n_, "A", 0, channels, channels, channels, channels, 0, 0));
    CHECK(0 == notcurses_render(nc_));
    cell d = CELL_TRIVIAL_INITIALIZER;
    CHECK(1 == ncplane_at_yx(n_, 0, 0, &d));
    CHECK(channels == d.channels);
    REQUIRE(cell_simple_p(&d));
    CHECK('A' == d.gcluster);
  }

  // 1d gradients over multiple cells
  SUBCASE("Gradient1D") {
    int sbytes;
    CHECK(0 == ncplane_set_fg(n_, 0x444444));
    CHECK(1 == ncplane_putegc_yx(n_, 0, 0, "A", &sbytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_cursor_move_yx(n_, 0, 0));
    uint64_t chan1 = 0, chan2 = 0;
    channels_set_fg_rgb(&chan1, 0x88, 0x99, 0x77);
    channels_set_fg_rgb(&chan2, 0x77, 0x99, 0x88);
    channels_set_bg(&chan1, 0);
    channels_set_bg(&chan2, 0);
    REQUIRE(0 == ncplane_gradient(n_, "A", 0, chan1, chan2, chan1, chan2, 0, 3));
    CHECK(0 == notcurses_render(nc_));
    cell d = CELL_TRIVIAL_INITIALIZER;
    CHECK(1 == ncplane_at_yx(n_, 0, 0, &d));
    CHECK(chan1 == d.channels);
    REQUIRE(cell_simple_p(&d));
    CHECK('A' == d.gcluster);
    CHECK(0 == ncplane_cursor_move_yx(n_, 0, 0));
    REQUIRE(0 == ncplane_gradient(n_, "A", 0, chan2, chan1, chan2, chan1, 0, 3));
    CHECK(1 == ncplane_at_yx(n_, 0, 0, &d));
    REQUIRE(cell_simple_p(&d));
    CHECK('A' == d.gcluster);
    CHECK(chan2 == d.channels);
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));

}
