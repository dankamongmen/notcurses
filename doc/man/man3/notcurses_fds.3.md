% notcurses_fds(3)
% nick black <nickblack@linux.com>
% v1.3.3

# NAME

notcurses_fds - dumping file descriptors and subprocesses to planes

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
struct ncplane;
struct ncfdplane;
struct ncsubproc;

typedef struct ncfdplane_options {
  void* curry; // parameter provided to callbacks
  bool follow; // keep reading after hitting end?
} ncfdplane_options;

typedef struct ncsubproc_options {
  ncfdplane_options popts;
  uint64_t restart_period;  // restart after exit
} ncsubproc_options;
```

**typedef int(*ncfdplane_callback)(struct ncfdplane* n, const void* buf, size_t s, void* curry);**

**typedef int(*ncfdplane_done_cb)(struct ncfdplane* n, int fderrno, void* curry);**

**struct ncfdplane* ncfdplane_create(struct ncplane* n, const ncfdplane_options* opts, int fd, ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn);**

**struct ncplane* ncfdplane_plane(struct ncfdplane* n);**

**int ncfdplane_destroy(struct ncfdplane* n);**

**struct ncsubproc* ncsubproc_createv(struct ncplane* n, const ncsubproc_options* opts, const char* bin,  char* const arg[], ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn);**

**struct ncsubproc* ncsubproc_createvp(struct ncplane* n, const ncsubproc_options* opts, const char* bin,  char* const arg[], ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn);**

**struct ncsubproc* ncsubproc_createvpe(struct ncplane* n, const ncsubproc_options* opts, const char* bin,  char* const arg[], char* const env[], ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn);**

**int ncsubproc_destroy(struct ncsubproc* n);**

# DESCRIPTION

These widgets cause a file descriptor to be read until EOF, and written to a
scrolling **ncplane**. The reading will take place in a notcurses-managed
context (the particulars of this context are not defined, and should not be
depended upon), which will invoke the provided callbacks with the data read.
Essentially, they are simply portable interfaces to asynchronous reading, with
**ncsubproc** also providing subprocess management.

If **ncsubproc_destroy** is called before the subprocess has exited, it will
be sent a SIGKILL. If **ncsubproc_destroy** or **ncfdplane_destroy** is called
while a callback is being invoked, the destroy function will block until the
callback is done being invoked. If a user callback returns non-0, the calling
object will destroy itself.

# NOTES

# RETURN VALUES

# SEE ALSO

**pidfd_open(2)**,
**notcurses(3)**,
**notcurses_ncplane(3)**
