#include <fcntl.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <pthread.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include "internal.h"

// release the memory and fd, but don't join the thread (since we might be
// getting called within the thread's context, on a callback).
static int
ncfdplane_destroy_inner(ncfdplane* n){
  int ret = close(n->fd);
  free(n);
  return ret;
}

// if pidfd is < 0, it won't be used in the poll()
static void
fdthread(ncfdplane* ncfp, int pidfd){
  struct pollfd pfds[2];
  memset(pfds, 0, sizeof(pfds));
  char* buf = malloc(BUFSIZ);
  int pevents;
  pfds[0].fd = ncfp->fd;
  pfds[0].events = POLLIN;
  const int fdcount = pidfd < 0 ? 1 : 2;
  if(fdcount > 1){
    pfds[1].fd = pidfd;
    pfds[1].events = POLLIN;
  }
  ssize_t r = 0;
  while((pevents = poll(pfds, fdcount, -1)) >= 0 || errno == EINTR){
    if(pfds[0].revents & POLLIN){
      while((r = read(ncfp->fd, buf, BUFSIZ)) >= 0){
        if(r == 0){
          break;
        }
        if( (r = ncfp->cb(ncfp, buf, r, ncfp->curry)) ){
          break;
        }
        if(ncfp->destroyed){
          break;
        }
      }
      // if we're not doing follow, break out on a zero-byte read
      if(r == 0 && !ncfp->follow){
        break;
      }
    }
    if(fdcount > 1 && pfds[1].revents & POLLIN){
      r = 0;
      break;
    }
  }
  if(r <= 0 && !ncfp->destroyed){
    ncfp->donecb(ncfp, r == 0 ? 0 : errno, ncfp->curry);
  }
  free(buf);
}

static void *
ncfdplane_thread(void* vncfp){
  fdthread(vncfp, -1);
  return NULL;
}

static int
set_fd_nonblocking(int fd){
  int flags = fcntl(fd, F_GETFL, 0);
  if(flags < 0){
    return -1;
  }
  if(flags & O_NONBLOCK){
    return 0;
  }
  flags |= O_NONBLOCK;
  if(fcntl(fd, F_SETFL, flags)){
    return -1;
  }
  return 0;
}

static ncfdplane*
ncfdplane_create_internal(ncplane* n, const ncfdplane_options* opts, int fd,
                          ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn,
                          bool thread){
  ncfdplane* ret = malloc(sizeof(*ret));
  if(ret == NULL){
    return ret;
  }
  ret->cb = cbfxn;
  ret->donecb = donecbfxn;
  ret->follow = opts->follow;
  ret->ncp = n;
  ret->destroyed = false;
  ncplane_set_scrolling(ret->ncp, true);
  ret->fd = fd;
  ret->curry = opts->curry;
  if(thread){
    if(pthread_create(&ret->tid, NULL, ncfdplane_thread, ret)){
      free(ret);
      return NULL;
    }
  }
  return ret;
}

ncfdplane* ncfdplane_create(ncplane* n, const ncfdplane_options* opts, int fd,
                            ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn){
  if(fd < 0 || !cbfxn || !donecbfxn){
    return NULL;
  }
  return ncfdplane_create_internal(n, opts, fd, cbfxn, donecbfxn, true);
}

ncplane* ncfdplane_plane(ncfdplane* n){
  return n->ncp;
}

int ncfdplane_destroy(ncfdplane* n){
  int ret = 0;
  if(n){
    if(pthread_equal(pthread_self(), n->tid)){
      n->destroyed = true; // ncfdplane_destroy_inner() is called on thread exit
    }else{
      void* vret = NULL;
      pthread_cancel(n->tid);
      ret |= pthread_join(n->tid, &vret);
      ret |= ncfdplane_destroy_inner(n);
    }
  }
  return ret;
}

// ncsubproc creates a pipe, retaining the read end. it clone()s a subprocess,
// getting a pidfd. the subprocess dup2()s the write end of the pipe onto file
// descriptors 1 and 2, exec()s, and begins running. the parent creates an
// ncfdplane around the read end, involving creation of a new thread. the
// parent then returns.

// FIXME introduced in linux 5.5
#ifndef CLONE_CLEAR_SIGHAND
#define CLONE_CLEAR_SIGHAND 0x100000000ULL
#endif

static pid_t
launch_pipe_process(int* pipe, int* pidfd){
  int pipes[2];
  if(pipe2(pipes, O_CLOEXEC)){ // can't use O_NBLOCK here (affects client)
    return -1;
  }
  struct clone_args clargs;
  memset(&clargs, 0, sizeof(clargs));
  clargs.pidfd = (uintptr_t)pidfd;
  clargs.flags = CLONE_CLEAR_SIGHAND | CLONE_FS | CLONE_PIDFD;
  pid_t p = syscall(__NR_clone3, &clargs, sizeof(clargs));
  if(p == 0){ // child
    if(dup2(pipes[1], STDOUT_FILENO) < 0 || dup2(pipes[1], STDERR_FILENO) < 0){
      fprintf(stderr, "Couldn't dup() %d (%s)\n", pipes[1], strerror(errno));
      raise(SIGKILL);
      exit(EXIT_FAILURE);
    }
  }else if(p > 0){ // parent
    *pipe = pipes[0];
    set_fd_nonblocking(*pipe);
  }
  return p;
}

// nuke the just-spawned process, and reap it. called before the subprocess
// reader thread is launched (which otherwise reaps the subprocess).
// FIXME rigourize and port this
static int
kill_and_wait_subproc(pid_t pid, int pidfd, int* status){
  syscall(__NR_pidfd_send_signal, pidfd, SIGKILL, NULL, 0);
  siginfo_t info;
  memset(&info, 0, sizeof(info));
  waitid(P_PIDFD, pidfd, &info, 0);
  // process ought be available immediately following waitid(), so supply
  // WNOHANG to avoid possible lockups due to weirdness
  if(pid != waitpid(pid, status, WNOHANG)){
    return -1;
  }
  return 0;
}

// need a poll on both main fd and pidfd
static void *
ncsubproc_thread(void* vncsp){
  int* status = malloc(sizeof(*status));
  if(status){
    ncsubproc* ncsp = vncsp;
    fdthread(ncsp->nfp, ncsp->pidfd);
    if(kill_and_wait_subproc(ncsp->pid, ncsp->pidfd, status)){
      *status = -1;
    }
    if(ncsp->nfp->destroyed){
      ncfdplane_destroy_inner(ncsp->nfp);
      free(ncsp);
    }
  }
  return status;
}

static ncfdplane*
ncsubproc_launch(ncplane* n, ncsubproc* ret, const ncsubproc_options* opts, int fd,
                 ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn){
  ncfdplane_options popts = {
    .curry = opts->curry,
    .follow = true,
  };
  ret->nfp = ncfdplane_create_internal(n, &popts, fd, cbfxn, donecbfxn, false);
  if(ret->nfp == NULL){
    return NULL;
  }
  if(pthread_create(&ret->nfp->tid, NULL, ncsubproc_thread, ret)){
    ncfdplane_destroy_inner(ret->nfp);
    ret->nfp = NULL;
  }
  return ret->nfp;
}

ncsubproc* ncsubproc_createv(ncplane* n, const ncsubproc_options* opts,
                             const char* bin,  char* const arg[],
                             ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn){
  if(!cbfxn || !donecbfxn){
    return NULL;
  }
  int fd = -1;
  ncsubproc* ret = malloc(sizeof(*ret));
  if(ret == NULL){
    return NULL;
  }
  ret->pid = launch_pipe_process(&fd, &ret->pidfd);
  if(ret->pid == 0){
    execv(bin, arg);
    fprintf(stderr, "Error execv()ing %s\n", bin);
    exit(EXIT_FAILURE);
  }else if(ret->pid < 0){
    free(ret);
    return NULL;
  }
  if((ret->nfp = ncsubproc_launch(n, ret, opts, fd, cbfxn, donecbfxn)) == NULL){
    kill_and_wait_subproc(ret->pid, ret->pidfd, NULL);
    free(ret);
    return NULL;
  }
  return ret;
}

ncsubproc* ncsubproc_createvp(ncplane* n, const ncsubproc_options* opts,
                              const char* bin,  char* const arg[],
                              ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn){
  if(!cbfxn || !donecbfxn){
    return NULL;
  }
  int fd = -1;
  ncsubproc* ret = malloc(sizeof(*ret));
  if(ret == NULL){
    return NULL;
  }
  ret->pid = launch_pipe_process(&fd, &ret->pidfd);
  if(ret->pid == 0){
    execvp(bin, arg);
    fprintf(stderr, "Error execv()ing %s\n", bin);
    raise(SIGKILL);
    exit(EXIT_FAILURE);
  }else if(ret->pid < 0){
    free(ret);
    return NULL;
  }
  if((ret->nfp = ncsubproc_launch(n, ret, opts, fd, cbfxn, donecbfxn)) == NULL){
    kill_and_wait_subproc(ret->pid, ret->pidfd, NULL);
    free(ret);
    return NULL;
  }
  return ret;
}

ncsubproc* ncsubproc_createvpe(ncplane* n, const ncsubproc_options* opts,
                       const char* bin,  char* const arg[], char* const env[],
                       ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn){
  if(!cbfxn || !donecbfxn){
    return NULL;
  }
  int fd = -1;
  ncsubproc* ret = malloc(sizeof(*ret));
  if(ret == NULL){
    return NULL;
  }
  ret->pid = launch_pipe_process(&fd, &ret->pidfd);
  if(ret->pid == 0){
#ifdef __FreeBSD__
    exect(bin, arg, env);
#else
    execvpe(bin, arg, env);
#endif
    fprintf(stderr, "Error execv()ing %s\n", bin);
    exit(EXIT_FAILURE);
  }else if(ret->pid < 0){
    free(ret);
    return NULL;
  }
  if((ret->nfp = ncsubproc_launch(n, ret, opts, fd, cbfxn, donecbfxn)) == NULL){
    kill_and_wait_subproc(ret->pid, ret->pidfd, NULL);
    free(ret);
    return NULL;
  }
  return ret;
}

// FIXME rigourize and port
int ncsubproc_destroy(ncsubproc* n){
  int ret = 0;
  if(n){
    void* vret = NULL;
    ret = syscall(__NR_pidfd_send_signal, n->pidfd, SIGKILL, NULL, 0);
    // the thread waits on the subprocess via pidfd, and then exits. don't try
    // to cancel the thread; rely on killing the subprocess.
    pthread_join(n->nfp->tid, &vret);
    free(n);
    if(vret == NULL){
      ret = -1;
    }else{
      ret = *(int*)vret;
      free(vret);
    }
  }
  return ret;
}

ncplane* ncsubproc_plane(ncsubproc* n){
  return n->nfp->ncp;
}
