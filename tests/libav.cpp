#include <notcurses.h>
#include "main.h"

class LibavTest : public :: testing::Test {
 protected:
  void SetUp() override {
    setlocale(LC_ALL, nullptr);
    if(getenv("TERM") == nullptr){
      GTEST_SKIP();
    }
    notcurses_options nopts{};
    nopts.inhibit_alternate_screen = true;
    nopts.outfp = fopen("/dev/tty", "wb");
    nc_ = notcurses_init(&nopts);
    ASSERT_NE(nullptr, nc_);
    ncp_ = notcurses_stdplane(nc_);
    ASSERT_NE(nullptr, ncp_);
  }

  void TearDown() override {
    if(nc_){
      EXPECT_EQ(0, notcurses_stop(nc_));
    }
  }

  notcurses* nc_{};
  ncplane* ncp_{};
};

TEST_F(LibavTest, LoadImage) {
  int averr;
  int dimy, dimx;
  ncplane_dim_yx(ncp_, &dimy, &dimx);
  auto ncv = ncplane_visual_open(ncp_, "../tests/dsscaw-purp.png", &averr);
  EXPECT_EQ(0, averr);
  ASSERT_NE(nullptr, ncv);
  auto frame = ncvisual_decode(ncv, &averr);
  ASSERT_NE(nullptr, frame);
  // EXPECT_EQ(AVERROR_EOF, averr);
  EXPECT_EQ(dimy * 2, frame->height);
  EXPECT_EQ(dimx, frame->width);
  ncvisual_destroy(ncv);
}

// FIXME ought run through full video, not just first frame
TEST_F(LibavTest, LoadVideo) {
  int averr;
  int dimy, dimx;
  ncplane_dim_yx(ncp_, &dimy, &dimx);
  auto ncv = ncplane_visual_open(ncp_, "../tests/bob.mkv", &averr);
  EXPECT_EQ(0, averr);
  ASSERT_NE(nullptr, ncv);
  auto frame = ncvisual_decode(ncv, &averr);
  ASSERT_NE(nullptr, frame);
  // EXPECT_EQ(0, averr);
  EXPECT_EQ(dimy * 2, frame->height);
  EXPECT_EQ(dimx, frame->width);
  ncvisual_destroy(ncv);
}
