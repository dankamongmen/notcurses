#include <notcurses.h>
#include "main.h"

class LibavTest : public :: testing::Test {
 protected:
  void SetUp() override {
    notcurses_options nopts{};
    nopts.outfd = STDIN_FILENO;
    nc_ = notcurses_init(&nopts);
    ASSERT_NE(nullptr, nc_);
    n_ = notcurses_stdplane(nc_);
    ASSERT_NE(nullptr, n_);
  }

  void TearDown() override {
    if(nc_){
      EXPECT_EQ(0, notcurses_stop(nc_));
    }
  }

  struct notcurses* nc_{};
  struct ncplane* n_{};
};

TEST_F(LibavTest, LoadImage) {
  int ret = notcurses_visual_open(nc_, "../tools/dsscaw-purp.png");
  ASSERT_EQ(0, ret);
}

TEST_F(LibavTest, LoadVideo) {
  int ret = notcurses_visual_open(nc_, "../tools/atliens.mkv");
  ASSERT_EQ(0, ret);
}
