#include <unistd.h>
#include "internal.h"

ncfdplane* ncfdplane_create(ncplane* n, const ncfdplane_options* opts, int fd,
                            ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn){
  if(fd < 0 || !cbfxn || !donecbfxn){
    return NULL;
  }
  ncfdplane* ret = malloc(sizeof(*ret));
  if(ret){
    ret->cb = cbfxn;
    ret->donecb = donecbfxn;
    ret->follow = opts->follow;
    ret->ncp = n;
    ret->fd = fd;
  }
  return ret;
}

int ncfdplane_destroy(ncfdplane* n){
  int ret = 0;
  if(n){
    ret = close(n->fd);
    free(n);
  }
  return ret;
}

ncsubproc* ncsubproc_createv(ncplane* n, const ncsubproc_options* opts,
                             const char* bin,  char* const arg[],
                             ncfdplane_callback cbfxn, ncfdplane_done_cb donecbfxn){
  if(!cbfxn || !donecbfxn){
    return NULL;
  }
  int fd = -1;
  ncsubproc* ret = malloc(sizeof(*ret));
  if(ret){
    // FIXME launch process, create ncfdplane with pipe
    if((ret->nfp = ncfdplane_create(n, &opts->popts, fd, cbfxn, donecbfxn)) == NULL){
      // FIXME kill process
      free(ret);
      return NULL;
    }
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
  if(ret){
    // FIXME launch process, create ncfdplane with pipe
    if((ret->nfp = ncfdplane_create(n, &opts->popts, fd, cbfxn, donecbfxn)) == NULL){
      // FIXME kill process
      free(ret);
      return NULL;
    }
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
  if(ret){
    // FIXME launch process, create ncfdplane with pipe
    if((ret->nfp = ncfdplane_create(n, &opts->popts, fd, cbfxn, donecbfxn)) == NULL){
      // FIXME kill process
      free(ret);
      return NULL;
    }
  }
  return ret;
}

int ncsubproc_destroy(ncsubproc* n){
  if(n){
    free(n);
  }
  return 0;
}
