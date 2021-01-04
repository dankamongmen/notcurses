#if !defined(NOTCURSES_MACOS_COMPAT_H)
#define NOTCURSES_MACOS_COMPAT_H

#include <poll.h>

#if !defined(TIMER_ABSTIME)
// Value doesn't matter, mac builds don't use it anyway
#define TIMER_ABSTIME 0
#endif

#if defined(__cplusplus)
extern "C" {
#endif

int clock_nanosleep (int clockid, int flags, const struct timespec *request, struct timespec *remain);
int pipe2 (int pipefd[2], int flags);
int ppoll (struct pollfd *fds, nfds_t nfds, const struct timespec *tmo_p, const sigset_t *sigmask);
void macos_init ();

#if defined(__cplusplus)
}
#endif

#endif // NOTCURSES_MACOS_COMPAT_H