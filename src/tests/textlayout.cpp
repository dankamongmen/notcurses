#include "main.h"

TEST_CASE("TextLayout") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  const char str[] = "this is going to be broken up";

  SUBCASE("LayoutUnaligned") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 2,
      .cols = 20,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto sp = ncplane_create(n_, &nopts);
    REQUIRE(sp);
    size_t bytes;
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_UNALIGNED, str, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(str));
    char* line = ncplane_contents(sp, 0, 0, 2, 20);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "this is going to be broken up"));
    free(line);
    CHECK(0 == ncplane_destroy(sp));
  }

  SUBCASE("LayoutLeft") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 2,
      .cols = 20,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto sp = ncplane_create(n_, &nopts);
    REQUIRE(sp);
    size_t bytes;
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, str, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(str));
    char* line = ncplane_contents(sp, 0, 0, 2, 20);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "this is going to be broken up"));
    free(line);
    CHECK(0 == ncplane_destroy(sp));
  }

  SUBCASE("LayoutRight") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 2,
      .cols = 20,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto sp = ncplane_create(n_, &nopts);
    REQUIRE(sp);
    size_t bytes;
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_RIGHT, str, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(str));
    char* line = ncplane_contents(sp, 0, 0, 2, 20);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "this is going to be broken up"));
    free(line);
    CHECK(0 == ncplane_destroy(sp));
  }

  SUBCASE("LayoutCenter") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 2,
      .cols = 20,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto sp = ncplane_create(n_, &nopts);
    REQUIRE(sp);
    size_t bytes;
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_CENTER, str, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(str));
    char* line = ncplane_contents(sp, 0, 0, 2, 20);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "this is going to be broken up"));
    free(line);
    CHECK(0 == ncplane_destroy(sp));
  }

  // lay out text where a word ends on the boundary
  SUBCASE("LayoutOnBoundary") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 3,
      .cols = 10,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto sp = ncplane_create(n_, &nopts);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "my nuclear arms";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_CENTER, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, 0, 0);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "my nuclear arms"));
    free(line);
    CHECK(0 == ncplane_destroy(sp));
  }

  // lay out text where a word crosses the boundary
  SUBCASE("LayoutCrossBoundary") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 3,
      .cols = 10,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto sp = ncplane_create(n_, &nopts);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "my grasping arms";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_CENTER, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, 0, 0);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "my grasping arms"));
    free(line);
    CHECK(0 == ncplane_destroy(sp));
  }

  // ensure we're honoring newlines
  SUBCASE("LayoutNewlines") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 5,
      .cols = 5,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto sp = ncplane_create(n_, &nopts);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "a\nb\nc\nd\ne";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, 0, 0);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "abcde")); // FIXME should have newlines
    free(line);
    CHECK(0 == ncplane_destroy(sp));
  }

  // ensure we're honoring newlines at the start/end of rows
  SUBCASE("LayoutNewlinesAtBorders") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 5,
      .cols = 3,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto sp = ncplane_create(n_, &nopts);
    REQUIRE(sp);
    const char boundstr[] = "ab\ncde\nfgh";
    size_t bytes;
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, 0, 0);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "abcdefgh"));
    free(line);
    CHECK(0 == ncplane_destroy(sp));
  }

  // lay out text where a wide word crosses the boundary
  SUBCASE("LayoutCrossBoundaryWide") {
    if(notcurses_canutf8(nc_)){
      struct ncplane_options nopts = {
        .y = 0,
        .x = 0,
        .rows = 2,
        .cols = 7,
        .userptr = nullptr,
        .name = nullptr,
        .resizecb = nullptr,
        .flags = 0,
        .margin_b = 0, .margin_r = 0,
      };
      auto sp = ncplane_create(n_, &nopts);
      REQUIRE(sp);
      size_t bytes;
      const char boundstr[] = "a 血的神";
      CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_CENTER, boundstr, &bytes));
      CHECK(0 == notcurses_render(nc_));
      CHECK(bytes == strlen(boundstr));
      char* line = ncplane_contents(sp, 0, 0, 0, 0);
      REQUIRE(line);
      CHECK(0 == strcmp(line, boundstr));
      free(line);
      CHECK(0 == ncplane_destroy(sp));
    }
  }

  // a long word (one requiring a split no matter what) ought not force the
  // next line, but instead be printed where it starts
  SUBCASE("LayoutTransPlanar") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 3,
      .cols = 10,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto sp = ncplane_create(n_, &nopts);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "my thermonuclear arms";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_CENTER, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, 0, 0);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "my thermonuclear arms"));
    free(line);
    CHECK(0 == ncplane_destroy(sp));
  }

  // a long word (one requiring a split no matter what) ought not force the
  // next line, but instead be printed where it starts
  SUBCASE("LayoutTransPlanarWide") {
    if(notcurses_canutf8(nc_)){
      struct ncplane_options nopts = {
        .y = 0,
        .x = 0,
        .rows = 3,
        .cols = 10,
        .userptr = nullptr,
        .name = nullptr,
        .resizecb = nullptr,
        .flags = 0,
        .margin_b = 0, .margin_r = 0,
      };
      auto sp = ncplane_create(n_, &nopts);
      REQUIRE(sp);
      size_t bytes;
      const char boundstr[] = "1 我能吞下玻璃";
      CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_CENTER, boundstr, &bytes));
      CHECK(0 == notcurses_render(nc_));
      CHECK(bytes == strlen(boundstr));
      char* line = ncplane_contents(sp, 0, 0, 0, 0);
      REQUIRE(line);
      CHECK(0 == strcmp(line, "1 我能吞下玻璃"));
      free(line);
      CHECK(0 == ncplane_destroy(sp));
    }
  }

  SUBCASE("LayoutLeadingSpaces") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 3,
      .cols = 18,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto sp = ncplane_create(n_, &nopts);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "   \n my thermonuclear arms";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_CENTER, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, 0, 0);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "    my thermonuclear arms"));
    free(line);
    CHECK(0 == ncplane_destroy(sp));
  }

  // create a plane of two rows, and fill exactly one with one word
  SUBCASE("LayoutFills1DPlane") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 2,
      .cols = 15,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto sp = ncplane_create(n_, &nopts);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "quarkgluonfart ";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, 0, 0);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "quarkgluonfart "));
    free(line);
    CHECK(0 == ncplane_destroy(sp));
  }

  // create a plane of two rows, and fill exactly one with words
  SUBCASE("LayoutFills1DPlaneWords") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 2,
      .cols = 17,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto sp = ncplane_create(n_, &nopts);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "quark gluon fart ";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, 0, 0);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "quark gluon fart "));
    free(line);
    CHECK(0 == ncplane_destroy(sp));
  }

  // create a plane of two rows, and exactly fill the first line
  SUBCASE("LayoutFillsSingleLine") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 2,
      .cols = 13,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto sp = ncplane_create(n_, &nopts);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "quantum balls";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, 0, 0);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "quantum balls"));
    free(line);
    CHECK(0 == ncplane_destroy(sp));
  }

  // create a plane of three rows, and exactly fill two with regular ol' words
  SUBCASE("LayoutFillsPlane") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 3,
      .cols = 14,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto sp = ncplane_create(n_, &nopts);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "quantum balls scratchy no?! ";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, 0, 0);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "quantum balls scratchy no?! "));
    free(line);
    CHECK(0 == ncplane_destroy(sp));
  }

  // create a plane of three rows, and exactly fill two, with no spaces
  SUBCASE("LayoutFillsPlaneNoSpaces") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 3,
      .cols = 6,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto sp = ncplane_create(n_, &nopts);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "0123456789AB";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, 0, 0);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "0123456789AB"));
    free(line);
    CHECK(0 == ncplane_destroy(sp));
  }

  // create a plane of three rows, and exactly fill two with wide chars
  SUBCASE("LayoutFillsPlaneWide") {
    if(notcurses_canutf8(nc_)){
      struct ncplane_options nopts = {
        .y = 0,
        .x = 0,
        .rows = 3,
        .cols = 7,
        .userptr = nullptr,
        .name = nullptr,
        .resizecb = nullptr,
        .flags = 0,
        .margin_b = 0, .margin_r = 0,
      };
      auto sp = ncplane_create(n_, &nopts);
      REQUIRE(sp);
      size_t bytes;
      const char boundstr[] = "我能吞 下玻璃 ";
      CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, boundstr, &bytes));
      CHECK(0 == notcurses_render(nc_));
      CHECK(bytes == strlen(boundstr));
      char* line = ncplane_contents(sp, 0, 0, 0, 0);
      REQUIRE(line);
      CHECK(0 == strcmp(line, "我能吞 下玻璃 "));
      free(line);
      CHECK(0 == ncplane_destroy(sp));
    }
  }

  // if we don't have scrolling enabled, puttext() with more text than will
  // fit on the plane ought return error, but print what it can.
  SUBCASE("LayoutLongNoScroll") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 2,
      .cols = 14,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto sp = ncplane_create(n_, &nopts);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "quantum balls scratchy no?! truly! arrrrp";
    int res = ncplane_puttext(sp, 0, NCALIGN_LEFT, boundstr, &bytes);
    CHECK(0 > res);
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes < strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, 0, 0);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "quantum balls scratchy no?! "));
    free(line);
    CHECK(0 == ncplane_destroy(sp));
  }

  SUBCASE("LayoutLongScroll") {
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = 2,
      .cols = 13,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto sp = ncplane_create(n_, &nopts);
    REQUIRE(sp);
    ncplane_set_scrolling(sp, true);
    size_t bytes;
    const char boundstr[] = "quantum balls scratchy?! true! arrrrp";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, 0, 0);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "scratchy?! true! arrrrp"));
    free(line);
    CHECK(0 == ncplane_destroy(sp));
  }

  SUBCASE("LayoutLongLines") {
    // straight from the zoo demo, which didn't work at first
    const int READER_COLS = 71; // equal to longest line
    const int READER_ROWS = 4;
    const char text[] =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin ornare "
      "neque ac ipsum viverra, vestibulum hendrerit leo consequat. Integer "
      "velit, pharetra sed nisl quis, porttitor ornare purus. Cras ac "
      "sollicitudin dolor, eget elementum dolor. Quisque lobortis sagittis.";
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = READER_ROWS,
      .cols = READER_COLS,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto sp = ncplane_create(n_, &nopts);
    REQUIRE(sp);
    size_t bytes;
    ncplane_home(sp);
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, text, &bytes));
    CHECK(bytes == strlen(text));
    CHECK(0 == notcurses_render(nc_));
    char* line = ncplane_contents(sp, 0, 0, 0, 0);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Proin ornare neque ac ipsum viverra, vestibulum hendrerit leo consequat. Integer velit, pharetra sed nisl quis, porttitor ornare purus. Cras ac sollicitudin dolor, eget elementum dolor. Quisque lobortis sagittis."));
    free(line);
    CHECK(0 == ncplane_destroy(sp));
  }

  SUBCASE("LayoutZooText") {
    // straight from the zoo demo, which didn't work at first
    const int READER_COLS = 64;
    const int READER_ROWS = 8;
    const char text[] =
      "Notcurses provides several widgets to quickly build vivid TUIs.\n\n"
      "This NCReader widget facilitates free-form text entry complete with readline-style bindings. " "NCSelector allows a single option to be selected from a list. " "NCMultiselector allows 0..n options to be selected from a list of n items. "
      "NCFdplane streams a file descriptor, while NCSubproc spawns a subprocess and streams its output. "
      "A variety of plots are supported, and menus can be placed along the top and/or bottom of any plane.\n\n"
      "Widgets can be controlled with the keyboard and/or mouse. They are implemented atop ncplanes, and these planes can be manipulated like all others.";
    struct ncplane_options nopts{};
    nopts.rows = READER_ROWS;
    nopts.cols = READER_COLS;
    nopts.flags = NCPLANE_OPTION_VSCROLL;
    auto sp = ncplane_create(n_, &nopts);
    REQUIRE(sp);
    size_t bytes;
    ncplane_home(sp);
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, text, &bytes));
    CHECK(bytes == strlen(text));
    CHECK(0 == notcurses_render(nc_));
    char* line = ncplane_contents(sp, 0, 0, 0, 0);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "to be selected from a list of n items. NCFdplane streams a file descriptor, while NCSubproc spawns a subprocess and streams its output. A variety of plots are supported, and menus can be placed along the top and/or bottom of any plane.\nWidgets can be controlled with the keyboard and/or mouse. They are implemented atop ncplanes, and these planes can be manipulated like all others."));
    free(line);
    CHECK(0 == ncplane_destroy(sp));
  }

  SUBCASE("LayoutZooTextNoScroll") {
    // straight from the zoo demo, which didn't work at first
    const int READER_COLS = 64;
    const int READER_ROWS = 15;
    const char text[] =
      "Notcurses provides several widgets to quickly build vivid TUIs.\n\n"
      "This NCReader widget facilitates free-form text entry complete with readline-style bindings. "
      "NCSelector allows a single option to be selected from a list. "
      "NCMultiselector allows 0..n options to be selected from a list of n items. "
      "NCFdplane streams a file descriptor, while NCSubproc spawns a subprocess and streams its output. "
      "A variety of plots are supported, and menus can be placed along the top and/or bottom of any plane.\n\n"
      "Widgets can be controlled with the keyboard and/or mouse. They are implemented atop ncplanes, and these planes can be manipulated like all others.";
    struct ncplane_options nopts = {
      .y = 0,
      .x = 0,
      .rows = READER_ROWS,
      .cols = READER_COLS,
      .userptr = nullptr,
      .name = nullptr,
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto sp = ncplane_create(n_, &nopts);
    REQUIRE(sp);
    size_t bytes;
    ncplane_home(sp);
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, text, &bytes));
    CHECK(bytes == strlen(text));
    CHECK(0 == notcurses_render(nc_));
    char* line = ncplane_contents(sp, 0, 0, 0, 0);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "Notcurses provides several widgets to quickly build vivid TUIs."
                            "This NCReader widget facilitates free-form text entry complete "
                            "with readline-style bindings. NCSelector allows a single option "
                            "to be selected from a list. NCMultiselector allows 0..n options "
                            "to be selected from a list of n items. NCFdplane streams a file "
                            "descriptor, while NCSubproc spawns a subprocess and streams its "
                            "output. A variety of plots are supported, and menus can be placed "
                            "along the top and/or bottom of any plane.Widgets can be controlled "
                            "with the keyboard and/or mouse. They are implemented atop ncplanes, "
                            "and these planes can be manipulated like all others."));
    free(line);
    CHECK(0 == ncplane_destroy(sp));
  }

  // there shouldn't be any leftover material from lines scrolled off the plane
  SUBCASE("ScrollClearsEnd") {
    const char text[] = "1234\n12\n54";
    struct ncplane_options nopts{};
    nopts.rows = 2;
    nopts.cols = 5;
    auto sp = ncplane_create(n_, &nopts);
    REQUIRE(sp);
    ncplane_set_scrolling(sp, true);
    size_t bytes;
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, text, &bytes));
    CHECK(bytes == strlen(text));
    CHECK(0 == notcurses_render(nc_));
    char* line = ncplane_contents(sp, 0, 0, 0, 0);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "1254"));
    free(line);
    CHECK(0 == ncplane_destroy(sp));
  }

  // test that multiple new lines are treated as such, both on the plane
  // originally, and in any autogrown region.
  SUBCASE("MultipleNewlines") {
    struct ncplane_options nopts{};
    nopts.rows = 3;
    nopts.cols = 10;
    nopts.flags = NCPLANE_OPTION_VSCROLL | NCPLANE_OPTION_AUTOGROW;
    auto nn = ncplane_create(n_, &nopts);
    REQUIRE(nn);
    size_t b;
    CHECK(0 == ncplane_puttext(nn, -1, NCALIGN_LEFT, "\n\n", &b));
    unsigned y, x;
    ncplane_cursor_yx(nn, &y, &x);
    CHECK(2 == y);
    CHECK(0 == x);
    CHECK(3 == ncplane_puttext(nn, -1, NCALIGN_LEFT, "erp", &b));
    ncplane_cursor_yx(nn, &y, &x);
    CHECK(2 == y);
    CHECK(3 == x);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_puttext(nn, -1, NCALIGN_LEFT, "\n", &b));
    ncplane_cursor_yx(nn, &y, &x);
    CHECK(3 == y);
    CHECK(0 == x);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_puttext(nn, -1, NCALIGN_LEFT, "\n\n", &b));
    ncplane_cursor_yx(nn, &y, &x);
    CHECK(5 == y);
    CHECK(0 == x);
    CHECK(0 == notcurses_render(nc_));
    CHECK(3 == ncplane_puttext(nn, -1, NCALIGN_LEFT, "erp\n", &b));
    ncplane_cursor_yx(nn, &y, &x);
    CHECK(6 == y);
    CHECK(0 == x);
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(nn));
  }

  CHECK(0 == notcurses_stop(nc_));

}
