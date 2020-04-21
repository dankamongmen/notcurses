#include <fcntl.h>
#include <unistd.h>
#include <asm/unistd.h>
#include <pthread.h>
#include <sys/wait.h>
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

static void *
ncfdplane_thread(void* vncfp){
  ncfdplane* ncfp = vncfp;
  char* buf = malloc(BUFSIZ);
  ssize_t r;
  while((r = read(ncfp->fd, buf, BUFSIZ)) >= 0){
    if(r == 0){
      break;
    }
    if( (r = ncfp->cb(ncfp, buf, r, ncfp->curry)) ){
      break;
    }
  }
  // FIXME need to continue reading on pipe/socket
  if(r <= 0){
    ncfp->donecb(ncfp, r == 0 ? 0 : errno, ncfp->curry);
  }
  free(buf);
  if(ncfp->destroyed){
    ncfdplane_destroy_inner(ncfp);
  }
  return NULL;
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

static pid_t
launch_pipe_process(int* pipe, int* pidfd){
  int pipes[2];
  if(pipe2(pipes, O_CLOEXEC)){
    return -1;
  }
  struct clone_args clargs = {
    .flags = CLONE_CLEAR_SIGHAND | CLONE_FS | CLONE_PIDFD,
    .pidfd = (uintptr_t)pidfd,
    .child_tid = 0, // FIXME
    .parent_tid = 0, // FIXME
    .exit_signal = SIGCHLD,  // FIXME maybe switch it up for doctest?
    .stack = 0,           // automatically created by clone3()
    .stack_size = 0,         // automatically set by clone3()
    .tls = 0, // FIXME
    .set_tid = 0, // FIXME
    .set_tid_size = 0, // FIXME
  };
  pid_t p = syscall(__NR_clone3, &clargs, sizeof(clargs));
  if(p == 0){
    if(dup2(pipes[1], STDOUT_FILENO) < 0 || dup2(pipes[1], STDERR_FILENO) < 0){
      return -1;
    }
  }else{
    *pipe = pipes[0];
  }
  return p;
}

static int
kill_and_wait_subproc(pid_t pid){
  kill(pid, SIGTERM);
  int status;
  waitpid(pid, &status, 0); // FIXME rigorurize this up
  return 0;
}

// need a poll on both main fd and pidfd
static void *
ncsubproc_thread(void* vncsp){
  ncsubproc* ncsp = vncsp;
  char* buf = malloc(BUFSIZ);
  ssize_t r;
  while((r = read(ncsp->nfp->fd, buf, BUFSIZ)) >= 0){
    if(r == 0){
      break;
    }
    if( (r = ncsp->nfp->cb(ncsp->nfp, buf, r, ncsp->nfp->curry)) ){
      break;
    }
  }
  // FIXME need to continue reading on pipe/socket
  if(r <= 0){
    ncsp->nfp->donecb(ncsp->nfp, r == 0 ? 0 : errno, ncsp->nfp->curry);
  }
  free(buf);
  if(ncsp->nfp->destroyed){
    ncfdplane_destroy_inner(ncsp->nfp);
    free(ncsp);
  }
  return NULL;
}

static ncfdplane*
ncsubproc_launch(ncplane* n, ncsubproc* ret, const ncsubproc_options* opts, int fd,
                 ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn){
  ret->nfp = ncfdplane_create_internal(n, &opts->popts, fd, cbfxn, donecbfxn, false);
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
    kill_and_wait_subproc(ret->pid);
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
    exit(EXIT_FAILURE);
  }else if(ret->pid < 0){
    free(ret);
    return NULL;
  }
  if((ret->nfp = ncsubproc_launch(n, ret, opts, fd, cbfxn, donecbfxn)) == NULL){
    kill_and_wait_subproc(ret->pid);
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
    kill_and_wait_subproc(ret->pid);
    free(ret);
    return NULL;
  }
  return ret;
}

int ncsubproc_destroy(ncsubproc* n){
  if(n){
    free(n);
  }
  return 0;
}
