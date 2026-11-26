#ifndef NOTCURSES_NCPORT
#define NOTCURSES_NCPORT

#ifdef __cplusplus
// modern c++ with modules support gets upset about exposed static linkage, and
// some systems implement byteswapping as static inlines. for c++, go ahead and
// roll our own using c++20/23 functionality.
#include <bit>
template<std::integral T>
constexpr inline T htole(T x) noexcept {
  if constexpr(std::endian::native == std::endian::big){
    return std::byteswap<T>(x);
  }
  return x;
}
extern "C" {
#endif

// Platform-dependent preprocessor material (includes and definitions) needed
// to compile against Notcurses. A critical definition is htole(), which forces
// 32-bit values to little-endian (as used in the nccell gcluster field). This
// ought be defined so that it's a a no-op on little-endian builds.

#ifndef __MINGW32__                               // All but Windows
#include <netinet/in.h>
#endif

#if defined(__linux__)                            // Linux
#ifndef __cplusplus
#include <byteswap.h>
#define htole(x) (__bswap_32(htonl(x)))
#endif
#elif defined(__APPLE__)                          // macOS
#ifndef __cplusplus
#include <libkern/OSByteOrder.h>
#define htole(x) (OSSwapInt32(htonl(x)))
#endif
#elif defined(__gnu_hurd__)                       // Hurd
#include <string.h>
#ifndef __cplusplus
#include <byteswap.h>
#define htole(x) (__bswap_32(htonl(x)))
#endif
#define wcwidth(w) 1 // FIXME lol, no
#define wcswidth(w, s) (int)(wcslen(w)) // FIXME lol, no
#elif defined(__MINGW32__)                        // Windows
#include <string.h>
#define wcwidth(w) 1 // FIXME lol, no
#define wcswidth(w, s) (int)(wcslen(w)) // FIXME lol, no
#ifndef __cplusplus
#define htole(x) (x) // FIXME are all windows installs LE? ugh
#endif
#else                                             // BSDs
#include <sys/endian.h>
#ifndef __cplusplus
#define htole(x) (bswap32(htonl(x)))
#endif
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif
