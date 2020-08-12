#include "main.h"
#include "internal.h"

TEST_CASE("TextLayout") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  ncplane* ncp_ = notcurses_stdplane(nc_);
  REQUIRE(ncp_);

  const char str[] = "this is going to be broken up";

  SUBCASE("LayoutLeft") {
    auto sp = ncplane_new(nc_, 2, 20, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, str, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(str));
    char* line = ncplane_contents(sp, 0, 0, 2, 20);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "this is going to be broken up"));
    free(line);
    ncplane_destroy(sp);
  }

  SUBCASE("LayoutRight") {
    auto sp = ncplane_new(nc_, 2, 20, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_RIGHT, str, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(str));
    char* line = ncplane_contents(sp, 0, 0, 2, 20);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "this is going to be broken up"));
    free(line);
    ncplane_destroy(sp);
  }

  SUBCASE("LayoutCenter") {
    auto sp = ncplane_new(nc_, 2, 20, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_CENTER, str, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(str));
    char* line = ncplane_contents(sp, 0, 0, 2, 20);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "this is going to be broken up"));
    free(line);
    ncplane_destroy(sp);
  }

  // lay out text where a word ends on the boundary
  SUBCASE("LayoutOnBoundary") {
    auto sp = ncplane_new(nc_, 2, 10, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "my nuclear arms";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_CENTER, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, -1, -1);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "my nucleararms"));
    free(line);
    ncplane_destroy(sp);
  }

  // lay out text where a word crosses the boundary
  SUBCASE("LayoutCrossBoundary") {
    auto sp = ncplane_new(nc_, 3, 10, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "my grasping arms";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_CENTER, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, -1, -1);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "mygraspingarms"));
    free(line);
    ncplane_destroy(sp);
  }

  // lay out text where a wide word crosses the boundary
  SUBCASE("LayoutCrossBoundaryWide") {
    if(enforce_utf8()){
      auto sp = ncplane_new(nc_, 2, 6, 0, 0, nullptr);
      REQUIRE(sp);
      size_t bytes;
      const char boundstr[] = "a 血的神";
      CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_CENTER, boundstr, &bytes));
      CHECK(0 == notcurses_render(nc_));
      CHECK(bytes == strlen(boundstr));
      char* line = ncplane_contents(sp, 0, 0, -1, -1);
      REQUIRE(line);
      CHECK(0 == strcmp(line, "a血的神"));
      free(line);
      ncplane_destroy(sp);
    }
  }

  // a long word (one requiring a split no matter what) ought not force the
  // next line, but instead be printed where it starts
  SUBCASE("LayoutTransPlanar") {
    auto sp = ncplane_new(nc_, 3, 10, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "my thermonuclear arms";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_CENTER, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, -1, -1);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "my thermonucleararms"));
    free(line);
    ncplane_destroy(sp);
  }

  // a long word (one requiring a split no matter what) ought not force the
  // next line, but instead be printed where it starts
  SUBCASE("LayoutTransPlanarWide") {
    if(enforce_utf8()){
      auto sp = ncplane_new(nc_, 2, 8, 0, 0, nullptr);
      REQUIRE(sp);
      size_t bytes;
      const char boundstr[] = "1 我能吞下玻璃";
      CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_CENTER, boundstr, &bytes));
      CHECK(0 == notcurses_render(nc_));
      CHECK(bytes == strlen(boundstr));
      char* line = ncplane_contents(sp, 0, 0, -1, -1);
      REQUIRE(line);
      CHECK(0 == strcmp(line, "1 我能吞下玻璃"));
      free(line);
      ncplane_destroy(sp);
    }
  }

  SUBCASE("LayoutLeadingSpaces") {
    auto sp = ncplane_new(nc_, 3, 10, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "  \t\n my thermonuclear arms";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_CENTER, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, -1, -1);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "my thermonucleararms"));
    free(line);
    ncplane_destroy(sp);
  }

  // create a plane of a single row, and fill it exactly with one word
  SUBCASE("LayoutFills1DPlane") {
    auto sp = ncplane_new(nc_, 1, 14, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "quarkgluonfart";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, -1, -1);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "quarkgluonfart"));
    free(line);
    ncplane_destroy(sp);
  }

  // create a plane of a single row, and fill it exactly with words
  SUBCASE("LayoutFills1DPlaneWords") {
    auto sp = ncplane_new(nc_, 1, 16, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "quark gluon fart";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, -1, -1);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "quark gluon fart"));
    free(line);
    ncplane_destroy(sp);
  }

  // create a plane of two rows, and exactly fill the first line
  SUBCASE("LayoutFillsSingleLine") {
    auto sp = ncplane_new(nc_, 2, 13, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "quantum balls";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, -1, -1);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "quantum balls"));
    free(line);
    ncplane_destroy(sp);
  }

  // create a plane of two rows, and exactly fill both
  SUBCASE("LayoutFillsPlane") {
    auto sp = ncplane_new(nc_, 2, 13, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "quantum balls scratchy no?!";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, -1, -1);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "quantum ballsscratchy no?!"));
    free(line);
    ncplane_destroy(sp);
  }

  // create a plane of two rows, and exactly fill both, with no spaces
  SUBCASE("LayoutFillsPlaneNoSpaces") {
    auto sp = ncplane_new(nc_, 2, 6, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "0123456789AB";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, -1, -1);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "0123456789AB"));
    free(line);
    ncplane_destroy(sp);
  }

  // create a plane of two rows, and exactly fill both with wide chars
  SUBCASE("LayoutFillsPlaneWide") {
    if(enforce_utf8()){
      auto sp = ncplane_new(nc_, 2, 6, 0, 0, nullptr);
      REQUIRE(sp);
      size_t bytes;
      const char boundstr[] = "我能吞 下玻璃";
      CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, boundstr, &bytes));
      CHECK(0 == notcurses_render(nc_));
      CHECK(bytes == strlen(boundstr));
      char* line = ncplane_contents(sp, 0, 0, -1, -1);
      REQUIRE(line);
      CHECK(0 == strcmp(line, "我能吞下玻璃"));
      free(line);
      ncplane_destroy(sp);
    }
  }

  SUBCASE("LayoutLongNoScroll") {
    auto sp = ncplane_new(nc_, 2, 13, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    const char boundstr[] = "quantum balls scratchy no?! truly! arrrrp";
    int res = ncplane_puttext(sp, 0, NCALIGN_LEFT, boundstr, &bytes);
    CHECK(0 > res);
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes < strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, -1, -1);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "quantum ballsscratchy no?!"));
    free(line);
    ncplane_destroy(sp);
  }

  SUBCASE("LayoutLongScroll") {
    auto sp = ncplane_new(nc_, 2, 13, 0, 0, nullptr);
    REQUIRE(sp);
    ncplane_set_scrolling(sp, true);
    size_t bytes;
    const char boundstr[] = "quantum balls scratchy?! true! arrrrp";
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, boundstr, &bytes));
    CHECK(0 == notcurses_render(nc_));
    CHECK(bytes == strlen(boundstr));
    char* line = ncplane_contents(sp, 0, 0, -1, -1);
    REQUIRE(line);
    CHECK(0 == strcmp(line, "scratchy?!true! arrrrp"));
    free(line);
    ncplane_destroy(sp);
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
    auto sp = ncplane_new(nc_, READER_ROWS, READER_COLS, 0, 0, nullptr);
    REQUIRE(sp);
    size_t bytes;
    ncplane_home(sp);
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, text, &bytes));
    CHECK(bytes == strlen(text));
    CHECK(0 == notcurses_render(nc_));
    char* line = ncplane_contents(sp, 0, 0, -1, -1);
    REQUIRE(line);
    // FIXME check line
    free(line);
    ncplane_destroy(sp);
  }

  SUBCASE("LayoutZooText") {
    // straight from the zoo demo, which didn't work at first
    const int READER_COLS = 64;
    const int READER_ROWS = 8;
    const char text[] =
      "Notcurses provides several widgets to quickly build vivid TUIs.\n\n"
      "This NCReader widget facilitates free-form text entry complete with readline-style bindings. "
      "NCSelector allows a single option to be selected from a list. "
      "NCMultiselector allows 0..n options to be selected from a list of n items. "
      "NCFdplane streams a file descriptor, while NCSubproc spawns a subprocess and streams its output. "
      "A variety of plots are supported, and menus can be placed along the top and/or bottom of any plane.\n\n"
      "Widgets can be controlled with the keyboard and/or mouse. They are implemented atop ncplanes, and these planes can be manipulated like all others.";
    auto sp = ncplane_new(nc_, READER_ROWS, READER_COLS, 0, 0, nullptr);
    REQUIRE(sp);
    ncplane_set_scrolling(sp, true);
    size_t bytes;
    ncplane_home(sp);
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, text, &bytes));
    CHECK(bytes == strlen(text));
    CHECK(0 == notcurses_render(nc_));
    char* line = ncplane_contents(sp, 0, 0, -1, -1);
    REQUIRE(line);
    // FIXME check line
    free(line);
    ncplane_destroy(sp);
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
    auto sp = ncplane_new(nc_, READER_ROWS, READER_COLS, 0, 0, nullptr);
    REQUIRE(sp);
    ncplane_set_scrolling(sp, true);
    size_t bytes;
    ncplane_home(sp);
    CHECK(0 < ncplane_puttext(sp, 0, NCALIGN_LEFT, text, &bytes));
    CHECK(bytes == strlen(text));
    CHECK(0 == notcurses_render(nc_));
    char* line = ncplane_contents(sp, 0, 0, -1, -1);
    REQUIRE(line);
    // FIXME check line
    free(line);
    ncplane_destroy(sp);
  }

  CHECK(0 == notcurses_stop(nc_));

}
