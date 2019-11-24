#include <stdio.h>
#include "egcpool.h"

#define POOL_MINIMUM_ALLOC BUFSIZ

int egcpool_grow(egcpool* pool, size_t len, bool force){
  const size_t poolfree = pool->poolsize - pool->poolused;
  // proactively get more space if we have less than 10% free. this doesn't
  // guarantee that we'll have enough space to insert the string -- we could
  // theoretically have every 10th byte free, and be unable to write even a
  // two-byte egc -- so we might have to allocate after an expensive search :/.
  if(poolfree >= len && poolfree * 10 > pool->poolsize && !force){
    return 0;
  }
  size_t newsize = pool->poolsize * 2;
  if(newsize < POOL_MINIMUM_ALLOC){
    newsize = POOL_MINIMUM_ALLOC;
  }
  while(len > newsize - pool->poolsize){ // ensure we make enough space
    newsize *= 2;
  }
  // offsets only have 24 bits available...
  if(newsize >= 1u << 23u){
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
