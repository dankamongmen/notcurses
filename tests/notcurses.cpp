#include <notcurses.h>
#include "main.h"

TEST(Notcurses, BasicLifetime) {
  ASSERT_EQ(0, notcurses_init());
  EXPECT_EQ(0, notcurses_stop());
}
