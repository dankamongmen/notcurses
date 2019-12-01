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
    nopts.retain_cursor = true;
    nopts.pass_through_esc = true;
    nopts.outfp = stdin;
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
  auto ncv = ncplane_visual_open(ncp_, "../tests/dsscaw-purp.png", &averr);
  EXPECT_EQ(0, averr);
  ASSERT_NE(nullptr, ncv);
  ASSERT_NE(nullptr, ncvisual_decode(ncv, &averr));
  EXPECT_EQ(AVERROR_EOF, averr);
  ncvisual_destroy(ncv);
}

// FIXME ought run through full video, not just first frame
TEST_F(LibavTest, LoadVideo) {
  int averr;
  auto ncv = ncplane_visual_open(ncp_, "../tests/atliens.mkv", &averr);
  EXPECT_EQ(0, averr);
  ASSERT_NE(nullptr, ncv);
  ASSERT_NE(nullptr, ncvisual_decode(ncv, &averr));
  EXPECT_EQ(0, averr);
  ncvisual_destroy(ncv);
}
