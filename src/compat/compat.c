#ifndef __linux__
#ifndef __FreeBSD__
#ifdef  __MINGW64__
#include <string.h>
#include <stdlib.h>
char* strndup(const char* str, size_t size){
  size_t t = strlen(str);
  if(t > size){
    t = size;
  }
  ++t;
  char* r = malloc(t);
  if(r){
    memcpy(r, str, t);
    r[t] = '\0';
  }
  return r;
}
#else
#include <time.h>
#include <stdint.h>
#include "compat/compat.h"
#if !defined(__DragonFly_version) || __DragonFly_version < 500907
// clock_nanosleep is unavailable on DragonFly BSD and Mac OS X
int clock_nanosleep(clockid_t clockid, int flags, const struct timespec *request,
                    struct timespec *remain){
  struct timespec now;
  if(clock_gettime(clockid, &now)){
    return -1;
  }
  uint64_t nowns = timespec_to_ns(&now);
  uint64_t targns = timespec_to_ns(request);
  if(flags != TIMER_ABSTIME){
    targns += nowns;
  }
  if(nowns < targns){
    uint64_t waitns = targns - nowns;
    struct timespec waitts = {
      .tv_sec = waitns / 1000000000,
      .tv_nsec = waitns % 1000000000,
    };
    return nanosleep(&waitts, remain);
  }
  return 0;

}
#endif
#endif
#endif
#endif
