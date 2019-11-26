#include <stdio.h>
#include "egcpool.h"

#define POOL_MINIMUM_ALLOC BUFSIZ

int egcpool_grow(egcpool* pool, size_t len){
  size_t newsize = pool->poolsize * 2;
  if(newsize < POOL_MINIMUM_ALLOC){
    newsize = POOL_MINIMUM_ALLOC;
  }
  while(len > newsize - pool->poolsize){ // ensure we make enough space
    newsize *= 2;
  }
  // offsets only have 24 bits available...
  if(newsize >= 1u << 24u){
    return -1;
  }
  typeof(*pool->pool)* tmp = realloc(pool->pool, newsize);
  if(tmp == NULL){
    return -1;
  }
  pool->pool = tmp;
  memset(pool->pool + pool->poolsize, 0, newsize - pool->poolsize);
  pool->poolsize = newsize;
  return 0;
}
