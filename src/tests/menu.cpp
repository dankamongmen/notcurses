#include "main.h"
#include <cstring>
#include <iostream>

TEST_CASE("Menu") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);
  REQUIRE(0 == ncplane_cursor_move_yx(n_, 0, 0));

  // an empty menu ought be rejected
  SUBCASE("EmptyMenuTopReject") {
    struct ncmenu_options opts{};
    struct ncmenu* ncm = ncmenu_create(n_, &opts);
    REQUIRE(nullptr == ncm);
    CHECK(0 == notcurses_render(nc_));
    ncmenu_destroy(ncm);
  }

  SUBCASE("EmptyMenuBottomReject") {
    struct ncmenu_options opts{};
    opts.flags = NCMENU_OPTION_BOTTOM;
    struct ncmenu* ncm = ncmenu_create(n_, &opts);
    REQUIRE(nullptr == ncm);
    CHECK(0 == notcurses_render(nc_));
    ncmenu_destroy(ncm);
  }

  // an empty section ought be rejected
  SUBCASE("EmptySectionReject") {
    struct ncmenu_options opts{};
    struct ncmenu_section sections[] = {
      { .name = "Empty", .itemcount = 0, .items = nullptr, .shortcut = ncinput(), },
    };
    opts.sections = sections;
    opts.sectioncount = sizeof(sections) / sizeof(*sections);
    struct ncmenu* ncm = ncmenu_create(n_, &opts);
    REQUIRE(nullptr == ncm);
    CHECK(0 == notcurses_render(nc_));
    ncmenu_destroy(ncm);
  }

  // a section with only separators ought be rejected
  SUBCASE("SeparatorSectionReject") {
    struct ncmenu_item empty_items[] = {
      { .desc = nullptr, .shortcut = ncinput(), },
    };
    struct ncmenu_section sections[] = {
      { .name = "Empty", .itemcount = 1, .items = empty_items, .shortcut = ncinput(), },
    };
    struct ncmenu_options opts{};
    opts.sections = sections;
    opts.sectioncount = sizeof(sections) / sizeof(*sections);
    struct ncmenu* ncm = ncmenu_create(n_, &opts);
    REQUIRE(nullptr == ncm);
    CHECK(0 == notcurses_render(nc_));
    ncmenu_destroy(ncm);
  }

  SUBCASE("MenuOneSection") {
    struct ncmenu_item file_items[] = {
      { .desc = "I would like a new file", .shortcut = ncinput(), },
    };
    struct ncmenu_section sections[] = {
      { .name = "File", .itemcount = sizeof(file_items) / sizeof(*file_items), .items = file_items, .shortcut = ncinput(), },
    };
    struct ncmenu_options opts{};
    opts.sections = sections;
    opts.sectioncount = sizeof(sections) / sizeof(*sections);
    struct ncmenu* ncm = ncmenu_create(n_, &opts);
    REQUIRE(nullptr != ncm);
    CHECK(0 == notcurses_render(nc_));
    ncmenu_destroy(ncm);
  }

  // don't call ncmenu_destroy(), invoking destruction in notcurses_stop().
  SUBCASE("MenuNoFree") {
    struct ncmenu_item file_items[] = {
      { .desc = "I would like a new file", .shortcut = ncinput(), },
    };
    struct ncmenu_section sections[] = {
      { .name = "File", .itemcount = sizeof(file_items) / sizeof(*file_items), .items = file_items, .shortcut = ncinput(), },
    };
    struct ncmenu_options opts{};
    opts.sections = sections;
    opts.sectioncount = sizeof(sections) / sizeof(*sections);
    struct ncmenu* ncm = ncmenu_create(n_, &opts);
    REQUIRE(nullptr != ncm);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("RightAlignedSection") {
    struct ncmenu_item items[] = {
      { .desc = "Yet another crappy item", .shortcut = {}, },
    };
    struct ncmenu_section sections[] = {
      { .name = "Left section", .itemcount = sizeof(items) / sizeof(*items),
        .items = items, .shortcut = {}, },
      { .name = nullptr, .itemcount = sizeof(items) / sizeof(*items),
        .items = items, .shortcut = {}, },
      { .name = "Right section", .itemcount = sizeof(items) / sizeof(*items),
        .items = items, .shortcut = {}, },
    };
    struct ncmenu_options opts{};
    opts.sections = sections;
    opts.sectioncount = sizeof(sections) / sizeof(*sections);
    struct ncmenu* ncm = ncmenu_create(n_, &opts);
    REQUIRE(nullptr != ncm);
    CHECK(0 == notcurses_render(nc_));
    ncmenu_destroy(ncm);
  }

  // you must have sections, not just an alignment NULL section
  SUBCASE("OnlyAlignRejected") {
    struct ncmenu_section sections[] = {
      { .name = nullptr, .itemcount = 0, .items = nullptr, .shortcut = {}, },
    };
    struct ncmenu_options opts{};
    opts.sections = sections;
    opts.sectioncount = sizeof(sections) / sizeof(*sections);
    struct ncmenu* ncm = ncmenu_create(n_, &opts);
    REQUIRE(nullptr == ncm);
  }

  // you can only shift to right alignment once in a menu
  SUBCASE("DoubleAlignRejected") {
    struct ncmenu_item items[] = {
      { .desc = "Yet another crappy item", .shortcut = {}, },
    };
    struct ncmenu_section sections[] = {
      { .name = "Left section", .itemcount = sizeof(items) / sizeof(*items),
        .items = items, .shortcut = {}, },
      { .name = nullptr, .itemcount = sizeof(items) / sizeof(*items),
        .items = items, .shortcut = {}, },
      { .name = nullptr, .itemcount = sizeof(items) / sizeof(*items),
        .items = items, .shortcut = {}, },
      { .name = "Right section", .itemcount = sizeof(items) / sizeof(*items),
        .items = items, .shortcut = {}, },
    };
    struct ncmenu_options opts{};
    opts.sections = sections;
    opts.sectioncount = sizeof(sections) / sizeof(*sections);
    struct ncmenu* ncm = ncmenu_create(n_, &opts);
    REQUIRE(nullptr == ncm);
  }

  SUBCASE("VeryLongMenu") {
    struct ncmenu_item items[] = {
      { .desc = "Generic menu entry", .shortcut = ncinput(), },
    };
    struct ncmenu_section sections[] = {
      { .name = "antidisestablishmentarianism", .itemcount = sizeof(items) / sizeof(*items), .items = items, .shortcut = ncinput(), },
      { .name = "floccinaucinihilipilification", .itemcount = sizeof(items) / sizeof(*items), .items = items, .shortcut = ncinput(), },
      { .name = "pneumonoultramicroscopicsilicovolcanoconiosis", .itemcount = sizeof(items) / sizeof(*items), .items = items, .shortcut = ncinput(), },
      { .name = "supercalifragilisticexpialidocious", .itemcount = sizeof(items) / sizeof(*items), .items = items, .shortcut = ncinput(), },
      { .name = "Incomprehensibilities", .itemcount = sizeof(items) / sizeof(*items), .items = items, .shortcut = ncinput(), },
    };
    struct ncmenu_options opts{};
    opts.sections = sections;
    opts.sectioncount = sizeof(sections) / sizeof(*sections);
    struct ncmenu* ncm = ncmenu_create(n_, &opts);
    REQUIRE(nullptr != ncm);
    CHECK(0 == notcurses_render(nc_));
    ncmenu_destroy(ncm);
  }

  CHECK(0 == notcurses_stop(nc_));
}
