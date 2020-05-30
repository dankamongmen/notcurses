#include "main.h"
#include "ncpp/Visual.hh"

using namespace ncpp;

TEST_CASE("Ncpp"
          * doctest::description("Basic C++ wrapper tests")) {

  // we ought be able to construct a NotCurses object with a nullptr FILE
  // or even just no argument (decays to nullptr).
  SUBCASE("ConstructNotCurses") {
    NotCurses nc;
    CHECK(nc.stop());
  }

  SUBCASE("ConstructNotCursesNullFILE") {
    NotCurses ncnull(nullptr);
    CHECK(ncnull.stop());
  }

  // we ought be able to get a new NotCurses object after stop()ping one.
  SUBCASE("ConstructNotCursesTwice") {
    NotCurses nc;
    CHECK(nc.stop());
    NotCurses ncnull(nullptr);
    CHECK(ncnull.stop());
  }

  SUBCASE("VisualFromFile") {
    NotCurses nc;
    nc_err_e err;
    {
      Visual v = Visual(find_data("changes.jpg"), &err);
      CHECK(NCERR_SUCCESS == err);
    }
    CHECK(nc.stop());
  }

  SUBCASE("VisualFromRGBA") {
    NotCurses nc;
    uint32_t rgba[] = {
      0x4080c0ff,
      0x105090ff,
    };
    {
      Visual v = Visual(rgba, 1, sizeof(rgba), sizeof(rgba) / sizeof(*rgba));
      // FIXME check geometry
    }
    CHECK(nc.stop());
  }

  SUBCASE("VisualFromPlane") {
    NotCurses nc;
    {
      auto n = nc.get_stdplane();
      REQUIRE(n);
      // FIXME load something onto standard plane, load it into visual, erase
      // plane, render visual, check for equivalence...
      {
        Visual v = Visual(*n, 0, 0, -1, -1);
      }
    }
    CHECK(nc.stop());
  }

}
