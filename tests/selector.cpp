#include "main.h"
#include <cstring>
#include <iostream>

TEST_CASE("SelectorTest") {
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
  REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));

  SUBCASE("EmptySelector") {
    struct selector_options opts{};
    struct ncselector* ncs = ncselector_create(notcurses_stdplane(nc_), 0, 0, &opts);
    REQUIRE(nullptr != ncs);
    CHECK(0 == notcurses_render(nc_));
    CHECK(nullptr == ncselector_selected(ncs));
    struct ncplane* ncsp = ncselector_plane(ncs);
    REQUIRE(nullptr != ncsp);
    int dimy, dimx;
    ncplane_dim_yx(ncsp, &dimy, &dimx);
    CHECK(4 == dimy);
    CHECK(5 == dimx);
    ncselector_destroy(ncs, nullptr);
  }

  SUBCASE("TitledSelector") {
    struct selector_options opts{};
    opts.title = strdup("hey hey whaddya say");
    struct ncselector* ncs = ncselector_create(notcurses_stdplane(nc_), 0, 0, &opts);
    REQUIRE(nullptr != ncs);
    CHECK(0 == notcurses_render(nc_));
    struct ncplane* ncsp = ncselector_plane(ncs);
    REQUIRE(nullptr != ncsp);
    int dimy, dimx;
    ncplane_dim_yx(ncsp, &dimy, &dimx);
    CHECK(6 == dimy);
    CHECK(strlen(opts.title) + 4 == dimx);
    ncselector_destroy(ncs, nullptr);
  }

  SUBCASE("SecondarySelector") {
    struct selector_options opts{};
    opts.secondary = strdup("this is not a title, but it's not *not* a title");
    struct ncselector* ncs = ncselector_create(notcurses_stdplane(nc_), 0, 0, &opts);
    REQUIRE(nullptr != ncs);
    CHECK(0 == notcurses_render(nc_));
    struct ncplane* ncsp = ncselector_plane(ncs);
    REQUIRE(nullptr != ncsp);
    int dimy, dimx;
    ncplane_dim_yx(ncsp, &dimy, &dimx);
    CHECK(4 == dimy);
    CHECK(strlen(opts.secondary) + 2 == dimx);
    ncselector_destroy(ncs, nullptr);
  }

  SUBCASE("FooterSelector") {
    struct selector_options opts{};
    opts.footer = strdup("i am a lone footer, little old footer");
    struct ncselector* ncs = ncselector_create(notcurses_stdplane(nc_), 0, 0, &opts);
    REQUIRE(nullptr != ncs);
    CHECK(0 == notcurses_render(nc_));
    struct ncplane* ncsp = ncselector_plane(ncs);
    REQUIRE(nullptr != ncsp);
    int dimy, dimx;
    ncplane_dim_yx(ncsp, &dimy, &dimx);
    CHECK(4 == dimy);
    CHECK(strlen(opts.footer) + 2 == dimx);
    ncselector_destroy(ncs, nullptr);
  }

  SUBCASE("PopulatedSelector") {
    selector_item items[] = {
      { strdup("op1"), strdup("this is option 1"), },
      { strdup("2ndop"), strdup("this is option #2"), },
      { strdup("tres"), strdup("option the third"), },
    };
    struct selector_options opts{};
    opts.items = items;
    opts.itemcount = sizeof(items) / sizeof(*items);
    struct ncselector* ncs = ncselector_create(notcurses_stdplane(nc_), 0, 0, &opts);
    REQUIRE(nullptr != ncs);
    CHECK(0 == notcurses_render(nc_));
    struct ncplane* ncsp = ncselector_plane(ncs);
    REQUIRE(nullptr != ncsp);
    int dimy, dimx;
    ncplane_dim_yx(ncsp, &dimy, &dimx);
    CHECK(7 == dimy);
    CHECK(15 < dimx);
    ncselector_destroy(ncs, nullptr);
  }

  SUBCASE("EmptySelectorMovement") {
    struct selector_options opts{};
    struct ncselector* ncs = ncselector_create(notcurses_stdplane(nc_), 0, 0, &opts);
    REQUIRE(nullptr != ncs);
    CHECK(0 == notcurses_render(nc_));
    auto sel = ncselector_selected(ncs);
    REQUIRE(nullptr == sel);
    ncselector_nextitem(ncs, &sel);
    REQUIRE(nullptr == sel);
    CHECK(0 == notcurses_render(nc_));
    ncselector_previtem(ncs, &sel);
    REQUIRE(nullptr == sel);
    CHECK(0 == notcurses_render(nc_));
    ncselector_destroy(ncs, nullptr);
  }

  SUBCASE("SelectorMovement") {
    selector_item items[] = {
      { strdup("op1"), strdup("this is option 1"), },
      { strdup("2ndop"), strdup("this is option #2"), },
      { strdup("tres"), strdup("option the third"), },
    };
    struct selector_options opts{};
    opts.items = items;
    opts.itemcount = sizeof(items) / sizeof(*items);
    struct ncselector* ncs = ncselector_create(notcurses_stdplane(nc_), 0, 0, &opts);
    REQUIRE(nullptr != ncs);
    auto sel = ncselector_selected(ncs);
    REQUIRE(nullptr != sel);
    CHECK(0 == strcmp(sel, items[0].option));
    free(sel);
    CHECK(0 == notcurses_render(nc_));
    ncselector_nextitem(ncs, &sel);
    REQUIRE(nullptr != sel);
    CHECK(0 == strcmp(sel, items[1].option));
    free(sel);
    CHECK(0 == notcurses_render(nc_));
    ncselector_previtem(ncs, &sel);
    REQUIRE(nullptr != sel);
    CHECK(0 == strcmp(sel, items[0].option));
    free(sel);
    CHECK(0 == notcurses_render(nc_));
    // wrap around from the top to bottom...
    ncselector_previtem(ncs, &sel);
    REQUIRE(nullptr != sel);
    CHECK(0 == strcmp(sel, items[2].option));
    free(sel);
    CHECK(0 == notcurses_render(nc_));
    // ...and back to the top
    ncselector_nextitem(ncs, &sel);
    REQUIRE(nullptr != sel);
    CHECK(0 == strcmp(sel, items[0].option));
    free(sel);
    CHECK(0 == notcurses_render(nc_));
    ncselector_destroy(ncs, nullptr);
  }

  // Provide three items, limited to 1 shown at a time
  SUBCASE("ScrollingSelectorOne") {
    selector_item items[] = {
      { strdup("op1"), strdup("this is option 1"), },
      { strdup("2ndop"), strdup("this is option #2"), },
      { strdup("tres"), strdup("option the third"), },
    };
    struct selector_options opts{};
    opts.maxdisplay = 1;
    opts.items = items;
    opts.itemcount = sizeof(items) / sizeof(*items);
    struct ncselector* ncs = ncselector_create(notcurses_stdplane(nc_), 0, 0, &opts);
    REQUIRE(nullptr != ncs);
    CHECK(0 == notcurses_render(nc_));
    auto sel = ncselector_selected(ncs);
    REQUIRE(nullptr != sel);
    struct ncplane* ncsp = ncselector_plane(ncs);
    REQUIRE(nullptr != ncsp);
    int dimy, dimx;
    ncplane_dim_yx(ncsp, &dimy, &dimx);
    CHECK(5 == dimy);
    ncselector_destroy(ncs, nullptr);
  }

  // Provide three items, limited to 2 shown at a time
  SUBCASE("ScrollingSelectorTwo") {
    selector_item items[] = {
      { strdup("op1"), strdup("this is option 1"), },
      { strdup("2ndop"), strdup("this is option #2"), },
      { strdup("tres"), strdup("option the third"), },
    };
    struct selector_options opts{};
    opts.maxdisplay = 2;
    opts.items = items;
    opts.itemcount = sizeof(items) / sizeof(*items);
    struct ncselector* ncs = ncselector_create(notcurses_stdplane(nc_), 0, 0, &opts);
    REQUIRE(nullptr != ncs);
    CHECK(0 == notcurses_render(nc_));
    struct ncplane* ncsp = ncselector_plane(ncs);
    REQUIRE(nullptr != ncsp);
    int dimy, dimx;
    ncplane_dim_yx(ncsp, &dimy, &dimx);
    CHECK(6 == dimy);
    ncselector_destroy(ncs, nullptr);
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));
}
