#ifndef NOTCURSES_TIMESPEC
#define NOTCURSES_TIMESPEC

#include <time.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int timespec_subtract(struct timespec *result, const struct timespec *time1,
                      struct timespec *time0);

static inline int64_t
timespec_subtract_ns(const struct timespec* time1, const struct timespec* time0){
  int64_t ns = time1->tv_sec * 1000000000 + time1->tv_nsec;
  ns -= time0->tv_sec * 1000000000 + time0->tv_nsec;
  return ns;
}

#ifdef __cplusplus
}
#endif

#endif
