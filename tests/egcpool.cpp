#include <notcurses.h>
#include "egcpool.h"
#include "main.h"

class EGCPoolTest : public :: testing::Test {
 protected:
  void SetUp() override {
  }

  void TearDown() override {
    egcpool_dump(&pool_);
  }

  egcpool pool_{};

};

TEST_F(EGCPoolTest, Initialized) {
  EXPECT_EQ(nullptr, pool_.pool);
  EXPECT_EQ(0, pool_.poolsize);
  EXPECT_EQ(0, pool_.poolwrite);
  EXPECT_EQ(0, pool_.poolused);
}

TEST_F(EGCPoolTest, AddAndRemove) {
  const char* wstr = "﷽";
  size_t ulen;
  ASSERT_EQ(0, egcpool_stash(&pool_, wstr, &ulen));
  EXPECT_NE(nullptr, pool_.pool);
  EXPECT_STREQ(pool_.pool, wstr);
  EXPECT_LT(0, pool_.poolsize);
  EXPECT_EQ(ulen + 1, pool_.poolused);
  EXPECT_LT(0, pool_.poolwrite);
  EXPECT_LE(pool_.poolused, pool_.poolsize);
  egcpool_release(&pool_, 0);
  EXPECT_EQ('\0', *pool_.pool);
  EXPECT_LT(0, pool_.poolsize);
  EXPECT_EQ(0, pool_.poolused);
  EXPECT_LT(0, pool_.poolwrite);
}

TEST_F(EGCPoolTest, AddTwiceRemoveFirst) {
  const char* wstr = "血";
  size_t u1, u2;
  int o1 = egcpool_stash(&pool_, wstr, &u1);
  int o2 = egcpool_stash(&pool_, wstr, &u2);
  ASSERT_LT(o1, o2);
  EXPECT_NE(nullptr, pool_.pool);
  EXPECT_STREQ(pool_.pool + o1, wstr);
  EXPECT_STREQ(pool_.pool + o2, wstr);
  EXPECT_LT(0, pool_.poolsize);
  EXPECT_EQ(u1 + u2 + 2, pool_.poolused);
  EXPECT_EQ(u1 + u2 + 2, pool_.poolwrite);
  EXPECT_LE(pool_.poolused, pool_.poolsize);
  egcpool_release(&pool_, o1);
  EXPECT_EQ('\0', pool_.pool[o1]);
  EXPECT_EQ(u2 + 1, pool_.poolused);
  EXPECT_LT(0, pool_.poolwrite);
}

TEST_F(EGCPoolTest, AddTwiceRemoveSecond) {
  const char* wstr = "血";
  size_t u1, u2;
  int o1 = egcpool_stash(&pool_, wstr, &u1);
  int o2 = egcpool_stash(&pool_, wstr, &u2);
  ASSERT_LT(o1, o2);
  EXPECT_NE(nullptr, pool_.pool);
  EXPECT_STREQ(pool_.pool + o1, wstr);
  EXPECT_STREQ(pool_.pool + o2, wstr);
  EXPECT_LT(0, pool_.poolsize);
  EXPECT_EQ(u1 + u2 + 2, pool_.poolused);
  EXPECT_EQ(u1 + u2 + 2, pool_.poolwrite);
  EXPECT_LE(pool_.poolused, pool_.poolsize);
  egcpool_release(&pool_, o2);
  EXPECT_EQ('\0', pool_.pool[o2]);
  EXPECT_EQ(u2 + 1, pool_.poolused);
  EXPECT_LT(0, pool_.poolwrite);
}
