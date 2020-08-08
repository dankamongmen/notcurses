#include "main.h"
#include <iostream>

auto panelcb(struct nctablet* t, bool toptobottom) -> int {
  CHECK(nctablet_ncplane(t));
  CHECK(!nctablet_userptr(t));
  CHECK(toptobottom);
  // FIXME verify geometry is as expected
  return 0;
}

auto cbfxn(struct nctablet* t, bool toptobottom) -> int {
  (void)toptobottom;
  int* userptr = static_cast<int*>(nctablet_userptr(t));
  int y;
fprintf(stderr, "WRITEBACK CBFXN: %d %d\n", *userptr, y);
  ncplane_yx(nctablet_ncplane(t), &y, NULL);
  *userptr += y;
  return 4;
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
    CHECK(ncreel_validate(nr));
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
    CHECK(ncreel_validate(nr));
    CHECK(!ncreel_prev(nr));
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
  }

  SUBCASE("OneTablet") {
    ncreel_options r{};
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    struct nctablet* t = ncreel_add(nr, nullptr, nullptr, panelcb, nullptr);
    REQUIRE(t);
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    CHECK(0 == ncreel_del(nr, t));
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
  }

  SUBCASE("MovementWithOneTablet") {
    ncreel_options r{};
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    struct nctablet* t = ncreel_add(nr, nullptr, nullptr, panelcb, nullptr);
    REQUIRE(t);
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    CHECK(ncreel_next(nr));
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    CHECK(ncreel_prev(nr));
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    CHECK(0 == ncreel_del(nr, t));
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
  }

  SUBCASE("DeleteActiveTablet") {
    ncreel_options r{};
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    struct nctablet* t = ncreel_add(nr, nullptr, nullptr, panelcb, nullptr);
    REQUIRE(t);
    CHECK(0 == ncreel_del(nr, ncreel_focused(nr)));
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
  }

  SUBCASE("NoBorder") {
    ncreel_options r{};
    r.bordermask = NCBOXMASK_LEFT | NCBOXMASK_RIGHT |
                    NCBOXMASK_TOP | NCBOXMASK_BOTTOM;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
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
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
  }

  SUBCASE("NoTopBottomBorder") {
    ncreel_options r{};
    r.bordermask = NCBOXMASK_TOP | NCBOXMASK_BOTTOM;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
  }

  SUBCASE("NoSideBorders") {
    ncreel_options r{};
    r.bordermask = NCBOXMASK_LEFT | NCBOXMASK_RIGHT;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
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
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
  }

  // Layout tests. Add some tablets, move around, and verify that they all
  // have the expected locations/contents/geometries.
  SUBCASE("ThreeCycleDown") {
    ncreel_options r{};
    channels_set_bg_alpha(&r.bgchannel, 3);
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    int order[3];
    nctablet* tabs[3];
    for(size_t n = 0 ; n < sizeof(order) / sizeof(*order) ; ++n){
      order[n] = -1;
      tabs[n] = ncreel_add(nr, nullptr, nullptr, cbfxn, &order[n]);
      REQUIRE(tabs[n]);
      CHECK(tabs[0] == nr->tablets);
      CHECK_EQ(0, ncreel_redraw(nr));
      CHECK_EQ(0, notcurses_render(nc_));
      CHECK(ncreel_validate(nr));
    }
    for(size_t n = 0 ; n < sizeof(order) / sizeof(*order) ; ++n){
      CHECK_EQ(2 - n, order[n]);
    }
    ncreel_next(nr);
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    for(size_t n = 0 ; n < sizeof(order) / sizeof(*order) ; ++n){
      CHECK_EQ(2 - n + 1, order[n]);
    }
    ncreel_next(nr);
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    for(size_t n = 0 ; n < sizeof(order) / sizeof(*order) ; ++n){
      CHECK_EQ(2 - n + 2, order[n]);
    }
    ncreel_prev(nr);
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    for(size_t n = 0 ; n < sizeof(order) / sizeof(*order) ; ++n){
      CHECK_EQ(2 - n + 3, order[n]);
    }
    ncreel_prev(nr);
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    for(size_t n = 0 ; n < sizeof(order) / sizeof(*order) ; ++n){
      CHECK_EQ(2 - n + 4, order[n]);
    }
  }

  CHECK(0 == notcurses_stop(nc_));
}
