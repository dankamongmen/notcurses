#include "main.h"
#include "internal.h"

// some simple tests to ensure the libunistring we've compiled/linked against
// behaves as expected.

TEST_CASE("Libunistring") {
  auto nc_ = testing_notcurses();
  if(!nc_){
    return;
  }
  ncplane* ncp_ = notcurses_stdplane(nc_);
  REQUIRE(ncp_);

  SUBCASE("WordbreakChars") {
    const wchar_t breakers[] = {
      L'\u0020', // space
      L'\u2000', // en quad
      L'\u2001', // em quad
      L'\u2002', // en quad
      L'\u2003', // em quad
      L'\u2004', // three-per-em space
      0
    }, *b;
    for(b = breakers ; *b ; ++b){
      CHECK(iswordbreak(*b));
    }
  }

  SUBCASE("LinebreakChars") {
    const wchar_t breakers[] = {
      L'\u000a', // linefeed
      L'\u000b', // vertical tab
      L'\u000c', // formfeed
      0
    }, *b;
    for(b = breakers ; *b ; ++b){
      CHECK(islinebreak(*b));
    }
  }

  CHECK(0 == notcurses_stop(nc_));

}
