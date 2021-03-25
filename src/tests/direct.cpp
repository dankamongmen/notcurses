#include "main.h"
#include <notcurses/direct.h>

TEST_CASE("DirectMode") {
  struct ncdirect* nc_ = ncdirect_init(NULL, stdout, 0);
  if(!nc_){
    return;
  }

  SUBCASE("SetItalic") {
    if(nc_->tcache.sgr){
      CHECK(0 == ncdirect_set_styles(nc_, NCSTYLE_ITALIC));
    }else{
      CHECK(0 != ncdirect_set_styles(nc_, NCSTYLE_ITALIC));
    }
    printf("DirectMode *italic*!\n");
    fflush(stdout);
    if(nc_->tcache.sgr0){
      CHECK(0 == ncdirect_off_styles(nc_, NCSTYLE_ITALIC));
    }else{
      CHECK(0 != ncdirect_off_styles(nc_, NCSTYLE_ITALIC));
    }
  }

  SUBCASE("SetBold") {
    if(nc_->tcache.sgr){
      CHECK(0 == ncdirect_set_styles(nc_, NCSTYLE_BOLD));
      printf("DirectMode *bold*!\n");
      fflush(stdout);
      CHECK(0 == ncdirect_off_styles(nc_, NCSTYLE_BOLD));
    }
  }

  SUBCASE("SetUnderline") {
    if(nc_->tcache.sgr){
      CHECK(0 == ncdirect_set_styles(nc_, NCSTYLE_UNDERLINE));
      printf("DirectMode *underline*!\n");
      fflush(stdout);
      CHECK(0 == ncdirect_off_styles(nc_, NCSTYLE_UNDERLINE));
    }
  }

  SUBCASE("SetStruck") {
    if(nc_->tcache.sgr){
      CHECK(0 == ncdirect_set_styles(nc_, NCSTYLE_STRUCK));
      printf("DirectMode *struck*!\n");
      fflush(stdout);
      CHECK(0 == ncdirect_off_styles(nc_, NCSTYLE_STRUCK));
    }
  }

#ifndef NOTCURSES_USE_MULTIMEDIA
  SUBCASE("VisualDisabled"){
    CHECK(!ncdirect_canopen_images(nc_));
  }
#else
  SUBCASE("ImagesEnabled"){
    CHECK(ncdirect_canopen_images(nc_));
  }

  SUBCASE("LoadImage") {
    CHECK(0 == ncdirect_render_image(nc_, find_data("changes.jpg"), NCALIGN_LEFT, NCBLIT_1x1, NCSCALE_STRETCH));
  }
#endif

  CHECK(0 == ncdirect_stop(nc_));

  // make sure that we can pass undefined flags and still create the ncdirect
  SUBCASE("FutureFlags") {
    auto fnc = ncdirect_init(NULL, stdout, ~0ULL);
    REQUIRE(nullptr != fnc);
    CHECK(0 == ncdirect_stop(fnc));
  }

}
