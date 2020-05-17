#include "main.h"
#include <iostream>

auto panelcb(struct nctablet* t, int begx, int begy, int maxx, int maxy, bool cliptop) -> int {
  CHECK(nctablet_ncplane(t));
  CHECK(begx < maxx);
  CHECK(begy < maxy);
  CHECK(!nctablet_userptr(t));
  CHECK(!cliptop);
  // FIXME verify geometry is as expected
  return 0;
}

TEST_CASE("Reels") {
  notcurses_options nopts{};
  nopts.inhibit_alternate_screen = true;
  nopts.suppress_banner = true;
  struct notcurses* nc_ = notcurses_init(&nopts, nullptr);
  if(!nc_){
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);
  REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));

  SUBCASE("InitLinear") {
    ncreel_options r = { };
    struct ncreel* nr = ncreel_create(n_, &r, -1);
    REQUIRE(nr);
  }

  SUBCASE("InitLinearInfinite") {
    ncreel_options r{};
    r.flags = NCREEL_OPTIONS_INFINITESCROLL;
    struct ncreel* nr = ncreel_create(n_, &r, -1);
    REQUIRE(nr);
  }

  SUBCASE("InitCircular") {
    ncreel_options r{};
    r.flags = NCREEL_OPTIONS_INFINITESCROLL | NCREEL_OPTIONS_CIRCULAR;
    struct ncreel* nr = ncreel_create(n_, &r, -1);
    REQUIRE(nr);
    REQUIRE(0 == ncreel_destroy(nr));
  }

  // circular is not allowed to be true when infinitescroll is false
  SUBCASE("FiniteCircleRejected") {
    ncreel_options r{};
    r.flags = NCREEL_OPTIONS_CIRCULAR;
    struct ncreel* nr = ncreel_create(n_, &r, -1);
    REQUIRE(!nr);
  }

  // We ought be able to invoke ncreel_next() and ncreel_prev() safely,
  // even if there are no tablets. They both ought return nullptr.
  SUBCASE("MovementWithoutTablets") {
    ncreel_options r{};
    struct ncreel* nr = ncreel_create(n_, &r, -1);
    REQUIRE(nr);
    CHECK(!ncreel_next(nr));
    // CHECK_EQ(0, ncreel_validate(n_, pr));
    CHECK(!ncreel_prev(nr));
    // CHECK_EQ(0, ncreel_validate(n_, pr));
  }

  SUBCASE("OneTablet") {
    ncreel_options r{};
    struct ncreel* nr = ncreel_create(n_, &r, -1);
    REQUIRE(nr);
    struct nctablet* t = ncreel_add(nr, nullptr, nullptr, panelcb, nullptr);
    REQUIRE(t);
    // CHECK_EQ(0, ncreel_validate(n_, pr));
    CHECK(0 == ncreel_del(nr, t));
    // CHECK_EQ(0, ncreel_validate(n_, pr));
  }

  SUBCASE("MovementWithOneTablet") {
    ncreel_options r{};
    struct ncreel* nr = ncreel_create(n_, &r, -1);
    REQUIRE(nr);
    struct nctablet* t = ncreel_add(nr, nullptr, nullptr, panelcb, nullptr);
    REQUIRE(t);
    // CHECK_EQ(0, ncreel_validate(n_, pr));
    CHECK(ncreel_next(nr));
    // CHECK_EQ(0, ncreel_validate(n_, pr));
    CHECK(ncreel_prev(nr));
    // CHECK_EQ(0, ncreel_validate(n_, pr));
    CHECK(0 == ncreel_del(nr, t));
    // CHECK_EQ(0, ncreel_validate(n_, pr));
  }

  SUBCASE("DeleteActiveTablet") {
    ncreel_options r{};
    struct ncreel* nr = ncreel_create(n_, &r, -1);
    REQUIRE(nr);
    struct nctablet* t = ncreel_add(nr, nullptr, nullptr, panelcb, nullptr);
    REQUIRE(t);
    CHECK(0 == ncreel_del_focused(nr));
  }

  SUBCASE("NoBorder") {
    ncreel_options r{};
    r.bordermask = NCBOXMASK_LEFT | NCBOXMASK_RIGHT |
                    NCBOXMASK_TOP | NCBOXMASK_BOTTOM;
    struct ncreel* nr = ncreel_create(n_, &r, -1);
    REQUIRE(nr);
  }

  SUBCASE("BadBorderBitsRejected") {
    ncreel_options r{};
    r.bordermask = NCBOXMASK_LEFT * 2;
    struct ncreel* nr = ncreel_create(n_, &r, -1);
    REQUIRE(!nr);
  }

  SUBCASE("NoTabletBorder") {
    ncreel_options r{};
    r.tabletmask = NCBOXMASK_LEFT | NCBOXMASK_RIGHT |
                    NCBOXMASK_TOP | NCBOXMASK_BOTTOM;
    struct ncreel* nr = ncreel_create(n_, &r, -1);
    REQUIRE(nr);
  }

  SUBCASE("NoTopBottomBorder") {
    ncreel_options r{};
    r.bordermask = NCBOXMASK_TOP | NCBOXMASK_BOTTOM;
    struct ncreel* nr = ncreel_create(n_, &r, -1);
    REQUIRE(nr);
  }

  SUBCASE("NoSideBorders") {
    ncreel_options r{};
    r.bordermask = NCBOXMASK_LEFT | NCBOXMASK_RIGHT;
    struct ncreel* nr = ncreel_create(n_, &r, -1);
    REQUIRE(nr);
  }

  SUBCASE("BadTabletBorderBitsRejected") {
    ncreel_options r{};
    r.tabletmask = NCBOXMASK_LEFT * 2;
    struct ncreel* nr = ncreel_create(n_, &r, -1);
    REQUIRE(!nr);
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
    ncreel_options r{};
    r.loff = 1;
    r.roff = 1;
    r.toff = 1;
    r.boff = 1;
    CHECK_EQ(0, clear());
    PANEL* base = make_targwin(n_);
    REQUIRE_NE(nullptr, base);
    WINDOW* basew = panel_window(base);
    REQUIRE_NE(nullptr, basew);
    struct ncreel* nr = ncreel_create(basew, &r, -1);
    REQUIRE_NE(nullptr, pr);
    CHECK_EQ(0, ncreel_validate(basew, pr));
    REQUIRE_EQ(0, ncreel_destroy(nr));
    CHECK_EQ(OK, del_panel(base));
    CHECK_EQ(OK, delwin(basew));
  }

  SUBCASE("SubwinNoncreelBorders") {
    ncreel_options r{};
    r.loff = 1;
    r.roff = 1;
    r.toff = 1;
    r.boff = 1;
    r.bordermask = NCBOXMASK_LEFT | NCBOXMASK_RIGHT |
                    NCBOXMASK_TOP | NCBOXMASK_BOTTOM;
    CHECK_EQ(0, clear());
    PANEL* base = make_targwin(n_);
    REQUIRE_NE(nullptr, base);
    WINDOW* basew = panel_window(base);
    REQUIRE_NE(nullptr, basew);
    struct ncreel* nr = ncreel_create(basew, &r, -1);
    REQUIRE_NE(nullptr, pr);
    CHECK_EQ(0, ncreel_validate(basew, pr));
    REQUIRE_EQ(0, ncreel_destroy(nr));
    CHECK_EQ(OK, del_panel(base));
    CHECK_EQ(OK, delwin(basew));
  }

  SUBCASE("SubwinNoOffsetGeom") {
    ncreel_options r{};
    CHECK_EQ(0, clear());
    PANEL* base = make_targwin(n_);
    REQUIRE_NE(nullptr, base);
    WINDOW* basew = panel_window(base);
    REQUIRE_NE(nullptr, basew);
    struct ncreel* nr = ncreel_create(basew, &r, -1);
    REQUIRE_NE(nullptr, pr);
    CHECK_EQ(0, ncreel_validate(basew, pr));
    REQUIRE_EQ(0, ncreel_destroy(nr));
    CHECK_EQ(OK, del_panel(base));
    CHECK_EQ(OK, delwin(basew));
  }
  */

  SUBCASE("TransparentBackground") {
    ncreel_options r{};
    channels_set_bg_alpha(&r.bgchannel, 3);
    struct ncreel* nr = ncreel_create(n_, &r, -1);
    REQUIRE(nr);
    // FIXME
  }

  CHECK(0 == notcurses_stop(nc_));
}
