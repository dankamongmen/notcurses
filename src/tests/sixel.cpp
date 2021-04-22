#include "main.h"
#include "visual-details.h"
#include <vector>

TEST_CASE("Sixels") {
  auto nc_ = testing_notcurses();
  REQUIRE(nullptr != nc_);
  ncplane* ncp_ = notcurses_stdplane(nc_);
  REQUIRE(ncp_);
  auto n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  // FIXME force Sixel support
  if(notcurses_check_pixel_support(nc_) <= 0){
    CHECK(0 == nc_->tcache.bitmap_supported);
    CHECK(!notcurses_stop(nc_));
    return;
  }

#ifdef NOTCURSES_USE_MULTIMEDIA
  SUBCASE("SixelBlit") {
    CHECK(1 == ncplane_set_base(n_, "&", 0, 0));
    auto ncv = ncvisual_from_file(find_data("natasha-blur.png"));
    REQUIRE(ncv);
    struct ncvisual_options vopts{};
    vopts.blitter = NCBLIT_PIXEL;
    vopts.flags = NCVISUAL_OPTION_NODEGRADE;
    auto newn = ncvisual_render(nc_, ncv, &vopts);
    CHECK(newn);
    CHECK(0 == notcurses_render(nc_));
    struct ncplane_options nopts = {
      .y = ncplane_dim_y(newn) * 3 / 4,
      .x = 0,
      .rows = ncplane_dim_y(newn) / 4,
      .cols = ncplane_dim_x(newn) / 2,
      .userptr = nullptr, .name = "blck",
      .resizecb = nullptr,
      .flags = 0,
      .margin_b = 0, .margin_r = 0,
    };
    auto blockerplane = ncplane_create(newn, &nopts);
    REQUIRE(nullptr != blockerplane);
    uint64_t chan = CHANNELS_RGB_INITIALIZER(0, 0, 0, 0, 0, 0);
    CHECK(1 == ncplane_set_base(blockerplane, " ", 0, chan));
    CHECK(0 == notcurses_render(nc_));
sleep(2);
    CHECK(1 == ncplane_set_base(n_, "%", 0, 0));
    CHECK(0 == notcurses_render(nc_));
sleep(2);
    // FIXME at this point currently, we get a degraded back of the orca
    // test via conversion back to image? unsure
    CHECK(0 == ncplane_destroy(newn));
    CHECK(0 == ncplane_destroy(blockerplane));
    ncvisual_destroy(ncv);
  }
#endif

  CHECK(!notcurses_stop(nc_));
}
