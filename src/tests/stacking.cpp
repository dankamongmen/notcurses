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
  unsigned dimy, dimx;
  struct ncplane* n_ = notcurses_stddim_yx(nc_, &dimy, &dimx);
  REQUIRE(nullptr != n_);

  // whenever the foreground matches the background (using RGB color, *not*
  // default colors not palette-indexed color), we ought emit a space with the
  // specified background, or a full block with the specified foreground (only
  // if UTF8 is available). default colors must not be merged (palette-indexed
  // could be, but it's pointless). the transformation must only take place at
  // raster time--the original output must be recoverable from the plane.
  SUBCASE("FgMatchesBgRGB") {
    // first we write an a with the desired background, but a distinct
    // foreground. then we write an a with the two matching (via RGB).
    // this ought generate a space with the desired background on the
    // second cell.
    ncplane_set_fg_default(n_);
    CHECK(0 == ncplane_set_bg_rgb(n_, 0x808080));
    CHECK(1 == ncplane_putchar(n_, 'a'));
    CHECK(0 == ncplane_set_fg_rgb(n_, 0x808080));
    CHECK(1 == ncplane_putchar(n_, 'a')); // ought become a space
    // now we write an x with the desired foreground, but a distinct
    // background. then we write an x with the two matching. this ought
    // generate a full block with the desired foreground if UTF8 is
    // available, and a space with the desired background otherwise.
    ncplane_set_bg_default(n_);
    CHECK(1 == ncplane_putchar(n_, 'x'));
    CHECK(0 == ncplane_set_bg_rgb(n_, 0x808080));
    CHECK(1 == ncplane_putchar(n_, 'x')); // ought become a space/block
    CHECK(0 == notcurses_render(nc_));
    // now we check the output. the plane ought have the characters as written,
    // but we ought have rasterized the optimal forms.
    uint64_t channels;
    auto pblit = ncplane_at_yx(n_, 0, 1, nullptr, &channels);
    CHECK(0 == strcmp("a", pblit));
    CHECK(0x808080 == ncchannels_bg_rgb(channels));
    CHECK(0x808080 == ncchannels_fg_rgb(channels));
    free(pblit);
    pblit = ncplane_at_yx(n_, 0, 3, nullptr, &channels);
    CHECK(0 == strcmp("x", pblit));
    CHECK(0x808080 == ncchannels_bg_rgb(channels));
    CHECK(0x808080 == ncchannels_fg_rgb(channels));
    free(pblit);
    auto rblit = notcurses_at_yx(nc_, 0, 1, nullptr, &channels);
    CHECK(0 == strcmp(" ", rblit));
    CHECK(0x808080 == ncchannels_bg_rgb(channels));
    free(rblit);
    rblit = notcurses_at_yx(nc_, 0, 3, nullptr, &channels);
    if(notcurses_canutf8(nc_)){
      CHECK(0x808080 == ncchannels_fg_rgb(channels));
      // FIXME we're not yet this advanced, and use space instead
      // CHECK(0 == strcmp(u8"\u2588", rblit));
      CHECK(0 == strcmp(u8" ", rblit));
    }else{
      CHECK(0 == strcmp(" ", rblit));
      CHECK(0x808080 == ncchannels_bg_rgb(channels));
    }
    free(rblit);
  }

  SUBCASE("LowerAtopUpperWhite") {
    struct ncplane_options opts = {
      .y = 0, .x = 0, .rows = 1, .cols = 1,
      .userptr = nullptr, .name = "top",
      .resizecb = nullptr,
      .flags = 0,
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
      .transcolor = 0, .pxoffy = 0, .pxoffx = 0,
    };
    CHECK(top == ncvisual_blit(nc_, ncv, &vopts));
    ncvisual_destroy(ncv);

    // create an ncvisual of 2 rows, 1 column, with the top 0xffffff
    const uint32_t botv[] = {htole(0xffffffff), htole(0)};
    ncv = ncvisual_from_rgba(botv, 2, 4, 1);
    REQUIRE(nullptr != ncv);
    vopts.n = n_;
    vopts.flags |= NCVISUAL_OPTION_CHILDPLANE;
    auto newn = ncvisual_blit(nc_, ncv, &vopts);
    REQUIRE(nullptr != newn);
    ncvisual_destroy(ncv);
    ncplane_move_below(newn, top);

    CHECK(0 == notcurses_render(nc_));
    uint64_t channels;
    auto egc = notcurses_at_yx(nc_, 0, 0, nullptr, &channels);
    REQUIRE(nullptr != egc);
    CHECK(0 == strcmp(u8" ", egc));
    free(egc);
    CHECK(0xffffff == ncchannels_fg_rgb(channels));
    CHECK(0xffffff == ncchannels_bg_rgb(channels));
    CHECK(0 == ncplane_destroy(top));
    CHECK(0 == ncplane_destroy(newn));
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
      .transcolor = 0, .pxoffy = 0, .pxoffx = 0,
    };
    CHECK(top == ncvisual_blit(nc_, ncv, &vopts));
    ncvisual_destroy(ncv);

    // create an ncvisual of 2 rows, 1 column, with the bottom 0xffffff
    const uint32_t botv[] = {htole(0), htole(0xffffffff)};
    ncv = ncvisual_from_rgba(botv, 2, 4, 1);
    REQUIRE(nullptr != ncv);
    vopts.n = n_;
    vopts.flags |= NCVISUAL_OPTION_CHILDPLANE;
    auto newn = ncvisual_blit(nc_, ncv, &vopts);
    REQUIRE(nullptr != newn);
    ncvisual_destroy(ncv);
    ncplane_move_below(newn, top);

    CHECK(0 == notcurses_render(nc_));
    uint64_t channels;
    auto egc = notcurses_at_yx(nc_, 0, 0, nullptr, &channels);
    REQUIRE(nullptr != egc);
    CHECK(0 == strcmp(u8" ", egc));
    free(egc);
    CHECK(0xffffff == ncchannels_fg_rgb(channels));
    CHECK(0xffffff == ncchannels_bg_rgb(channels));
    CHECK(0 == ncplane_destroy(top));
    CHECK(0 == ncplane_destroy(newn));
  }

  SUBCASE("StackedQuadHalves") {
    if(notcurses_canquadrant(nc_)){
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
        .transcolor = 0, .pxoffy = 0, .pxoffx = 0,
      };
      CHECK(top == ncvisual_blit(nc_, ncv, &vopts));
      ncvisual_destroy(ncv);

      // create an ncvisual of 2 rows, 2 columns, with the bottom 0xffffff
      const uint32_t botv[] = {htole(0), htole(0), htole(0xff00ff00), htole(0xff00ff00)};
      ncv = ncvisual_from_rgba(botv, 2, 8, 2);
      REQUIRE(nullptr != ncv);
      vopts.n = n_;
      vopts.flags = NCVISUAL_OPTION_CHILDPLANE;
      auto newn = ncvisual_blit(nc_, ncv, &vopts); 
      REQUIRE(nullptr != newn);
      ncvisual_destroy(ncv);
      ncplane_move_below(newn, top);

      CHECK(0 == notcurses_render(nc_));
      uint64_t channels;
      auto egc = notcurses_at_yx(nc_, 0, 0, nullptr, &channels);
      REQUIRE(nullptr != egc);
      CHECK(0 == strcmp(u8" ", egc));
      free(egc);
      CHECK(0x00ff00 == ncchannels_fg_rgb(channels));
      CHECK(0x00ff00 == ncchannels_bg_rgb(channels));
      CHECK(0 == ncplane_destroy(top));
      CHECK(0 == ncplane_destroy(newn));
    }
  }

  SUBCASE("StackedQuadCrossed") {
    if(notcurses_canquadrant(nc_)){
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
        .transcolor = 0, .pxoffy = 0, .pxoffx = 0,
      };
      CHECK(top == ncvisual_blit(nc_, ncv, &vopts));
      ncvisual_destroy(ncv);

      // create an ncvisual of 2 rows, 2 columns, with the tr, bl 0xffffff
      const uint32_t botv[] = {htole(0), htole(0xffffffff), htole(0xffffffff), htole(0)};
      ncv = ncvisual_from_rgba(botv, 2, 8, 2);
      REQUIRE(nullptr != ncv);
      vopts.n = n_;
      vopts.flags = NCVISUAL_OPTION_CHILDPLANE;
      auto newn = ncvisual_blit(nc_, ncv, &vopts);
      REQUIRE(nullptr != newn);
      ncvisual_destroy(ncv);
      ncplane_move_below(newn, top);

      CHECK(0 == notcurses_render(nc_));
      uint64_t channels;
      auto egc = notcurses_at_yx(nc_, 0, 0, nullptr, &channels);
      REQUIRE(nullptr != egc);
      CHECK(0 == strcmp(u8" ", egc)); // quadrant upper left and lower right
      free(egc);
      CHECK(0xffffff == ncchannels_fg_rgb(channels));
      CHECK(0xffffff == ncchannels_bg_rgb(channels));
      CHECK(0 == ncplane_destroy(top));
      CHECK(0 == ncplane_destroy(newn));
    }
  }

  // common teardown
  CHECK(0 == notcurses_stop(nc_));
}
