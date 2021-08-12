#ifndef NOTCURSES_NCPORT
#define NOTCURSES_NCPORT

#ifdef __cplusplus
extern "C" {
#endif

// Platform-dependent preprocessor material (includes and definitions) needed
// to compile against Notcurses. A critical definition is htole(), which forces
// 32-bit values to little-endian (as used in the nccell gcluster field). This
// ought be defined so that it's a a no-op on little-endian builds.

#if defined(__MINGW64__)                          // Windows
// FIXME placeholders, need real solutions here
#define wcwidth(w) 1
#define htole(x) (x) // FIXME are all windows installs LE?
#else                                             // Non-Windows, UNIX-common
#include <poll.h>
#include <netinet/in.h>
#include <termios.h>
#if defined(__linux__) || defined(__gnu_hurd__)   // Linux/Hurd
#include <byteswap.h>
#define htole(x) (__bswap_32(htonl(x)))
#elif defined(__APPLE__)                          // macOS
#include <libkern/OSByteOrder.h>
#define htole(x) (OSSwapInt32(htonl(x)))
#else                                             // BSD
#include <sys/endian.h>
#define htole(x) (bswap32(htonl(x)))
#endif
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif
