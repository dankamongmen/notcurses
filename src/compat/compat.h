#ifndef NOTCURSES_COMPAT
#define NOTCURSES_COMPAT

#include <time.h>

#define NANOSECS_IN_SEC 1000000000ul

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

#endif
