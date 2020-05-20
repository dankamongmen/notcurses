#define NCPP_EXCEPTIONS_PLEASE
#include "main.h"

TEST_CASE("Exceptions") {

  SUBCASE("Notcurses") {
    CHECK_THROWS_AS(ncpp::NotCurses::get_instance(), ncpp::invalid_state_error);
  }

}
