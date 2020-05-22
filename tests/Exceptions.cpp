#define NCPP_EXCEPTIONS_PLEASE
#include "main.h"

using namespace ncpp;

TEST_CASE("Exceptions") {

  SUBCASE("GetInstance") {
    CHECK_THROWS_AS(NotCurses::get_instance(), invalid_state_error);
  }

  SUBCASE("ResetStats") {
    NotCurses nc;
    CHECK_THROWS_AS(nc.reset_stats(nullptr), invalid_argument);
    CHECK(nc.stop());
  }

  // ncpp only allows one notcurses object at a time (why?)
  SUBCASE("OnlyOneNotCurses") {
    NotCurses nc;
    std::unique_ptr<NotCurses> nc2;
    // FIXME attempts to match ::init_error have failed thus far :/
    CHECK_THROWS(nc2.reset(new NotCurses()));
    CHECK(nc.stop());
  }

}
