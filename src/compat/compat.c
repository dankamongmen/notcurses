#include "compat/compat.h"
#include <pthread.h>
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
int set_fd_cloexec(int fd, unsigned state, unsigned* oldstate){ // FIXME
  (void)fd;
  (void)state;
  (void)oldstate;
  return 0;
}
pid_t waitpid(pid_t pid, int* state, int options){ // FIXME
  (void)options;
  (void)pid;
  (void)state;
  /*
  WaitForSingleObject(pid, INFINITE);
  long unsigned pstate;
  GetExitCodeProcess(pid, &pstate);
  *state = pstate;
  CloseHandle(pid);
  return pid;
  */
  return 0;
}
#else // not windows
#include <time.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
int set_fd_nonblocking(int fd, unsigned state, unsigned* oldstate){
  int flags = fcntl(fd, F_GETFL);
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

int set_fd_cloexec(int fd, unsigned state, unsigned* oldstate){
  int flags = fcntl(fd, F_GETFD);
  if(flags < 0){
    return -1;
  }
  if(oldstate){
    *oldstate = flags & O_CLOEXEC;
  }
  if(state){
    if(flags & O_CLOEXEC){
      return 0;
    }
    flags |= O_CLOEXEC;
  }else{
    if(!(flags & O_CLOEXEC)){
      return 0;
    }
    flags &= ~O_CLOEXEC;
  }
  if(fcntl(fd, F_SETFD, flags)){
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

// initializes a pthread_cond_t to use CLOCK_MONOTONIC (as opposed to the
// default CLOCK_REALTIME) if possible. if not possible, initializes a
// regular ol' CLOCK_REALTIME condvar.
int pthread_condmonotonic_init(pthread_cond_t* cond){
  pthread_condattr_t cat;
  if(pthread_condattr_init(&cat)){
    return -1;
  }
#ifndef __APPLE__
  if(pthread_condattr_setclock(&cat, CLOCK_MONOTONIC)){
    pthread_condattr_destroy(&cat);
    return -1;
  }
#endif
  if(pthread_cond_init(cond, &cat)){
    pthread_condattr_destroy(&cat);
    return -1;
  }
  pthread_condattr_destroy(&cat);
  return 0;
}
