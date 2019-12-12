#include <notcurses.h>
#include "egcpool.h"
#include "main.h"

class EGCPoolTest : public :: testing::Test {
 protected:
  void SetUp() override {
    setlocale(LC_ALL, "");
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
  wstr = "▓";
  ulen = utf8_egc_len(wstr, &c);
  ASSERT_LT(0, ulen);
  EXPECT_LT(0, c);
}

// we're gonna run both a composed latin a with grave, and then a latin a with
// a combining nonspacing grave
TEST_F(EGCPoolTest, UTF8EGCCombining) {
  const char* w1 = "\u00e0"; // (utf8: c3 a0)
  const char* w2 = "\u0061\u0300"; // (utf8: 61 cc 80)
  const char* w3 = "\u0061"; // (utf8: 61)
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
  const char* wstr = "\ufdfd"; // bismallih
  int c;
  auto ulen = utf8_egc_len(wstr, &c);
  ASSERT_LE(0, egcpool_stash(&pool_, wstr, ulen));
  ASSERT_EQ(1, c); // not considered wide, believe it or not
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
  const char* wstr = "\u8840"; // cjk unified ideograph, wide
  int c1, c2; // column counts
  auto u1 = utf8_egc_len(wstr, &c1); // bytes consumed
  auto u2 = utf8_egc_len(wstr, &c2);
  int o1 = egcpool_stash(&pool_, wstr, u1);
  int o2 = egcpool_stash(&pool_, wstr, u2);
  ASSERT_LE(0, o1);
  ASSERT_LT(o1, o2);
  ASSERT_EQ(2, c1);
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
  const char* wstr = "\u8840"; // cjk unified ideograph, wide
  int c1, c2; // column counts
  auto u1 = utf8_egc_len(wstr, &c1); // bytes consumed
  auto u2 = utf8_egc_len(wstr, &c2);
  int o1 = egcpool_stash(&pool_, wstr, u1);
  int o2 = egcpool_stash(&pool_, wstr, u2);
  ASSERT_LT(o1, o2);
  ASSERT_EQ(2, c1);
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

// POOL_MINIMUM_ALLOC is the minimum size of an egcpool once it goes active.
// add EGCs to it past this boundary, and verify that they're all still
// accurate.
TEST_F(EGCPoolTest, ForceReallocation) {
  std::vector<int> candidates;
  char* firstalloc = nullptr;
  for(auto i = 0u ; i < 1u << 20u ; ++i){
    char mb[MB_CUR_MAX + 1];
    wchar_t wcs = i + 0x80;
    auto r = wctomb(mb, wcs);
    if(r < 0){
      candidates.push_back(-1);
      continue;
    }
    ASSERT_GE(sizeof(mb), r);
    mb[r] = '\0';
    candidates.push_back(egcpool_stash(&pool_, mb, r));
    ASSERT_GT(1u << 24u, candidates[i]);
    if(!firstalloc){
      firstalloc = pool_.pool;
    }
  }
  // verify that we moved the pool at least once
  ASSERT_NE(pool_.pool, firstalloc);
  for(auto i = 0u ; i < candidates.size() ; ++i){
    auto stored = pool_.pool + candidates[i];
    char mb[MB_CUR_MAX + 1];
    wchar_t wcs = i + 0x80;
    auto r = wctomb(mb, wcs);
    if(r < 0){
      ASSERT_EQ(-1, candidates[i]);
      continue;
    }
    ASSERT_LT(0, r);
    mb[r] = '\0';
    EXPECT_STREQ(mb, stored);
  }
}

// POOL_MINIMUM_ALLOC is the minimum size of an egcpool once it goes active.
// add EGCs to it past this boundary, and verify that they're all still
// accurate.
TEST_F(EGCPoolTest, ForceReallocationWithRemovals) {
  std::vector<int> candidates;
  char* curpool = nullptr;
  for(auto i = 0u ; i < 1u << 20u ; ++i){
    char mb[MB_CUR_MAX + 1];
    wchar_t wcs = (i % 0x1000) + 0x80;
    auto r = wctomb(mb, wcs);
    if(r < 0){
      candidates.push_back(-1);
      continue;
    }
    ASSERT_GE(sizeof(mb), r);
    mb[r] = '\0';
    candidates.push_back(egcpool_stash(&pool_, mb, r));

    ASSERT_GT(1u << 24u, candidates[i]);
    if(pool_.pool != curpool){
      // cut through and release a bunch of them
      if(curpool){
        for(auto j = 0u ; j < i ; j += 13){
          if(candidates[j] >= 0){
            egcpool_release(&pool_, candidates[j]);
            candidates[j] = -1;
          }
        }
      }
      curpool = pool_.pool;
    }
  }
  int no = 0;
  for(auto i = 0u ; i < candidates.size() ; ++i){
    if(candidates[i] == -1){
      ++no;
      continue;
    }
    auto stored = pool_.pool + candidates[i];
    char mb[MB_CUR_MAX + 1];
    wchar_t wcs = (i % 0x1000) + 0x80;
    auto r = wctomb(mb, wcs);
    if(r < 0){
      ASSERT_EQ(-1, candidates[i]);
      continue;
    }
    ASSERT_LT(0, r);
    mb[r] = '\0';
    EXPECT_STREQ(mb, stored);
  }
  ASSERT_GT(candidates.size() / 13, no);
}
