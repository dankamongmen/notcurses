#ifndef NOTCURSES_EGCPOOL
#define NOTCURSES_EGCPOOL

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

int timespec_subtract(struct timespec *result, const struct timespec *time0,
                      struct timespec *time1);

#ifdef __cplusplus
}
#endif

#endif
