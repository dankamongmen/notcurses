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
    ncselector_destroy(ncs, nullptr);
  }

  SUBCASE("TitledSelector") {
    struct selector_options opts{};
    opts.title = strdup("hey hey whaddya say");
    struct ncselector* ncs = ncselector_create(notcurses_stdplane(nc_), 0, 0, &opts);
    REQUIRE(nullptr != ncs);
    CHECK(0 == notcurses_render(nc_));
    ncselector_destroy(ncs, nullptr);
  }

  SUBCASE("SecondarySelector") {
    struct selector_options opts{};
    opts.secondary = strdup("this is not a title, but it's not *not* a title");
    struct ncselector* ncs = ncselector_create(notcurses_stdplane(nc_), 0, 0, &opts);
    REQUIRE(nullptr != ncs);
    CHECK(0 == notcurses_render(nc_));
    ncselector_destroy(ncs, nullptr);
  }

  SUBCASE("FooterSelector") {
    struct selector_options opts{};
    opts.secondary = strdup("i am a lone footer, little old footer");
    struct ncselector* ncs = ncselector_create(notcurses_stdplane(nc_), 0, 0, &opts);
    REQUIRE(nullptr != ncs);
    CHECK(0 == notcurses_render(nc_));
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
    ncselector_destroy(ncs, nullptr);
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));
}
