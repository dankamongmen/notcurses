#include "input.h"
#include "internal.h"
#include "notcurses/direct.h"
#include <ncurses.h> // needed for some definitions, see terminfo(3ncurses)
#include <term.h>
#include <ctype.h>
#include <signal.h>

// CSI (Control Sequence Indicators) originate in the terminal itself, and are
// not reported in their bare form to the user. For our purposes, these usually
// indicate a mouse event or a cursor location report.
#define CSIPREFIX "\x1b["
static const uint32_t NCKEY_CSI = 0x90; // guaranteed not to match anything else

static sig_atomic_t resize_seen;

// called for SIGWINCH and SIGCONT
void sigwinch_handler(int signo){
  resize_seen = signo;
}

int cbreak_mode(int ttyfd, const struct termios* tpreserved){
  if(ttyfd < 0){
    return 0;
  }
  // assume it's not a true terminal (e.g. we might be redirected to a file)
  struct termios modtermios;
  memcpy(&modtermios, tpreserved, sizeof(modtermios));
  // see termios(3). disabling ECHO and ICANON means input will not be echoed
  // to the screen, input is made available without enter-based buffering, and
  // line editing is disabled. since we have not gone into raw mode, ctrl+c
  // etc. still have their typical effects. ICRNL maps return to 13 (Ctrl+M)
  // instead of 10 (Ctrl+J).
  modtermios.c_lflag &= (~ECHO & ~ICANON);
  modtermios.c_iflag &= ~ICRNL;
  if(tcsetattr(ttyfd, TCSANOW, &modtermios)){
    logerror("Error disabling echo / canonical on %d (%s)\n", ttyfd, strerror(errno));
    return -1;
  }
  return 0;
}

// Disable signals originating from the terminal's line discipline, i.e.
// SIGINT (^C), SIGQUIT (^\), and SIGTSTP (^Z). They are enabled by default.
int notcurses_linesigs_disable(notcurses* n){
  if(n->ttyfd < 0){
    return 0;
  }
  struct termios tios;
  if(tcgetattr(n->ttyfd, &tios)){
    logerror("Couldn't preserve terminal state for %d (%s)\n", n->ttyfd, strerror(errno));
    return -1;
  }
  tios.c_lflag &= ~ISIG;
  if(tcsetattr(n->ttyfd, TCSANOW, &tios)){
    logerror("Error disabling signals on %d (%s)\n", n->ttyfd, strerror(errno));
    return -1;
  }
  return 0;
}

// Restore signals originating from the terminal's line discipline, i.e.
// SIGINT (^C), SIGQUIT (^\), and SIGTSTP (^Z), if disabled.
int notcurses_linesigs_enable(notcurses* n){
  if(n->ttyfd < 0){
    return 0;
  }
  struct termios tios;
  if(tcgetattr(n->ttyfd, &tios)){
    logerror("Couldn't preserve terminal state for %d (%s)\n", n->ttyfd, strerror(errno));
    return -1;
  }
  tios.c_lflag |= ~ISIG;
  if(tcsetattr(n->ttyfd, TCSANOW, &tios)){
    logerror("Error disabling signals on %d (%s)\n", n->ttyfd, strerror(errno));
    return -1;
  }
  return 0;
}

static inline int
pop_input_keypress(ncinputlayer* nc){
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
unpop_keypress(ncinputlayer* nc, int kpress){
  ++nc->inputbuf_occupied;
  if(nc->inputbuf_valid_starts-- == 0){
    nc->inputbuf_valid_starts = sizeof(nc->inputbuf) / sizeof(*nc->inputbuf) - 1;
  }
  nc->inputbuf[nc->inputbuf_valid_starts] = kpress;
}

// we assumed escapes can only be composed of 7-bit chars
typedef struct esctrie {
  uint32_t special;       // composed key terminating here
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

static void
input_free_esctrie(esctrie** eptr){
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
ncinputlayer_add_input_escape(ncinputlayer* nc, const char* esc, uint32_t special){
  if(esc[0] != NCKEY_ESC || strlen(esc) < 2){ // assume ESC prefix + content
    logerror("Not an escape: %s (0x%x)\n", esc, special);
    return -1;
  }
  if(!nckey_supppuab_p(special) && special != NCKEY_CSI){
    logerror("Not a supplementary-b PUA char: %u (0x%x)\n", special, special);
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
        if(((*cur)->trie = malloc(tsize)) == NULL){
          return -1;
        }
        memset((*cur)->trie, 0, tsize);
      }
      cur = &(*cur)->trie[validate];
    }
  }while(*esc);
  // it appears that multiple keys can be mapped to the same escape string. as
  // an example, see "kend" and "kc1" in st ("simple term" from suckless) :/.
  if((*cur)->special != NCKEY_INVALID){ // already had one here!
    logwarn("Warning: already added escape (got 0x%x, wanted 0x%x)\n", (*cur)->special, special);
  }else{
    (*cur)->special = special;
  }
  return 0;
}

// We received the CSI prefix. Extract the data payload. Right now, we handle
// mouse and cursor location reports. The former is three parameters starting
// with '<' and ending with 'm' or 'M'; the latter is two ending with 'R'.
// Both use 1-biased coordinates, so a 0 can be safely rejected.
static uint32_t
handle_csi(ncinputlayer* nc, ncinput* ni, int leftmargin, int topmargin){
  // stash the first parameter away. it's encoded if the CSI ends up being a
  // mouse event, and otherwise it's the cursor's column (x) coordinate.
  int param1 = -1;
  bool mouse = false;
  enum {
    PARAM1,  // reading first param (button + modifiers) plus delimiter
    PARAM2,  // reading second param (x coordinate) plus delimiter
    PARAM3,  // reading third param (y coordinate) plus terminator
  } state = PARAM1;
  int param = 0; // numeric translation of param thus far
  uint32_t id = (uint32_t)-1;
  while(nc->inputbuf_occupied){
    int candidate = pop_input_keypress(nc);
    logdebug("candidate: %c (%d)\n", candidate, candidate);
    if(state == PARAM1){
      // if !mouse and candidate is '>', set mouse. otherwise it ought be a
      // digit or a semicolon.
      if(candidate == '<'){
        if(mouse){
          break; // shouldn't see it twice
        }
        mouse = true;
      }else if(candidate == ';'){
        param1 = param;
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
        ni->ctrl = param & 0x10;
        ni->alt = param & 0x08;
        ni->shift = param & 0x04;
        param = 0;
      }else if(isdigit(candidate)){
        param *= 10;
        param += candidate - '0';
      }else{
        break;
      }
    }else if(state == PARAM2){
      if(candidate == 'R'){ // cursor location report
        if(mouse){
          logwarn("Invalid mouse param (%d/%d)\n", param1, param);
          break;
        }
        if(param <= 0 || param1 <= 0){
          logwarn("Invalid cursor location param (%d/%d)\n", param1, param);
          break;
        }
        logdebug("Cursor location report %d/%d\n", param1, param);
        ni->x = param - 1;
        ni->y = param1 - 1;
        return NCKEY_CURSOR_LOCATION_REPORT;
      }else if(candidate == ';'){
        state = PARAM3;
        if(param == 0){
          logwarn("Invalid mouse param (%d/%d)\n", param1, param);
          break;
        }
        ni->x = param - 1 - leftmargin;
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
          logwarn("Invalid mouse param (%d/%d)\n", param1, param);
          break;
        }
        ni->y = param - 1 - topmargin;
        ni->id = id;
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
  // FIXME increment the input_error stat
  logerror("Error processing CSI (%d left)\n", nc->inputbuf_occupied);
  return (uint32_t)-1;
}

// add the keypress we just read to our input queue (assuming there is room).
// if there is a full UTF8 codepoint or keystroke (composed or otherwise),
// return it, and pop it from the queue. if there is a full cursor location
// report, push that to the location report queue.
// we need to match escapes from terminfo, certainly, but what about
// unexpected escapes? the williams automaton is our guide:
// https://vt100.net/emu/dec_ansi_parser. here are the basics:
//
// six sets of input trump everything, setting a known state from all sources,
// only two of which are directly available in 7-bit mode:
//
//  0x18/0x1a (cancel): ground state
//  0x1b (escape): escape state
//
// the other four require two 7-bit characters:
//
//  0x90: DCS, also reached via 0x1b+0x50
//  0x9b: CSI, also reached via 0x1b+0x5b
//  0x9d: OSC, also reached via 0x1b+0x5d
//  0x98/0x9e/0x9f: sos/pm/apc string, also reached via 0x1b+{0x58/0x5e/0x5f}
//
// if interrupted in the middle of an existing sequence, it must not be acted
// upon, and should probably not be delivered as bulk input.
//
// CSI is most relevant to us here, since mouse and cursor reports arrive that
// way. CSI is properly terminated by 0x40--0x7e. the parameter space covers
// 0x30--0x39 and the delimiter 0x3b (';').
static uint32_t
handle_getc(ncinputlayer* nc, int kpress, ncinput* ni, int leftmargin, int topmargin){
  if(kpress < 0){
    return -1;
  }
  if(kpress == NCKEY_ESC){
    const esctrie* esc = nc->inputescapes;
    int candidate = 0;
    const esctrie* csi = NULL;
    while(esc && nc->inputbuf_occupied){
      if(esc->special != NCKEY_INVALID){
        if(esc->special == NCKEY_CSI){
          csi = esc;
        }else{
          break;
        }
      }
      candidate = pop_input_keypress(nc);
      logdebug("trie candidate: %c %d (%d)\n", candidate, esc->special, candidate);
      if(esc->trie == NULL){
        esc = NULL;
      }else if(candidate >= 0x80 || candidate < 0){
        esc = NULL;
      }else{
        esc = esc->trie[candidate];
      }
      logtrace("move to %p (%u)\n", esc, esc ? esc->special : 0);
    }
    if(esc && esc->special != NCKEY_INVALID){
      return esc->special;
    }
    if(csi){
      // FIXME might need to unpop more than one
      unpop_keypress(nc, candidate);
      return handle_csi(nc, ni, leftmargin, topmargin);
    }
    // interpret it as alt + candidate FIXME broken for first char matching
    // trie, second char not -- will read as alt+second char...
    if(candidate > 0 && candidate < 0x80){
      ni->alt = true;
      return candidate;
    }
    // FIXME ungetc on failure! walk trie backwards or something
  }
  if(kpress == 0x7f){ // ASCII del, treated as backspace
    return NCKEY_BACKSPACE;
  }
  if(kpress < 0x80){
    return kpress;
  }
  char cpoint[MB_CUR_MAX + 1];
  memset(cpoint, 0, sizeof(cpoint));
  size_t cpointlen = 0;
  cpoint[cpointlen] = kpress;
  wchar_t w;
  mbstate_t mbstate;
  while(++cpointlen <= (size_t)MB_CUR_MAX && nc->inputbuf_occupied){
    int candidate = pop_input_keypress(nc);
    if(candidate < 0x80){
      unpop_keypress(nc, candidate);
    }
    cpoint[cpointlen] = candidate;
//fprintf(stderr, "CANDIDATE: %d cpointlen: %zu cpoint: %d\n", candidate, cpointlen, cpoint[cpointlen]);
    // FIXME how the hell does this work with 16-bit wchar_t?
    memset(&mbstate, 0, sizeof(mbstate));
    size_t r;
    if((r = mbrtowc(&w, cpoint, cpointlen + 1, &mbstate)) != (size_t)-1 &&
        r != (size_t)-2){
      return w;
    }
  }
  ++nc->stats->s.input_errors;
  return -1;
}

// blocks up through ts (infinite with NULL ts), returning number of events
// (0 on timeout) or -1 on error/interruption.
#ifndef __MINGW64__
static int
block_on_input(int fd, const struct timespec* ts){
  struct pollfd pfd = {
    .fd = fd,
    .events = POLLIN,
    .revents = 0,
  };
  sigset_t smask;
  sigfillset(&smask);
  sigdelset(&smask, SIGCONT);
  sigdelset(&smask, SIGWINCH);
#ifdef POLLRDHUP
  pfd.events |= POLLRDHUP;
#endif
  int events;
#ifdef __APPLE__
  int timeoutms = ts ? ts->tv_sec * 1000 + ts->tv_nsec / 1000000 : -1;
  while((events = poll(&pfd, 1, timeoutms)) < 0){ // FIXME smask?
#else
  while((events = ppoll(&pfd, 1, ts, &smask)) < 0){
#endif
    if(events == 0){
      return 0;
    }
    if(errno != EINTR && errno != EAGAIN){
      return -1;
    }
    if(resize_seen){
      return 1;
    }
  }
  return events;
}
#else
// FIXME windows block_on_input()
#endif

static inline size_t
input_queue_space(const ncinputlayer* nc){
  return sizeof(nc->inputbuf) / sizeof(*nc->inputbuf) - nc->inputbuf_occupied;
}

static int
enqueue_cursor_report(ncinputlayer* nc, const ncinput* ni){
  cursorreport* clr = malloc(sizeof(*clr));
  if(clr == NULL){
    return -1;
  }
  clr->y = ni->y;
  clr->x = ni->x;
  clr->next = NULL;
  // i don't think we ever want to have more than one here. we don't actually
  // have any control logic which leads to multiple outstanding requests, so
  // any that arrive are presumably garbage from the bulk input (and probably
  // ought be returned to the user).
  free(nc->creport_queue);
  nc->creport_queue = clr;
  return 0;
}

static uint32_t
handle_queued_input(ncinputlayer* nc, ncinput* ni,
                    int leftmargin, int topmargin){
  ncinput nireal;
  if(ni == NULL){
    ni = &nireal;
  }
  uint32_t ret;
  do{
    // if there was some error in getc(), we still dole out the existing queue
    if(nc->inputbuf_occupied == 0){
      return -1;
    }
    int r = pop_input_keypress(nc);
    ret = handle_getc(nc, r, ni, leftmargin, topmargin);
    if(ret != (uint32_t)-1){
      ni->id = ret;
    }
    if(ret == NCKEY_CURSOR_LOCATION_REPORT){
      enqueue_cursor_report(nc, ni);
    }
  }while(ret == NCKEY_CURSOR_LOCATION_REPORT);
  return ret;
}

// this is the only function which actually reads, and it can be called from
// either our context (looking for cursor reports) or the user's. all it does
// is attempt to fill up the input ringbuffer, exiting either when that
// condition is met, or when we get an EAGAIN. it does no processing.
static int
fill_buffer(ncinputlayer* nc){
  ssize_t r = 0;
  size_t rlen;
//fprintf(stderr, "OCCUPY: %u@%u read: %d %zd\n", nc->inputbuf_occupied, nc->inputbuf_write_at, nc->inputbuf[nc->inputbuf_write_at], r);
  if((rlen = input_queue_space(nc)) > 0){
    // if we have at least as much available as we do room to the end, read
    // only to the end. otherwise, read as much as we have available.
    if(rlen >= sizeof(nc->inputbuf) / sizeof(*nc->inputbuf) - nc->inputbuf_write_at){
      rlen = sizeof(nc->inputbuf) / sizeof(*nc->inputbuf) - nc->inputbuf_write_at;
    }
    if((r = read(nc->infd, nc->inputbuf + nc->inputbuf_write_at, rlen)) > 0){
      nc->inputbuf_write_at += r;
      if(nc->inputbuf_write_at == sizeof(nc->inputbuf) / sizeof(*nc->inputbuf)){
        nc->inputbuf_write_at = 0;
      }
      nc->inputbuf_occupied += r;
    }else if(r < 0){
      if(errno != EAGAIN && errno != EBUSY && errno != EWOULDBLOCK){
        return -1;
      }
    }
  }
  return 0;
}

// user-mode call to actual input i/o, which will get the next character from
// the input buffer.
static uint32_t
handle_input(ncinputlayer* nc, ncinput* ni, int leftmargin, int topmargin){
  fill_buffer(nc);
  // highest priority is resize notifications, since they don't queue
  if(resize_seen){
    resize_seen = 0;
    return NCKEY_SIGNAL;
  }
  return handle_queued_input(nc, ni, leftmargin, topmargin);
}

static uint32_t
handle_ncinput(ncinputlayer* nc, ncinput* ni, int leftmargin, int topmargin){
  if(ni){
    memset(ni, 0, sizeof(*ni));
  }
  uint32_t r = handle_input(nc, ni, leftmargin, topmargin);
  // ctrl (*without* alt) + letter maps to [1..26], and is independent of shift
  // FIXME need to distinguish between:
  //  - Enter and ^J
  //  - Tab and ^I
  bool ctrl = r > 0 && r <= 26;
  if(ctrl){
    if(r == '\n' || r == '\r'){
      r = NCKEY_ENTER;
      ctrl = false;
    }else if(r == '\t'){ // FIXME infocmp: ht=^I, use that
      ctrl = false;
    }else{
      r += 'A' - 1;
    }
  }
  if(ni){
    ni->id = r;
    if(ctrl){
      ni->ctrl = true;
    }
    // FIXME set shift
  }
  return r;
}

// helper so we can do counter increment at a single location
static inline uint32_t
ncinputlayer_prestamp(ncinputlayer* nc, const struct timespec *ts,
                      ncinput* ni, int leftmargin, int topmargin){
//fprintf(stderr, "PRESTAMP OCCUPADO: %d\n", nc->inputbuf_occupied);
  if(nc->inputbuf_occupied){
    return handle_queued_input(nc, ni, leftmargin, topmargin);
  }
  errno = 0;
  if(block_on_input(nc->infd, ts) > 0){
//fprintf(stderr, "%d events from input!\n", events);
    return handle_ncinput(nc, ni, leftmargin, topmargin);
  }
//fprintf(stderr, "ERROR: %d events from input!\n", events);
  return -1;
}

// infp has already been set non-blocking
uint32_t notcurses_get(notcurses* nc, const struct timespec* ts, ncinput* ni){
  uint32_t r = ncinputlayer_prestamp(&nc->tcache.input, ts, ni,
                                     nc->margin_l, nc->margin_t);
  if(r != (uint32_t)-1){
    uint64_t stamp = nc->tcache.input.input_events++; // need increment even if !ni
    if(ni){
      ni->seqnum = stamp;
    }
    ++nc->stats.s.input_events;
  }
  return r;
}

uint32_t notcurses_getc(notcurses* nc, const struct timespec* ts,
                        const void* unused, ncinput* ni){
  (void)unused; // FIXME remove for abi3
  return notcurses_get(nc, ts, ni);
}

uint32_t ncdirect_get(struct ncdirect* n, const struct timespec* ts, ncinput* ni){
  uint32_t r = ncinputlayer_prestamp(&n->tcache.input, ts, ni, 0, 0);
  if(r != (uint32_t)-1){
    uint64_t stamp = n->tcache.input.input_events++; // need increment even if !ni
    if(ni){
      ni->seqnum = stamp;
    }
  }
  return r;
}

uint32_t ncdirect_getc(ncdirect* nc, const struct timespec *ts,
                       const void* unused, ncinput* ni){
  (void)unused; // FIXME remove for abi3
  return ncdirect_get(nc, ts, ni);
}

// load all known special keys from terminfo, and build the input sequence trie
static int
prep_special_keys(ncinputlayer* nc){
  static const struct {
    const char* tinfo;
    uint32_t key;
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
    { .tinfo = "kf31",  .key = NCKEY_F31, },
    { .tinfo = "kf32",  .key = NCKEY_F32, },
    { .tinfo = "kf33",  .key = NCKEY_F33, },
    { .tinfo = "kf34",  .key = NCKEY_F34, },
    { .tinfo = "kf35",  .key = NCKEY_F35, },
    { .tinfo = "kf36",  .key = NCKEY_F36, },
    { .tinfo = "kf37",  .key = NCKEY_F37, },
    { .tinfo = "kf38",  .key = NCKEY_F38, },
    { .tinfo = "kf39",  .key = NCKEY_F39, },
    { .tinfo = "kf40",  .key = NCKEY_F40, },
    { .tinfo = "kf41",  .key = NCKEY_F41, },
    { .tinfo = "kf42",  .key = NCKEY_F42, },
    { .tinfo = "kf43",  .key = NCKEY_F43, },
    { .tinfo = "kf44",  .key = NCKEY_F44, },
    { .tinfo = "kf45",  .key = NCKEY_F45, },
    { .tinfo = "kf46",  .key = NCKEY_F46, },
    { .tinfo = "kf47",  .key = NCKEY_F47, },
    { .tinfo = "kf48",  .key = NCKEY_F48, },
    { .tinfo = "kf49",  .key = NCKEY_F49, },
    { .tinfo = "kf50",  .key = NCKEY_F50, },
    { .tinfo = "kf51",  .key = NCKEY_F51, },
    { .tinfo = "kf52",  .key = NCKEY_F52, },
    { .tinfo = "kf53",  .key = NCKEY_F53, },
    { .tinfo = "kf54",  .key = NCKEY_F54, },
    { .tinfo = "kf55",  .key = NCKEY_F55, },
    { .tinfo = "kf56",  .key = NCKEY_F56, },
    { .tinfo = "kf57",  .key = NCKEY_F57, },
    { .tinfo = "kf58",  .key = NCKEY_F58, },
    { .tinfo = "kf59",  .key = NCKEY_F59, },
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
    if(seq[0] != NCKEY_ESC){
//fprintf(stderr, "Terminfo's %s is not an escape sequence (%zub)\n", k->tinfo, strlen(seq));
      continue;
    }
    logdebug("support for terminfo's %s: %s\n", k->tinfo, seq);
    if(ncinputlayer_add_input_escape(nc, seq, k->key)){
      return -1;
    }
  }
  if(ncinputlayer_add_input_escape(nc, CSIPREFIX, NCKEY_CSI)){
    return -1;
  }
  return 0;
}

void ncinputlayer_stop(ncinputlayer* nilayer){
  if(pthread_cond_destroy(&nilayer->creport_cond)){
    logerror("Error destroying cqueue condvar\n");
  }
  if(pthread_mutex_destroy(&nilayer->lock)){
    logerror("Error destroying mutex\n");
  }
  cursorreport* clr;
  while( (clr = nilayer->creport_queue) ){
    nilayer->creport_queue = clr->next;
    free(clr);
  }
  if(nilayer->ttyfd >= 0){
    close(nilayer->ttyfd);
  }
  // do *not* close infd; it's just a fileno extracted from stdin
  input_free_esctrie(&nilayer->inputescapes);
}

typedef enum {
  STATE_NULL,
  STATE_ESC,  // escape; aborts any active sequence
  STATE_CSI,  // control sequence introducer
  STATE_DCS,  // device control string
  // XTVERSION replies with DCS > | ... ST
  STATE_XTVERSION1,
  STATE_XTVERSION2,
  // XTGETTCAP replies with DCS 1 + r for a good request, or 0 + r for bad
  STATE_XTGETTCAP1, // XTGETTCAP, got '0/1' (DCS 0/1 + r Pt ST)
  STATE_XTGETTCAP2, // XTGETTCAP, got '+' (DCS 0/1 + r Pt ST)
  STATE_XTGETTCAP3, // XTGETTCAP, got 'r' (DCS 0/1 + r Pt ST)
  STATE_XTGETTCAP_TERMNAME1, // got property 544E, 'TN' (terminal name) first hex nibble
  STATE_XTGETTCAP_TERMNAME2, // got property 544E, 'TN' (terminal name) second hex nibble
  STATE_DCS_DRAIN,  // throw away input until we hit escape
  STATE_BG1,        // got '1'
  STATE_BG2,        // got second '1'
  STATE_BGSEMI,     // got '11;', draining string to ESC ST
  STATE_TDA1, // tertiary DA, got '!'
  STATE_TDA2, // tertiary DA, got '|', first hex nibble
  STATE_TDA3, // tertiary DA, second hex nibble
  STATE_SDA,  // secondary DA (CSI > Pp ; Pv ; Pc c)
  STATE_SDA_VER,  // secondary DA, got semi, reading to next semi
  STATE_SDA_DRAIN, // drain secondary DA to 'c'
  STATE_DA,   // primary DA   (CSI ? ... c) OR XTSMGRAPHICS OR DECRPM
  STATE_DA_1, // got '1', XTSMGRAPHICS color registers or primary DA
  STATE_DA_1_SEMI, // got '1;'
  STATE_DA_1_0, // got '1;0', XTSMGRAPHICS color registers or VT101
  STATE_DA_1_SEMI_2, // got '1;2', XTSMGRAPHICS color reg fail or VT100
  STATE_DA_6, // got '6', could be VT102 or VT220/VT320/VT420
  STATE_DA_DRAIN, // drain out the primary DA to an alpha
  STATE_SIXEL,// XTSMGRAPHICS about Sixel geometry (got '2')
  STATE_SIXEL_SEMI1,   // got first semicolon in sixel geometry, want Ps
  STATE_SIXEL_SUCCESS, // got Ps == 0, want second semicolon
  STATE_SIXEL_WIDTH,   // reading maximum sixel width until ';'
  STATE_SIXEL_HEIGHT,  // reading maximum sixel height until 'S'
  STATE_SIXEL_CREGS,   // reading max color registers until 'S'
  STATE_XTSMGRAPHICS_DRAIN, // drain out XTSMGRAPHICS to 'S'
  STATE_APPSYNC_REPORT, // got DECRPT ?2026
  STATE_APPSYNC_REPORT_DRAIN, // drain out decrpt to 'y'
  // a cursor location report comes back as CSI row ; col R. a pixel geometry
  // report comes back as CSI 4 ; height ; width t. so we handle them the same
  // until we hit either a second semicolon or an 'R'. at the second ';', we
  // verify that the first variable was '4', and continue to 't'.
  STATE_CURSOR_OR_PIXELGEOM, // reading row of cursor location to ';'
  STATE_CURSOR_COL, // reading col of cursor location to 'R' or ';'
  STATE_PIXELS_WIDTH,  // reading screen width in pixels to ';'
} initstates_e;

typedef struct query_state {
  struct tinfo* tcache;
  // if the terminal unambiguously identifies itself in response to our
  // queries, use that identification for advanced feature support.
  queried_terminals_e qterm;
  char* version;        // terminal version, if detected. heap-allocated.
  // stringstate is the state at which this string was initialized, and can be
  // one of STATE_XTVERSION1, STATE_XTGETTCAP_TERMNAME1, STATE_TDA1, and STATE_BG1
  initstates_e state, stringstate;
  int numeric;           // currently-lexed numeric
  char runstring[80];    // running string
  size_t stridx;         // position to write in string
  uint32_t bg;           // queried default background or 0
  int pixelwidth;        // screen width in pixels
  int pixelheight;       // screen height in pixels
  int cursor_y, cursor_x;// cursor location
  int cursor_or_pixel;   // holding cell until we determine which state

  bool xtgettcap_good;   // high when we've received DCS 1
  bool appsync;          // application-synchronized updates advertised
} query_state;

static int
ruts_numeric(int* numeric, unsigned char c){
  if(!isdigit(c)){
    return -1;
  }
  int digit = c - '0';
  if(INT_MAX / 10 - digit < *numeric){ // would overflow
    return -1;
  }
  *numeric *= 10;
  *numeric += digit;
  return 0;
}

static int
ruts_hex(int* numeric, unsigned char c){
  if(!isxdigit(c)){
    return -1;
  }
  int digit;
  if(isdigit(c)){
    digit = c - '0';
  }else if(islower(c)){
    digit = c - 'a' + 10;
  }else if(isupper(c)){
    digit = c - 'A' + 10;
  }else{
    return -1; // should be impossible to reach
  }
  if(INT_MAX / 10 - digit < *numeric){ // would overflow
    return -1;
  }
  *numeric *= 16;
  *numeric += digit;
  return 0;
}

// add a decoded hex byte to the string
static int
ruts_string(query_state* inits, initstates_e state){
  if(inits->stridx == sizeof(inits->runstring)){
    return -1; // overflow, too long
  }
  if(inits->numeric > 255){
    return -1;
  }
  unsigned char c = inits->numeric;
  if(!isprint(c)){
    return -1;
  }
  inits->stringstate = state;
  inits->runstring[inits->stridx] = c;
  inits->runstring[++inits->stridx] = '\0';
  return 0;
}

// extract the terminal version from the running string, following 'prefix'
static int
extract_version(query_state* qstate, size_t slen){
  size_t bytes = strlen(qstate->runstring + slen) + 1;
  qstate->version = malloc(bytes);
  if(qstate->version == NULL){
    return -1;
  }
  memcpy(qstate->version, qstate->runstring + slen, bytes);
  return 0;
}

static int
extract_xtversion(query_state* qstate, size_t slen, char suffix){
  if(suffix){
    if(qstate->runstring[qstate->stridx - 1] != suffix){
      return -1;
    }
    qstate->runstring[qstate->stridx - 1] = '\0';
  }
  return extract_version(qstate, slen);
}

static int
stash_string(query_state* inits){
//fprintf(stderr, "string terminator after %d [%s]\n", inits->stringstate, inits->runstring);
  switch(inits->stringstate){
    case STATE_XTVERSION1:{
      static const struct {
        const char* prefix;
        char suffix;
        queried_terminals_e term;
      } xtvers[] = {
        { .prefix = "XTerm(", .suffix = ')', .term = TERMINAL_XTERM, },
        { .prefix = "WezTerm ", .suffix = 0, .term = TERMINAL_WEZTERM, },
        { .prefix = "contour ", .suffix = 0, .term = TERMINAL_CONTOUR, },
        { .prefix = "kitty(", .suffix = ')', .term = TERMINAL_KITTY, },
        { .prefix = "foot(", .suffix = ')', .term = TERMINAL_FOOT, },
        { .prefix = "mlterm(", .suffix = ')', .term = TERMINAL_MLTERM, },
        { .prefix = "tmux ", .suffix = 0, .term = TERMINAL_TMUX, },
        { .prefix = "iTerm2 ", .suffix = 0, .term = TERMINAL_ITERM, },
        { .prefix = NULL, .suffix = 0, .term = TERMINAL_UNKNOWN, },
      }, *xtv;
      for(xtv = xtvers ; xtv->prefix ; ++xtv){
        if(strncmp(inits->runstring, xtv->prefix, strlen(xtv->prefix)) == 0){
          if(extract_xtversion(inits, strlen(xtv->prefix), xtv->suffix) == 0){
            inits->qterm = xtv->term;
          }
          break;
        }
      }
      if(xtv->prefix == NULL){
        logwarn("Unrecognizable XTVERSION [%s]\n", inits->runstring);
      }
      break;
    }case STATE_XTGETTCAP_TERMNAME1:
      if(strcmp(inits->runstring, "xterm-kitty") == 0){
        inits->qterm = TERMINAL_KITTY;
      }else if(strcmp(inits->runstring, "mlterm") == 0){
        // MLterm prior to late 3.9.1 only reports via XTGETTCAP
        inits->qterm = TERMINAL_MLTERM;
      }
      break;
    case STATE_TDA1:
      if(strcmp(inits->runstring, "~VTE") == 0){
        inits->qterm = TERMINAL_VTE;
      }else if(strcmp(inits->runstring, "~~TY") == 0){
        inits->qterm = TERMINAL_TERMINOLOGY;
      }else if(strcmp(inits->runstring, "FOOT") == 0){
        inits->qterm = TERMINAL_FOOT;
      }
      break;
    case STATE_BG1:{
      int r, g, b;
      if(sscanf(inits->runstring, "rgb:%02x/%02x/%02x", &r, &g, &b) == 3){
        // great! =]
      }else if(sscanf(inits->runstring, "rgb:%04x/%04x/%04x", &r, &g, &b) == 3){
        r /= 256;
        g /= 256;
        b /= 256;
      }else{
        break;
      }
      inits->bg = (r << 16u) | (g << 8u) | b;
      break;
    }default:
// don't generally enable this -- XTerm terminates TDA with ST
//fprintf(stderr, "invalid string [%s] stashed %d\n", inits->runstring, inits->stringstate);
      break;
  }
  inits->runstring[0] = '\0';
  inits->stridx = 0;
  return 0;
}

// use the version extracted from Secondary Device Attributes, assuming that
// it is Alacritty (we ought check the specified terminfo database entry).
// Alacritty writes its crate version with each more significant portion
// multiplied by 100^{portion ID}, where major, minor, patch are 2, 1, 0.
// what happens when a component exceeds 99? who cares. support XTVERSION.
static char*
set_sda_version(query_state* inits){
  int maj, min, patch;
  if(inits->numeric <= 0){
    return NULL;
  }
  maj = inits->numeric / 10000;
  min = (inits->numeric % 10000) / 100;
  patch = inits->numeric % 100;
  if(maj >= 100 || min >= 100 || patch >= 100){
    return NULL;
  }
  // 3x components (two digits max each), 2x '.', NUL would suggest 9 bytes,
  // but older gcc __builtin___sprintf_chk insists on 13. fuck it. FIXME.
  char* buf = malloc(13);
  if(buf){
    sprintf(buf, "%d.%d.%d", maj, min, patch);
  }
  return buf;
}

// FIXME ought implement the full Williams automaton
// FIXME sloppy af in general
// returns 1 after handling the Device Attributes response, 0 if more input
// ought be fed to the machine, and -1 on an invalid state transition.
static int
pump_control_read(query_state* inits, unsigned char c){
  logdebug("state: %2d char: %1c %3d %02x\n", inits->state, isprint(c) ? c : ' ', c, c);
  if(c == NCKEY_ESC){
    inits->state = STATE_ESC;
    return 0;
  }
  switch(inits->state){
    case STATE_NULL:
      // not an escape -- throw into user queue
      break;
    case STATE_ESC:
      inits->numeric = 0;
      if(c == '['){
        inits->state = STATE_CSI;
      }else if(c == 'P'){
        inits->state = STATE_DCS;
      }else if(c == '\\'){
        if(stash_string(inits)){
          return -1;
        }
        inits->state = STATE_NULL;
      }else if(c == '1'){
        inits->state = STATE_BG1;
      }
      break;
    case STATE_BG1:
      if(c == '1'){
        inits->state = STATE_BG2;
      }else{
        // FIXME
      }
      break;
    case STATE_BG2:
      if(c == ';'){
        inits->state = STATE_BGSEMI;
        inits->stridx = 0;
        inits->runstring[0] = '\0';
      }else{
        // FIXME
      }
      break;
    case STATE_BGSEMI: // drain string
      if(c == '\x07'){ // contour sends this at the end for some unknown reason
        if(stash_string(inits)){
          return -1;
        }
        inits->state = STATE_NULL;
        break;
      }
      inits->numeric = c;
      if(ruts_string(inits, STATE_BG1)){
        return -1;
      }
      break;
    case STATE_CSI: // terminated by 0x40--0x7E ('@'--'~')
      if(c == '?'){
        inits->state = STATE_DA; // could also be DECRPM/XTSMGRAPHICS
      }else if(c == '>'){
        // SDA yields up Alacritty's crate version, but it doesn't unambiguously
        // identify Alacritty. If we've got any other version information, skip
        // directly to STATE_SDA_DRAIN, rather than doing STATE_SDA_VER.
        if(inits->qterm || inits->version){
          loginfo("Identified terminal already; ignoring DA2\n");
          inits->state = STATE_SDA_DRAIN;
        }else{
          inits->state = STATE_SDA;
        }
      }else if(isdigit(c)){
        inits->numeric = 0;
        if(ruts_numeric(&inits->numeric, c)){
          return -1;
        }
        inits->state = STATE_CURSOR_OR_PIXELGEOM;
      }else if(c >= 0x40 && c <= 0x7E){
        inits->state = STATE_NULL;
      }
      break;
    case STATE_CURSOR_OR_PIXELGEOM:
      if(isdigit(c)){
        if(ruts_numeric(&inits->numeric, c)){
          return -1;
        }
      }else if(c == ';'){
        inits->cursor_or_pixel = inits->numeric;
        inits->state = STATE_CURSOR_COL;
        inits->numeric = 0;
      }else{
        inits->state = STATE_NULL;
      }
      break;
    case STATE_CURSOR_COL:
      if(isdigit(c)){
        if(ruts_numeric(&inits->numeric, c)){
          return -1;
        }
      }else if(c == 'R'){
//fprintf(stderr, "CURSOR X: %d\n", inits->numeric);
        inits->cursor_x = inits->numeric;
        inits->cursor_y = inits->cursor_or_pixel;
        inits->state = STATE_NULL;
      }else if(c == ';'){
        if(inits->cursor_or_pixel != 4){
          logerror("expected 4 to lead pixel report, got %d\n", inits->cursor_or_pixel);
          return -1;
        }
        inits->pixelheight = inits->numeric;
        inits->state = STATE_PIXELS_WIDTH;
        inits->numeric = 0;
      }else{
        inits->state = STATE_NULL;
      }
      break;
    case STATE_PIXELS_WIDTH:
      if(isdigit(c)){
        if(ruts_numeric(&inits->numeric, c)){
          return -1;
        }
      }else if(c == 't'){
//fprintf(stderr, "CURSOR X: %d\n", inits->numeric);
        inits->pixelwidth = inits->numeric;
        inits->state = STATE_NULL;
      }else{
        inits->state = STATE_NULL;
      }
      break;
    case STATE_DCS: // terminated by ST
      if(c == '\\'){
//fprintf(stderr, "terminated DCS\n");
        inits->state = STATE_NULL;
      }else if(c == '1'){
        inits->state = STATE_XTGETTCAP1;
        inits->xtgettcap_good = true;
      }else if(c == '0'){
        inits->state = STATE_XTGETTCAP1;
        inits->xtgettcap_good = false;
      }else if(c == '>'){
        inits->state = STATE_XTVERSION1;
      }else if(c == '!'){
        inits->state = STATE_TDA1;
      }else{
        inits->state = STATE_DCS_DRAIN;
      }
      break;
    case STATE_DCS_DRAIN:
      // we drain to ST, which is an escape, and thus already handled, so...
      break;
    case STATE_XTVERSION1:
      if(c == '|'){
        inits->state = STATE_XTVERSION2;
        inits->stridx = 0;
        inits->runstring[0] = '\0';
      }else{
        // FIXME error?
      }
      break;
    case STATE_XTVERSION2:
      inits->numeric = c;
      if(ruts_string(inits, STATE_XTVERSION1)){
        return -1;
      }
      break;
    case STATE_XTGETTCAP1:
      if(c == '+'){
        inits->state = STATE_XTGETTCAP2;
      }else{
        // FIXME malformed
      }
      break;
    case STATE_XTGETTCAP2:
      if(c == 'r'){
        inits->state = STATE_XTGETTCAP3;
      }else{
        // FIXME malformed
      }
      break;
    case STATE_XTGETTCAP3:
      if(c == '='){
        if(inits->numeric == 0x544e){
          inits->state = STATE_XTGETTCAP_TERMNAME1;
          inits->stridx = 0;
          inits->numeric = 0;
          inits->runstring[0] = '\0';
        }else{
          inits->state = STATE_DCS_DRAIN;
        }
      }else if(ruts_hex(&inits->numeric, c)){
        return -1;
      }
      break;
    case STATE_XTGETTCAP_TERMNAME1:
      if(ruts_hex(&inits->numeric, c)){
        return -1;
      }
      inits->state = STATE_XTGETTCAP_TERMNAME2;
      break;
    case STATE_XTGETTCAP_TERMNAME2:
      if(ruts_hex(&inits->numeric, c)){
        return -1;
      }
      inits->state = STATE_XTGETTCAP_TERMNAME1;
      if(ruts_string(inits, STATE_XTGETTCAP_TERMNAME1)){
        return -1;
      }
      inits->numeric = 0;
      break;
    case STATE_TDA1:
      if(c == '|'){
        inits->state = STATE_TDA2;
        inits->stridx = 0;
        inits->runstring[0] = '\0';
      }else{
        // FIXME
      }
      break;
    case STATE_TDA2:
      if(ruts_hex(&inits->numeric, c)){
        return -1;
      }
      inits->state = STATE_TDA3;
      break;
    case STATE_TDA3:
      if(ruts_hex(&inits->numeric, c)){
        return -1;
      }
      inits->state = STATE_TDA2;
      if(ruts_string(inits, STATE_TDA1)){
        inits->state = STATE_DCS_DRAIN; // FIXME return -1?
      }
      inits->numeric = 0;
      break;
    case STATE_SDA:
      if(c == ';'){
        inits->state = STATE_SDA_VER;
        inits->numeric = 0;
      }else if(c == 'c'){
        inits->state = STATE_NULL;
      }
      break;
    case STATE_SDA_VER:
      if(c == ';'){
        inits->state = STATE_SDA_DRAIN;
        loginfo("Got DA2 Pv: %u\n", inits->numeric);
        // if a version was set, we couldn't have arrived here. alacritty
        // writes its crate version here, in an encoded form. nothing else
        // necessarily does, though, so allow failure. this value will be
        // interpreted as the version only if TERM indicates alacritty.
        inits->version = set_sda_version(inits);
      }else if(ruts_numeric(&inits->numeric, c)){
        return -1;
      }
      break;
    case STATE_SDA_DRAIN:
      if(c == 'c'){
        inits->state = STATE_NULL;
      }
      break;
    // primary device attributes and XTSMGRAPHICS replies are generally
    // indistinguishable until well into the escape. one can get:
    // XTSMGRAPHICS: CSI ? Pi ; Ps ; Pv S {Pi: 123} {Ps: 0123}
    // DECRPM: CSI ? Pd ; Ps $ y {Pd: many} {Ps: 01234}
    // DA: CSI ? 1 ; 2 c  ("VT100 with Advanced Video Option")
    //     CSI ? 1 ; 0 c  ("VT101 with No Options")
    //     CSI ? 4 ; 6 c  ("VT132 with Advanced Video and Graphics")
    //     CSI ? 6 c  ("VT102")
    //     CSI ? 7 c  ("VT131")
    //     CSI ? 1 2 ; Ps c  ("VT125")
    //     CSI ? 6 2 ; Ps c  ("VT220")
    //     CSI ? 6 3 ; Ps c  ("VT320")
    //     CSI ? 6 4 ; Ps c  ("VT420")
    case STATE_DA: // return success on end of DA
//fprintf(stderr, "DA: %c\n", c);
      if(c == '1'){
        inits->state = STATE_DA_1;
      }else if(c == '2'){
        if(ruts_numeric(&inits->numeric, c)){ // stash for DECRPT
          return -1;
        }
        inits->state = STATE_SIXEL;
      }else if(c == '4' || c == '7'){ // VT132, VT131
        inits->state = STATE_DA_DRAIN;
      }else if(c == '6'){
        inits->state = STATE_DA_6;
      }else if(c >= 0x40 && c <= 0x7E){
        inits->state = STATE_NULL;
      }
      break;
    case STATE_DA_1:
//fprintf(stderr, "DA1: %c\n", c);
      if(c >= 0x40 && c <= 0x7E){
        inits->state = STATE_NULL;
        return 1;
      }else if(c == ';'){
        inits->state = STATE_DA_1_SEMI;
      } // FIXME error?
      break;
    case STATE_DA_1_SEMI:
//fprintf(stderr, "DA1SEMI: %c\n", c);
      if(c == '2'){ // VT100 with Advanced Video Option *or* setcregs failure
        inits->state = STATE_DA_1_SEMI_2;
      }else if(c == '0'){
        inits->state = STATE_DA_1_0;
      }else if(c >= 0x40 && c <= 0x7E){
        inits->state = STATE_NULL;
      } // FIXME error?
      break;
    case STATE_DA_1_SEMI_2:
      if(c == 'S'){
        inits->state = STATE_NULL;
      }else if(c >= 0x40 && c <= 0x7E){
        inits->state = STATE_NULL;
        return 1;
      }
      break;
    case STATE_DA_1_0:
//fprintf(stderr, "DA_!_0: %c\n", c);
      if(c >= 0x40 && c <= 0x7E){
        inits->state = STATE_NULL;
        return 1;
      }else if(c == ';'){
        inits->state = STATE_SIXEL_CREGS;
      } // FIXME error?
      break;
    case STATE_SIXEL_CREGS:
      if(c == 'S'){
//fprintf(stderr, "CREGS: %d\n", inits->numeric);
        inits->tcache->color_registers = inits->numeric;
        inits->state = STATE_NULL;
      }else if(ruts_numeric(&inits->numeric, c)){
        return -1;
      }
      break;
    case STATE_DA_6:
      if(c >= 0x40 && c <= 0x7E){
        inits->state = STATE_NULL;
        return 1;
      }
      // FIXME
      break;
    case STATE_DA_DRAIN:
      if(c >= 0x40 && c <= 0x7E){
        inits->state = STATE_NULL;
        return 1;
      }
      break;
    case STATE_SIXEL:
      if(c == ';'){
        if(inits->numeric == 2026){
          inits->state = STATE_APPSYNC_REPORT;
        }else{
          inits->state = STATE_SIXEL_SEMI1;
        }
      }else if(ruts_numeric(&inits->numeric, c)){
        return -1;
      }else{
        // FIXME error?
      }
      break;
    case STATE_SIXEL_SEMI1:
//fprintf(stderr, "SIXLE`l`: %c\n", c);
      if(c == '0'){
        inits->state = STATE_SIXEL_SUCCESS;
      }else if(c == '2'){
        inits->state = STATE_XTSMGRAPHICS_DRAIN;
      }else{
        // FIXME error?
      }
      break;
    case STATE_SIXEL_SUCCESS:
      if(c == ';'){
        inits->numeric = 0;
        inits->state = STATE_SIXEL_WIDTH;
      }else{
        // FIXME error?
      }
      break;
    case STATE_SIXEL_WIDTH:
      if(c == ';'){
//fprintf(stderr, "HEIGHT: %d\n", inits->numeric);
        inits->tcache->sixel_maxx = inits->numeric;
        inits->state = STATE_SIXEL_HEIGHT;
        inits->numeric = 0;
      }else if(ruts_numeric(&inits->numeric, c)){
        return -1;
      }
      break;
    case STATE_SIXEL_HEIGHT:
      if(c == 'S'){
//fprintf(stderr, "HEIGHT: %d\n", inits->numeric);
        inits->tcache->sixel_maxy_pristine = inits->numeric;
        inits->state = STATE_NULL;
      }else if(ruts_numeric(&inits->numeric, c)){
        return -1;
      }
      break;
    case STATE_XTSMGRAPHICS_DRAIN:
      if(c == 'S'){
        inits->state = STATE_NULL;
      }
      break;
    case STATE_APPSYNC_REPORT:
      if(c == '2'){
        inits->appsync = 1;
        inits->state = STATE_APPSYNC_REPORT_DRAIN;
      }
      break;
    case STATE_APPSYNC_REPORT_DRAIN:
      if(c == 'y'){
        inits->state = STATE_NULL;
      }
      break;
    default:
      fprintf(stderr, "Reached invalid init state %d\n", inits->state);
      return -1;
  }
  return 0;
}

// complete the terminal detection process
static int
control_read(int ttyfd, query_state* qstate){
  unsigned char* buf;
  ssize_t s;

  if((buf = malloc(BUFSIZ)) == NULL){
    return -1;
  }
  errno = 0;
  do{
    while((s = read(ttyfd, buf, BUFSIZ)) != -1){
      for(ssize_t idx = 0; idx < s ; ++idx){
        int r = pump_control_read(qstate, buf[idx]);
        if(r == 1){ // success!
          free(buf);
  //fprintf(stderr, "at end, derived terminal %d\n", inits.qterm);
          return 0;
        }else if(r < 0){
          goto err;
        }
      }
    }
  }while(errno == EINTR || errno == EAGAIN);
err:
  fprintf(stderr, "Reading control replies failed on %d (%s)\n", ttyfd, strerror(errno));
  free(buf);
  return -1;
}

int ncinputlayer_init(tinfo* tcache, FILE* infp, queried_terminals_e* detected,
                      unsigned* appsync, int* cursor_y, int* cursor_x,
                      ncsharedstats* stats){
  ncinputlayer* nilayer = &tcache->input;
  if(pthread_mutex_init(&nilayer->lock, NULL)){
    return -1;
  }
  nilayer->stats = stats;
  nilayer->inputescapes = NULL;
  nilayer->infd = fileno(infp);
  nilayer->ttyfd = isatty(nilayer->infd) ? -1 : get_tty_fd(infp);
  if(prep_special_keys(nilayer)){
    pthread_mutex_destroy(&nilayer->lock);
    return -1;
  }
  nilayer->inputbuf_occupied = 0;
  nilayer->inputbuf_valid_starts = 0;
  nilayer->inputbuf_write_at = 0;
  nilayer->input_events = 0;
  nilayer->creport_queue = NULL;
  nilayer->user_wants_data = false;
  nilayer->inner_wants_data = false;
  pthread_cond_init(&nilayer->creport_cond, NULL);
  int csifd = nilayer->ttyfd >= 0 ? nilayer->ttyfd : nilayer->infd;
  if(isatty(csifd)){
    query_state inits = {
      .tcache = tcache,
      .state = STATE_NULL,
      .qterm = *detected,
      .cursor_x = -1,
      .cursor_y = -1,
    };
    if(control_read(csifd, &inits)){
      input_free_esctrie(&nilayer->inputescapes);
      free(inits.version);
      pthread_cond_destroy(&nilayer->creport_cond);
      pthread_mutex_destroy(&nilayer->lock);
      return -1;
    }
    tcache->bg_collides_default = inits.bg;
    tcache->termversion = inits.version;
    *detected = inits.qterm;
    *appsync = inits.appsync;
    if(cursor_x){
      *cursor_x = inits.cursor_x - 1;
    }
    if(cursor_y){
      *cursor_y = inits.cursor_y - 1;
    }
    if(inits.pixelwidth && inits.pixelheight){
      tcache->pixy = inits.pixelheight;
      tcache->pixx = inits.pixelwidth;
    }
  }
  return 0;
}

// go through the accumulated input and find any cursor location reports.
// enqueue them, and zero them out from the buffer (but do not decrement the
// number of available characters).
// FIXME optimize this via remembered offset + invalidation
static void
scan_for_clrs(ncinputlayer* ni){
  unsigned pos = ni->inputbuf_valid_starts;
  //unsigned count = ni->inputbuf_occupied;
  // FIXME need handle_csi to work from an arbitrary place
  if(ni->inputbuf[pos] == '\x1b'){
    logdebug("Got the escape at %u\n", pos);
    if(++pos == sizeof(ni->inputbuf) / sizeof(*ni->inputbuf)){
      pos = 0;
    }
    logdebug("Got %c (%d) %u\n", ni->inputbuf[pos], ni->inputbuf[pos], pos);
    if(ni->inputbuf[pos] == '['){
      logdebug("Got the CSI at %u\n", pos);
      pop_input_keypress(ni);
      pop_input_keypress(ni);
      ncinput nin;
      if(handle_csi(ni, &nin, 0, 0) == NCKEY_CURSOR_LOCATION_REPORT){
        enqueue_cursor_report(ni, &nin);
      }
    }
  }
}

// assuming the user context is not active, go through current data looking
// for a cursor location report. if we find none, block on input, and read if
// appropriate. we can be interrupted by a new user context. we enter holding
// the input lock, and leave holding the input lock, giving it up only while
// blocking for readable action.
void ncinput_extract_clrs(ncinputlayer* ni){
  do{
    if(ni->inputbuf_occupied){
      scan_for_clrs(ni);
      if(ni->creport_queue){
        logdebug("Found a CLR, returning\n");
        return;
      }
      logdebug("No CLR available, reading\n");
    }
    size_t rlen = input_queue_space(ni);
    if(rlen){
      if(rlen >= sizeof(ni->inputbuf) / sizeof(*ni->inputbuf) - ni->inputbuf_write_at){
        rlen = sizeof(ni->inputbuf) / sizeof(*ni->inputbuf) - ni->inputbuf_write_at;
      }
      logdebug("Reading %zu from %d\n", rlen, ni->infd);
      ssize_t r;
      if((r = read(ni->infd, ni->inputbuf + ni->inputbuf_write_at, rlen)) > 0){
        logdebug("Read %zu from %d\n", r, ni->infd);
        ni->inputbuf_write_at += r;
        if(ni->inputbuf_write_at == sizeof(ni->inputbuf) / sizeof(*ni->inputbuf)){
          ni->inputbuf_write_at = 0;
        }
        ni->inputbuf_occupied += r;
        continue;
      }else if(r < 0){
        ++ni->stats->s.input_errors;
      }
      ni->inner_wants_data = true;
      pthread_mutex_unlock(&ni->lock);
      // specify a NULL timeout, meaning we block as long as we need, until
      // there's input available, or we are interrupted by a signal.
      logdebug("Blocking on input");
      if(block_on_input(ni->infd, NULL) < 1){
        pthread_mutex_lock(&ni->lock); // interrupted?
        break;
      }
      ni->inner_wants_data = false;
      logdebug("Reacquiring input lock");
      pthread_mutex_lock(&ni->lock);
    }
  }while(!ni->user_wants_data);
}
