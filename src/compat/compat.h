#ifndef NOTCURSES_COMPAT
#define NOTCURSES_COMPAT

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <stdint.h>
#include <sys/types.h>

#define NANOSECS_IN_SEC 1000000000ul

#ifdef __APPLE__
#define TIMER_ABSTIME 1
#endif

#ifdef  __MINGW64__
#define NL "\r\n"
#include <Lmcons.h>
#include <winsock2.h>
#define tcgetattr(x, y) (0)
#define tcsetattr(x, y, z) (0)
#define ECHO      0
#define ICANON    0
#define ICRNL     0
#define INLCR     0
#define ISIG      0
#define TCSAFLUSH 0
#define TCSANOW   0
#define O_NOCTTY  0
#define O_CLOEXEC O_NOINHERIT
#define O_NONBLOCK 0
#define O_DIRECTORY 0
#define S_IFLNK 0
#define SA_SIGINFO 0
#define SA_RESETHAND 0
#define SIGQUIT 0
#define SIGCONT 0
#define SIGWINCH 0
// FIXME actually implement this honoring c: CLOCK_MONOTONIC
#define pthread_cond_clockwait(a, b, c, d) pthread_cond_timedwait(a, b, d)
#define gettimeofday mingw_gettimeofday
#define sigaddset(x, y)
typedef struct siginfo_t {
  int aieeee;
} siginfo_t;
// not declared in MSYS2 header files, but implemented...?
int faccessat(int dirfd, const char *pathname, int mode, int flags);
#define sigset_t int
#define nl_langinfo(x) "UTF-8"
#define ppoll(w, x, y, z) WSAPoll((w), (x), (y))
pid_t waitpid(pid_t pid, int* ret, int flags);
struct winsize {
 unsigned short ws_row;
 unsigned short ws_col;
 unsigned short ws_xpixel;
 unsigned short ws_ypixel;
};
#define WNOHANG 0
#else
#define NL "\n"
#include <poll.h>
#include <termios.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#endif

int set_fd_nonblocking(int fd, unsigned state, unsigned* oldstate);
int set_fd_cloexec(int fd, unsigned state, unsigned* oldstate);

static inline uint64_t
timespec_to_ns(const struct timespec* ts){
  return ts->tv_sec * NANOSECS_IN_SEC + ts->tv_nsec;
}

static inline struct timespec*
ns_to_timespec(uint64_t ns, struct timespec* ts){
  ts->tv_sec = ns / NANOSECS_IN_SEC;
  ts->tv_nsec = ns % NANOSECS_IN_SEC;
  return ts;
}

// compatibility wrappers for code available only on certain operating systems.
// this file is not installed, but only consumed during compilation. if we're
// on an operating system which implements a given function, it won't be built.
int clock_nanosleep(clockid_t clockid, int flags,
                    const struct timespec *request,
                    struct timespec *remain);

#ifdef __cplusplus
}
#else
char* strndup(const char* str, size_t size);
#endif

#endif
