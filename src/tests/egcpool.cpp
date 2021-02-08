#include <vector>
#include "main.h"
#include "egcpool.h"

TEST_CASE("EGCpool") {
  egcpool pool_{};

  SUBCASE("Initialized") {
    CHECK(!pool_.pool);
    CHECK(!pool_.poolsize);
    CHECK(!pool_.poolwrite);
    CHECK(!pool_.poolused);
  }

  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  if(!notcurses_canutf8(nc_)){
    CHECK(0 == notcurses_stop(nc_));
    return;
  }
  CHECK(0 == notcurses_stop(nc_));

  SUBCASE("UTF8EGC") {
    const char* wstr = "☢";
    int c;
    auto ulen = utf8_egc_len(wstr, &c);
    REQUIRE(0 < ulen);
    CHECK(0 < c);
    wstr = "▓";
    ulen = utf8_egc_len(wstr, &c);
    REQUIRE(0 < ulen);
    CHECK(0 < c);
  }

  // we're gonna run both a composed latin a with grave, and then a latin a with
  // a combining nonspacing grave
  SUBCASE("UTF8EGCCombining") {
    const char* w1 = "\u00e0"; // (utf8: c3 a0)
    const char* w2 = "\u0061\u0300"; // (utf8: 61 cc 80)
    const char* w3 = "\u0061"; // (utf8: 61)
    int c1, c2, c3;
    auto u1 = utf8_egc_len(w1, &c1);
    auto u2 = utf8_egc_len(w2, &c2);
    auto u3 = utf8_egc_len(w3, &c3);
    REQUIRE(2 == u1);
    REQUIRE(3 == u2);
    REQUIRE(1 == u3);
    REQUIRE(1 == c1);
    REQUIRE(1 == c2);
    REQUIRE(1 == c3);
  }

  SUBCASE("AddAndRemove") {
    const char* wstr = "\ufdfd"; // bismallih
    int c;
    auto ulen = utf8_egc_len(wstr, &c);
    REQUIRE(0 <= egcpool_stash(&pool_, wstr, ulen));
    REQUIRE(1 == c); // not considered wide, believe it or not
    CHECK(pool_.pool);
    CHECK(!strcmp(pool_.pool, wstr));
    CHECK(0 < pool_.poolsize);
    CHECK(ulen + 1 == pool_.poolused);
    CHECK(0 < pool_.poolwrite);
    CHECK(pool_.poolused <= pool_.poolsize);
    egcpool_release(&pool_, 0);
    CHECK_EQ('\0', *pool_.pool);
    CHECK(0 < pool_.poolsize);
    CHECK(0 == pool_.poolused);
    CHECK(0 < pool_.poolwrite);
  }

  SUBCASE("AddTwiceRemoveFirst") {
    const char* wstr = "\u8840"; // cjk unified ideograph, wide
    int c1, c2; // column counts
    auto u1 = utf8_egc_len(wstr, &c1); // bytes consumed
    auto u2 = utf8_egc_len(wstr, &c2);
    int o1 = egcpool_stash(&pool_, wstr, u1);
    int o2 = egcpool_stash(&pool_, wstr, u2);
    REQUIRE(0 <= o1);
    REQUIRE(o1 < o2);
    REQUIRE(2 == c1);
    REQUIRE(c1 == c2);
    CHECK(pool_.pool);
    CHECK(!strcmp(pool_.pool + o1, wstr));
    CHECK(!strcmp(pool_.pool + o2, wstr));
    CHECK(0 < pool_.poolsize);
    CHECK(u1 + u2 + 2 == pool_.poolused);
    CHECK(u1 + u2 + 2 == pool_.poolwrite);
    CHECK(pool_.poolused < pool_.poolsize);
    egcpool_release(&pool_, o1);
    CHECK('\0' == pool_.pool[o1]);
    CHECK(u2 + 1 == pool_.poolused);
    CHECK(0 < pool_.poolwrite);
  }

  SUBCASE("AddTwiceRemoveSecond") {
    const char* wstr = "\u8840"; // cjk unified ideograph, wide
    int c1, c2; // column counts
    auto u1 = utf8_egc_len(wstr, &c1); // bytes consumed
    auto u2 = utf8_egc_len(wstr, &c2);
    int o1 = egcpool_stash(&pool_, wstr, u1);
    int o2 = egcpool_stash(&pool_, wstr, u2);
    REQUIRE(o1 < o2);
    REQUIRE(2 == c1);
    REQUIRE(c1 == c2);
    CHECK(pool_.pool);
    CHECK(!strcmp(pool_.pool + o1, wstr));
    CHECK(!strcmp(pool_.pool + o2, wstr));
    CHECK(0 < pool_.poolsize);
    CHECK(u1 + u2 + 2 == pool_.poolused);
    CHECK(u1 + u2 + 2 == pool_.poolwrite);
    CHECK(pool_.poolused <= pool_.poolsize);
    egcpool_release(&pool_, o2);
    CHECK('\0' == pool_.pool[o2]);
    CHECK(u2 + 1 == pool_.poolused);
    CHECK(0 < pool_.poolwrite);
  }

  // POOL_MINIMUM_ALLOC is the minimum size of an egcpool once it goes active.
  // add EGCs to it past this boundary, and verify that they're all still
  // accurate.
  SUBCASE("ForceReallocation") {
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
      REQUIRE(sizeof(mb) >= r);
      mb[r] = '\0';
      candidates.push_back(egcpool_stash(&pool_, mb, r));
      REQUIRE((1u << 24u) > candidates[i]);
      if(!firstalloc){
        firstalloc = pool_.pool;
      }
    }
    // verify that we moved the pool at least once
    REQUIRE(pool_.pool != firstalloc);
    for(auto i = 0u ; i < candidates.size() ; ++i){
      auto stored = pool_.pool + candidates[i];
      char mb[MB_CUR_MAX + 1];
      wchar_t wcs = i + 0x80;
      auto r = wctomb(mb, wcs);
      if(r < 0){
        REQUIRE(-1 == candidates[i]);
        continue;
      }
      REQUIRE(0 < r);
      mb[r] = '\0';
      CHECK(!strcmp(mb, stored));
    }
  }

  // POOL_MINIMUM_ALLOC is the minimum size of an egcpool once it goes active.
  // add EGCs to it past this boundary, and verify that they're all still
  // accurate.
  SUBCASE("ForceReallocationWithRemovals") {
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
      REQUIRE(sizeof(mb) >= r);
      mb[r] = '\0';
      candidates.push_back(egcpool_stash(&pool_, mb, r));

      REQUIRE((1u << 24u) > candidates[i]);
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
        REQUIRE(-1 == candidates[i]);
        continue;
      }
      REQUIRE(0 < r);
      mb[r] = '\0';
      CHECK(!strcmp(mb, stored));
    }
    CHECK(candidates.size() / 13 > no);
  }

  // common cleanup
  egcpool_dump(&pool_);

}

TEST_CASE("EGCpoolLong" * doctest::skip(true)) {
  egcpool pool_{};

  // ensure that a hard error comes up when we fill the EGCpool
  SUBCASE("ExhaustPool") {
    wchar_t wcs = 0x4e00;
    uint64_t total = 0;
    while(true){
      char mb[MB_CUR_MAX + 1];
      auto r = wctomb(mb, wcs);
      CHECK(0 < r);
      REQUIRE(sizeof(mb) >= r);
      mb[r] = '\0';
      int loc = egcpool_stash(&pool_, mb, r);
      if(loc < 0){
        break;
      }
      REQUIRE(loc == total);
      total += r + 1;
      CHECK(egcpool_check_validity(&pool_, loc));
      CHECK((1u << 25) > loc);
      if(++wcs == 0x9fa5){
        wcs = 0x4e00;
      }
    }
    CHECK((1u << 25) <= total);
  }

  // common cleanup
  egcpool_dump(&pool_);

}
