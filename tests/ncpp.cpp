#include "main.h"

using namespace ncpp;

TEST_CASE("Ncpp") {

  // we ought be able to construct a NotCurses object with a nullptr FILE
  // or even just no argument (decays to nullptr).
  SUBCASE("ConstructNullFILE") {
    NotCurses nc;
    CHECK(nc.stop());
    NotCurses ncnull(nullptr);
    CHECK(ncnull.stop());
  }

}
