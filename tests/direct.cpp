#include "main.h"

TEST_CASE("DirectMode") {
  // common initialization
  if(getenv("TERM") == nullptr){
    return;
  }
  FILE* outfp_ = fopen("/dev/tty", "wb");
  REQUIRE(nullptr != outfp_);
  struct ncdirect* nc_ = ncdirect_init(NULL, outfp_);
  REQUIRE(nullptr != nc_);

  SUBCASE("SetItalic") {
    CHECK(0 == ncdirect_styles_set(nc_, NCSTYLE_ITALIC));
    CHECK(0 == ncdirect_styles_off(nc_, NCSTYLE_ITALIC));
  }

  SUBCASE("SetBold") {
    CHECK(0 == ncdirect_styles_set(nc_, NCSTYLE_BOLD));
    CHECK(0 == ncdirect_styles_off(nc_, NCSTYLE_BOLD));
  }

  SUBCASE("SetUnderline") {
    CHECK(0 == ncdirect_styles_set(nc_, NCSTYLE_UNDERLINE));
    CHECK(0 == ncdirect_styles_off(nc_, NCSTYLE_UNDERLINE));
  }

  // common teardown
  CHECK(0 == ncdirect_stop(nc_));
  CHECK(0 == fclose(outfp_));
}
