#include "main.h"
#include <cfenv>
#include <iostream>

// run ncmetric, and then change any localized decimal separator into our
// proud imperial yankee capitalist democratic one dot under god period.
// manifest destiny, bitchhhhhhhezzzz!
char* impericize_ncmetric(uintmax_t val, uintmax_t decimal, char* buf,
                          int omitdec, unsigned mult, int uprefix) {
  const char* decisep = localeconv()->decimal_point;
  REQUIRE(decisep);
  REQUIRE(1 ==  strlen(decisep));
  REQUIRE(ncmetric(val, decimal, buf, omitdec, mult, uprefix));
  char* commie = buf;
  while( (commie = strstr(commie, decisep)) ){
    *commie = '.'; // https://dank.qemfd.net/images/16whcc.jpg
    ++commie;
  }
  return buf;
}

TEST_CASE("Metric") {
  const char* decisep = localeconv()->decimal_point;
  REQUIRE(decisep);
  REQUIRE(1 ==  strlen(decisep));
  REQUIRE(0 ==  fesetround(FE_TOWARDZERO));

  SUBCASE("CornerInts") {
    char buf[PREFIXSTRLEN + 1];
    impericize_ncmetric(0, 1, buf, 0, 1000, '\0');
    CHECK(!strcmp("0.00", buf));
    impericize_ncmetric(0, 1, buf, 0, 1024, 'i');
    CHECK(!strcmp("0.00", buf)); // no suffix on < mult
    impericize_ncmetric(1, 1, buf, 0, 1000, '\0');
    CHECK(!strcmp("1.00", buf));
    impericize_ncmetric(1, 1, buf, 0, 1024, 'i');
    CHECK(!strcmp("1.00", buf));
    impericize_ncmetric(0, 1, buf, 1, 1000, '\0');
    CHECK(!strcmp("0", buf));
    impericize_ncmetric(0, 1, buf, 1, 1024, 'i');
    CHECK(!strcmp("0", buf)); // no suffix on < mult
    impericize_ncmetric(1, 1, buf, 1, 1000, '\0');
    CHECK(!strcmp("1", buf));
    impericize_ncmetric(1, 1, buf, 1, 1024, 'i');
    CHECK(!strcmp("1", buf));
    impericize_ncmetric(999, 1, buf, 1, 1000, '\0');
    CHECK(!strcmp("999", buf));
    impericize_ncmetric(1000, 1, buf, 1, 1000, '\0');
    CHECK(!strcmp("1K", buf));
    impericize_ncmetric(1000, 1, buf, 1, 1000, 'i');
    CHECK(!strcmp("1Ki", buf));
    impericize_ncmetric(1000, 1, buf, 1, 1024, 'i');
    CHECK(!strcmp("1000", buf)); // FIXME should be 0.977Ki
    impericize_ncmetric(1023, 1, buf, 1, 1000, '\0');
    CHECK(!strcmp("1.02K", buf));
    impericize_ncmetric(1023, 1, buf, 1, 1024, 'i');
    CHECK(!strcmp("1023", buf));
    impericize_ncmetric(1024, 1, buf, 1, 1000, '\0');
    CHECK(!strcmp("1.02K", buf));
    impericize_ncmetric(1024, 1, buf, 1, 1024, 'i');
    CHECK(!strcmp("1Ki", buf));
    impericize_ncmetric(1025, 1, buf, 1, 1000, '\0');
    CHECK(!strcmp("1.02K", buf));
    impericize_ncmetric(1025, 1, buf, 0, 1024, 'i');
    CHECK(!strcmp("1.00Ki", buf));
    impericize_ncmetric(1025, 1, buf, 1, 1024, 'i');
    CHECK(!strcmp("1.00Ki", buf));
    impericize_ncmetric(4096, 1, buf, 1, 1000, '\0');
    CHECK(!strcmp("4.09K", buf));
    impericize_ncmetric(4096, 1, buf, 1, 1024, 'i');
    CHECK(!strcmp("4Ki", buf));
  }

  SUBCASE("Maxints") {
    char buf[PREFIXSTRLEN + 1];
    // FIXME these will change based on the size of intmax_t and uintmax_t
    impericize_ncmetric(INTMAX_MAX - 1, 1, buf, 0, 1000, '\0');
    CHECK(!strcmp("9.22E", buf));
    impericize_ncmetric(INTMAX_MAX, 1, buf, 0, 1000, '\0');
    CHECK(!strcmp("9.22E", buf));
    impericize_ncmetric(UINTMAX_MAX - 1, 1, buf, 0, 1000, '\0');
    CHECK(!strcmp("18.44E", buf));
    impericize_ncmetric(UINTMAX_MAX, 1, buf, 0, 1000, '\0');
    CHECK(!strcmp("18.44E", buf));
  }

  SUBCASE("Maxints1024") {
    char buf[PREFIXSTRLEN + 1], gold[PREFIXSTRLEN + 1];
    // FIXME these will change based on the size of intmax_t and uintmax_t
    ncmetric(INTMAX_MAX - 1, 1, buf, 0, 1024, 'i');
    sprintf(gold, "%.2fEi", ((double)(INTMAX_MAX - (1ull << 53))) / (1ull << 60));
    CHECK(!strcmp(gold, buf));
    REQUIRE(ncmetric(INTMAX_MAX + 1ull, 1, buf, 0, 1024, 'i'));
    sprintf(gold, "%.2fEi", ((double)(INTMAX_MAX + 1ull)) / (1ull << 60));
    CHECK(!strcmp(gold, buf));
    impericize_ncmetric(UINTMAX_MAX - 1, 1, buf, 0, 1024, 'i');
    CHECK(!strcmp("15.99Ei", buf));
    impericize_ncmetric(UINTMAX_MAX, 1, buf, 0, 1024, 'i');
    CHECK(!strcmp("15.99Ei", buf));
    ncmetric(UINTMAX_MAX - (1ull << 53), 1, buf, 0, 1024, 'i');
    sprintf(gold, "%.2fEi", ((double)UINTMAX_MAX - (1ull << 53)) / (1ull << 60));
    CHECK(!strcmp(gold, buf));
  }

  const char suffixes[] = "\0KMGTPE";
  const wchar_t smallsuffixes[] = L"yzafpnµm";

  SUBCASE("PowersOfTen") {
    char gold[PREFIXSTRLEN + 1];
    char buf[PREFIXSTRLEN + 1];
    uintmax_t goldval = 1;
    uintmax_t val = 1;
    size_t i = 0;
    do{
      ncmetric(val, 1, buf, 0, 1000, '\0');
      const int sidx = i / 3;
      snprintf(gold, sizeof(gold), "%ju%s00%c", goldval, decisep, suffixes[sidx]);
      CHECK(!strcmp(gold, buf));
      if(UINTMAX_MAX / val < 10){
        break;
      }
      val *= 10;
      if((goldval *= 10) == 1000){
        goldval = 1;
      }
    }while(++i < sizeof(suffixes) * 3);
    // If we ran through all our suffixes, that's a problem
    CHECK(sizeof(suffixes) * 3 >  i);
  }

  SUBCASE("PowersOfTenNoDec") {
    char gold[PREFIXSTRLEN + 1];
    char buf[PREFIXSTRLEN + 1];
    uintmax_t goldval = 1;
    uintmax_t val = 1;
    size_t i = 0;
    do{
      ncmetric(val, 1, buf, 1, 1000, '\0');
      const int sidx = i / 3;
      snprintf(gold, sizeof(gold), "%ju%c", goldval, suffixes[sidx]);
      CHECK(!strcmp(gold, buf));
      if(UINTMAX_MAX / val < 10){
        break;
      }
      val *= 10;
      if((goldval *= 10) == 1000){
        goldval = 1;
      }
    }while(++i < sizeof(suffixes) * 3);
    // If we ran through all our suffixes, that's a problem
    CHECK(sizeof(suffixes) * 3 >  i);
  }

  SUBCASE("PowersOfTwo") {
    char gold[BPREFIXSTRLEN + 1];
    char buf[BPREFIXSTRLEN + 1];
    uintmax_t goldval = 1;
    uintmax_t val = 1;
    size_t i = 0;
    do{
      ncmetric(val, 1, buf, 0, 1024, 'i');
      const int sidx = i / 10;
      snprintf(gold, sizeof(gold), "%ju%s00%ci", goldval, decisep, suffixes[sidx]);
      CHECK(!strcmp(gold, buf));
      if(UINTMAX_MAX / val < 10){
        break;
      }
      val *= 2;
      if((goldval *= 2) == 1024){
        goldval = 1;
      }
    }while(++i < sizeof(suffixes) * 10);
    // If we ran through all our suffixes, that's a problem
    CHECK(sizeof(suffixes) * 10 >  i);
  }

  SUBCASE("PowersOfTwoNoDec") {
    char gold[BPREFIXSTRLEN + 1];
    char buf[BPREFIXSTRLEN + 1];
    uintmax_t goldval = 1;
    uintmax_t val = 1;
    size_t i = 0;
    do{
      ncmetric(val, 1, buf, 1, 1024, 'i');
      const int sidx = i / 10;
      snprintf(gold, sizeof(gold), "%ju%ci", goldval, suffixes[sidx]);
      CHECK(!strcmp(gold, buf));
      if(UINTMAX_MAX / val < 10){
        break;
      }
      val *= 2;
      if((goldval *= 2) == 1024){
        goldval = 1;
      }
    }while(++i < sizeof(suffixes) * 10);
    // If we ran through all our suffixes, that's a problem
    CHECK(sizeof(suffixes) * 10 >  i);
  }

  SUBCASE("PowersOfTwoAsTens") {
    char gold[PREFIXSTRLEN + 1];
    char buf[PREFIXSTRLEN + 1];
    uintmax_t vfloor = 1;
    uintmax_t val = 1;
    size_t i = 0;
    do{
      ncmetric(val, 1, buf, 0, 1000, '\0');
      const int sidx = i / 10;
      snprintf(gold, sizeof(gold), "%.2f%c", ((double)val) / vfloor, suffixes[sidx]);
      CHECK(!strcmp(gold, buf));
      if(UINTMAX_MAX / val < 10){
        break;
      }
      val *= 2;
      if(i % 10 == 9){
        vfloor *= 1000;
      }
    }while(++i < sizeof(suffixes) * 10);
    // If we ran through all our suffixes, that's a problem
    CHECK(sizeof(suffixes) * 10 >  i);
  }

  SUBCASE("PowersOfTenAsTwos") {
    char gold[BPREFIXSTRLEN + 1];
    char buf[BPREFIXSTRLEN + 1];
    uintmax_t vfloor = 1;
    uintmax_t val = 1;
    size_t i = 0;
    do{
      ncmetric(val, 1, buf, 0, 1024, 'i');
      const int sidx = i ? (i - 1) / 3 : 0;
      snprintf(gold, sizeof(gold), "%.2f%ci", ((double)val) / vfloor, suffixes[sidx]);
      CHECK(!strcmp(gold, buf));
      if(UINTMAX_MAX / val < 10){
        break;
      }
      val *= 10;
      if(i && i % 3 == 0){
        vfloor *= 1024;
      }
    }while(++i < sizeof(suffixes) * 10);
    // If we ran through all our suffixes, that's a problem
    CHECK(sizeof(suffixes) * 10 >  i);
  }

  SUBCASE("PowersOfTenMinusOne") {
    char gold[PREFIXSTRLEN + 1];
    char buf[PREFIXSTRLEN + 1];
    uintmax_t vfloor = 1;
    uintmax_t val = 1;
    size_t i = 0;
    do{
      ncmetric(val - 1, 1, buf, 0, 1000, '\0');
      const int sidx = i ? (i - 1) / 3 : 0;
      snprintf(gold, sizeof(gold), "%.2f%c", ((double)(val - 1)) / vfloor, suffixes[sidx]);
      CHECK(!strcmp(gold, buf));
      if(UINTMAX_MAX / val < 10){
        break;
      }
      val *= 10;
      if(i && i % 3 == 0){
        vfloor *= 1000;
      }
    }while(++i < sizeof(suffixes) * 3);
    // If we ran through all our suffixes, that's a problem
    CHECK(sizeof(suffixes) * 3 >  i);
  }

  SUBCASE("PowersOfTenPlusOne") {
    char gold[PREFIXSTRLEN + 1];
    char buf[PREFIXSTRLEN + 1];
    uintmax_t vfloor = 1;
    uintmax_t val = 1;
    size_t i = 0;
    do{
      ncmetric(val + 1, 1, buf, 0, 1000, '\0');
      const int sidx = i / 3;
      snprintf(gold, sizeof(gold), "%.2f%c", ((double)(val + 1)) / vfloor, suffixes[sidx]);
      CHECK(!strcmp(gold, buf));
      if(UINTMAX_MAX / val < 10){
        break;
      }
      val *= 10;
      if(i % 3 == 2){
        vfloor *= 1000;
      }
    }while(++i < sizeof(suffixes) * 3);
    // If we ran through all our suffixes, that's a problem
    CHECK(sizeof(suffixes) * 3 > i);
  }

  SUBCASE("PowersOfTenMinusOneAsTwos") {
    char gold[BPREFIXSTRLEN + 1];
    char buf[BPREFIXSTRLEN + 1];
    uintmax_t vfloor = 1;
    uintmax_t val = 1;
    size_t i = 0;
    do{
      ncmetric(val - 1, 1, buf, 0, 1024, 'i');
      const int sidx = i ? (i - 1) / 3 : 0;
      snprintf(gold, sizeof(gold), "%.2f%ci", ((double)(val - 1)) / vfloor, suffixes[sidx]);
      CHECK(!strcmp(gold, buf));
      if(UINTMAX_MAX / val < 10){
        break;
      }
      val *= 10;
      if(i && i % 3 == 0){
        vfloor *= 1024;
      }
    }while(++i < sizeof(suffixes) * 10);
    // If we ran through all our suffixes, that's a problem
    CHECK(sizeof(suffixes) * 10 > i);
  }

  constexpr auto GIG = 1000000000ull;

  // Output ought be scaled down for output while maintaining precision during
  // computation of that output. For instance, we might feed a number of
  // nanoseconds, but want output in seconds.
  // This requires 'decimal' = GIG.
  SUBCASE("ScaledGigSupra") {
    char gold[PREFIXSTRLEN + 1];
    snprintf(gold, sizeof(gold), "%.2f", 9.029); // 9.02
    char buf[PREFIXSTRLEN + 1];
    uintmax_t val = 9027854993;
    uintmax_t decimal = GIG;
    REQUIRE(qprefix(val, decimal, buf, 0));
    CHECK(!strcmp(buf, gold));
  }

  SUBCASE("ScaledGigUnity") {
    char gold[PREFIXSTRLEN + 1];
    snprintf(gold, sizeof(gold), "%.2f", 1.0); // 1.00
    char buf[PREFIXSTRLEN + 1];
    uintmax_t decimal = GIG;
    uintmax_t val = decimal;
    REQUIRE(qprefix(val, decimal, buf, 0));
    CHECK(!strcmp(buf, gold));
  }

  SUBCASE("ScaledGigJustAbove") {
    char gold[PREFIXSTRLEN + 1];
    snprintf(gold, sizeof(gold), "%.2f", 1.0); // 1.00
    char buf[PREFIXSTRLEN + 1];
    uintmax_t val = 1000000001;
    uintmax_t decimal = GIG;
    REQUIRE(qprefix(val, decimal, buf, 0));
    CHECK(!strcmp(buf, gold));
  }

  SUBCASE("ScaledGigJustBelow") {
    char gold[PREFIXSTRLEN + 1];
    snprintf(gold, sizeof(gold), "%.2fm", 999.999); //999.99
    char buf[PREFIXSTRLEN + 1];
    uintmax_t val = 999999999;
    uintmax_t decimal = GIG;
    REQUIRE(qprefix(val, decimal, buf, 0));
    CHECK(!strcmp(buf, gold));
  }

  SUBCASE("ScaledGigSub") {
    char gold[PREFIXSTRLEN + 1];
    snprintf(gold, sizeof(gold), "%.2fm", 27.859); // 27.85
    char buf[PREFIXSTRLEN + 1];
    uintmax_t val = 27854993;
    uintmax_t decimal = GIG;
    REQUIRE(qprefix(val, decimal, buf, 0));
    CHECK(!strcmp(buf, gold));
  }

  SUBCASE("ScaledGigSubSub") {
    char gold[PREFIXSTRLEN + 1];
    snprintf(gold, sizeof(gold), "%.2fm", 7.859); // 7.85
    char buf[PREFIXSTRLEN + 1];
    uintmax_t val = 7854993;
    uintmax_t decimal = GIG;
    REQUIRE(qprefix(val, decimal, buf, 0));
    CHECK(!strcmp(buf, gold));
  }

  SUBCASE("SmallCorners") {
    char buf[PREFIXSTRLEN + 1];
    impericize_ncmetric(1, 1000, buf, 0, 1000, '\0');
    CHECK(!strcmp("1.00m", buf));
    impericize_ncmetric(1, 1024, buf, 0, 1024, 'i');
    CHECK(!strcmp("1.00mi", buf));

    impericize_ncmetric(1, 1000000000000000000ull, buf, 0, 1000, '\0');
    CHECK(!strcmp("1.00a", buf));

    impericize_ncmetric(19, 10000000000000000ull, buf, 0, 1000, '\0');
    CHECK(!strcmp("1.89f", buf));
    impericize_ncmetric(99, 10000000000000000ull, buf, 0, 1000, '\0');
    CHECK(!strcmp("9.89f", buf));

    impericize_ncmetric(100, 10000000000000000ull, buf, 0, 1000, '\0');
    CHECK(!strcmp("10.00f", buf));

    impericize_ncmetric(1, 10, buf, 0, 1000, '\0');
    CHECK(!strcmp("100.00m", buf));
    impericize_ncmetric(1, 100, buf, 0, 1000, '\0');
    CHECK(!strcmp("10.00m", buf));
    impericize_ncmetric(10, 100, buf, 0, 1000, '\0');
    CHECK(!strcmp("100.00m", buf));
    impericize_ncmetric(10, 1000, buf, 0, 1000, '\0');
    CHECK(!strcmp("10.00m", buf));
    impericize_ncmetric(100, 1000, buf, 0, 1000, '\0');
    CHECK(!strcmp("100.00m", buf));
    impericize_ncmetric(1000, 1000, buf, 0, 1000, '\0');
    CHECK(!strcmp("1.00", buf));

    impericize_ncmetric(100, 1000000000000000ull, buf, 0, 1000, '\0');
    CHECK(!strcmp("100.00f", buf));

    impericize_ncmetric(100, 1000000000000ull, buf, 0, 1000, '\0');
    CHECK(!strcmp("100.00p", buf));
  }

  SUBCASE("NegativePowersOfTen") {
    char gold[PREFIXSTRLEN + 1];
    char buf[PREFIXSTRLEN + 1];
    uintmax_t goldval = 1;
    uintmax_t val = 1;
    size_t i = 0;
    do{
      REQUIRE(qprefix(val, 1000000000000000ull, buf, '\0'));
      const int sidx = i / 3 + 3;
      snprintf(gold, sizeof(gold), "%ju%s00%lc", goldval, decisep, smallsuffixes[sidx]);
      CHECK(!strcmp(gold, buf));
      if(UINTMAX_MAX / val < 10){
        break;
      }
      val *= 10;
      if((goldval *= 10) == 1000){
        goldval = 1;
      }
    }while(++i < (wcslen(smallsuffixes) - 3) * 3);
    // If we ran through all our suffixes, that's a problem
    CHECK(sizeof(smallsuffixes) / sizeof(*smallsuffixes) * 3 > i);
  }

}
