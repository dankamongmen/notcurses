#include <ncurses.h> // needed for some definitions, see terminfo(3ncurses)
#include <term.h>
#include <ctype.h>
#include <sys/poll.h>
#include "internal.h"

// CSI (Control Sequence Indicators) originate in the terminal itself, and are
// not reported in their bare form to the user. For our purposes, these usually
// indicate a mouse event.
#define CSIPREFIX "\x1b[<"
static const char32_t NCKEY_CSI = 1;

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

// assumes there is space, as you presumably just popped it
static inline void
unpop_keypress(notcurses* nc, int kpress){
  ++nc->inputbuf_occupied;
  if(nc->inputbuf_valid_starts-- == 0){
    nc->inputbuf_valid_starts = sizeof(nc->inputbuf) / sizeof(*nc->inputbuf) - 1;
  }
  nc->inputbuf[nc->inputbuf_valid_starts] = kpress;
}

// we assumed escapes can only be composed of 7-bit chars
typedef struct esctrie {
  char32_t special;       // composed key terminating here
  struct esctrie** trie;  // if non-NULL, next level of radix-128 trie
} esctrie;

static esctrie*
create_esctrie_node(int special){
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

static int
notcurses_add_input_escape(notcurses* nc, const char* esc, char32_t special){
  if(esc[0] != ESC || strlen(esc) < 2){ // assume ESC prefix + content
    fprintf(stderr, "Not an escape: %s (0x%x)\n", esc, special);
    return -1;
  }
  if(!wchar_supppuab_p(special) && special != NCKEY_CSI){
    fprintf(stderr, "Not a supplementary-b PUA char: %lc (0x%x)\n", special, special);
    return -1;
  }
  esctrie** cur = &nc->inputescapes;
  do{
//fprintf(stderr, "ADDING: %s (%zu) for %d\n", esc, strlen(esc), special);
    ++esc;
    int validate = *esc;
    if(validate < 0 || validate >= 0x80){
      return -1;
    }
    if(*cur == NULL){
      if((*cur = create_esctrie_node(NCKEY_INVALID)) == NULL){
        return -1;
      }
    }
    if(validate){
      if((*cur)->trie == NULL){
        const size_t tsize = sizeof((*cur)->trie) * 0x80;
        (*cur)->trie = malloc(tsize);
        memset((*cur)->trie, 0, tsize);
      }
      cur = &(*cur)->trie[validate];
    }
  }while(*esc);
  // it appears that multiple keys can be mapped to the same escape string. as
  // an example, see "kend" and "kc1" in st ("simple term" from suckless) :/.
  if((*cur)->special != NCKEY_INVALID){ // already had one here!
    fprintf(stderr, "Warning: already added escape (got 0x%x, wanted 0x%x)\n", (*cur)->special, special);
  }else{
    (*cur)->special = special;
  }
  return 0;
}

// We received the CSI prefix. Extract the data payload.
static char32_t
handle_csi(notcurses* nc, ncinput* ni){
  enum {
    PARAM1,  // reading first param (button + modifiers) plus delimiter
    PARAM2,  // reading second param (x coordinate) plus delimiter
    PARAM3,  // reading third param (y coordinate) plus terminator
  } state = PARAM1;
  int param = 0; // numeric translation of param thus far
  char32_t id = (char32_t)-1;
  while(nc->inputbuf_occupied){
    int candidate = pop_input_keypress(nc);
    if(state == PARAM1){
      if(candidate == ';'){
        state = PARAM2;
        // modifiers: 32 (motion) 16 (control) 8 (alt) 4 (shift)
        // buttons 4, 5, 6, 7: adds 64
        // buttons 8, 9, 10, 11: adds 128
        if(param >= 0 && param < 64){
          if(param % 4 == 3){
            id = NCKEY_RELEASE;
          }else{
            id = NCKEY_BUTTON1 + (param % 4);
          }
        }else if(param >= 64 && param < 128){
          id = NCKEY_BUTTON4 + (param % 4);
        }else if(param >= 128 && param < 192){
          id = NCKEY_BUTTON8 + (param % 4);
        }else{
          break;
        }
        param = 0;
      }else if(isdigit(candidate)){
        param *= 10;
        param += candidate - '0';
      }else{
        break;
      }
    }else if(state == PARAM2){
      if(candidate == ';'){
        state = PARAM3;
        if(param == 0){
          break;
        }
        if(ni){
          ni->x = param - 1;
        }
        param = 0;
      }else if(isdigit(candidate)){
        param *= 10;
        param += candidate - '0';
      }else{
        break;
      }
    }else if(state == PARAM3){
      if(candidate == 'm' || candidate == 'M'){
        if(candidate == 'm'){
          id = NCKEY_RELEASE;
        }
        if(param == 0){
          break;
        }
        if(ni){
          ni->y = param - 1;
          ni->id = id;
        }
        return id;
      }else if(isdigit(candidate)){
        param *= 10;
        param += candidate - '0';
      }else{
        break;
      }
    }
  }
  // FIXME ungetc on failure! walk trie backwards or something
  return (char32_t)-1;
}

// add the keypress we just read to our input queue (assuming there is room).
// if there is a full UTF8 codepoint or keystroke (composed or otherwise),
// return it, and pop it from the queue.
static char32_t
handle_getc(notcurses* nc, int kpress, ncinput* ni){
// fprintf(stderr, "KEYPRESS: %d\n", kpress);
  if(kpress < 0){
    return -1;
  }
  if(kpress == ESC){
    const esctrie* esc = nc->inputescapes;
    while(esc && esc->special == NCKEY_INVALID && nc->inputbuf_occupied){
      int candidate = pop_input_keypress(nc);
      if(esc->trie == NULL){
        esc = NULL;
      }else if(candidate >= 0x80 || candidate < 0){
        esc = NULL;
      }else{
        esc = esc->trie[candidate];
      }
    }
    if(esc && esc->special != NCKEY_INVALID){
      if(esc->special == NCKEY_CSI){
        return handle_csi(nc, ni);
      }
      return esc->special;
    }
    // FIXME ungetc on failure! walk trie backwards or something
  }
  if(kpress == 0x7f){ // ASCII del, treated as backspace
    return NCKEY_BACKSPACE;
  }
  if(kpress < 0x80){
    return kpress;
  }
  char cpoint[MB_CUR_MAX];
  size_t cpointlen = 0;
  cpoint[cpointlen] = kpress;
  // FIXME need to stop as soon as we have a full codepoint urk
  while(++cpointlen < MB_CUR_MAX - 1 && nc->inputbuf_occupied){
    int candidate = pop_input_keypress(nc);
    if(candidate < 0x80){
      unpop_keypress(nc, candidate);
    }
    cpoint[cpointlen] = candidate;
  }
  cpoint[cpointlen] = '\0';
  wchar_t w;
  // FIXME how the hell does this work with 16-bit wchar_t?
  if(mbtowc(&w, cpoint, cpointlen) < 0){
    return (wchar_t)-1;
  }
  return w;
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
  sigset_t scratchmask;
  if(!sigmask){
    sigemptyset(&scratchmask);
    sigmask = &scratchmask;
  }
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

static char32_t
handle_input(notcurses* nc, ncinput* ni){
  int r;
  // getc() returns unsigned chars cast to ints
  while(!input_queue_full(nc) && (r = getc(nc->ttyinfp)) >= 0){
    nc->inputbuf[nc->inputbuf_write_at] = (unsigned char)r;
// fprintf(stderr, "OCCUPY: %u@%u read: %d\n", nc->inputbuf_occupied, nc->inputbuf_write_at, nc->inputbuf[nc->inputbuf_write_at]);
    if(++nc->inputbuf_write_at == sizeof(nc->inputbuf) / sizeof(*nc->inputbuf)){
      nc->inputbuf_write_at = 0;
    }
    ++nc->inputbuf_occupied;
  }
  // highest priority is resize notifications, since they don't queue
  if(resize_seen){
    resize_seen = 0;
    return NCKEY_RESIZE;
  }
  // if there was some error in getc(), we still dole out the existing queue
  if(nc->inputbuf_occupied == 0){
    return -1;
  }
  r = pop_input_keypress(nc);
  return handle_getc(nc, r, ni);
}

// infp has already been set non-blocking
char32_t notcurses_getc(notcurses* nc, const struct timespec *ts,
                        sigset_t* sigmask, ncinput* ni){
  errno = 0;
  char32_t r = handle_input(nc, ni);
  if(r == (char32_t)-1){
    if(errno == EAGAIN || errno == EWOULDBLOCK){
      block_on_input(nc->ttyinfp, ts, sigmask);
      return handle_input(nc, ni);
    }
    return r;
  }
  return r;
}

int prep_special_keys(notcurses* nc){
  static const struct {
    const char* tinfo;
    char32_t key;
  } keys[] = {
    { .tinfo = "kcub1", .key = NCKEY_LEFT, },
    { .tinfo = "kcuf1", .key = NCKEY_RIGHT, },
    { .tinfo = "kcuu1", .key = NCKEY_UP, },
    { .tinfo = "kcud1", .key = NCKEY_DOWN, },
    { .tinfo = "kdch1", .key = NCKEY_DEL, },
    { .tinfo = "kbs",   .key = NCKEY_BACKSPACE, },
    { .tinfo = "kich1", .key = NCKEY_INS, },
    { .tinfo = "kend",  .key = NCKEY_END, },
    { .tinfo = "khome", .key = NCKEY_HOME, },
    { .tinfo = "knp",   .key = NCKEY_PGDOWN, },
    { .tinfo = "kpp",   .key = NCKEY_PGUP, },
    { .tinfo = "kf0",   .key = NCKEY_F01, },
    { .tinfo = "kf1",   .key = NCKEY_F01, },
    { .tinfo = "kf2",   .key = NCKEY_F02, },
    { .tinfo = "kf3",   .key = NCKEY_F03, },
    { .tinfo = "kf4",   .key = NCKEY_F04, },
    { .tinfo = "kf5",   .key = NCKEY_F05, },
    { .tinfo = "kf6",   .key = NCKEY_F06, },
    { .tinfo = "kf7",   .key = NCKEY_F07, },
    { .tinfo = "kf8",   .key = NCKEY_F08, },
    { .tinfo = "kf9",   .key = NCKEY_F09, },
    { .tinfo = "kf10",  .key = NCKEY_F10, },
    { .tinfo = "kf11",  .key = NCKEY_F11, },
    { .tinfo = "kf12",  .key = NCKEY_F12, },
    { .tinfo = "kf13",  .key = NCKEY_F13, },
    { .tinfo = "kf14",  .key = NCKEY_F14, },
    { .tinfo = "kf15",  .key = NCKEY_F15, },
    { .tinfo = "kf16",  .key = NCKEY_F16, },
    { .tinfo = "kf17",  .key = NCKEY_F17, },
    { .tinfo = "kf18",  .key = NCKEY_F18, },
    { .tinfo = "kf19",  .key = NCKEY_F19, },
    { .tinfo = "kf20",  .key = NCKEY_F20, },
    { .tinfo = "kf21",  .key = NCKEY_F21, },
    { .tinfo = "kf22",  .key = NCKEY_F22, },
    { .tinfo = "kf23",  .key = NCKEY_F23, },
    { .tinfo = "kf24",  .key = NCKEY_F24, },
    { .tinfo = "kf25",  .key = NCKEY_F25, },
    { .tinfo = "kf26",  .key = NCKEY_F26, },
    { .tinfo = "kf27",  .key = NCKEY_F27, },
    { .tinfo = "kf28",  .key = NCKEY_F28, },
    { .tinfo = "kf29",  .key = NCKEY_F29, },
    { .tinfo = "kf30",  .key = NCKEY_F30, },
    { .tinfo = "kent",  .key = NCKEY_ENTER, },
    { .tinfo = "kclr",  .key = NCKEY_CLS, },
    { .tinfo = "kc1",   .key = NCKEY_DLEFT, },
    { .tinfo = "kc3",   .key = NCKEY_DRIGHT, },
    { .tinfo = "ka1",   .key = NCKEY_ULEFT, },
    { .tinfo = "ka3",   .key = NCKEY_URIGHT, },
    { .tinfo = "kb2",   .key = NCKEY_CENTER, },
    { .tinfo = "kbeg",  .key = NCKEY_BEGIN, },
    { .tinfo = "kcan",  .key = NCKEY_CANCEL, },
    { .tinfo = "kclo",  .key = NCKEY_CLOSE, },
    { .tinfo = "kcmd",  .key = NCKEY_COMMAND, },
    { .tinfo = "kcpy",  .key = NCKEY_COPY, },
    { .tinfo = "kext",  .key = NCKEY_EXIT, },
    { .tinfo = "kprt",  .key = NCKEY_PRINT, },
    { .tinfo = "krfr",  .key = NCKEY_REFRESH, },
    { .tinfo = NULL,    .key = NCKEY_INVALID, }
  }, *k;
  for(k = keys ; k->tinfo ; ++k){
    char* seq = tigetstr(k->tinfo);
    if(seq == NULL || seq == (char*)-1){
//fprintf(stderr, "no support for terminfo's %s\n", k->tinfo);
      continue;
    }
    if(seq[0] != ESC){
//fprintf(stderr, "Terminfo's %s is not an escape sequence (%zub)\n", k->tinfo, strlen(seq));
      continue;
    }
//fprintf(stderr, "support for terminfo's %s: %s\n", k->tinfo, seq);
    if(notcurses_add_input_escape(nc, seq, k->key)){
      fprintf(stderr, "Couldn't add support for %s\n", k->tinfo);
      return -1;
    }
  }
  if(notcurses_add_input_escape(nc, CSIPREFIX, NCKEY_CSI)){
    fprintf(stderr, "Couldn't add support for %s\n", k->tinfo);
    return -1;
  }
  return 0;
}
