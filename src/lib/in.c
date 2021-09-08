#include <stdio.h>
#include "internal.h"
#include "in.h"

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
  // we do not close any of these file descriptors; we don't own them!
  int termfd;           // terminal fd: -1 with no controlling terminal, or
                        //  if stdin is a terminal, and on Windows Terminal.
  int stdinfd;          // stdin fd. always 0.
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
create_inputctx(tinfo* ti){
  inputctx* i = malloc(sizeof(*i));
  if(i){
    i->csize = 64;
    if( (i->csrs = malloc(sizeof(*i->csrs) * i->csize)) ){
      i->isize = BUFSIZ;
      if( (i->inputs = malloc(sizeof(*i->inputs) * i->isize)) ){
        // FIXME set up infd/handle
        i->termfd = -1;
        i->stdinfd = -1;
        i->ti = ti;
        i->cvalid = i->ivalid = 0;
        i->cwrite = i->iwrite = 0;
        i->cread = i->iread = 0;
        i->ibufvalid = i->ibufwrite = 0;
        i->ibufread = 0;
        return i;
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
    // we *do not* own any file descriptors or handles; don't close them!
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

int init_inputlayer(tinfo* ti){
  inputctx* ictx = create_inputctx(ti);
  if(ictx == NULL){
    return -1;
  }
  if(pthread_create(&ictx->tid, NULL, input_thread, ictx)){
    free_inputctx(ictx);
    return -1;
  }
  // FIXME give ti a reference to ictx
  loginfo("spun up input thread\n");
  return 0;
}

int stop_inputlayer(tinfo* ti){
  // FIXME get ictx reference from ti, kill thread, free_inputctx()
  return 0;
}
