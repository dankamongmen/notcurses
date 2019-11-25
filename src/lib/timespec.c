#include <time.h>
#include "timespec.h"

int timespec_subtract(struct timespec *result, const struct timespec *time0,
                      struct timespec *time1){
  if(time0->tv_nsec < time1->tv_nsec){
    int nsec = (time1->tv_nsec - time0->tv_nsec) / 1000000000 + 1;
    time1->tv_nsec -= 1000000000 * nsec;
    time1->tv_sec += nsec;
  }
  if(time0->tv_nsec - time1->tv_nsec > 1000000000){
    int nsec = (time0->tv_nsec - time1->tv_nsec) / 1000000000;
    time1->tv_nsec += 1000000000 * nsec;
    time1->tv_sec -= nsec;
  }
  result->tv_sec = time0->tv_sec - time1->tv_sec;
  result->tv_nsec = time0->tv_nsec - time1->tv_nsec;
  return time0->tv_sec < time1->tv_sec;
}
