#ifndef NOTCURSES_EGCPOOL
#define NOTCURSES_EGCPOOL

#include <wchar.h>
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "notcurses/notcurses.h"

#ifdef __cplusplus
extern "C" {
#endif

// cells only provide storage for a single 7-bit character. if there's anything
// more than that, it's spilled into the egcpool, and the cell is given an
// offset. when a cell is released, the memory it owned is zeroed out, and
// recognizable as use for another cell.

typedef struct egcpool {
  char* pool;         // ringbuffer of attached extension storage
  int poolsize;       // total number of bytes in pool
  int poolused;       // bytes actively used, grow when this gets too large
  int poolwrite;      // next place to *look for* a place to write
} egcpool;

#define POOL_MINIMUM_ALLOC BUFSIZ
#define POOL_MAXIMUM_BYTES (1u << 30u) // max 1GB

static inline void
egcpool_init(egcpool* p){
  memset(p, 0, sizeof(*p));
}

static inline int
egcpool_grow(egcpool* pool, size_t len){
  size_t newsize = pool->poolsize * 2;
  if(newsize < POOL_MINIMUM_ALLOC){
    newsize = POOL_MINIMUM_ALLOC;
  }
  while(len > newsize - pool->poolsize){ // ensure we make enough space
    newsize *= 2;
  }
  if(newsize > POOL_MAXIMUM_BYTES){
    return -1;
  }
  // nasty cast here because c++ source might include this header :/
  char* tmp = (char*)realloc(pool->pool, newsize);
  if(tmp == NULL){
    return -1;
  }
  pool->pool = tmp;
  memset(pool->pool + pool->poolsize, 0, newsize - pool->poolsize);
  pool->poolsize = newsize;
  return 0;
}

// Eat an EGC from the UTF-8 string input. This consists of extracting a
// multibyte via mbrtowc, then continuing to extract any which have zero
// width until hitting another spacing character or a NUL terminator. Writes
// the number of columns occupied to '*colcount'. Returns the number of bytes
// consumed, not including any NUL terminator. Note that neither the number
// of bytes nor columns is necessarily equivalent to the number of decoded code
// points. Such are the ways of Unicode.
static inline int
utf8_egc_len(const char* gcluster, int* colcount){
  size_t ret = 0;
  *colcount = 0;
  wchar_t wc;
  int r;
  mbstate_t mbt;
  memset(&mbt, 0, sizeof(mbt));
  do{
    r = mbrtowc(&wc, gcluster, MB_CUR_MAX, &mbt);
    if(r < 0){
      return -1;
    }else if(r){
      int cols = wcwidth(wc);
      if(cols){
        if(*colcount){ // this must be starting a new EGC, exit and do not claim
          break;
        }
        *colcount += cols;
      }
      ret += r;
      gcluster += r;
    }
  }while(r);
  return ret;
}

// if we're inserting a EGC of |len| bytes, ought we proactively realloc?
static inline bool
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

// stash away the provided UTF8, NUL-terminated grapheme cluster. the cluster
// should not be less than 2 bytes (such a cluster should be directly stored in
// the cell). returns -1 on error, and otherwise a non-negative offset. 'ulen'
// must be the number of bytes to lift from egc (utf8_egc_len()).
static inline int
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
        duplicated = strdup(egc);
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
          memcpy(pool->pool + curpos, egc, len - 1);
          pool->pool[curpos + len - 1] = '\0';
          pool->poolwrite = curpos + len;
          pool->poolused += len;
          free(duplicated);
//fprintf(stderr, "Stashing AT %d\n", curpos);
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
  assert(false);
  return -1; // should never get here
}

// Run a consistency check on the offset; ensure it's a valid, non-empty EGC.
static inline bool
egcpool_check_validity(const egcpool* pool, int offset){
  if(offset >= pool->poolsize){
    fprintf(stderr, "Offset 0x%06x greater than size (%d)\n", offset, pool->poolsize);
    return false;
  }
  const char* egc = pool->pool + offset;
  if(*egc == '\0'){
    fprintf(stderr, "Bad offset 0x%06x: empty\n", offset);
    return false;
  }
  mbstate_t mbstate;
  memset(&mbstate, 0, sizeof(mbstate));
  do{
    wchar_t wcs;
    int r = mbrtowc(&wcs, egc, strlen(egc), &mbstate);
    if(r < 0){
      fprintf(stderr, "Invalid UTF8 at offset 0x%06x, len %zu [%s]\n",
              offset, strlen(egc), strerror(errno));
      return false;
    }
    egc += r;
  }while(*egc);
  return true;
}

// remove the egc from the pool. start at offset, and zero out everything until
// we find a zero (our own NUL terminator). remove that number of bytes from
// the usedcount.
static inline void
egcpool_release(egcpool* pool, int offset){
  size_t freed = 1; // account for free(d) NUL terminator
  assert(egcpool_check_validity(pool, offset));
  while(pool->pool[offset]){
    pool->pool[offset] = '\0';
    ++freed;
    ++offset;
    assert(offset < pool->poolsize);
  }
  pool->poolused -= freed;
  // FIXME ought we update pool->poolwrite?
}

static inline void
egcpool_dump(egcpool* pool){
  free(pool->pool);
  pool->pool = NULL;
  pool->poolsize = 0;
  pool->poolwrite = 0;
  pool->poolused = 0;
}

static inline const char*
egcpool_extended_gcluster(const egcpool* pool, const cell* c){
  uint32_t idx = cell_egc_idx(c);
  return pool->pool + idx;
}

// Duplicate the contents of EGCpool 'src' onto another, wiping out any prior
// contents in 'dst'.
static inline int
egcpool_dup(egcpool* dst, const egcpool* src){
  char* tmp;
  if((tmp = (char*)realloc(dst->pool, src->poolsize)) == NULL){
    return -1;
  }
  dst->pool = tmp;
  dst->poolsize = src->poolsize;
  dst->poolused = src->poolused;
  dst->poolwrite = src->poolwrite;
  memcpy(dst->pool, src->pool, src->poolsize);
  return 0;
}

#ifdef __cplusplus
}
#endif

#endif
