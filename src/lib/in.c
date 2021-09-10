#include <stdio.h>
#include "internal.h"
#include "in.h"

static sig_atomic_t resize_seen;

// called for SIGWINCH and SIGCONT
void sigwinch_handler(int signo){
  resize_seen = signo;
}

// data collected from responses to our terminal queries.
typedef struct termqueries {
  int celly, cellx;     // cell geometry on startup
  int pixy, pixx;       // pixel geometry on startup
  int cursory, cursorx; // cursor location on startup
  unsigned kittygraphs; // are kitty graphics supported?
  int sixely, sixelx;   // maximum sixel size
  int cregs;            // sixel color registers
  unsigned appsync;     // application-sync supported?
} termqueries;

typedef struct cursorloc {
  int y, x;             // 0-indexed cursor location
} cursorloc;

// local state for the input thread
typedef struct inputctx {
  int termfd;           // terminal fd: -1 with no controlling terminal, or
                        //  if stdin is a terminal, and on Windows Terminal.
  int stdinfd;          // bulk in fd. always >= 0 (almost always 0). we do not
                        //  own this descriptor, and must not close() it.
#ifdef __MINGW64__
  HANDLE stdinhandle;   // handle to input terminal
#endif
  unsigned char ibuf[BUFSIZ]; // we dump raw reads into this ringbuffer, and
                              //  process them all post-read
  int ibufvalid;      // we mustn't read() if ibufvalid == sizeof(ibuf)
  int ibufwrite;      // we write here next
  int ibufread;       // first valid byte here (if any are valid)

  cursorloc* csrs;    // cursor reports are dumped here
  ncinput* inputs;    // processed input is dumped here
  int csize, isize;   // total number of slots in csrs/inputs
  int cvalid, ivalid; // population count of csrs/inputs
  int cwrite, iwrite; // slot where we'll write the next csr/input;
                      //  we cannot write if valid == size
  int cread, iread;   // slot from which clients read the next csr/input;
                      //  they cannot read if valid == 0
  tinfo* ti;          // link back to tinfo
  pthread_t tid;      // tid for input thread
} inputctx;

static inline inputctx*
create_inputctx(tinfo* ti, FILE* infp){
  inputctx* i = malloc(sizeof(*i));
  if(i){
    i->csize = 64;
    if( (i->csrs = malloc(sizeof(*i->csrs) * i->csize)) ){
      i->isize = BUFSIZ;
      if( (i->inputs = malloc(sizeof(*i->inputs) * i->isize)) ){
        if((i->stdinfd = fileno(infp)) >= 0){
          if(set_fd_nonblocking(i->stdinfd, 1, &ti->stdio_blocking_save) == 0){
            i->termfd = tty_check(i->stdinfd) ? -1 : get_tty_fd(infp);
            i->ti = ti;
            i->cvalid = i->ivalid = 0;
            i->cwrite = i->iwrite = 0;
            i->cread = i->iread = 0;
            i->ibufvalid = i->ibufwrite = 0;
            i->ibufread = 0;
            logdebug("input descriptors: %d/%d\n", i->stdinfd, i->termfd);
            return i;
          }
        }
        free(i->inputs);
      }
      free(i->csrs);
    }
    free(i);
  }
  return NULL;
}

static inline void
free_inputctx(inputctx* i){
  if(i){
    // we *do not* own stdinfd; don't close() it! we do own termfd.
    if(i->termfd >= 0){
      close(i->termfd);
    }
    // do not kill the thread here, either.
    free(i->inputs);
    free(i->csrs);
    free(i);
  }
}

// how many bytes can a single read fill in the ibuf? this might be fewer than
// the actual number of free bytes, due to reading on the right or left side.
static inline size_t
space_for_read(inputctx* ictx){
  // if we are valid everywhere, there's no space to read into.
  if(ictx->ibufvalid == sizeof(ictx->ibuf)){
    return 0;
  }
  // if we are valid nowhere, we can read into the head of the buffer
  if(ictx->ibufvalid == 0){
    ictx->ibufread = 0;
    ictx->ibufwrite = 0;
    return sizeof(ictx->ibuf);
  }
  // otherwise, we can read either from ibufwrite to the end of the buffer,
  // or from ibufwrite to ibufread.
  if(ictx->ibufwrite < ictx->ibufread){
    return ictx->ibufread - ictx->ibufwrite;
  }
  return sizeof(ictx->ibuf) - ictx->ibufwrite;
}

// populate the ibuf with any new data from the specified file descriptor.
static void
read_input_nblock(inputctx* ictx, int fd){
  if(fd >= 0){
    size_t space = space_for_read(ictx);
    if(space == 0){
      return;
    }
    ssize_t r = read(fd, ictx->ibuf + ictx->ibufwrite, space);
    if(r >= 0){
      ictx->ibufwrite += r;
      if(ictx->ibufwrite == sizeof(ictx->ibuf)){
        ictx->ibufwrite = 0;
      }
      ictx->ibufvalid += r;
      space -= r;
      loginfo("read %lldB from %d (%lluB left)\n", (long long)r, fd, (unsigned long long)space);
      if(space == 0){
        return;
      }
      // might have been falsely limited by space (only reading on the right).
      // this will recurse one time at most.
      if(ictx->ibufwrite == 0){
        read_input_nblock(ictx, fd);
      }
    }
  }
}

// walk the matching automaton from wherever we were.
static void
process_ibuf(inputctx* ictx){
  // FIXME
}

// populate the ibuf with any new data, up through its size, but do not block.
// don't loop around this call without some kind of readiness notification.
static void
read_inputs_nblock(inputctx* ictx){
  // first we read from the terminal, if that's a distinct source.
  read_input_nblock(ictx, ictx->termfd);
  // now read bulk, possibly with term escapes intermingled within (if there
  // was not a distinct terminal source).
  read_input_nblock(ictx, ictx->stdinfd);
}

static void*
input_thread(void* vmarshall){
  inputctx* ictx = vmarshall;
  for(;;){
    read_inputs_nblock(ictx);
    // process anything we've read
    process_ibuf(ictx);
  }
  return NULL;
}

int init_inputlayer(tinfo* ti, FILE* infp){
  inputctx* ictx = create_inputctx(ti, infp);
  if(ictx == NULL){
    return -1;
  }
  if(pthread_create(&ictx->tid, NULL, input_thread, ictx)){
    free_inputctx(ictx);
    return -1;
  }
  ti->ictx = ictx;
  loginfo("spun up input thread\n");
  return 0;
}

int stop_inputlayer(tinfo* ti){
  int ret = 0;
  if(ti){
    if(ti->ictx){
      loginfo("tearing down input thread\n");
      ret |= cancel_and_join("input", ti->ictx->tid, NULL);
      ret |= set_fd_nonblocking(ti->ictx->stdinfd, ti->stdio_blocking_save, NULL);
      free_inputctx(ti->ictx);
      ti->ictx = NULL;
    }
  }
  return ret;
}

int inputready_fd(const inputctx* ictx){
  return ictx->stdinfd;
}

// infp has already been set non-blocking
uint32_t notcurses_get(notcurses* nc, const struct timespec* ts, ncinput* ni){
  /*
  uint32_t r = ncinputlayer_prestamp(&nc->tcache, ts, ni,
                                     nc->margin_l, nc->margin_t);
  if(r != (uint32_t)-1){
    uint64_t stamp = nc->tcache.input.input_events++; // need increment even if !ni
    if(ni){
      ni->seqnum = stamp;
    }
    ++nc->stats.s.input_events;
  }
  return r;
  */
  return -1;
}

uint32_t notcurses_getc(notcurses* nc, const struct timespec* ts,
                        const void* unused, ncinput* ni){
  (void)unused; // FIXME remove for abi3
  return notcurses_get(nc, ts, ni);
}

uint32_t ncdirect_get(struct ncdirect* n, const struct timespec* ts, ncinput* ni){
  /*
  uint32_t r = ncinputlayer_prestamp(&n->tcache, ts, ni, 0, 0);
  if(r != (uint32_t)-1){
    uint64_t stamp = n->tcache.input.input_events++; // need increment even if !ni
    if(ni){
      ni->seqnum = stamp;
    }
  }
  return r;
  */
  return -1;
}

uint32_t ncdirect_getc(ncdirect* nc, const struct timespec *ts,
                       const void* unused, ncinput* ni){
  (void)unused; // FIXME remove for abi3
  return ncdirect_get(nc, ts, ni);
}

