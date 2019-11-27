#include <notcurses.h>
#include "main.h"

class LibavTest : public :: testing::Test {
 protected:
  void SetUp() override {
  }

  void TearDown() override {
  }
};

TEST_F(LibavTest, LoadImage) {
  int ret = notcurses_visual_open(nullptr, "../tools/dsscaw-purp.png");
  ASSERT_EQ(0, ret);
}

TEST_F(LibavTest, LoadVideo) {
  int ret = notcurses_visual_open(nullptr, "../tools/atliens.mkv");
  ASSERT_EQ(0, ret);
}
