#include "main.h"
#include <iostream>

auto ncreel_validate(const ncreel* n) -> bool {
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
      if(y < cury + 1){
        if(wentaround){
          return false;
        }
        wentaround = true;
      }else if(y == cury){
        return false;
      }
      unsigned ylen, xlen;
      ncplane_dim_yx(np, &ylen, &xlen);
      cury = y + ylen - 1;
    }
  }while((t = t->next) != n->tablets);
  cury = INT_MAX;
  wentaround = false;
  do{
    const ncplane* np = t->p;
    if(np){
      int y, x;
      unsigned ylen, xlen;
      ncplane_yx(np, &y, &x);
      ncplane_dim_yx(np, &ylen, &xlen);
//fprintf(stderr, "backwards: %p (%p) @ %d\n", t, np, y);
      if(y + static_cast<int>(ylen) - 1 > cury - 1){
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

auto panelcb(nctablet* t, bool toptobottom) -> int {
  CHECK(nctablet_plane(t));
  CHECK(!nctablet_userptr(t));
  CHECK(toptobottom);
  // FIXME verify geometry is as expected
  return 0;
}

auto cbfxn(nctablet* t, bool toptobottom) -> int {
  (void)toptobottom;
  int* userptr = static_cast<int*>(nctablet_userptr(t));
  ++*userptr;
  return 4;
}

int check_allborders(nctablet* t, bool drawfromtop) {
  (void)drawfromtop;
  auto ncp = nctablet_plane(t);
  REQUIRE(ncp);
  unsigned rows, cols;
  ncplane_dim_yx(ncp, &rows, &cols);
  unsigned srows, scols;
  ncplane_dim_yx(notcurses_stdplane(ncplane_notcurses(ncp)), &srows, &scols);
  CHECK(srows >= rows + 3);
  CHECK(scols == cols + 4);
  return 1;
}

int check_noborders(nctablet* t, bool drawfromtop) {
  (void)drawfromtop;
  auto ncp = nctablet_plane(t);
  REQUIRE(ncp);
  unsigned rows, cols;
  ncplane_dim_yx(ncp, &rows, &cols);
  unsigned srows, scols;
  ncplane_dim_yx(notcurses_stdplane(ncplane_notcurses(ncp)), &srows, &scols);
  CHECK(srows == rows);
  CHECK(scols == cols);
  return 1;
}

int check_notborders(nctablet* t, bool drawfromtop) {
  (void)drawfromtop;
  auto ncp = nctablet_plane(t);
  REQUIRE(ncp);
  unsigned rows, cols;
  ncplane_dim_yx(ncp, &rows, &cols);
  unsigned srows, scols;
  ncplane_dim_yx(notcurses_stdplane(ncplane_notcurses(ncp)), &srows, &scols);
  CHECK(srows == rows + 2); // we get maximum possible size to try out
  CHECK(scols == cols + 2);
  return 1;
}

int check_norborders(nctablet* t, bool drawfromtop) {
  (void)drawfromtop;
  auto ncp = nctablet_plane(t);
  REQUIRE(ncp);
  unsigned rows, cols;
  ncplane_dim_yx(ncp, &rows, &cols);
  unsigned srows, scols;
  ncplane_dim_yx(notcurses_stdplane(ncplane_notcurses(ncp)), &srows, &scols);
  CHECK(srows >= rows + 1);
  CHECK(scols == cols + 2);
  return 1;
}

TEST_CASE("Reels") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);
  REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));

  // create a reel, but don't explicitly destroy it, thus testing the
  // context shutdown cleanup path
  SUBCASE("ImplicitDestroy") {
    ncreel_options r{};
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    CHECK(0 == notcurses_render(nc_));
  }

  // attempt to bind a single plane to two different reels, ensuring that it is
  // refused by the second (and testing that error path). this ought result in
  // the shared plane (and thus the original widget) also being destroyed.
  SUBCASE("RefuseBoundStandardPlane") {
    ncreel_options r{};
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    CHECK(0 == notcurses_render(nc_));
    struct ncreel* fail = ncreel_create(n_, &r);
    CHECK(nullptr == fail);
    CHECK(0 == notcurses_render(nc_));
  }

  // now do the same, but with a plane we have created.
  SUBCASE("RefuseBoundCreatedPlane") {
    struct ncplane_options nopts{};
    nopts.rows = ncplane_dim_y(n_);
    nopts.cols = ncplane_dim_x(n_);
    auto ncp = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != ncp);
    ncreel_options r{};
    struct ncreel* nr = ncreel_create(ncp, &r);
    REQUIRE(nr);
    CHECK(0 == notcurses_render(nc_));
    struct ncreel* fail = ncreel_create(ncp, &r);
    CHECK(nullptr == fail);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("InitLinear") {
    ncreel_options r{};
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    CHECK(0 == notcurses_render(nc_));
    ncreel_destroy(nr);
  }

  SUBCASE("InitLinearInfinite") {
    ncreel_options r{};
    r.flags = NCREEL_OPTION_INFINITESCROLL;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    CHECK(0 == notcurses_render(nc_));
    ncreel_destroy(nr);
  }

  SUBCASE("InitCircular") {
    ncreel_options r{};
    r.flags = NCREEL_OPTION_INFINITESCROLL | NCREEL_OPTION_CIRCULAR;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    CHECK(ncreel_validate(nr));
    CHECK(0 == notcurses_render(nc_));
    ncreel_destroy(nr);
  }

  // circular is not allowed to be true when infinitescroll is false
  SUBCASE("FiniteCircleRejected") {
    ncreel_options r{};
    r.flags = NCREEL_OPTION_CIRCULAR;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(!nr);
    CHECK(0 == notcurses_render(nc_));
    ncreel_destroy(nr);
  }

  // We ought be able to invoke ncreel_next() and ncreel_prev() safely,
  // even if there are no tablets. They both ought return nullptr.
  SUBCASE("MovementWithoutTablets") {
    ncreel_options r{};
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    CHECK(!ncreel_next(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    CHECK(!ncreel_prev(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    ncreel_destroy(nr);
  }

  SUBCASE("OneTablet") {
    ncreel_options r{};
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    nctablet* t = ncreel_add(nr, nullptr, nullptr, panelcb, nullptr);
    REQUIRE(t);
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    CHECK(0 == ncreel_del(nr, t));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    ncreel_destroy(nr);
  }

  SUBCASE("MovementWithOneTablet") {
    ncreel_options r{};
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    nctablet* t = ncreel_add(nr, nullptr, nullptr, panelcb, nullptr);
    REQUIRE(t);
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    CHECK(ncreel_next(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    CHECK(ncreel_prev(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    CHECK(0 == ncreel_del(nr, t));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    ncreel_destroy(nr);
  }

  SUBCASE("DeleteActiveTablet") {
    ncreel_options r{};
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    nctablet* t = ncreel_add(nr, nullptr, nullptr, panelcb, nullptr);
    REQUIRE(t);
    CHECK(0 == ncreel_del(nr, ncreel_focused(nr)));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    ncreel_destroy(nr);
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
    ncreel_destroy(nr);
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
    ncreel_destroy(nr);
  }

  SUBCASE("NoTopBottomBorder") {
    ncreel_options r{};
    r.bordermask = NCBOXMASK_TOP | NCBOXMASK_BOTTOM;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    ncreel_destroy(nr);
  }

  SUBCASE("NoSideBorders") {
    ncreel_options r{};
    r.bordermask = NCBOXMASK_LEFT | NCBOXMASK_RIGHT;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    ncreel_destroy(nr);
  }

  SUBCASE("BadTabletBorderBitsRejected") {
    ncreel_options r{};
    r.tabletmask = NCBOXMASK_LEFT * 2;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(!nr);
  }

  SUBCASE("TransparentBackground") {
    ncreel_options r{};
    unsigned dimy, dimx;
    notcurses_term_dim_yx(nc_, &dimy, &dimx);
    uint64_t channels = 0;
    ncchannels_set_bg_alpha(&channels, NCALPHA_TRANSPARENT);
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = dimy,
      .cols = dimx,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto ncp = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != ncp);
    CHECK(0 == ncplane_set_base(ncp, "", 0, channels));
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    CHECK_EQ(0, ncreel_redraw(nr));
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    ncreel_destroy(nr);
  }

  // Layout tests. Add some tablets, move around, and verify that they all
  // have the expected locations/contents/geometries.
  SUBCASE("ThreeCycleDown") {
    ncreel_options r{};
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
      REQUIRE(nctablet_plane(tabs[n]));
      CHECK_EQ(0, notcurses_render(nc_));
      CHECK(ncreel_validate(nr));
    }
    int expectedy = 1;
    for(size_t n = 0 ; n < sizeof(order) / sizeof(*order) ; ++n){
      CHECK_LE(0, order[n]);
      int y;
      ncplane_yx(ncplane_parent(nctablet_plane(tabs[n])), &y, nullptr);
      CHECK(y == expectedy);
      expectedy += 7;
    }
    ncreel_next(nr);
    CHECK(tabs[1] == nr->tablets);
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    expectedy = 1;
    for(size_t n = 0 ; n < sizeof(order) / sizeof(*order) ; ++n){
      CHECK_LE(1, order[n]);
      int y;
      ncplane_yx(ncplane_parent(nctablet_plane(tabs[n])), &y, nullptr);
      CHECK(y == expectedy);
      expectedy += 7;
    }
    ncreel_next(nr);
    CHECK(tabs[2] == nr->tablets);
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    expectedy = 1;
    for(size_t n = 0 ; n < sizeof(order) / sizeof(*order) ; ++n){
      CHECK_LE(2, order[n]);
      int y;
      ncplane_yx(ncplane_parent(nctablet_plane(tabs[n])), &y, nullptr);
      CHECK(y == expectedy);
      expectedy += 7;
    }
    ncreel_prev(nr);
    CHECK(tabs[1] == nr->tablets);
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    expectedy = 1;
    for(size_t n = 0 ; n < sizeof(order) / sizeof(*order) ; ++n){
      CHECK_LE(3, order[n]);
      int y;
      ncplane_yx(ncplane_parent(nctablet_plane(tabs[n])), &y, nullptr);
      CHECK(y == expectedy);
      expectedy += 7;
    }
    ncreel_prev(nr);
    CHECK(tabs[0] == nr->tablets);
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    expectedy = 1;
    for(size_t n = 0 ; n < sizeof(order) / sizeof(*order) ; ++n){
      CHECK_LE(4, order[n]);
      int y;
      ncplane_yx(ncplane_parent(nctablet_plane(tabs[n])), &y, nullptr);
      CHECK(y == expectedy);
      expectedy += 7;
    }
    ncreel_destroy(nr);
  }

  // Layout tests. Add some tablets, move around, and verify that they all
  // have the expected locations/contents/geometries.
  SUBCASE("ThreeCycleDownNoTabletBorders") {
    ncreel_options r{};
    r.tabletmask = 0xf;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    int order[3];
    nctablet* tabs[3];
    for(size_t n = 0 ; n < sizeof(order) / sizeof(*order) ; ++n){
      order[n] = -1;
      tabs[n] = ncreel_add(nr, nullptr, nullptr, cbfxn, &order[n]);
      REQUIRE(tabs[n]);
      CHECK(tabs[0] == nr->tablets);
      CHECK_EQ(0, notcurses_render(nc_));
      CHECK(ncreel_validate(nr));
    }
    int expectedy = 1;
    for(size_t n = 0 ; n < sizeof(order) / sizeof(*order) ; ++n){
      CHECK_LE(-1, order[n]);
      int y;
      ncplane_yx(ncplane_parent(nctablet_plane(tabs[n])), &y, nullptr);
      CHECK(y == expectedy);
      expectedy += 5;
    }
    ncreel_next(nr);
    CHECK(tabs[1] == nr->tablets);
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    expectedy = 1;
    for(size_t n = 0 ; n < sizeof(order) / sizeof(*order) ; ++n){
      CHECK_LE(1, order[n]);
      int y;
      ncplane_yx(ncplane_parent(nctablet_plane(tabs[n])), &y, nullptr);
      CHECK(y == expectedy);
      expectedy += 5;
    }
    ncreel_next(nr);
    CHECK(tabs[2] == nr->tablets);
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    expectedy = 1;
    for(size_t n = 0 ; n < sizeof(order) / sizeof(*order) ; ++n){
      CHECK_LE(2, order[n]);
      int y;
      ncplane_yx(ncplane_parent(nctablet_plane(tabs[n])), &y, nullptr);
      CHECK(y == expectedy);
      expectedy += 5;
    }
    ncreel_prev(nr);
    CHECK(tabs[1] == nr->tablets);
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    expectedy = 1;
    for(size_t n = 0 ; n < sizeof(order) / sizeof(*order) ; ++n){
      CHECK_LE(3, order[n]);
      int y;
      ncplane_yx(ncplane_parent(nctablet_plane(tabs[n])), &y, nullptr);
      CHECK(y == expectedy);
      expectedy += 5;
    }
    ncreel_prev(nr);
    CHECK(tabs[0] == nr->tablets);
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    expectedy = 1;
    for(size_t n = 0 ; n < sizeof(order) / sizeof(*order) ; ++n){
      CHECK_LE(4, order[n]);
      int y;
      ncplane_yx(ncplane_parent(nctablet_plane(tabs[n])), &y, nullptr);
      CHECK(y == expectedy);
      expectedy += 5;
    }
    ncreel_destroy(nr);
  }

  // tablet size checks----------------------------------------------------
  // check that, with all borders, the tablets are the correct size
  SUBCASE("AllBordersSize") {
    ncreel_options r{};
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    REQUIRE(nullptr != ncreel_add(nr, nullptr, nullptr, check_allborders, nullptr));
    CHECK_EQ(0, notcurses_render(nc_));
    ncreel_destroy(nr);
  }

  // check that, without any borders, the tablets are the correct size
  SUBCASE("NoBordersSize") {
    ncreel_options r{};
    r.tabletmask = 0xf;
    r.bordermask = 0xf;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    REQUIRE(nullptr != ncreel_add(nr, nullptr, nullptr, check_noborders, nullptr));
    CHECK_EQ(0, notcurses_render(nc_));
    ncreel_destroy(nr);
  }

  // check that, without tablet borders (but with reel borders), the tablets
  // are the correct size
  SUBCASE("NoTabletBordersSize") {
    ncreel_options r{};
    r.tabletmask = 0xf;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    REQUIRE(nullptr != ncreel_add(nr, nullptr, nullptr, check_notborders, nullptr));
    CHECK_EQ(0, notcurses_render(nc_));
    ncreel_destroy(nr);
  }

  // check that, without reel borders (but with tablet borders), the tablets
  // are the correct size
  SUBCASE("NoReelBordersSize") {
    ncreel_options r{};
    r.bordermask = 0xf;
    struct ncreel* nr = ncreel_create(n_, &r);
    REQUIRE(nr);
    CHECK_EQ(0, notcurses_render(nc_));
    CHECK(ncreel_validate(nr));
    REQUIRE(nullptr != ncreel_add(nr, nullptr, nullptr, check_norborders, nullptr));
    CHECK_EQ(0, notcurses_render(nc_));
    ncreel_destroy(nr);
  }

  CHECK(0 == notcurses_stop(nc_));
}
