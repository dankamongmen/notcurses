#include "main.h"
#include <cstring>
#include <iostream>

TEST_CASE("MenuTest") {
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

  // an empty menu ought be rejected
  SUBCASE("EmptyMenuTopReject") {
    struct ncmenu_options opts{};
    struct ncmenu* ncm = ncmenu_create(nc_, &opts);
    REQUIRE(nullptr == ncm);
    CHECK(0 == notcurses_render(nc_));
    ncmenu_destroy(ncm);
  }

  SUBCASE("EmptyMenuBottomReject") {
    struct ncmenu_options opts{};
    opts.bottom = true;
    struct ncmenu* ncm = ncmenu_create(nc_, &opts);
    REQUIRE(nullptr == ncm);
    CHECK(0 == notcurses_render(nc_));
    ncmenu_destroy(ncm);
  }

  // an empty section ought be rejected
  SUBCASE("EmptySectionReject") {
    struct ncmenu_options opts{};
    struct ncmenu_section sections[] = {
      { .name = strdup("Empty"), .itemcount = 0, .items = nullptr,
        .xoff = -1, .bodycols = -1, .itemselected = -1, },
    };
    opts.sections = sections;
    opts.sectioncount = sizeof(sections) / sizeof(*sections);
    struct ncmenu* ncm = ncmenu_create(nc_, &opts);
    REQUIRE(nullptr == ncm);
    CHECK(0 == notcurses_render(nc_));
    ncmenu_destroy(ncm);
  }

  // a section with only separators ought be rejected
  SUBCASE("SeparatorSectionReject") {
    struct ncmenu_item empty_items[] = {
      { .desc = nullptr, .shortcut = {}, },
    };
    struct ncmenu_section sections[] = {
      { .name = strdup("Empty"), .itemcount = 1, .items = empty_items,
        .xoff = -1, .bodycols = -1, .itemselected = -1, },
    };
    struct ncmenu_options opts{};
    opts.sections = sections;
    opts.sectioncount = sizeof(sections) / sizeof(*sections);
    struct ncmenu* ncm = ncmenu_create(nc_, &opts);
    REQUIRE(nullptr == ncm);
    CHECK(0 == notcurses_render(nc_));
    ncmenu_destroy(ncm);
  }

  SUBCASE("MenuOneSection") {
    struct ncmenu_item file_items[] = {
      { .desc = strdup("I would like a new file"), .shortcut = {}, },
    };
    struct ncmenu_section sections[] = {
      { .name = strdup("File"), .itemcount = sizeof(file_items) / sizeof(*file_items), .items = file_items,
        .xoff = -1, .bodycols = -1, .itemselected = -1, },
    };
    struct ncmenu_options opts{};
    opts.sections = sections;
    opts.sectioncount = sizeof(sections) / sizeof(*sections);
    struct ncmenu* ncm = ncmenu_create(nc_, &opts);
    REQUIRE(nullptr != ncm);
    CHECK(0 == notcurses_render(nc_));
    ncmenu_destroy(ncm);
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));
}
