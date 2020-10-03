#define NCPP_EXCEPTIONS_PLEASE
#include "main.h"
#include <memory>

using namespace ncpp;

TEST_CASE("Exceptions") {

  notcurses_options nopts{};
  nopts.flags = NCOPTION_SUPPRESS_BANNERS |
                NCOPTION_INHIBIT_SETLOCALE;
  nopts.loglevel = NCLOGLEVEL_VERBOSE;

  SUBCASE("GetInstance") {
    CHECK_THROWS_AS(NotCurses::get_instance(), invalid_state_error);
  }

  SUBCASE("ResetStats") {
    NotCurses nc{ nopts };
    CHECK_THROWS_AS(nc.reset_stats(nullptr), invalid_argument);
  }

  // ncpp only allows one notcurses object at a time (why?)
  SUBCASE("OnlyOneNotCurses") {
    NotCurses nc{ nopts };
    std::unique_ptr<NotCurses> nc2;
    // FIXME attempts to match ::init_error have failed thus far :/
    CHECK_THROWS(nc2.reset(new NotCurses()));
  }

}
