#include "main.h"
#include <cfenv>
#include <iostream>

// run enmetric, and then change any localized decimal separator into our
// proud imperial yankee capitalist democratic one dot under god period.
// manifest destiny, bitchhhhhhhezzzz!
char* impericize_enmetric(uintmax_t val, unsigned decimal, char* buf,
                          int omitdec, unsigned mult, int uprefix) {
  const char* decisep = localeconv()->decimal_point;
  REQUIRE(decisep);
  REQUIRE(1 ==  strlen(decisep));
  REQUIRE(enmetric(val, decimal, buf, omitdec, mult, uprefix));
  char* commie = buf;
  while( (commie = strstr(commie, decisep)) ){
    *commie = '.'; // https://dank.qemfd.net/images/16whcc.jpg
    ++commie;
  }
  return buf;
}

TEST_CASE("Enmetric") {
  const char* decisep = localeconv()->decimal_point;
  REQUIRE(decisep);
  REQUIRE(1 ==  strlen(decisep));

  SUBCASE("CornerInts") {
    char buf[PREFIXSTRLEN + 1];
    impericize_enmetric(0, 1, buf, 0, 1000, '\0');
    CHECK(!strcmp("0.00", buf));
    impericize_enmetric(0, 1, buf, 0, 1024, 'i');
    CHECK(!strcmp("0.00", buf)); // no suffix on < mult
    impericize_enmetric(1, 1, buf, 0, 1000, '\0');
    CHECK(!strcmp("1.00", buf));
    impericize_enmetric(1, 1, buf, 0, 1024, 'i');
    CHECK(!strcmp("1.00", buf));
    impericize_enmetric(0, 1, buf, 1, 1000, '\0');
    CHECK(!strcmp("0", buf));
    impericize_enmetric(0, 1, buf, 1, 1024, 'i');
    CHECK(!strcmp("0", buf)); // no suffix on < mult
    impericize_enmetric(1, 1, buf, 1, 1000, '\0');
    CHECK(!strcmp("1", buf));
    impericize_enmetric(1, 1, buf, 1, 1024, 'i');
    CHECK(!strcmp("1", buf));
    impericize_enmetric(999, 1, buf, 1, 1000, '\0');
    CHECK(!strcmp("999", buf));
    impericize_enmetric(1000, 1, buf, 1, 1000, '\0');
    CHECK(!strcmp("1K", buf));
    impericize_enmetric(1000, 1, buf, 1, 1000, 'i');
    CHECK(!strcmp("1Ki", buf));
    impericize_enmetric(1000, 1, buf, 1, 1024, 'i');
    CHECK(!strcmp("1000", buf)); // FIXME should be 0.977Ki
    impericize_enmetric(1023, 1, buf, 1, 1000, '\0');
    CHECK(!strcmp("1.02K", buf));
    impericize_enmetric(1023, 1, buf, 1, 1024, 'i');
    CHECK(!strcmp("1023", buf));
    impericize_enmetric(1024, 1, buf, 1, 1000, '\0');
    CHECK(!strcmp("1.02K", buf));
    impericize_enmetric(1024, 1, buf, 1, 1024, 'i');
    CHECK(!strcmp("1Ki", buf));
    impericize_enmetric(1025, 1, buf, 1, 1000, '\0');
    CHECK(!strcmp("1.02K", buf));
    impericize_enmetric(1025, 1, buf, 0, 1024, 'i');
    CHECK(!strcmp("1.00Ki", buf));
    impericize_enmetric(1025, 1, buf, 1, 1024, 'i');
    CHECK(!strcmp("1.00Ki", buf));
    impericize_enmetric(4096, 1, buf, 1, 1000, '\0');
    CHECK(!strcmp("4.09K", buf));
    impericize_enmetric(4096, 1, buf, 1, 1024, 'i');
    CHECK(!strcmp("4Ki", buf));
  }

  SUBCASE("Maxints") {
    char buf[PREFIXSTRLEN + 1];
    // FIXME these will change based on the size of intmax_t and uintmax_t
    impericize_enmetric(INTMAX_MAX - 1, 1, buf, 0, 1000, '\0');
    CHECK(!strcmp("9.22E", buf));
    impericize_enmetric(INTMAX_MAX, 1, buf, 0, 1000, '\0');
    CHECK(!strcmp("9.22E", buf));
    impericize_enmetric(UINTMAX_MAX - 1, 1, buf, 0, 1000, '\0');
    CHECK(!strcmp("18.44E", buf));
    impericize_enmetric(UINTMAX_MAX, 1, buf, 0, 1000, '\0');
    CHECK(!strcmp("18.44E", buf));
  }

  SUBCASE("Maxints1024") {
    REQUIRE(0 ==  fesetround(FE_TOWARDZERO));
    char buf[PREFIXSTRLEN + 1], gold[PREFIXSTRLEN + 1];
    // FIXME these will change based on the size of intmax_t and uintmax_t
    enmetric(INTMAX_MAX - 1, 1, buf, 0, 1024, 'i');
    sprintf(gold, "%.2fEi", ((double)(INTMAX_MAX - (1ull << 53))) / (1ull << 60));
    CHECK(!strcmp(gold, buf));
    enmetric(INTMAX_MAX + 1ull, 1, buf, 0, 1024, 'i');
    sprintf(gold, "%.2fEi", ((double)(INTMAX_MAX + 1ull)) / (1ull << 60));
    CHECK(!strcmp(gold, buf));
    impericize_enmetric(UINTMAX_MAX - 1, 1, buf, 0, 1024, 'i');
    CHECK(!strcmp("15.99Ei", buf));
    impericize_enmetric(UINTMAX_MAX, 1, buf, 0, 1024, 'i');
    CHECK(!strcmp("15.99Ei", buf));
    enmetric(UINTMAX_MAX - (1ull << 53), 1, buf, 0, 1024, 'i');
    sprintf(gold, "%.2fEi", ((double)UINTMAX_MAX - (1ull << 53)) / (1ull << 60));
    CHECK(!strcmp(gold, buf));
  }

  const char suffixes[] = "\0KMGTPE";

  SUBCASE("PowersOfTen") {
    char gold[PREFIXSTRLEN + 1];
    char buf[PREFIXSTRLEN + 1];
    uintmax_t goldval = 1;
    uintmax_t val = 1;
    size_t i = 0;
    do{
      enmetric(val, 1, buf, 0, 1000, '\0');
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
      enmetric(val, 1, buf, 1, 1000, '\0');
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
      enmetric(val, 1, buf, 0, 1024, 'i');
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
      enmetric(val, 1, buf, 1, 1024, 'i');
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
    REQUIRE(0 ==  fesetround(FE_TOWARDZERO));
    do{
      enmetric(val, 1, buf, 0, 1000, '\0');
      const int sidx = i / 10;
      snprintf(gold, sizeof(gold), "%.2f%c",
          ((double)val) / vfloor, suffixes[sidx]);
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
    REQUIRE(0 ==  fesetround(FE_TOWARDZERO));
    do{
      enmetric(val, 1, buf, 0, 1024, 'i');
      const int sidx = i ? (i - 1) / 3 : 0;
      snprintf(gold, sizeof(gold), "%.2f%ci",
          ((double)val) / vfloor, suffixes[sidx]);
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
    REQUIRE(0 ==  fesetround(FE_TOWARDZERO));
    do{
      enmetric(val - 1, 1, buf, 0, 1000, '\0');
      const int sidx = i ? (i - 1) / 3 : 0;
      snprintf(gold, sizeof(gold), "%.2f%c",
          ((double)(val - 1)) / vfloor, suffixes[sidx]);
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
    REQUIRE(0 == fesetround(FE_TOWARDZERO));
    do{
      enmetric(val + 1, 1, buf, 0, 1000, '\0');
      const int sidx = i / 3;
      snprintf(gold, sizeof(gold), "%.2f%c",
          ((double)(val + 1)) / vfloor, suffixes[sidx]);
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
    REQUIRE(0 == fesetround(FE_TOWARDZERO));
    do{
      enmetric(val - 1, 1, buf, 0, 1024, 'i');
      const int sidx = i ? (i - 1) / 3 : 0;
      snprintf(gold, sizeof(gold), "%.2f%ci",
          ((double)(val - 1)) / vfloor, suffixes[sidx]);
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

  // Output ought be scaled down for output while maintaining precision during
  // computation of that output. For instance, we might feed a number of
  // nanoseconds, but want output in seconds.
  // This requires 'decimal' = 1000000000.
  SUBCASE("ScaledGig") {
    char gold[PREFIXSTRLEN + 1] = "9.02";
    char buf[PREFIXSTRLEN + 1];
    uintmax_t val = 9027854993;
    uintmax_t decimal = 1000000000;
    REQUIRE(qprefix(val, decimal, buf, 0));
    CHECK(!strcmp(buf, gold));
  }

}
