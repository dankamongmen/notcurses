#include <lib/egcpool.h>

// if we're inserting a EGC of |len| bytes, ought we proactively realloc?
__attribute__ ((nonnull (1))) static inline bool
egcpool_alloc_justified(const egcpool* pool, int len){
  const int poolfree = pool->poolsize - pool->poolused;
  // proactively get more space if we have less than 10% free. this doesn't
  // guarantee that we'll have enough space to insert the string -- we could
  // theoretically have every 10th byte free, and be unable to write even a
  // two-byte egc -- so we might have to allocate after an expensive search :/.
  if(poolfree >= len && poolfree * 10 > pool->poolsize){
    return false;
  }
  return true;
}

__attribute__ ((nonnull (1, 2))) static inline int
stash(egcpool* pool, const char* egc, int curpos, int len){
  memcpy(pool->pool + curpos, egc, len - 1);
  pool->pool[curpos + len - 1] = '\0';
  pool->poolwrite = curpos + len;
  pool->poolused += len;
//fprintf(stderr, "Stashing AT %d\n", curpos);
  return curpos;
}

__attribute__ ((nonnull (1, 2))) int
egcpool_stash(egcpool* pool, const char* egc, size_t ulen){
  int len = ulen + 1; // count the NUL terminator
  if(len <= 2){ // should never be empty, nor a single byte + NUL
    return -1;
  }
  // the first time through, we don't force a grow unless we expect ourselves
  // to have too little space. once we've done a search, we do force the grow.
  // we should thus never have more than two iterations of this loop.
  bool searched = false;
  // we might have to realloc our underlying pool. it is possible that this EGC
  // is actually *in* that pool, in which case our pointer will be invalidated.
  // to be safe, duplicate prior to a realloc, and free along all paths.
  char* duplicated = NULL;
  do{
    if(egcpool_alloc_justified(pool, len) || searched){
      if(!duplicated){
        // cast (and avoidance of strndup) to facilitate c++ inclusions
        if((duplicated = (char *)malloc(ulen + 1)) == NULL){
          return -1;
        }
        memcpy(duplicated, egc, ulen);
        duplicated[ulen] = '\0';
      }
      if(egcpool_grow(pool, len) && searched){
        free(duplicated);
        return -1;
      }
      egc = duplicated;
    }
    // we now look for a place to lay out this egc. we need |len| zeroes in a
    // row. starting at pool->poolwrite, look for such a range of unused
    // memory. if we find it, write it out, and update used count. if we come
    // back to where we started, force a growth and try again.
    int curpos = pool->poolwrite;
//fprintf(stderr, "Stashing [%s] %d starting at %d\n", egc, len, curpos);
    do{
      if(curpos == pool->poolsize){
        curpos = 0;
      }
      if(pool->pool[curpos]){ // can't write if there's stuff here
        ++curpos;
      }else if(curpos && pool->pool[curpos - 1]){ // don't kill someone's NUL
        ++curpos;
      }else if(pool->poolsize - curpos < len){ // can't wrap around
        if(pool->poolwrite > curpos){
          break;
        }
        curpos = 0; // can this skip pool->poolwrite?
      }else{ // promising! let's see if there's enough space
        int need = len;
        size_t trial = curpos;
        while(--need){
          if(pool->pool[++trial]){ // alas, not enough space here
            break;
          }
        }
        if(need == 0){ // found a suitable space, copy it!
          stash(pool, egc, curpos, len);
          free(duplicated);
          return curpos;
        }
        if(pool->poolwrite > curpos && pool->poolwrite - (len - need) < curpos){
          break;
        }
        curpos += len - need;
      }
    }while(curpos != pool->poolwrite);
  }while( (searched = !searched) );
  free(duplicated);
  logerror("error finding egcpool writepos (%zu)", ulen);
  return -1; // should never get here
}
