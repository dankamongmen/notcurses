#include "main.h"

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

}
