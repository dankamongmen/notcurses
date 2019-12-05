#include <sys/poll.h>
#include "internal.h"

static const unsigned char ESC = 0x1b; // 27

sig_atomic_t resize_seen = 0;

static inline int
pop_input_keypress(notcurses* nc){
  int candidate = nc->inputbuf[nc->inputbuf_valid_starts];
// fprintf(stderr, "DEOCCUPY: %u@%u read: %d\n", nc->inputbuf_occupied, nc->inputbuf_valid_starts, nc->inputbuf[nc->inputbuf_valid_starts]);
  if(++nc->inputbuf_valid_starts == sizeof(nc->inputbuf) / sizeof(*nc->inputbuf)){
    nc->inputbuf_valid_starts = 0;
  }
  --nc->inputbuf_occupied;
  return candidate;
}

// we assumed escapes can only be composed of 7-bit chars
typedef struct esctrie {
  ncspecial_key special; // escape terminating here
  struct esctrie** trie;  // if non-NULL, next level of radix-128 trie
} esctrie;

static esctrie*
create_esctrie_node(ncspecial_key special){
  esctrie* e = malloc(sizeof(*e));
  if(e){
    e->special = special;
    e->trie = NULL;
  }
  return e;
}

void input_free_esctrie(esctrie** eptr){
  esctrie* e;
  if( (e = *eptr) ){
    if(e->trie){
      int z;
      for(z = 0 ; z < 0x80 ; ++z){
        if(e->trie[z]){
          input_free_esctrie(&e->trie[z]);
        }
      }
      free(e->trie);
    }
    free(e);
  }
}

int input_add_escape(notcurses* nc, const char* esc, ncspecial_key special){
  esctrie** cur;
  fprintf(stderr, "ADDING: %s for %d\n", esc, special);
  if(esc[0] != ESC || strlen(esc) < 2){ // assume ESC prefix + content
    return -1;
  }
  do{
    ++esc;
    int validate = *esc;
    if(validate < 0 || validate >= 0x80){
      return -1;
    }
    if(nc->inputescapes == NULL){
      cur = &nc->inputescapes;
    }else if(validate){
      if((*cur)->trie == NULL){
        const size_t tsize = sizeof((*cur)->trie) * 0x80;
        (*cur)->trie = malloc(tsize);
        memset((*cur)->trie, 0, tsize);
      }
      cur = &(*cur)->trie[validate];
    }
    if(*cur == NULL){
      if((*cur = create_esctrie_node(NCKEY_INVALID)) == NULL){
        return -1;
      }
    }
  }while(*esc);
  if((*cur)->special){ // already had one here!
    return -1;
  }
  (*cur)->special = special;
  return 0;
}

// add the keypress we just read to our input queue (assuming there is room).
// if there is a full UTF8 codepoint or keystroke (composed or otherwise),
// return it, and pop it from the queue.
static int
handle_getc(notcurses* nc, cell* c, int kpress, ncspecial_key* special){
// fprintf(stderr, "KEYPRESS: %d\n", kpress);
  if(kpress < 0){
    return -1;
  }
  if(kpress == ESC){
    // FIXME delay a little waiting for more?
    while(nc->inputbuf_occupied){
      int candidate = pop_input_keypress(nc);
      // FIXME walk trie via candidate, exiting (and ungetc()ing) on failure
fprintf(stderr, "CANDIDATE: %c\n", candidate);
    }
  }
  *special = 0;
  if(kpress == 0x04){ // ctrl-d, treated as EOF
    return -1;
  }
  if(kpress < 0x80){
    c->gcluster = kpress;
  }else{
    // FIXME load up zee utf8
  }
  return 1;
}

// blocks up through ts (infinite with NULL ts), returning number of events
// (0 on timeout) or -1 on error/interruption.
static int
block_on_input(FILE* fp, const struct timespec* ts, sigset_t* sigmask){
  struct pollfd pfd = {
    .fd = fileno(fp),
    .events = POLLIN | POLLRDHUP,
    .revents = 0,
  };
  sigdelset(sigmask, SIGWINCH);
  sigdelset(sigmask, SIGINT);
  sigdelset(sigmask, SIGQUIT);
  sigdelset(sigmask, SIGSEGV);
  return ppoll(&pfd, 1, ts, sigmask);
}

static bool
input_queue_full(const notcurses* nc){
  return nc->inputbuf_occupied == sizeof(nc->inputbuf) / sizeof(*nc->inputbuf);
}

static int
handle_input(notcurses* nc, cell* c, ncspecial_key* special){
  int r;
  c->gcluster = 0;
  // getc() returns unsigned chars cast to ints
  while(!input_queue_full(nc) && (r = getc(nc->ttyinfp)) >= 0){
    nc->inputbuf[nc->inputbuf_write_at] = (unsigned char)r;
fprintf(stderr, "OCCUPY: %u@%u read: %d\n", nc->inputbuf_occupied, nc->inputbuf_write_at, nc->inputbuf[nc->inputbuf_write_at]);
    if(++nc->inputbuf_write_at == sizeof(nc->inputbuf) / sizeof(*nc->inputbuf)){
      nc->inputbuf_write_at = 0;
    }
    ++nc->inputbuf_occupied;
  }
  // highest priority is resize notifications, since they don't queue
  if(resize_seen){
    resize_seen = 0;
    *special = NCKEY_RESIZE;
    return 1;
  }
  // if there was some error in getc(), we still dole out the existing queue
  if(nc->inputbuf_occupied == 0){
    return -1;
  }
  r = pop_input_keypress(nc);
  return handle_getc(nc, c, r, special);
}

// infp has always been set non-blocking
int notcurses_getc(notcurses* nc, cell* c, ncspecial_key* special,
                   const struct timespec *ts, sigset_t* sigmask){
  errno = 0;
  int r = handle_input(nc, c, special);
  if(r > 0){
    return r;
  }
  if(errno == EAGAIN || errno == EWOULDBLOCK){
    block_on_input(nc->ttyinfp, ts, sigmask);
    r = handle_input(nc, c, special);
  }
  return r;
}
