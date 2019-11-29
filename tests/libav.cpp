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
  auto ncv = ncplane_visual_open(ncp_, "../tests/dsscaw-purp.png");
  ASSERT_NE(nullptr, ncv);
  ASSERT_NE(nullptr, ncvisual_decode(ncv));
  ncvisual_destroy(ncv);
}

TEST_F(LibavTest, LoadVideo) {
  auto ncv = ncplane_visual_open(ncp_, "../tests/atliens.mkv");
  ASSERT_NE(nullptr, ncv);
  ASSERT_NE(nullptr, ncvisual_decode(ncv));
  ncvisual_destroy(ncv);
}
