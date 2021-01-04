#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

#include "macos-compat.h"

// Time code based on: https://github.com/ChisholmKyle/PosixMachTiming/tree/master/src
static inline void time_diff (const struct timespec* in, struct timespec* out)
{
  out->tv_sec = in->tv_sec - out->tv_sec;
  out->tv_nsec = in->tv_nsec - out->tv_nsec;

  if (out->tv_sec < 0) {
    out->tv_sec = 0;
    out->tv_nsec = 0;
    return;
  }

  if (out->tv_nsec > 0) {
    return;
  }

  if (out->tv_sec == 0) {
    out->tv_sec = 0;
    out->tv_nsec = 0;
  } else {
    out->tv_sec = out->tv_sec - 1;
    out->tv_nsec = out->tv_nsec + 1000000000;
  }
}

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
int clock_nanosleep (int clockid, int flags, const struct timespec *request, struct timespec *remain)
{
  struct timespec delta;
  int rv = clock_gettime (CLOCK_MONOTONIC, &delta);
  if (rv == 0) {
    time_diff (request, &delta);
    rv = nanosleep (&delta, NULL);
  }

  return rv;
}
#pragma clang diagnostic pop

static inline int set_fd_flags (int fd, int flags)
{
  if ((flags & O_CLOEXEC) == O_CLOEXEC) {
    int ret = fcntl (fd, F_SETFD, FD_CLOEXEC);
    if (ret != 0) {
      return ret;
    }
  }

  if ((flags & O_NONBLOCK) == O_NONBLOCK) {
    return fcntl (fd, F_SETFL, O_NONBLOCK);
  }

  return 0;
}

int pipe2 (int pipefd[2], int flags)
{
  int ret = pipe (pipefd);
  if (ret != 0) {
    return ret;
  }

  ret = set_fd_flags (pipefd[0], flags);
  if (ret != 0) {
    return ret;
  }

  return set_fd_flags (pipefd[1], flags);
}

static pthread_mutex_t ppoll_lock;

int ppoll (struct pollfd *fds, nfds_t nfds, const struct timespec *tmo_p, const sigset_t *sigmask)
{
  pthread_mutex_lock (&ppoll_lock);

  sigset_t origmask;
  int timeout = (tmo_p == NULL) ? -1 : (tmo_p->tv_sec * 1000 + tmo_p->tv_nsec / 1000000);
  pthread_sigmask (SIG_SETMASK, sigmask, &origmask);
  int ret= poll (fds, nfds, timeout);
  pthread_sigmask (SIG_SETMASK, &origmask, NULL);

  pthread_mutex_unlock (&ppoll_lock);

  return ret;
}

void macos_init ()
{
  pthread_mutex_init (&ppoll_lock, NULL);
}
