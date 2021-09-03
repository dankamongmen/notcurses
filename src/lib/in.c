#include <stdio.h>
#include "internal.h"

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
  cursorloc* csrs;    // cursor reports are dumped here
  ncinput* inputs;    // processed input is dumped here
  int csize, isize;   // total number of slots in csrs/inputs
  int cvalid, ivalid; // population count of csrs/inputs
  int cwrite, iwrite; // slot where we'll write the next csr/input;
                      //  we cannot write if valid == size
  int cread, iread;   // slot from which to read the next csr/input;
                      //  we cannot read if valid == 0
  tinfo* ti;          // link back to tinfo
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
        i->ti = ti;
        i->cvalid = i->ivalid = 0;
        i->cwrite = i->iwrite = 0;
        i->cread = i->iread = 0;
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
