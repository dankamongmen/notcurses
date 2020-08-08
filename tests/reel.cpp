#include "main.h"
#include <iostream>

auto panelcb(struct nctablet* t, bool cliptop) -> int {
  CHECK(nctablet_ncplane(t));
  CHECK(!nctablet_userptr(t));
  CHECK(!cliptop);
  // FIXME verify geometry is as expected
  return 0;
}

// debugging
bool ncreel_validate(const ncreel* n){
  if(n->tablets == NULL){
    return true;
  }
  const nctablet* t = n->tablets;
  int cury = -1;
  bool wentaround = false;
  do{
    const ncplane* np = t->p;
    if(np){
      int y, x;
      ncplane_yx(np, &y, &x);
//fprintf(stderr, "forvart: %p (%p) @ %d\n", t, np, y);
      if(y < cury){
        if(wentaround){
          return false;
        }
        wentaround = true;
      }else if(y == cury){
        return false;
      }
      cury = y;
    }
  }while((t = t->next) != n->tablets);
  cury = INT_MAX;
  wentaround = false;
  do{
    const ncplane* np = t->p;
    if(np){
      int y, x;
      ncplane_yx(np, &y, &x);
//fprintf(stderr, "backwards: %p (%p) @ %d\n", t, np, y);
      if(y > cury){
        if(wentaround){
          return false;
        }
        wentaround = true;
      }else if(y == cury){
        return false;
      }
      cury = y;
    }
  }while((t = t->prev) != n->tablets);
  return true;
}

TEST_CASE("Reels") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);
  REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));

  SUBCASE("InitLinear") {
    ncreel_options r = { };
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
  }

  SUBCASE("InitLinearInfinite") {
    ncreel_options r{};
    r.flags = NCREEL_OPTION_INFINITESCROLL;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
  }

  SUBCASE("InitCircular") {
    ncreel_options r{};
    r.flags = NCREEL_OPTION_INFINITESCROLL | NCREEL_OPTION_CIRCULAR;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    REQUIRE(0 == ncreel_destroy(nr));
  }

  // circular is not allowed to be true when infinitescroll is false
  SUBCASE("FiniteCircleRejected") {
    ncreel_options r{};
    r.flags = NCREEL_OPTION_CIRCULAR;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(!nr);
  }

  // We ought be able to invoke ncreel_next() and ncreel_prev() safely,
  // even if there are no tablets. They both ought return nullptr.
  SUBCASE("MovementWithoutTablets") {
    ncreel_options r{};
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    CHECK(!ncreel_next(nr));
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(!ncreel_prev(nr));
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
  }

  SUBCASE("OneTablet") {
    ncreel_options r{};
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    struct nctablet* t = ncreel_add(nr, nullptr, nullptr, panelcb, nullptr);
    REQUIRE(t);
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(0 == ncreel_del(nr, t));
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
  }

  SUBCASE("MovementWithOneTablet") {
    ncreel_options r{};
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    struct nctablet* t = ncreel_add(nr, nullptr, nullptr, panelcb, nullptr);
    REQUIRE(t);
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK(ncreel_next(nr));
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK(ncreel_prev(nr));
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK(0 == ncreel_del(nr, t));
    CHECK_EQ(0, ncreel_redraw(nr));
  }

  SUBCASE("DeleteActiveTablet") {
    ncreel_options r{};
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    struct nctablet* t = ncreel_add(nr, nullptr, nullptr, panelcb, nullptr);
    REQUIRE(t);
    CHECK(0 == ncreel_del(nr, ncreel_focused(nr)));
  }

  SUBCASE("NoBorder") {
    ncreel_options r{};
    r.bordermask = NCBOXMASK_LEFT | NCBOXMASK_RIGHT |
                    NCBOXMASK_TOP | NCBOXMASK_BOTTOM;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
  }

  SUBCASE("BadBorderBitsRejected") {
    ncreel_options r{};
    r.bordermask = NCBOXMASK_LEFT * 2;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(!nr);
  }

  SUBCASE("NoTabletBorder") {
    ncreel_options r{};
    r.tabletmask = NCBOXMASK_LEFT | NCBOXMASK_RIGHT |
                    NCBOXMASK_TOP | NCBOXMASK_BOTTOM;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
  }

  SUBCASE("NoTopBottomBorder") {
    ncreel_options r{};
    r.bordermask = NCBOXMASK_TOP | NCBOXMASK_BOTTOM;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
  }

  SUBCASE("NoSideBorders") {
    ncreel_options r{};
    r.bordermask = NCBOXMASK_LEFT | NCBOXMASK_RIGHT;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
  }

  SUBCASE("BadTabletBorderBitsRejected") {
    ncreel_options r{};
    r.tabletmask = NCBOXMASK_LEFT * 2;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(!nr);
  }

  SUBCASE("TransparentBackground") {
    ncreel_options r{};
    channels_set_bg_alpha(&r.bgchannel, 3);
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    // FIXME
  }

  CHECK(0 == notcurses_stop(nc_));
}
