#include "main.h"

// These tests address cases where box characters on two overlapping planes
// interact in non-trivial ways. A simple example is a U2580 UPPER HALF BLOCK
// (▀) with a white foreground and transparent background, above a U2584 LOWER
// HALF BLOCK (▄) with a white foreground and transparent background. One might
// expect the result to be an entirely white cell, but by typical Notcurses
// rendering rules, we would instead get a white upper half and transparent
// lower half:
//
// - after first cell, glyph is locked U2584, fg is locked white, bg transparent
// - second cell can't override glyph nor fg, and background remains transparent
//
// we will instead special-case block-drawing characters.
// see https://github.com/dankamongmen/notcurses/issues/1068
TEST_CASE("Stacking") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  if(!notcurses_canutf8(nc_)){
    CHECK(0 == notcurses_stop(nc_));
    return;
  }
  int dimy, dimx;
  struct ncplane* n_ = notcurses_stddim_yx(nc_, &dimy, &dimx);
  REQUIRE(nullptr != n_);

  SUBCASE("LowerAtopUpperWhite") {
    struct ncplane_options opts = {
      0, 0, 1, 1, nullptr, "top", nullptr, 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto top = ncplane_create(n_, &opts);
    REQUIRE(nullptr != top);
    // create an ncvisual of 2 rows, 1 column, with the bottom 0xffffff
    const uint32_t topv[] = {htole(0), htole(0xffffffff)};
    auto ncv = ncvisual_from_rgba(topv, 2, 4, 1);
    REQUIRE(nullptr != ncv);
    struct ncvisual_options vopts = {
      .n = top, .scaling = NCSCALE_NONE, .y = 0, .x = 0, .begy = 0, .begx = 0,
      .leny = 2, .lenx = 1, .blitter = NCBLIT_2x1, .flags = 0,
    };
    CHECK(top == ncvisual_render(nc_, ncv, &vopts));
    ncvisual_destroy(ncv);

    // create an ncvisual of 2 rows, 1 column, with the top 0xffffff
    const uint32_t botv[] = {htole(0xffffffff), htole(0)};
    ncv = ncvisual_from_rgba(botv, 2, 4, 1);
    REQUIRE(nullptr != ncv);
    vopts.n = n_;
    CHECK(n_ == ncvisual_render(nc_, ncv, &vopts));
    ncvisual_destroy(ncv);

    CHECK(0 == notcurses_render(nc_));
    uint64_t channels;
    auto egc = notcurses_at_yx(nc_, 0, 0, nullptr, &channels);
    REQUIRE(nullptr != egc);
    // ought yield space with white background FIXME currently just yields
    // a lower half block
    CHECK(0 == strcmp("\u2584", egc));
    CHECK(0xffffff == channels_fg_rgb(channels));
    CHECK(0xffffff == channels_bg_rgb(channels));
    ncplane_destroy(top);
  }

  SUBCASE("UpperAtopLowerWhite") {
    struct ncplane_options opts = {
      0, 0, 1, 1, nullptr, "top", nullptr, 0, 0, 0,
    };
    auto top = ncplane_create(n_, &opts);
    REQUIRE(nullptr != top);
    // create an ncvisual of 2 rows, 1 column, with the top 0xffffff
    const uint32_t topv[] = {htole(0xffffffff), htole(0)};
    auto ncv = ncvisual_from_rgba(topv, 2, 4, 1);
    REQUIRE(nullptr != ncv);
    struct ncvisual_options vopts = {
      .n = top, .scaling = NCSCALE_NONE, .y = 0, .x = 0, .begy = 0, .begx = 0,
      .leny = 2, .lenx = 1, .blitter = NCBLIT_2x1, .flags = 0,
    };
    CHECK(top == ncvisual_render(nc_, ncv, &vopts));
    ncvisual_destroy(ncv);

    // create an ncvisual of 2 rows, 1 column, with the bottom 0xffffff
    const uint32_t botv[] = {htole(0), htole(0xffffffff)};
    ncv = ncvisual_from_rgba(botv, 2, 4, 1);
    REQUIRE(nullptr != ncv);
    vopts.n = n_;
    CHECK(n_ == ncvisual_render(nc_, ncv, &vopts));
    ncvisual_destroy(ncv);

    CHECK(0 == notcurses_render(nc_));
    uint64_t channels;
    auto egc = notcurses_at_yx(nc_, 0, 0, nullptr, &channels);
    REQUIRE(nullptr != egc);
    // ought yield space with white background FIXME currently just yields
    // an upper half block
    CHECK(0 == strcmp("\u2580", egc));
    CHECK(0xffffff == channels_fg_rgb(channels));
    CHECK(0xffffff == channels_bg_rgb(channels));
    ncplane_destroy(top);
  }

  SUBCASE("StackedQuadHalves") {
    struct ncplane_options opts = {
      0, 0, 1, 1, nullptr, "top", nullptr, 0, 0, 0,
    };
    auto top = ncplane_create(n_, &opts);
    REQUIRE(nullptr != top);
    // create an ncvisual of 2 rows, 2 columns, with the top 0xffffff
    const uint32_t topv[] = {htole(0xff00ff00), htole(0xff00ff00), htole(0), htole(0)};
    auto ncv = ncvisual_from_rgba(topv, 2, 8, 2);
    REQUIRE(nullptr != ncv);
    struct ncvisual_options vopts = {
      .n = top, .scaling = NCSCALE_NONE, .y = 0, .x = 0, .begy = 0, .begx = 0,
      .leny = 2, .lenx = 2, .blitter = NCBLIT_2x2, .flags = 0,
    };
    CHECK(top == ncvisual_render(nc_, ncv, &vopts));
    ncvisual_destroy(ncv);

    // create an ncvisual of 2 rows, 2 columns, with the bottom 0xffffff
    const uint32_t botv[] = {htole(0), htole(0), htole(0xff00ff00), htole(0xff00ff00)};
    ncv = ncvisual_from_rgba(botv, 2, 8, 2);
    REQUIRE(nullptr != ncv);
    vopts.n = n_;
    CHECK(n_ == ncvisual_render(nc_, ncv, &vopts));
    ncvisual_destroy(ncv);

    CHECK(0 == notcurses_render(nc_));
    uint64_t channels;
    auto egc = notcurses_at_yx(nc_, 0, 0, nullptr, &channels);
    REQUIRE(nullptr != egc);
    // ought yield space with white background FIXME currently just yields
    // an upper half block
    CHECK(0 == strcmp("\u2580", egc));
    CHECK(0x00ff00 == channels_fg_rgb(channels));
    CHECK(0x00ff00 == channels_bg_rgb(channels));
    ncplane_destroy(top);
  }

  SUBCASE("StackedQuadCrossed") {
    ncplane_erase(n_);
    notcurses_refresh(nc_, nullptr, nullptr);
    struct ncplane_options opts = {
      0, 0, 1, 1, nullptr, "top", nullptr, 0, 0, 0,
    };
    auto top = ncplane_create(n_, &opts);
    REQUIRE(nullptr != top);
    // create an ncvisual of 2 rows, 2 columns, with the tl, br 0xffffff
    const uint32_t topv[] = {htole(0xffffffff), htole(0), htole(0), htole(0xffffffff)};
    auto ncv = ncvisual_from_rgba(topv, 2, 8, 2);
    REQUIRE(nullptr != ncv);
    struct ncvisual_options vopts = {
      .n = top, .scaling = NCSCALE_NONE, .y = 0, .x = 0, .begy = 0, .begx = 0,
      .leny = 2, .lenx = 2, .blitter = NCBLIT_2x2, .flags = 0,
    };
    CHECK(top == ncvisual_render(nc_, ncv, &vopts));
    ncvisual_destroy(ncv);

    // create an ncvisual of 2 rows, 2 columns, with the tr, bl 0xffffff
    const uint32_t botv[] = {htole(0), htole(0xffffffff), htole(0xffffffff), htole(0)};
    ncv = ncvisual_from_rgba(botv, 2, 8, 2);
    REQUIRE(nullptr != ncv);
    vopts.n = n_;
    CHECK(n_ == ncvisual_render(nc_, ncv, &vopts));
    ncvisual_destroy(ncv);

    CHECK(0 == notcurses_render(nc_));
    uint64_t channels;
    auto egc = notcurses_at_yx(nc_, 0, 0, nullptr, &channels);
    REQUIRE(nullptr != egc);
    // ought yield space with white background FIXME currently just yields
    // an upper half block
    CHECK(0 == strcmp("\u259a", egc)); // quadrant upper left and lower right
    CHECK(0xffffff == channels_fg_rgb(channels));
    CHECK(0xffffff == channels_bg_rgb(channels));
    ncplane_destroy(top);
  }

  // common teardown
  CHECK(0 == notcurses_stop(nc_));
}
