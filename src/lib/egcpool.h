#ifndef NOTCURSES_EGCPOOL
#define NOTCURSES_EGCPOOL

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// cells only provide storage for a single 7-bit character. if there's anything
// more than that, it's spilled into the egcpool, and the cell is given an
// offset. when a cell is released, the memory it owned is zeroed out, and
// recognizable as use for another cell.

typedef struct egcpool {
  char* pool;            // ringbuffer of attached extension storage
  size_t poolsize;       // total number of bytes in pool
  size_t poolused;       // bytes actively used, grow when this gets too large
  size_t poolwrite;      // next place to *look for* a place to write
} egcpool;

static inline void
egcpool_init(egcpool* p){
  memset(p, 0, sizeof(*p));
}

int egcpool_grow(egcpool* pool, size_t len, bool force);

// stash away the provided UTF8, NUL-terminated grapheme cluster. the cluster
// should not be less than 2 bytes (such a cluster should be directly stored in
// the cell). returns -1 on error, and otherwise a non-negative 24-bit offset.
static inline int
egcpool_stash(egcpool* pool, const char* egc){
  size_t len = strlen(egc) + 1; // count the NUL terminator
  if(len <= 2){ // should never be empty, nor a single byte + NUL
    return -1;
  }
  // the first time through, we don't force a grow unless we expect ourselves
  // to have too little space. once we've done a search, we do force the grow.
  // we should thus never have more than two iterations of this loop.
  bool searched = false;
  do{
    if(egcpool_grow(pool, len, false)){
      return -1;
    }
    // we now look for a place to lay out this egc. we need |len| zeroes in a
    // row. starting at pool->poolwrite, look for such a range of unused
    // memory. if we find it, write it out, and update used count. if we come
    // back to where we started, force a growth and try again.
    size_t curpos = pool->poolwrite; 
    do{
      if(curpos == pool->poolsize){
        curpos = 0;
      }
      if(pool->pool[curpos]){ // can't write if there's stuff here
        ++curpos;
      }else{ // promising! let's see if there's enough space
        size_t need = len;
        size_t trial = curpos;
        while(--need){
          if(++trial == pool->poolsize){
            trial = 0;
          }
          if(pool->pool[trial]){ // alas, not enough space here
            break;
          }
        }
        if(need == 0){ // found a suitable space, copy it!
          if(pool->poolsize - len > curpos){ // one chunk
            memcpy(pool->pool + curpos, egc, len);
            pool->poolwrite = curpos + len;
          }else{ // two chunks
            // FIXME are clients prepared for split egcs? i doubt it...
            size_t fchunk = pool->poolsize - curpos - 1;
            memcpy(pool->pool + curpos, egc, fchunk);
            memcpy(pool->pool, egc + fchunk, len - fchunk);
            pool->poolwrite = len - fchunk;
          }
          pool->poolused += len;
          return curpos;
        }
        curpos += len - need; // do we always hit pool->poolwrite properly?
      }
    }while(curpos != pool->poolwrite); 
  }while( (searched = !searched) );
  return -1; // should never get here
}

// remove the egc from the pool. start at offset, and zero out everything until
// we find a zero (our own NUL terminator). remove that number of bytes from
// the usedcount.
static inline void
egcpool_release(egcpool* pool, size_t offset){
  size_t freed = 1; // account for free(d) NUL terminator
  while(pool->pool[offset]){
    pool->pool[offset] = '\0';
    ++freed;
    if(++offset == pool->poolsize){
      offset = 0;
    }
  }
  pool->poolused -= freed;
  // FIXME ought we update pool->poolwrite?
}

static inline void
egcpool_dump(egcpool* pool){
  free(pool->pool);
  pool->poolsize = 0;
  pool->poolwrite = 0;
}

#ifdef __cplusplus
}
#endif

#endif
