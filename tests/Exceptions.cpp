#define NCPP_EXCEPTIONS_PLEASE
#include "main.h"

using namespace ncpp;

TEST_CASE("ExceptionsGetInstance") {
  CHECK_THROWS_AS(NotCurses::get_instance(), invalid_state_error);
}

TEST_CASE("ResetStats") {
  NotCurses nc;
  // FIXME attempts to match invalid_argument have failed thus far :/
  CHECK_THROWS(nc.reset_stats(nullptr));
  CHECK(nc.stop());
}

  // ncpp only allows one notcurses object at a time (why?)
TEST_CASE("OnlyOneNotCurses") {
  NotCurses nc;
  std::unique_ptr<NotCurses> nc2;
  // FIXME attempts to match ::init_error have failed thus far :/
  CHECK_THROWS(nc2.reset(new NotCurses()));
  CHECK(nc.stop());
}
