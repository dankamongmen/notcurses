#include "main.h"
#include <cstring>
#include <iostream>

TEST_CASE("MenuTest") {
  if(!enforce_utf8()){
    return;
  }
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
    ncmenu_destroy(nc_, ncm);
  }

  SUBCASE("EmptyMenuBottomReject") {
    struct ncmenu_options opts{};
    opts.bottom = true;
    struct ncmenu* ncm = ncmenu_create(nc_, &opts);
    REQUIRE(nullptr == ncm);
    CHECK(0 == notcurses_render(nc_));
    ncmenu_destroy(nc_, ncm);
  }

  // an empty section ought be rejected
  SUBCASE("EmptySectionReject") {
    struct ncmenu_options opts{};
    struct ncmenu_section sections[] = {
      { .name = strdup("Empty"), .itemcount = 0, .items = nullptr, .shortcut = ncinput(), },
    };
    opts.sections = sections;
    opts.sectioncount = sizeof(sections) / sizeof(*sections);
    struct ncmenu* ncm = ncmenu_create(nc_, &opts);
    REQUIRE(nullptr == ncm);
    CHECK(0 == notcurses_render(nc_));
    ncmenu_destroy(nc_, ncm);
  }

  // a section with only separators ought be rejected
  SUBCASE("SeparatorSectionReject") {
    struct ncmenu_item empty_items[] = {
      { .desc = nullptr, .shortcut = ncinput(), },
    };
    struct ncmenu_section sections[] = {
      { .name = strdup("Empty"), .itemcount = 1, .items = empty_items, .shortcut = ncinput(), },
    };
    struct ncmenu_options opts{};
    opts.sections = sections;
    opts.sectioncount = sizeof(sections) / sizeof(*sections);
    struct ncmenu* ncm = ncmenu_create(nc_, &opts);
    REQUIRE(nullptr == ncm);
    CHECK(0 == notcurses_render(nc_));
    ncmenu_destroy(nc_, ncm);
  }

  SUBCASE("MenuOneSection") {
    struct ncmenu_item file_items[] = {
      { .desc = strdup("I would like a new file"), .shortcut = ncinput(), },
    };
    struct ncmenu_section sections[] = {
      { .name = strdup("File"), .itemcount = sizeof(file_items) / sizeof(*file_items), .items = file_items, .shortcut = ncinput(), },
    };
    struct ncmenu_options opts{};
    opts.sections = sections;
    opts.sectioncount = sizeof(sections) / sizeof(*sections);
    struct ncmenu* ncm = ncmenu_create(nc_, &opts);
    REQUIRE(nullptr != ncm);
    CHECK(0 == notcurses_render(nc_));
    ncmenu_destroy(nc_, ncm);
  }

  // only one menu at a time per notcurses object
  SUBCASE("RejectDoubleMenu") {
    struct ncmenu_item file_items[] = {
      { .desc = strdup("I would like a new file"), .shortcut = ncinput(), },
    };
    struct ncmenu_section sections[] = {
      { .name = strdup("File"), .itemcount = sizeof(file_items) / sizeof(*file_items), .items = file_items, .shortcut = ncinput(), },
    };
    struct ncmenu_options opts{};
    opts.sections = sections;
    opts.sectioncount = sizeof(sections) / sizeof(*sections);
    struct ncmenu* ncm = ncmenu_create(nc_, &opts);
    REQUIRE(nullptr != ncm);
    struct ncmenu* ncmdup = ncmenu_create(nc_, &opts);
    REQUIRE(nullptr == ncmdup);
    opts.bottom = true;
    ncmdup = ncmenu_create(nc_, &opts);
    REQUIRE(nullptr == ncmdup);
    CHECK(0 == notcurses_render(nc_));
    ncmenu_destroy(nc_, ncm);
  }

  // don't call ncmenu_destroy(), invoking destruction in notcurses_stop()
  SUBCASE("MenuNoFree") {
    struct ncmenu_item file_items[] = {
      { .desc = strdup("I would like a new file"), .shortcut = ncinput(), },
    };
    struct ncmenu_section sections[] = {
      { .name = strdup("File"), .itemcount = sizeof(file_items) / sizeof(*file_items), .items = file_items, .shortcut = ncinput(), },
    };
    struct ncmenu_options opts{};
    opts.sections = sections;
    opts.sectioncount = sizeof(sections) / sizeof(*sections);
    struct ncmenu* ncm = ncmenu_create(nc_, &opts);
    REQUIRE(nullptr != ncm);
    CHECK(0 == notcurses_render(nc_));
  }

  SUBCASE("RightAlignedSection") {
    struct ncmenu_item items[] = {
      { .desc = strdup("Yet another crappy item"), .shortcut = {}, },
    };
    struct ncmenu_section sections[] = {
      { .name = strdup("Left section"), .itemcount = sizeof(items) / sizeof(*items),
        .items = items, .shortcut = {}, },
      { .name = nullptr, .itemcount = sizeof(items) / sizeof(*items),
        .items = items, .shortcut = {}, },
      { .name = strdup("Right section"), .itemcount = sizeof(items) / sizeof(*items),
        .items = items, .shortcut = {}, },
    };
    struct ncmenu_options opts{};
    opts.sections = sections;
    opts.sectioncount = sizeof(sections) / sizeof(*sections);
    struct ncmenu* ncm = ncmenu_create(nc_, &opts);
    REQUIRE(nullptr != ncm);
    CHECK(0 == notcurses_render(nc_));
  }

  // you must have sections, not just an alignment NULL section
  SUBCASE("OnlyAlignRejected") {
    struct ncmenu_section sections[] = {
      { .name = nullptr, .itemcount = 0, .items = nullptr, .shortcut = {}, },
    };
    struct ncmenu_options opts{};
    opts.sections = sections;
    opts.sectioncount = sizeof(sections) / sizeof(*sections);
    struct ncmenu* ncm = ncmenu_create(nc_, &opts);
    REQUIRE(nullptr == ncm);
  }

  // you can only shift to right alignment once in a menu
  SUBCASE("DoubleAlignRejected") {
    struct ncmenu_item items[] = {
      { .desc = strdup("Yet another crappy item"), .shortcut = {}, },
    };
    struct ncmenu_section sections[] = {
      { .name = strdup("Left section"), .itemcount = sizeof(items) / sizeof(*items),
        .items = items, .shortcut = {}, },
      { .name = nullptr, .itemcount = sizeof(items) / sizeof(*items),
        .items = items, .shortcut = {}, },
      { .name = nullptr, .itemcount = sizeof(items) / sizeof(*items),
        .items = items, .shortcut = {}, },
      { .name = strdup("Right section"), .itemcount = sizeof(items) / sizeof(*items),
        .items = items, .shortcut = {}, },
    };
    struct ncmenu_options opts{};
    opts.sections = sections;
    opts.sectioncount = sizeof(sections) / sizeof(*sections);
    struct ncmenu* ncm = ncmenu_create(nc_, &opts);
    REQUIRE(nullptr == ncm);
  }

  SUBCASE("VeryLongMenu") {
    struct ncmenu_item items[] = {
      { .desc = strdup("Generic menu entry"), .shortcut = ncinput(), },
    };
    struct ncmenu_section sections[] = {
      { .name = strdup("antidisestablishmentarianism"), .itemcount = sizeof(items) / sizeof(*items), .items = items, .shortcut = ncinput(), },
      { .name = strdup("floccinaucinihilipilification"), .itemcount = sizeof(items) / sizeof(*items), .items = items, .shortcut = ncinput(), },
      { .name = strdup("pneumonoultramicroscopicsilicovolcanoconiosis"), .itemcount = sizeof(items) / sizeof(*items), .items = items, .shortcut = ncinput(), },
      { .name = strdup("supercalifragilisticexpialidocious"), .itemcount = sizeof(items) / sizeof(*items), .items = items, .shortcut = ncinput(), },
      { .name = strdup("Incomprehensibilities"), .itemcount = sizeof(items) / sizeof(*items), .items = items, .shortcut = ncinput(), },
    };
    struct ncmenu_options opts{};
    opts.sections = sections;
    opts.sectioncount = sizeof(sections) / sizeof(*sections);
    struct ncmenu* ncm = ncmenu_create(nc_, &opts);
    REQUIRE(nullptr != ncm);
    CHECK(0 == notcurses_render(nc_));
    ncmenu_destroy(nc_, ncm);
  }

  CHECK(0 == notcurses_stop(nc_));
  CHECK(0 == fclose(outfp_));
}
