#include <sys/mman.h>
#include "visual-details.h"
#include "internal.h"
#include "png.h"

size_t compute_png_size(const ncvisual* ncv){
  return 0;
}

// write a PNG at the provided buffer using the ncvisual
int create_png(const ncvisual* ncv, void* buf, size_t* bsize){
  // FIXME fill it in
  return -1;
}

static inline size_t
mmap_round_size(size_t s){
  const size_t pgsize = 4096; // FIXME get page size, round up s
  return (s + pgsize - 1) / pgsize * pgsize;
}

// write a PNG, creating the buffer ourselves. it must be munmapped. the
// resulting length is written to *bsize on success. returns MMAP_FAILED
// on a failure.
void* create_png_mmap(const ncvisual* ncv, size_t* bsize){
  *bsize = compute_png_size(ncv);
  *bsize = mmap_round_size(*bsize);
  if(*bsize == 0){
    return MAP_FAILED;
  }
  // FIXME hugetlb?
  void* map = mmap(NULL, *bsize, PROT_WRITE|PROT_READ,
                   MAP_SHARED_VALIDATE|MAP_ANONYMOUS, -1, 0);
  if(map == MAP_FAILED){
    logerror("Couldn't get %zuB map\n", *bsize);
    return MAP_FAILED;
  }
  if(create_png(ncv, map, bsize)){
    munmap(map, *bsize);
    return MAP_FAILED;
  }
  return map;
}
