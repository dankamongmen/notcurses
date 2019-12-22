#include "main.h"
#include <cfenv>
#include <iostream>

class EnmetricTest : public :: testing::Test {
 protected:
  void SetUp() override {
    setlocale(LC_ALL, "");
    decisep_ = localeconv()->decimal_point;
    ASSERT_NE(nullptr, decisep_);
    ASSERT_EQ(1, strlen(decisep_));
  }

  void TearDown() override {
  }

  const char* decisep_{};

  char* impericize_enmetric(uintmax_t val, unsigned decimal, char* buf,
                            int omitdec, unsigned mult, int uprefix);
};

// run enmetric, and then change any localized decimal separator into our
// proud imperial yankee capitalist democratic one dot under god period.
// manifest destiny, bitchhhhhhhezzzz!
char* EnmetricTest::impericize_enmetric(uintmax_t val, unsigned decimal,
                                        char* buf, int omitdec, unsigned mult,
                                        int uprefix) {
  enmetric(val, decimal, buf, omitdec, mult, uprefix);
  char* commie = buf;
  while( (commie = strstr(commie, decisep_)) ){
    *commie = '.'; // https://dank.qemfd.net/images/16whcc.jpg
    ++commie;
  }
  return buf;
}

TEST_F(EnmetricTest, CornerInts) {
	char buf[PREFIXSTRLEN + 1];
	impericize_enmetric(0, 1, buf, 0, 1000, '\0');
	EXPECT_STREQ("0.00", buf);
	impericize_enmetric(0, 1, buf, 0, 1024, 'i');
	EXPECT_STREQ("0.00", buf); // no suffix on < mult
	impericize_enmetric(1, 1, buf, 0, 1000, '\0');
	EXPECT_STREQ("1.00", buf);
	impericize_enmetric(1, 1, buf, 0, 1024, 'i');
	EXPECT_STREQ("1.00", buf);
	impericize_enmetric(0, 1, buf, 1, 1000, '\0');
	EXPECT_STREQ("0", buf);
	impericize_enmetric(0, 1, buf, 1, 1024, 'i');
	EXPECT_STREQ("0", buf); // no suffix on < mult
	impericize_enmetric(1, 1, buf, 1, 1000, '\0');
	EXPECT_STREQ("1", buf);
	impericize_enmetric(1, 1, buf, 1, 1024, 'i');
	EXPECT_STREQ("1", buf);
	impericize_enmetric(999, 1, buf, 1, 1000, '\0');
	EXPECT_STREQ("999", buf);
	impericize_enmetric(1000, 1, buf, 1, 1000, '\0');
	EXPECT_STREQ("1K", buf);
	impericize_enmetric(1000, 1, buf, 1, 1000, 'i');
	EXPECT_STREQ("1Ki", buf);
	impericize_enmetric(1000, 1, buf, 1, 1024, 'i');
	EXPECT_STREQ("1000", buf); // FIXME should be 0.977Ki
	impericize_enmetric(1023, 1, buf, 1, 1000, '\0');
	EXPECT_STREQ("1.02K", buf);
	impericize_enmetric(1023, 1, buf, 1, 1024, 'i');
	EXPECT_STREQ("1023", buf);
	impericize_enmetric(1024, 1, buf, 1, 1000, '\0');
	EXPECT_STREQ("1.02K", buf);
	impericize_enmetric(1024, 1, buf, 1, 1024, 'i');
	EXPECT_STREQ("1Ki", buf);
	impericize_enmetric(1025, 1, buf, 1, 1000, '\0');
	EXPECT_STREQ("1.02K", buf);
	impericize_enmetric(1025, 1, buf, 0, 1024, 'i');
	EXPECT_STREQ("1.00Ki", buf);
	impericize_enmetric(1025, 1, buf, 1, 1024, 'i');
	EXPECT_STREQ("1.00Ki", buf);
	impericize_enmetric(4096, 1, buf, 1, 1000, '\0');
	EXPECT_STREQ("4.09K", buf);
	impericize_enmetric(4096, 1, buf, 1, 1024, 'i');
	EXPECT_STREQ("4Ki", buf);
}

TEST_F(EnmetricTest, Maxints) {
	char buf[PREFIXSTRLEN + 1];
	// FIXME these will change based on the size of intmax_t and uintmax_t
	impericize_enmetric(INTMAX_MAX - 1, 1, buf, 0, 1000, '\0');
	EXPECT_STREQ("9.22E", buf);
	impericize_enmetric(INTMAX_MAX, 1, buf, 0, 1000, '\0');
	EXPECT_STREQ("9.22E", buf);
	impericize_enmetric(UINTMAX_MAX - 1, 1, buf, 0, 1000, '\0');
	EXPECT_STREQ("18.44E", buf);
	impericize_enmetric(UINTMAX_MAX, 1, buf, 0, 1000, '\0');
	EXPECT_STREQ("18.44E", buf);
}

TEST_F(EnmetricTest, Maxints1024) {
	ASSERT_EQ(0, fesetround(FE_TOWARDZERO));
	char buf[PREFIXSTRLEN + 1], gold[PREFIXSTRLEN + 1];
	// FIXME these will change based on the size of intmax_t and uintmax_t
	enmetric(INTMAX_MAX - 1, 1, buf, 0, 1024, 'i');
	sprintf(gold, "%.2fEi", ((double)(INTMAX_MAX - (1ull << 53))) / (1ull << 60));
	EXPECT_STREQ(gold, buf);
	enmetric(INTMAX_MAX + 1ull, 1, buf, 0, 1024, 'i');
	sprintf(gold, "%.2fEi", ((double)(INTMAX_MAX + 1ull)) / (1ull << 60));
	EXPECT_STREQ(gold, buf);
	impericize_enmetric(UINTMAX_MAX - 1, 1, buf, 0, 1024, 'i');
	EXPECT_STREQ("15.99Ei", buf);
	impericize_enmetric(UINTMAX_MAX, 1, buf, 0, 1024, 'i');
	EXPECT_STREQ("15.99Ei", buf);
	enmetric(UINTMAX_MAX - (1ull << 53), 1, buf, 0, 1024, 'i');
	sprintf(gold, "%.2fEi", ((double)UINTMAX_MAX - (1ull << 53)) / (1ull << 60));
	EXPECT_STREQ(gold, buf);
}

const char suffixes[] = "\0KMGTPE";

TEST_F(EnmetricTest, PowersOfTen) {
	char gold[PREFIXSTRLEN + 1];
	char buf[PREFIXSTRLEN + 1];
	uintmax_t goldval = 1;
	uintmax_t val = 1;
	size_t i = 0;
	do{
		enmetric(val, 1, buf, 0, 1000, '\0');
		const int sidx = i / 3;
		snprintf(gold, sizeof(gold), "%ju%s00%c", goldval, decisep_, suffixes[sidx]);
		EXPECT_STREQ(gold, buf);
		if(UINTMAX_MAX / val < 10){
			break;
		}
		val *= 10;
		if((goldval *= 10) == 1000){
			goldval = 1;
		}
	}while(++i < sizeof(suffixes) * 3);
	// If we ran through all our suffixes, that's a problem
	EXPECT_GT(sizeof(suffixes) * 3, i);
}

TEST_F(EnmetricTest, PowersOfTenNoDec) {
	char gold[PREFIXSTRLEN + 1];
	char buf[PREFIXSTRLEN + 1];
	uintmax_t goldval = 1;
	uintmax_t val = 1;
	size_t i = 0;
	do{
		enmetric(val, 1, buf, 1, 1000, '\0');
		const int sidx = i / 3;
		snprintf(gold, sizeof(gold), "%ju%c", goldval, suffixes[sidx]);
		EXPECT_STREQ(gold, buf);
		if(UINTMAX_MAX / val < 10){
			break;
		}
		val *= 10;
		if((goldval *= 10) == 1000){
			goldval = 1;
		}
	}while(++i < sizeof(suffixes) * 3);
	// If we ran through all our suffixes, that's a problem
	EXPECT_GT(sizeof(suffixes) * 3, i);
}

TEST_F(EnmetricTest, PowersOfTwo) {
	char gold[BPREFIXSTRLEN + 1];
	char buf[BPREFIXSTRLEN + 1];
	uintmax_t goldval = 1;
	uintmax_t val = 1;
	size_t i = 0;
	do{
		enmetric(val, 1, buf, 0, 1024, 'i');
		const int sidx = i / 10;
		snprintf(gold, sizeof(gold), "%ju%s00%ci", goldval, decisep_, suffixes[sidx]);
		EXPECT_STREQ(gold, buf);
		if(UINTMAX_MAX / val < 10){
			break;
		}
		val *= 2;
		if((goldval *= 2) == 1024){
			goldval = 1;
		}
	}while(++i < sizeof(suffixes) * 10);
	// If we ran through all our suffixes, that's a problem
	EXPECT_GT(sizeof(suffixes) * 10, i);
}

TEST_F(EnmetricTest, PowersOfTwoNoDec) {
	char gold[BPREFIXSTRLEN + 1];
	char buf[BPREFIXSTRLEN + 1];
	uintmax_t goldval = 1;
	uintmax_t val = 1;
	size_t i = 0;
	do{
		enmetric(val, 1, buf, 1, 1024, 'i');
		const int sidx = i / 10;
		snprintf(gold, sizeof(gold), "%ju%ci", goldval, suffixes[sidx]);
		EXPECT_STREQ(gold, buf);
		if(UINTMAX_MAX / val < 10){
			break;
		}
		val *= 2;
		if((goldval *= 2) == 1024){
			goldval = 1;
		}
	}while(++i < sizeof(suffixes) * 10);
	// If we ran through all our suffixes, that's a problem
	EXPECT_GT(sizeof(suffixes) * 10, i);
}

TEST_F(EnmetricTest, PowersOfTwoAsTens) {
	char gold[PREFIXSTRLEN + 1];
	char buf[PREFIXSTRLEN + 1];
	uintmax_t vfloor = 1;
	uintmax_t val = 1;
	size_t i = 0;
	ASSERT_EQ(0, fesetround(FE_TOWARDZERO));
	do{
		enmetric(val, 1, buf, 0, 1000, '\0');
		const int sidx = i / 10;
		snprintf(gold, sizeof(gold), "%.2f%c",
				 ((double)val) / vfloor, suffixes[sidx]);
		EXPECT_STREQ(gold, buf);
		if(UINTMAX_MAX / val < 10){
			break;
		}
		val *= 2;
		if(i % 10 == 9){
			vfloor *= 1000;
		}
	}while(++i < sizeof(suffixes) * 10);
	// If we ran through all our suffixes, that's a problem
	EXPECT_GT(sizeof(suffixes) * 10, i);
}

TEST_F(EnmetricTest, PowersOfTenAsTwos) {
	char gold[BPREFIXSTRLEN + 1];
	char buf[BPREFIXSTRLEN + 1];
	uintmax_t vfloor = 1;
	uintmax_t val = 1;
	size_t i = 0;
	ASSERT_EQ(0, fesetround(FE_TOWARDZERO));
	do{
		enmetric(val, 1, buf, 0, 1024, 'i');
		const int sidx = i ? (i - 1) / 3 : 0;
		snprintf(gold, sizeof(gold), "%.2f%ci",
				 ((double)val) / vfloor, suffixes[sidx]);
		EXPECT_STREQ(gold, buf);
		if(UINTMAX_MAX / val < 10){
			break;
		}
		val *= 10;
		if(i && i % 3 == 0){
			vfloor *= 1024;
		}
	}while(++i < sizeof(suffixes) * 10);
	// If we ran through all our suffixes, that's a problem
	EXPECT_GT(sizeof(suffixes) * 10, i);
}

TEST_F(EnmetricTest, PowersOfTenMinusOne) {
	char gold[PREFIXSTRLEN + 1];
	char buf[PREFIXSTRLEN + 1];
	uintmax_t vfloor = 1;
	uintmax_t val = 1;
	size_t i = 0;
	ASSERT_EQ(0, fesetround(FE_TOWARDZERO));
	do{
		enmetric(val - 1, 1, buf, 0, 1000, '\0');
		const int sidx = i ? (i - 1) / 3 : 0;
		snprintf(gold, sizeof(gold), "%.2f%c",
				 ((double)(val - 1)) / vfloor, suffixes[sidx]);
		EXPECT_STREQ(gold, buf);
		if(UINTMAX_MAX / val < 10){
			break;
		}
		val *= 10;
		if(i && i % 3 == 0){
			vfloor *= 1000;
		}
	}while(++i < sizeof(suffixes) * 3);
	// If we ran through all our suffixes, that's a problem
	EXPECT_GT(sizeof(suffixes) * 3, i);
}

TEST_F(EnmetricTest, PowersOfTenPlusOne) {
	char gold[PREFIXSTRLEN + 1];
	char buf[PREFIXSTRLEN + 1];
	uintmax_t vfloor = 1;
	uintmax_t val = 1;
	size_t i = 0;
	ASSERT_EQ(0, fesetround(FE_TOWARDZERO));
	do{
		enmetric(val + 1, 1, buf, 0, 1000, '\0');
		const int sidx = i / 3;
		snprintf(gold, sizeof(gold), "%.2f%c",
				 ((double)(val + 1)) / vfloor, suffixes[sidx]);
		EXPECT_STREQ(gold, buf);
		if(UINTMAX_MAX / val < 10){
			break;
		}
		val *= 10;
		if(i % 3 == 2){
			vfloor *= 1000;
		}
	}while(++i < sizeof(suffixes) * 3);
	// If we ran through all our suffixes, that's a problem
	EXPECT_GT(sizeof(suffixes) * 3, i);
}

TEST_F(EnmetricTest, PowersOfTenMinusOneAsTwos) {
	char gold[BPREFIXSTRLEN + 1];
	char buf[BPREFIXSTRLEN + 1];
	uintmax_t vfloor = 1;
	uintmax_t val = 1;
	size_t i = 0;
	ASSERT_EQ(0, fesetround(FE_TOWARDZERO));
	do{
		enmetric(val - 1, 1, buf, 0, 1024, 'i');
		const int sidx = i ? (i - 1) / 3 : 0;
		snprintf(gold, sizeof(gold), "%.2f%ci",
				 ((double)(val - 1)) / vfloor, suffixes[sidx]);
		EXPECT_STREQ(gold, buf);
		if(UINTMAX_MAX / val < 10){
			break;
		}
		val *= 10;
		if(i && i % 3 == 0){
			vfloor *= 1024;
		}
	}while(++i < sizeof(suffixes) * 10);
	// If we ran through all our suffixes, that's a problem
	EXPECT_GT(sizeof(suffixes) * 10, i);
}
