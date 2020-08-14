#ifndef NOTCURSES_ENDIANNESS
#define NOTCURSES_ENDIANNESS

// We would just use ntohl() and friends, except we need them in both a
// (1) compile-time and (2) C++ enum context. They're not permitted as
// C++ enum initializers. Thus this sorry affair.

#ifdef __cplusplus
extern "C" {
#endif

enum ncendianness_t {
  NC_BIGENDIAN = 0x00000001llu,
  NC_LITENDIAN = 0x01000000llu,
};

// Sprinkle parentheses of salt and recite the ancient incantation...
#ifdef __BIG_ENDIAN__
enum { NC_ENDIANNESS = NC_BIGENDIAN };
#else
#ifdef __LITTLE_ENDIAN__
enum { NC_ENDIANNESS = NC_LITENDIAN };
#else
#ifdef BSD
#include <sys/endian.h>
#else
#include <endian.h>
#endif
#if __BYTE_ORDER == __BIG_ENDIAN
enum { NC_ENDIANNESS = NC_BIGENDIAN };
#elif __BYTE_ORDER == __LITTLE_ENDIAN
enum { NC_ENDIANNESS = NC_LITENDIAN };
#else
#error "Couldn't determine endianness"
#endif
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif
