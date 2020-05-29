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

  /*
  SUBCASE("VisualFromRGBA") {
    NotCurses nc;
    {
      Visual v = Visual.from_rgba("\x40\x80\xc0\xff", 1, 4, 1);
      CHECK(NCERR_SUCCESS == err);
    }
    CHECK(nc.stop());
  }
  */

}
