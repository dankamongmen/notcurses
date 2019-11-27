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
  auto ncv = notcurses_visual_open(nullptr, "../tools/dsscaw-purp.png");
  ASSERT_NE(nullptr, ncv);
  ncvisual_destroy(ncv);
}

TEST_F(LibavTest, LoadVideo) {
  auto ncv = notcurses_visual_open(nullptr, "../tools/atliens.mkv");
  ASSERT_NE(nullptr, ncv);
  ncvisual_destroy(ncv);
}
