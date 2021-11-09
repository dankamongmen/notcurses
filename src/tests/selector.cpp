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

  SUBCASE("EmptySelector") {
    struct ncselector_options opts{};
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* n = ncplane_create(n_, &nopts);
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
    auto title = strdup("hey hey whaddya say");
    opts.title = title;
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* n = ncplane_create(n_, &nopts);
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
    free(title);
  }

  SUBCASE("SecondarySelector") {
    struct ncselector_options opts{};
    auto secondary = strdup("this is not a title, but it's not *not* a title");
    opts.secondary = secondary;
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* n = ncplane_create(n_, &nopts);
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
    free(secondary);
  }

  SUBCASE("FooterSelector") {
    struct ncselector_options opts{};
    auto foot = strdup("i am a lone footer, little old footer");
    opts.footer = foot;
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* n = ncplane_create(n_, &nopts);
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
    free(foot);
  }

  SUBCASE("PopulatedSelector") {
    ncselector_item items[] = {
      { strdup("op1"), strdup("this is option 1"), },
      { strdup("2ndop"), strdup("this is option #2"), },
      { strdup("tres"), strdup("option the third"), },
      { NULL, NULL, },
    };
    struct ncselector_options opts{};
    opts.items = items;
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* n = ncplane_create(n_, &nopts);
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
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* n = ncplane_create(n_, &nopts);
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
      { strdup("op1"), strdup("this is option 1"), },
      { strdup("2ndop"), strdup("this is option #2"), },
      { strdup("tres"), strdup("option the third"), },
      { NULL, NULL, },
    };
    struct ncselector_options opts{};
    opts.items = items;
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* n = ncplane_create(n_, &nopts);
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
      { strdup("op1"), strdup("this is option 1"), },
      { strdup("2ndop"), strdup("this is option #2"), },
      { strdup("tres"), strdup("option the third"), },
      { NULL, NULL, },
    };
    struct ncselector_options opts{};
    opts.maxdisplay = 1;
    opts.items = items;
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* n = ncplane_create(n_, &nopts);
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
      { strdup("op1"), strdup("this is option 1"), },
      { strdup("2ndop"), strdup("this is option #2"), },
      { strdup("tres"), strdup("option the third"), },
      { NULL, NULL, },
    };
    struct ncselector_options opts{};
    opts.maxdisplay = 2;
    opts.items = items;
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 1,
      .cols = 1,
      .userptr = nullptr, .name = nullptr, .resizecb = nullptr, .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    struct ncplane* n = ncplane_create(n_, &nopts);
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
