#include "main.h"
#include <cfenv>
#include <iostream>

TEST(NotcursesPrefix, CornerInts) {
	char buf[PREFIXSTRLEN + 1];
	enmetric(0, 1, buf, 0, 1000, '\0');
	EXPECT_STREQ("0.00", buf);
	enmetric(0, 1, buf, 0, 1024, 'i');
	EXPECT_STREQ("0.00", buf); // no suffix on < mult
	enmetric(1, 1, buf, 0, 1000, '\0');
	EXPECT_STREQ("1.00", buf);
	enmetric(1, 1, buf, 0, 1024, 'i');
	EXPECT_STREQ("1.00", buf);
	enmetric(0, 1, buf, 1, 1000, '\0');
	EXPECT_STREQ("0", buf);
	enmetric(0, 1, buf, 1, 1024, 'i');
	EXPECT_STREQ("0", buf); // no suffix on < mult
	enmetric(1, 1, buf, 1, 1000, '\0');
	EXPECT_STREQ("1", buf);
	enmetric(1, 1, buf, 1, 1024, 'i');
	EXPECT_STREQ("1", buf);
	enmetric(999, 1, buf, 1, 1000, '\0');
	EXPECT_STREQ("999", buf);
	enmetric(1000, 1, buf, 1, 1000, '\0');
	EXPECT_STREQ("1K", buf);
	enmetric(1000, 1, buf, 1, 1000, 'i');
	EXPECT_STREQ("1Ki", buf);
	enmetric(1000, 1, buf, 1, 1024, 'i');
	EXPECT_STREQ("1000", buf); // FIXME should be 0.977Ki
	enmetric(1023, 1, buf, 1, 1000, '\0');
	EXPECT_STREQ("1.02K", buf);
	enmetric(1023, 1, buf, 1, 1024, 'i');
	EXPECT_STREQ("1023", buf);
	enmetric(1024, 1, buf, 1, 1000, '\0');
	EXPECT_STREQ("1.02K", buf);
	enmetric(1024, 1, buf, 1, 1024, 'i');
	EXPECT_STREQ("1Ki", buf);
	enmetric(1025, 1, buf, 1, 1000, '\0');
	EXPECT_STREQ("1.02K", buf);
	enmetric(1025, 1, buf, 0, 1024, 'i');
	EXPECT_STREQ("1.00Ki", buf);
	enmetric(1025, 1, buf, 1, 1024, 'i');
	EXPECT_STREQ("1.00Ki", buf);
	enmetric(4096, 1, buf, 1, 1000, '\0');
	EXPECT_STREQ("4.09K", buf);
	enmetric(4096, 1, buf, 1, 1024, 'i');
	EXPECT_STREQ("4Ki", buf);
}

TEST(NotcursesPrefix, Maxints) {
	char buf[PREFIXSTRLEN + 1];
	// FIXME these will change based on the size of intmax_t and uintmax_t
	enmetric(INTMAX_MAX - 1, 1, buf, 0, 1000, '\0');
	EXPECT_STREQ("9.22E", buf);
	enmetric(INTMAX_MAX, 1, buf, 0, 1000, '\0');
	EXPECT_STREQ("9.22E", buf);
	enmetric(UINTMAX_MAX - 1, 1, buf, 0, 1000, '\0');
	EXPECT_STREQ("18.44E", buf);
	enmetric(UINTMAX_MAX, 1, buf, 0, 1000, '\0');
	EXPECT_STREQ("18.44E", buf);
}

TEST(NotcursesPrefix, Maxints1024) {
	ASSERT_EQ(0, fesetround(FE_TOWARDZERO));
	char buf[PREFIXSTRLEN + 1], gold[PREFIXSTRLEN + 1];
	// FIXME these will change based on the size of intmax_t and uintmax_t
	enmetric(INTMAX_MAX - 1, 1, buf, 0, 1024, 'i');
	sprintf(gold, "%.2fEi", ((double)(INTMAX_MAX - (1ull << 53))) / (1ull << 60));
	EXPECT_STREQ(gold, buf);
	enmetric(INTMAX_MAX + 1ull, 1, buf, 0, 1024, 'i');
	sprintf(gold, "%.2fEi", ((double)(INTMAX_MAX + 1ull)) / (1ull << 60));
	EXPECT_STREQ(gold, buf);
	enmetric(UINTMAX_MAX - 1, 1, buf, 0, 1024, 'i');
	EXPECT_STREQ("15.99Ei", buf);
	enmetric(UINTMAX_MAX, 1, buf, 0, 1024, 'i');
	EXPECT_STREQ("15.99Ei", buf);
	enmetric(UINTMAX_MAX - (1ull << 53), 1, buf, 0, 1024, 'i');
	sprintf(gold, "%.2fEi", ((double)UINTMAX_MAX - (1ull << 53)) / (1ull << 60));
	EXPECT_STREQ(gold, buf);
}

const char suffixes[] = "\0KMGTPE";

TEST(NotcursesPrefix, PowersOfTen) {
	char gold[PREFIXSTRLEN + 1];
	char buf[PREFIXSTRLEN + 1];
	uintmax_t goldval = 1;
	uintmax_t val = 1;
	size_t i = 0;
	do{
		enmetric(val, 1, buf, 0, 1000, '\0');
		const int sidx = i / 3;
		snprintf(gold, sizeof(gold), "%ju.00%c", goldval, suffixes[sidx]);
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

TEST(NotcursesPrefix, PowersOfTenNoDec) {
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

TEST(NotcursesPrefix, PowersOfTwo) {
	char gold[BPREFIXSTRLEN + 1];
	char buf[BPREFIXSTRLEN + 1];
	uintmax_t goldval = 1;
	uintmax_t val = 1;
	size_t i = 0;
	do{
		enmetric(val, 1, buf, 0, 1024, 'i');
		const int sidx = i / 10;
		snprintf(gold, sizeof(gold), "%ju.00%ci", goldval, suffixes[sidx]);
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

TEST(NotcursesPrefix, PowersOfTwoNoDec) {
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

TEST(NotcursesPrefix, PowersOfTwoAsTens) {
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

TEST(NotcursesPrefix, PowersOfTenAsTwos) {
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

TEST(NotcursesPrefix, PowersOfTenMinusOne) {
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

TEST(NotcursesPrefix, PowersOfTenPlusOne) {
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

TEST(NotcursesPrefix, PowersOfTenMinusOneAsTwos) {
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
