#include "main.h"
#include "lib/visual-details.h"
#include <vector>
#include <iostream>
#include "lib/sixel.h"

/*
void print_bmap(const std::vector<uint32_t> rgba, int pixy, int pixx){
  for(int y = 0 ; y < pixy ; ++y){
    for(int x = 0 ; x < pixx ; ++x){
      std::cerr << "rgba[" << y << "][" << x << "] (" << y * x << "): " << std::hex << rgba[y * pixx + x] << std::dec << std::endl;
    }
  }
}
*/

TEST_CASE("Sixels") {
  auto nc_ = testing_notcurses();
  REQUIRE(nullptr != nc_);
  ncplane* ncp_ = notcurses_stdplane(nc_);
  REQUIRE(ncp_);
  auto n_ = notcurses_stdplane(nc_);
  REQUIRE(n_);

  // exercise ncvisual_from_sixel()
  SUBCASE("LoadSixel") {
    auto ncv = ncvisual_from_sixel("\x1bP0;1;0q\"1;1;1;6#0;2;66;18;80#0@-\x1b\\", 1, 1);
    REQUIRE(ncv);
    uint32_t p;
    ncvisual_at_yx(ncv, 0, 0, &p);
    CHECK(0xff == ncpixel_a(p));
    CHECK(0xa8 == ncpixel_r(p));
    CHECK(0x2d == ncpixel_g(p));
    CHECK(0xcc == ncpixel_b(p));
    ncvisual_destroy(ncv);
  }

  // remaining tests can only run with a Sixel backend
  if(notcurses_check_pixel_support(nc_) <= 0){
    CHECK(0 == notcurses_stop(nc_));
    return;
  }
  if(nc_->tcache.color_registers <= 0){
    CHECK(0 == notcurses_stop(nc_));
    return;
  }

#ifdef NOTCURSES_USE_MULTIMEDIA
  SUBCASE("SixelRoundtrip") {
    CHECK(1 == ncplane_set_base(n_, "&", 0, 0));
    auto ncv = ncvisual_from_file(find_data("worldmap.png").get());
    REQUIRE(ncv);
    struct ncvisual_options vopts{};
    vopts.n = n_;
    vopts.blitter = NCBLIT_PIXEL;
    vopts.flags = NCVISUAL_OPTION_NODEGRADE | NCVISUAL_OPTION_CHILDPLANE;
    vopts.scaling = NCSCALE_STRETCH;
    auto newn = ncvisual_blit(nc_, ncv, &vopts);
    REQUIRE(nullptr != newn);
    CHECK(0 == notcurses_render(nc_));
    auto rgb = ncsixel_as_rgba(newn->sprite->glyph.buf, newn->sprite->pixy, newn->sprite->pixx);
    REQUIRE(rgb);
    for(int y = 0 ; y < newn->sprite->pixy ; ++y){
      for(int x = 0 ; x < newn->sprite->pixx ; ++x){
//fprintf(stderr, "%03d/%03d NCV: %08x RGB: %08x\n", y, x, ncv->data[y * newn->sprite->pixx + x], rgb[y * newn->sprite->pixx + x]);
        // FIXME
        //CHECK(ncv->data[y * newn->sprite->pixx + x] == rgb[y * newn->sprite->pixx + x]);
      }
    }
    free(rgb);
    ncvisual_destroy(ncv);
  }

  SUBCASE("SixelBlit") {
    CHECK(1 == ncplane_set_base(n_, "&", 0, 0));
    auto ncv = ncvisual_from_file(find_data("natasha-blur.png").get());
    REQUIRE(ncv);
    struct ncvisual_options vopts{};
    vopts.n = n_;
    vopts.blitter = NCBLIT_PIXEL;
    vopts.flags = NCVISUAL_OPTION_NODEGRADE | NCVISUAL_OPTION_CHILDPLANE;
    vopts.scaling = NCSCALE_STRETCH;
    auto newn = ncvisual_blit(nc_, ncv, &vopts);
    REQUIRE(nullptr != newn);
    auto rgbold = ncsixel_as_rgba(newn->sprite->glyph.buf, newn->sprite->pixy, newn->sprite->pixx);
    REQUIRE(rgbold);
//print_bmap(rgbold, newn->sprite->pixy, newn->sprite->pixx);
    CHECK(0 == notcurses_render(nc_));
    struct ncplane_options nopts = {
      .y = (int)ncplane_dim_y(newn) * 3 / 4,
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
    uint64_t chan = NCCHANNELS_INITIALIZER(0, 0, 0, 0, 0, 0);
    CHECK(1 == ncplane_set_base(blockerplane, " ", 0, chan));
    CHECK(0 == notcurses_render(nc_));
    CHECK(1 == ncplane_set_base(n_, "%", 0, 0));
    CHECK(0 == notcurses_render(nc_));
    // FIXME at this point currently, we get a degraded back of the orca
    // test via conversion back to image? unsure
    auto rgbnew = ncsixel_as_rgba(newn->sprite->glyph.buf, newn->sprite->pixy, newn->sprite->pixx);
    REQUIRE(rgbnew);
//print_bmap(rgbnew, newn->sprite->pixy, newn->sprite->pixx);
    CHECK(0 == ncplane_destroy(newn));
    CHECK(0 == ncplane_destroy(blockerplane));
    free(rgbnew);
    free(rgbold);
    ncvisual_destroy(ncv);
  }
#endif

  CHECK(!notcurses_stop(nc_));
}
