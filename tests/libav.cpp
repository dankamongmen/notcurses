#include <notcurses.h>
#include "version.h"
#include "main.h"

#ifndef DISABLE_FFMPEG
#include <libavutil/pixdesc.h>
#include <libavutil/avconfig.h>
#include <libavcodec/avcodec.h> // ffmpeg doesn't reliably "C"-guard itself
class LibavTest : public :: testing::Test {
 protected:
  void SetUp() override {
    setlocale(LC_ALL, "");
    if(getenv("TERM") == nullptr){
      GTEST_SKIP();
    }
    notcurses_options nopts{};
    nopts.inhibit_alternate_screen = true;
    nopts.suppress_bannner = true;
    outfp_ = fopen("/dev/tty", "wb");
    ASSERT_NE(nullptr, outfp_);
    nc_ = notcurses_init(&nopts, outfp_);
    ASSERT_NE(nullptr, nc_);
    ncp_ = notcurses_stdplane(nc_);
    ASSERT_NE(nullptr, ncp_);
  }

  void TearDown() override {
    if(nc_){
      EXPECT_EQ(0, notcurses_stop(nc_));
    }
    if(outfp_){
      fclose(outfp_);
    }
  }

  notcurses* nc_{};
  ncplane* ncp_{};
  FILE* outfp_{};
};

TEST_F(LibavTest, LoadImage) {
  int averr;
  int dimy, dimx;
  ncplane_dim_yx(ncp_, &dimy, &dimx);
  auto ncv = ncplane_visual_open(ncp_, "../data/dsscaw-purp.png", &averr);
  ASSERT_NE(nullptr, ncv);
  ASSERT_EQ(0, averr);
  auto frame = ncvisual_decode(ncv, &averr);
  ASSERT_NE(nullptr, frame);
  ASSERT_EQ(0, averr);
  EXPECT_EQ(dimy * 2, frame->height);
  EXPECT_EQ(dimx, frame->width);
  EXPECT_EQ(0, ncvisual_render(ncv, 0, 0, 0, 0));
  EXPECT_EQ(0, notcurses_render(nc_));
  frame = ncvisual_decode(ncv, &averr);
  ASSERT_EQ(nullptr, frame);
  EXPECT_EQ(AVERROR_EOF, averr);
  ncvisual_destroy(ncv);
}

// FIXME ought run through full video, not just first frame
TEST_F(LibavTest, LoadVideo) {
  int averr;
  int dimy, dimx;
  ncplane_dim_yx(ncp_, &dimy, &dimx);
  auto ncv = ncplane_visual_open(ncp_, "../data/fm6.mkv", &averr);
  ASSERT_NE(nullptr, ncv);
  EXPECT_EQ(0, averr);
  auto frame = ncvisual_decode(ncv, &averr);
  ASSERT_NE(nullptr, frame);
  EXPECT_EQ(0, averr);
  EXPECT_EQ(dimy * 2, frame->height);
  EXPECT_EQ(dimx, frame->width);
  EXPECT_EQ(0, ncvisual_render(ncv, 0, 0, 0, 0));
  EXPECT_EQ(0, notcurses_render(nc_));
  ncvisual_destroy(ncv);
}

TEST_F(LibavTest, LoadVideoCreatePlane) {
  int averr;
  int dimy, dimx;
  ncplane_dim_yx(ncp_, &dimy, &dimx);
  auto ncv = ncvisual_open_plane(nc_, "../data/fm6.mkv", &averr, 0, 0, NCSCALE_STRETCH);
  ASSERT_NE(nullptr, ncv);
  EXPECT_EQ(0, averr);
  auto frame = ncvisual_decode(ncv, &averr);
  ASSERT_NE(nullptr, frame);
  EXPECT_EQ(0, averr);
  EXPECT_EQ(dimy * 2, frame->height);
  EXPECT_EQ(dimx, frame->width);
  EXPECT_EQ(0, ncvisual_render(ncv, 0, 0, 0, 0));
  EXPECT_EQ(0, notcurses_render(nc_));
  ncvisual_destroy(ncv);
}
#endif
