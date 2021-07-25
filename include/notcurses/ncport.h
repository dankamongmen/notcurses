#ifndef NOTCURSES_NCPORT
#define NOTCURSES_NCPORT

#ifdef __cplusplus
extern "C" {
#endif

// take host byte order and turn it into network (reverse on LE, no-op on BE),
// then reverse that, guaranteeing LE. htole(x) == ltohe(x).
#if defined(__linux__) || defined(__gnu_hurd__)
#include <poll.h>
#include <termios.h>
#include <byteswap.h>
#include <netinet/in.h>
#define htole(x) (__bswap_32(htonl(x)))
#elif defined(__APPLE__)
#include <poll.h>
#include <termios.h>
#include <netinet/in.h>
#include <libkern/OSByteOrder.h>
#define htole(x) (OSSwapInt32(htonl(x)))
#elif defined(__MINGW64__)
#include <winsock2.h>
#include <winsock.h>
#define htole(x) (_byteswap_ulong(htonl(x)))
// FIXME placeholders, need real solutions here
#define wcwidth(w) 1
#define wcswidth(w, s) (s)
#define sigset_t int
#define sigemptyset(x)
#define O_CLOEXEC O_NOINHERIT
#define O_DIRECTORY 0
#define S_IFLNK 0
#else // bsd
#include <poll.h>
#include <netinet/in.h>
#include <sys/endian.h>
#define htole(x) (bswap32(htonl(x)))
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif
