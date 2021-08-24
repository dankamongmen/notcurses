#include "main.h"

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
      // L'\u0009', // horizontal tab
      L'\u0020', // space
      // L'\u007c', // vertical line
      // L'\u00ad', // soft hyphen
      // L'\u058a', // armenian hyphen
      // L'\u0f0b', // tibetan mark intersyllabic tsheg
      // L'\u1361', // ethiopic wordspace
      L'\u1680', // ogham space mark
      //L'\u17d5', // khmer sign bariyoosan
      L'\u2000', // en quad
      L'\u2001', // em quad
      L'\u2002', // en quad
      L'\u2003', // em quad
      L'\u2004', // three-per-em space
      L'\u2005', // four-per-em space
      L'\u2006', // six-per-em space
      L'\u2008', // punctuation space
      L'\u2009', // thin space
      L'\u200a', // hair space
      //L'\u2010', // hyphen
      //L'\u2027', // hyphenation point
      0
    }, *b;
    for(b = breakers ; *b ; ++b){
      if(!iswordbreak(*b)){
        fprintf(stderr, "Unexpectedly fails to wordbreak: U+%04x [%lc]\n", *b, *b);
      }
      CHECK(iswordbreak(*b));
    }
    CHECK(!islinebreak(L'\u000d'));
  }

  // \u000d carriage return is *not* a linebreaker
  SUBCASE("LinebreakChars") {
    const wchar_t breakers[] = {
      L'\u000a', // linefeed
      L'\u000b', // vertical tab
      L'\u000c', // formfeed
      0
    }, *b;
    for(b = breakers ; *b ; ++b){
      if(!islinebreak(*b)){
        fprintf(stderr, "Unexpectedly fails to linebreak: U+%04x [%lc]\n", *b, *b);
      }
      CHECK(islinebreak(*b));
    }
    CHECK(!islinebreak(L'\u000d'));
  }

  CHECK(0 == notcurses_stop(nc_));

}
