#include "internal.h"

ncsubproc* ncsubproc_createv(ncplane* n, const ncsubproc_options* opts,
                             const char* bin,  char* const arg[]){
}

ncsubproc* ncsubproc_createvp(ncplane* n, const ncsubproc_options* opts,
                              const char* bin,  char* const arg[]){
}

ncsubproc* ncsubproc_createvpe(ncplane* n, const ncsubproc_options* opts,
                               const char* bin,  char* const arg[], char* const env[]){
}

int ncsubproc_destroy(ncsubproc* n){
  if(n){
    free(n);
  }
  return 0;
}
