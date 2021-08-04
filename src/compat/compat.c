#include "compat/compat.h"
#ifdef  __MINGW64__
#include <string.h>
#include <stdlib.h>
#include <synchapi.h>
#include <handleapi.h>
#include <processthreadsapi.h>
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
int set_fd_nonblocking(int fd, unsigned state, unsigned* oldstate){ // FIXME
  (void)fd;
  (void)state;
  (void)oldstate;
  return 0;
}
#define INFINITE 0xffffffff // FIXME
pid_t waitpid(pid_t pid, int* state, int options){
  (void)options; // FIXME honor WNOHANG
  WaitForSingleObject(pid, INFINITE);
  GetExitCodeProcess(pid, state);
  CloseHandle(pid);
  return pid;
}
#else // not windows
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
int set_fd_nonblocking(int fd, unsigned state, unsigned* oldstate){
  int flags = fcntl(fd, F_GETFL, 0);
  if(flags < 0){
    return -1;
  }
  if(oldstate){
    *oldstate = flags & O_NONBLOCK;
  }
  if(state){
    if(flags & O_NONBLOCK){
      return 0;
    }
    flags |= O_NONBLOCK;
  }else{
    if(!(flags & O_NONBLOCK)){
      return 0;
    }
    flags &= ~O_NONBLOCK;
  }
  if(fcntl(fd, F_SETFL, flags)){
    return -1;
  }
  return 0;
}
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
