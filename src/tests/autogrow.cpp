#include "main.h"

TEST_CASE("Autogrow") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  struct ncplane* n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  // verify that the standard plane has scrolling disabled initially, and that
  // we cannot enable it at runtime.
  SUBCASE("AutogrowDisabledStdplane") {
    CHECK(!ncplane_set_autogrow(n_, true));  // disabled at start?
    CHECK(!ncplane_set_autogrow(n_, false)); // attempt to enable failed?
  }

  // by default, a new plane ought not have autogrow enabled--but we ought be
  // able to enable(+disable) it, unlike the standard plane.
  SUBCASE("AutogrowDisabledNewPlane") {
    struct ncplane_options nopts{};
    nopts.rows = 10;
    nopts.cols = 10;
    auto np = ncplane_create(n_, &nopts);
    REQUIRE(np);
    CHECK(!ncplane_set_autogrow(np, true));  // ought be false by default
    CHECK(ncplane_set_autogrow(np, false));  // did we set it true?
    CHECK(!ncplane_set_autogrow(np, false)); // did we set it false?
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(np));
  }

  // with the NCPLANE_OPTION_AUTOGROW flag, the plane ought have autogrow
  // enabled upon creation. we ought be able to disable it.
  SUBCASE("AutogrowDisabledNewPlane") {
    struct ncplane_options nopts{};
    nopts.rows = 10;
    nopts.cols = 10;
    nopts.flags = NCPLANE_OPTION_AUTOGROW;
    auto np = ncplane_create(n_, &nopts);
    REQUIRE(np);
    CHECK(ncplane_set_autogrow(np, false));  // ought be true at creation
    CHECK(!ncplane_set_autogrow(np, true));  // did we set it false?
    CHECK(ncplane_set_autogrow(np, false));  // did we set it true?
    CHECK(0 == notcurses_render(nc_));
    CHECK(0 == ncplane_destroy(np));
  }

  CHECK(0 == notcurses_stop(nc_));

}
