#include <notcurses.h>
#include "egcpool.h"
#include "main.h"

class EGCPoolTest : public :: testing::Test {
 protected:
  void SetUp() override {
    setlocale(LC_ALL, nullptr);
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

TEST_F(EGCPoolTest, UTF8EGC) {
  const char* wstr = "☢";
  int c;
  auto ulen = utf8_egc_len(wstr, &c);
  ASSERT_LT(0, ulen);
  EXPECT_LT(0, c);
}

// we're gonna run both a composed latin a with grave, and then a latin a with
// a combining nonspacing grave
TEST_F(EGCPoolTest, UTF8EGCCombining) {
  const char* w1 = "à"; // U+00E0, U+0000         (c3 a0)
  const char* w2 = "à"; // U+0061, U+0300, U+0000 (61 cc 80)
  const char* w3 = "a"; // U+0061, U+0000         (61)
  int c1, c2, c3;
  auto u1 = utf8_egc_len(w1, &c1);
  auto u2 = utf8_egc_len(w2, &c2);
  auto u3 = utf8_egc_len(w3, &c3);
  ASSERT_EQ(2, u1);
  ASSERT_EQ(3, u2);
  ASSERT_EQ(1, u3);
  ASSERT_EQ(1, c1);
  ASSERT_EQ(1, c2);
  ASSERT_EQ(1, c3);
}

TEST_F(EGCPoolTest, AddAndRemove) {
  const char* wstr = "﷽";
  size_t ulen;
  int c; // column count
  ASSERT_LE(0, egcpool_stash(&pool_, wstr, &ulen, &c));
  ASSERT_LT(0, c);
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
  size_t u1, u2; // bytes consumed
  int c1, c2; // column counts
  int o1 = egcpool_stash(&pool_, wstr, &u1, &c1);
  int o2 = egcpool_stash(&pool_, wstr, &u2, &c2);
  ASSERT_LE(0, o1);
  ASSERT_LT(o1, o2);
  ASSERT_LT(0, c1);
  ASSERT_EQ(c1, c2);
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
  int c1, c2; // column counts
  int o1 = egcpool_stash(&pool_, wstr, &u1, &c1);
  int o2 = egcpool_stash(&pool_, wstr, &u2, &c2);
  ASSERT_LT(o1, o2);
  ASSERT_LT(0, c1);
  ASSERT_EQ(c1, c2);
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
