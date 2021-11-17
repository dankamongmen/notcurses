#include "main.h"
#include <cstring>
#include <iostream>

TEST_CASE("Selectors") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);
  REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));

  // selector can't be bound to the standard plane
  SUBCASE("RefuseStandardPlane") {
    ncselector_options s{};
    struct ncselector* ns = ncselector_create(n_, &s);
    REQUIRE(nullptr == ns);
  }

  // create a selector, but don't explicitly destroy it, thus testing the
  // context shutdown cleanup path
  SUBCASE("ImplicitDestroy") {
    ncselector_options s{};
    struct ncplane_options nopts{};
    nopts.rows = 1;
    nopts.cols = 1;
    struct ncplane* n = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != n);
    struct ncselector* ns = ncselector_create(n, &s);
    REQUIRE(ns);
    CHECK(0 == notcurses_render(nc_));
  }

  // now do the same, but with a plane we have created.
  SUBCASE("RefuseBoundCreatedPlane") {
    struct ncplane_options nopts{};
    nopts.rows = ncplane_dim_y(n_);
    nopts.cols = ncplane_dim_x(n_);
    auto ncp = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != ncp);
    ncselector_options s{};
    struct ncselector* ns = ncselector_create(ncp, &s);
    REQUIRE(ns);
    CHECK(0 == notcurses_render(nc_));
    struct ncselector* fail = ncselector_create(ncp, &s);
    CHECK(nullptr == fail);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("EmptySelector") {
    struct ncselector_options opts{};
    struct ncplane_options nopts{};
    nopts.rows = 1;
    nopts.cols = 1;
    struct ncplane* n = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != n);
    struct ncselector* ncs = ncselector_create(n, &opts);
    REQUIRE(nullptr != ncs);
    CHECK(0 == notcurses_render(nc_));
    CHECK(nullptr == ncselector_selected(ncs));
    struct ncplane* ncsp = ncselector_plane(ncs);
    REQUIRE(nullptr != ncsp);
    unsigned dimy, dimx;
    ncplane_dim_yx(ncsp, &dimy, &dimx);
    CHECK(4 == dimy);
    CHECK(5 == dimx);
    ncselector_destroy(ncs, nullptr);
  }

  SUBCASE("TitledSelector") {
    struct ncselector_options opts{};
    auto title = "hey hey whaddya say";
    opts.title = title;
    struct ncplane_options nopts{};
    nopts.rows = 1;
    nopts.cols = 1;
    struct ncplane* n = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != n);
    struct ncselector* ncs = ncselector_create(n, &opts);
    REQUIRE(nullptr != ncs);
    CHECK(0 == notcurses_render(nc_));
    struct ncplane* ncsp = ncselector_plane(ncs);
    REQUIRE(nullptr != ncsp);
    unsigned dimy, dimx;
    ncplane_dim_yx(ncsp, &dimy, &dimx);
    CHECK(6 == dimy);
    CHECK(strlen(opts.title) + 4 == dimx);
    ncselector_destroy(ncs, nullptr);
  }

  SUBCASE("SecondarySelector") {
    struct ncselector_options opts{};
    auto secondary = "this is not a title, but it's not *not* a title";
    opts.secondary = secondary;
    struct ncplane_options nopts{};
    nopts.rows = 1;
    nopts.cols = 1;
    struct ncplane* n = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != n);
    struct ncselector* ncs = ncselector_create(n, &opts);
    REQUIRE(nullptr != ncs);
    CHECK(0 == notcurses_render(nc_));
    struct ncplane* ncsp = ncselector_plane(ncs);
    REQUIRE(nullptr != ncsp);
    unsigned dimy, dimx;
    ncplane_dim_yx(ncsp, &dimy, &dimx);
    CHECK(4 == dimy);
    CHECK(strlen(opts.secondary) + 2 == dimx);
    ncselector_destroy(ncs, nullptr);
  }

  SUBCASE("FooterSelector") {
    struct ncselector_options opts{};
    auto foot = "i am a lone footer, little old footer";
    opts.footer = foot;
    struct ncplane_options nopts{};
    nopts.rows = 1;
    nopts.cols = 1;
    struct ncplane* n = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != n);
    struct ncselector* ncs = ncselector_create(n, &opts);
    REQUIRE(nullptr != ncs);
    CHECK(0 == notcurses_render(nc_));
    struct ncplane* ncsp = ncselector_plane(ncs);
    REQUIRE(nullptr != ncsp);
    unsigned dimy, dimx;
    ncplane_dim_yx(ncsp, &dimy, &dimx);
    CHECK(4 == dimy);
    CHECK(strlen(opts.footer) + 2 == dimx);
    ncselector_destroy(ncs, nullptr);
  }

  SUBCASE("PopulatedSelector") {
    ncselector_item items[] = {
      { "op1", "this is option 1", },
      { "2ndop", "this is option #2", },
      { "tres", "option the third", },
      { nullptr, nullptr, },
    };
    struct ncselector_options opts{};
    opts.items = items;
    struct ncplane_options nopts{};
    nopts.rows = 1;
    nopts.cols = 1;
    struct ncplane* n = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != n);
    struct ncselector* ncs = ncselector_create(n, &opts);
    REQUIRE(nullptr != ncs);
    CHECK(0 == notcurses_render(nc_));
    struct ncplane* ncsp = ncselector_plane(ncs);
    REQUIRE(nullptr != ncsp);
    unsigned dimy, dimx;
    ncplane_dim_yx(ncsp, &dimy, &dimx);
    CHECK(7 == dimy);
    CHECK(15 < dimx);
    ncselector_destroy(ncs, nullptr);
  }

  SUBCASE("EmptySelectorMovement") {
    struct ncselector_options opts{};
    struct ncplane_options nopts{};
    nopts.rows = 1;
    nopts.cols = 1;
    struct ncplane* n = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != n);
    struct ncselector* ncs = ncselector_create(n, &opts);
    REQUIRE(nullptr != ncs);
    CHECK(0 == notcurses_render(nc_));
    auto sel = ncselector_selected(ncs);
    REQUIRE(nullptr == sel);
    sel = ncselector_nextitem(ncs);
    REQUIRE(nullptr == sel);
    CHECK(0 == notcurses_render(nc_));
    sel = ncselector_previtem(ncs);
    REQUIRE(nullptr == sel);
    CHECK(0 == notcurses_render(nc_));
    ncselector_destroy(ncs, nullptr);
  }

  SUBCASE("SelectorMovement") {
    ncselector_item items[] = {
      { "op1", "this is option 1", },
      { "2ndop", "this is option #2", },
      { "tres", "option the third", },
      { nullptr, nullptr, },
    };
    struct ncselector_options opts{};
    opts.items = items;
    struct ncplane_options nopts{};
    nopts.rows = 1;
    nopts.cols = 1;
    struct ncplane* n = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != n);
    struct ncselector* ncs = ncselector_create(n, &opts);
    REQUIRE(nullptr != ncs);
    auto sel = ncselector_selected(ncs);
    REQUIRE(nullptr != sel);
    CHECK(0 == strcmp(sel, items[0].option));
    CHECK(0 == notcurses_render(nc_));
    sel = ncselector_nextitem(ncs);
    REQUIRE(nullptr != sel);
    CHECK(0 == strcmp(sel, items[1].option));
    CHECK(0 == notcurses_render(nc_));
    sel = ncselector_previtem(ncs);
    REQUIRE(nullptr != sel);
    CHECK(0 == strcmp(sel, items[0].option));
    CHECK(0 == notcurses_render(nc_));
    // wrap around from the top to bottom...
    sel = ncselector_previtem(ncs);
    REQUIRE(nullptr != sel);
    CHECK(0 == strcmp(sel, items[2].option));
    CHECK(0 == notcurses_render(nc_));
    // ...and back to the top
    sel = ncselector_nextitem(ncs);
    REQUIRE(nullptr != sel);
    CHECK(0 == strcmp(sel, items[0].option));
    CHECK(0 == notcurses_render(nc_));
    ncselector_destroy(ncs, nullptr);
  }

  // Provide three items, limited to 1 shown at a time
  SUBCASE("ScrollingSelectorOne") {
    ncselector_item items[] = {
      { "op1", "this is option 1", },
      { "2ndop", "this is option #2", },
      { "tres", "option the third", },
      { nullptr, nullptr, },
    };
    struct ncselector_options opts{};
    opts.maxdisplay = 1;
    opts.items = items;
    struct ncplane_options nopts{};
    nopts.rows = 1;
    nopts.cols = 1;
    struct ncplane* n = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != n);
    struct ncselector* ncs = ncselector_create(n, &opts);
    REQUIRE(nullptr != ncs);
    CHECK(0 == notcurses_render(nc_));
    auto sel = ncselector_selected(ncs);
    REQUIRE(nullptr != sel);
    CHECK(0 == strcmp(sel, items[0].option));
    CHECK(0 == notcurses_render(nc_));
    sel = ncselector_nextitem(ncs);
    REQUIRE(nullptr != sel);
    CHECK(0 == strcmp(sel, items[1].option));
    CHECK(0 == notcurses_render(nc_));
    sel = ncselector_previtem(ncs);
    REQUIRE(nullptr != sel);
    CHECK(0 == strcmp(sel, items[0].option));
    CHECK(0 == notcurses_render(nc_));
    // wrap around from the top to bottom...
    sel = ncselector_previtem(ncs);
    REQUIRE(nullptr != sel);
    CHECK(0 == strcmp(sel, items[2].option));
    CHECK(0 == notcurses_render(nc_));
    // ...and back to the top
    sel = ncselector_nextitem(ncs);
    REQUIRE(nullptr != sel);
    CHECK(0 == strcmp(sel, items[0].option));
    struct ncplane* ncsp = ncselector_plane(ncs);
    REQUIRE(nullptr != ncsp);
    unsigned dimy, dimx;
    ncplane_dim_yx(ncsp, &dimy, &dimx);
    CHECK(5 == dimy);
    ncselector_destroy(ncs, nullptr);
  }

  // Provide three items, limited to 2 shown at a time
  SUBCASE("ScrollingSelectorTwo") {
    ncselector_item items[] = {
      { "op1", "this is option 1", },
      { "2ndop", "this is option #2", },
      { "tres", "option the third", },
      { nullptr, nullptr, },
    };
    struct ncselector_options opts{};
    opts.maxdisplay = 2;
    opts.items = items;
    struct ncplane_options nopts{};
    nopts.rows = 1;
    nopts.cols = 1;
    struct ncplane* n = ncplane_create(n_, &nopts);
    REQUIRE(nullptr != n);
    struct ncselector* ncs = ncselector_create(n, &opts);
    REQUIRE(nullptr != ncs);
    CHECK(0 == notcurses_render(nc_));
    const char* sel = ncselector_selected(ncs);
    CHECK(0 == strcmp(sel, items[0].option));
    CHECK(0 == notcurses_render(nc_));
    sel = ncselector_nextitem(ncs);
    REQUIRE(nullptr != sel);
    CHECK(0 == strcmp(sel, items[1].option));
    CHECK(0 == notcurses_render(nc_));
    sel = ncselector_previtem(ncs);
    REQUIRE(nullptr != sel);
    CHECK(0 == strcmp(sel, items[0].option));
    CHECK(0 == notcurses_render(nc_));
    // wrap around from the top to bottom...
    sel = ncselector_previtem(ncs);
    REQUIRE(nullptr != sel);
    CHECK(0 == strcmp(sel, items[2].option));
    CHECK(0 == notcurses_render(nc_));
    // ...and back to the top
    sel = ncselector_nextitem(ncs);
    REQUIRE(nullptr != sel);
    CHECK(0 == strcmp(sel, items[0].option));
    struct ncplane* ncsp = ncselector_plane(ncs);
    REQUIRE(nullptr != ncsp);
    unsigned dimy, dimx;
    ncplane_dim_yx(ncsp, &dimy, &dimx);
    CHECK(6 == dimy);
    ncselector_destroy(ncs, nullptr);
  }

  CHECK(0 == notcurses_stop(nc_));
}
