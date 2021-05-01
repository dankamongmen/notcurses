#include "main.h"

TEST_CASE("Version") {
  auto nc_ = testing_notcurses();
  REQUIRE(nullptr != nc_);
  int major, minor, patch, tweak;
  notcurses_version_components(&major, &minor, &patch, &tweak);
  CHECK(atoi(NOTCURSES_VERSION_MAJOR) == major);
  CHECK(atoi(NOTCURSES_VERSION_MINOR) == minor);
  CHECK(atoi(NOTCURSES_VERSION_PATCH) == patch);
  CHECK(atoi(NOTCURSES_VERSION_TWEAK) == tweak);
  CHECK(0 == notcurses_stop(nc_));
}
