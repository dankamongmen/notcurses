#include "main.h"
#include <iostream>

int panelcb(struct tablet* t, int begx, int begy, int maxx, int maxy, bool cliptop){
  CHECK(tablet_ncplane(t));
  CHECK(begx < maxx);
  CHECK(begy < maxy);
  CHECK(!tablet_userptr(t));
  CHECK(!cliptop);
  // FIXME verify geometry is as expected
  return 0;
}

TEST_CASE("PanelReelTest") {
  if(getenv("TERM") == nullptr){
    return;
  }
  notcurses_options nopts{};
  nopts.inhibit_alternate_screen = true;
  nopts.suppress_bannner = true;
  FILE* outfp_ = fopen("/dev/tty", "wb");
  REQUIRE(outfp_);
  struct notcurses* nc_ = notcurses_init(&nopts, outfp_);
  REQUIRE(nc_);
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);
  REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));

  SUBCASE("InitLinear") {
    panelreel_options p = { };
    struct panelreel* pr = panelreel_create(n_, &p, -1);
    REQUIRE(pr);
  }

  SUBCASE("InitLinearInfinite") {
    panelreel_options p{};
    p.infinitescroll = true;
    struct panelreel* pr = panelreel_create(n_, &p, -1);
    REQUIRE(pr);
  }

  SUBCASE("InitCircular") {
    panelreel_options p{};
    p.infinitescroll = true;
    p.circular = true;
    struct panelreel* pr = panelreel_create(n_, &p, -1);
    REQUIRE(pr);
    REQUIRE(0 == panelreel_destroy(pr));
  }

  // circular is not allowed to be true when infinitescroll is false
  SUBCASE("FiniteCircleRejected") {
    panelreel_options p{};
    p.infinitescroll = false;
    p.circular = true;
    struct panelreel* pr = panelreel_create(n_, &p, -1);
    REQUIRE(!pr);
  }

  // We ought be able to invoke panelreel_next() and panelreel_prev() safely,
  // even if there are no tablets. They both ought return nullptr.
  SUBCASE("MovementWithoutTablets") {
    panelreel_options p{};
    p.infinitescroll = false;
    struct panelreel* pr = panelreel_create(n_, &p, -1);
    REQUIRE(pr);
    CHECK(!panelreel_next(pr));
    // CHECK_EQ(0, panelreel_validate(n_, pr));
    CHECK(!panelreel_prev(pr));
    // CHECK_EQ(0, panelreel_validate(n_, pr));
  }

  SUBCASE("OneTablet") {
    panelreel_options p{};
    p.infinitescroll = false;
    struct panelreel* pr = panelreel_create(n_, &p, -1);
    REQUIRE(pr);
    struct tablet* t = panelreel_add(pr, nullptr, nullptr, panelcb, nullptr);
    REQUIRE(t);
    // CHECK_EQ(0, panelreel_validate(n_, pr));
    CHECK(0 == panelreel_del(pr, t));
    // CHECK_EQ(0, panelreel_validate(n_, pr));
  }

  SUBCASE("MovementWithOneTablet") {
    panelreel_options p{};
    p.infinitescroll = false;
    struct panelreel* pr = panelreel_create(n_, &p, -1);
    REQUIRE(pr);
    struct tablet* t = panelreel_add(pr, nullptr, nullptr, panelcb, nullptr);
    REQUIRE(t);
    // CHECK_EQ(0, panelreel_validate(n_, pr));
    CHECK(panelreel_next(pr));
    // CHECK_EQ(0, panelreel_validate(n_, pr));
    CHECK(panelreel_prev(pr));
    // CHECK_EQ(0, panelreel_validate(n_, pr));
    CHECK(0 == panelreel_del(pr, t));
    // CHECK_EQ(0, panelreel_validate(n_, pr));
  }

  SUBCASE("DeleteActiveTablet") {
    panelreel_options p{};
    p.infinitescroll = false;
    struct panelreel* pr = panelreel_create(n_, &p, -1);
    REQUIRE(pr);
    struct tablet* t = panelreel_add(pr, nullptr, nullptr, panelcb, nullptr);
    REQUIRE(t);
    CHECK(0 == panelreel_del_focused(pr));
  }

  SUBCASE("NoBorder") {
    panelreel_options p{};
    p.bordermask = NCBOXMASK_LEFT | NCBOXMASK_RIGHT |
                    NCBOXMASK_TOP | NCBOXMASK_BOTTOM;
    struct panelreel* pr = panelreel_create(n_, &p, -1);
    REQUIRE(pr);
  }

  SUBCASE("BadBorderBitsRejected") {
    panelreel_options p{};
    p.bordermask = NCBOXMASK_LEFT * 2;
    struct panelreel* pr = panelreel_create(n_, &p, -1);
    REQUIRE(!pr);
  }

  SUBCASE("NoTabletBorder") {
    panelreel_options p{};
    p.tabletmask = NCBOXMASK_LEFT | NCBOXMASK_RIGHT |
                    NCBOXMASK_TOP | NCBOXMASK_BOTTOM;
    struct panelreel* pr = panelreel_create(n_, &p, -1);
    REQUIRE(pr);
  }

  SUBCASE("NoTopBottomBorder") {
    panelreel_options p{};
    p.bordermask = NCBOXMASK_TOP | NCBOXMASK_BOTTOM;
    struct panelreel* pr = panelreel_create(n_, &p, -1);
    REQUIRE(pr);
  }

  SUBCASE("NoSideBorders") {
    panelreel_options p{};
    p.bordermask = NCBOXMASK_LEFT | NCBOXMASK_RIGHT;
    struct panelreel* pr = panelreel_create(n_, &p, -1);
    REQUIRE(pr);
  }

  SUBCASE("BadTabletBorderBitsRejected") {
    panelreel_options p{};
    p.tabletmask = NCBOXMASK_LEFT * 2;
    struct panelreel* pr = panelreel_create(n_, &p, -1);
    REQUIRE(!pr);
  }

  /*
  // Make a target window occupying all but a containing perimeter of the
  // specified WINDOW (which will usually be n_).
  struct ncpanel* make_targwin(struct ncpanel* w) {
    cchar_t cc;
    int cpair = COLOR_GREEN;
    CHECK_EQ(OK, setcchar(&cc, L"W", 0, 0, &cpair));
    int x, y, xx, yy;
    getbegyx(w, y, x);
    getmaxyx(w, yy, xx);
    yy -= 2;
    xx -= 2;
    ++x;
    ++y;
    WINDOW* ww = subwin(w, yy, xx, y, x);
    CHECK_NE(nullptr, ww);
    PANEL* p = new_panel(ww);
    CHECK_NE(nullptr, p);
    CHECK_EQ(OK, wbkgrnd(ww, &cc));
    return p;
  }

  SUBCASE("InitWithinSubwin") {
    panelreel_options p{};
    p.loff = 1;
    p.roff = 1;
    p.toff = 1;
    p.boff = 1;
    CHECK_EQ(0, clear());
    PANEL* base = make_targwin(n_);
    REQUIRE_NE(nullptr, base);
    WINDOW* basew = panel_window(base);
    REQUIRE_NE(nullptr, basew);
    struct panelreel* pr = panelreel_create(basew, &p, -1);
    REQUIRE_NE(nullptr, pr);
    CHECK_EQ(0, panelreel_validate(basew, pr));
    REQUIRE_EQ(0, panelreel_destroy(pr));
    CHECK_EQ(OK, del_panel(base));
    CHECK_EQ(OK, delwin(basew));
  }

  SUBCASE("SubwinNoPanelreelBorders") {
    panelreel_options p{};
    p.loff = 1;
    p.roff = 1;
    p.toff = 1;
    p.boff = 1;
    p.bordermask = NCBOXMASK_LEFT | NCBOXMASK_RIGHT |
                    NCBOXMASK_TOP | NCBOXMASK_BOTTOM;
    CHECK_EQ(0, clear());
    PANEL* base = make_targwin(n_);
    REQUIRE_NE(nullptr, base);
    WINDOW* basew = panel_window(base);
    REQUIRE_NE(nullptr, basew);
    struct panelreel* pr = panelreel_create(basew, &p, -1);
    REQUIRE_NE(nullptr, pr);
    CHECK_EQ(0, panelreel_validate(basew, pr));
    REQUIRE_EQ(0, panelreel_destroy(pr));
    CHECK_EQ(OK, del_panel(base));
    CHECK_EQ(OK, delwin(basew));
  }

  SUBCASE("SubwinNoOffsetGeom") {
    panelreel_options p{};
    CHECK_EQ(0, clear());
    PANEL* base = make_targwin(n_);
    REQUIRE_NE(nullptr, base);
    WINDOW* basew = panel_window(base);
    REQUIRE_NE(nullptr, basew);
    struct panelreel* pr = panelreel_create(basew, &p, -1);
    REQUIRE_NE(nullptr, pr);
    CHECK_EQ(0, panelreel_validate(basew, pr));
    REQUIRE_EQ(0, panelreel_destroy(pr));
    CHECK_EQ(OK, del_panel(base));
    CHECK_EQ(OK, delwin(basew));
  }
  */

  SUBCASE("TransparentBackground") {
    panelreel_options p{};
    channels_set_bg_alpha(&p.bgchannel, 3);
    struct panelreel* pr = panelreel_create(n_, &p, -1);
    REQUIRE(pr);
    // FIXME
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));
}
