#include "input.h"
#include "internal.h"
#include "notcurses/direct.h"
#include <poll.h>
#include <ncurses.h> // needed for some definitions, see terminfo(3ncurses)
#include <term.h>
#include <ctype.h>
#include <signal.h>

// CSI (Control Sequence Indicators) originate in the terminal itself, and are
// not reported in their bare form to the user. For our purposes, these usually
// indicate a mouse event.
#define CSIPREFIX "\x1b[<"
static const char32_t NCKEY_CSI = 1;

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
    fprintf(stderr, "Error disabling echo / canonical on %d (%s)\n", ttyfd, strerror(errno));
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
ncinputlayer_add_input_escape(ncinputlayer* nc, const char* esc, char32_t special){
  if(esc[0] != NCKEY_ESC || strlen(esc) < 2){ // assume ESC prefix + content
    fprintf(stderr, "Not an escape: %s (0x%x)\n", esc, special);
    return -1;
  }
  if(!nckey_supppuab_p(special) && special != NCKEY_CSI){
    fprintf(stderr, "Not a supplementary-b PUA char: %u (0x%x)\n", special, special);
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
    fprintf(stderr, "Warning: already added escape (got 0x%x, wanted 0x%x)\n", (*cur)->special, special);
  }else{
    (*cur)->special = special;
  }
  return 0;
}

// We received the CSI prefix. Extract the data payload.
static char32_t
handle_csi(ncinputlayer* nc, ncinput* ni, int leftmargin, int topmargin){
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
      if(candidate == ';'){
        state = PARAM3;
        if(param == 0){
          break;
        }
        if(ni){
          ni->x = param - 1 - leftmargin;
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
          ni->y = param - 1 - topmargin;
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
handle_getc(ncinputlayer* nc, int kpress, ncinput* ni, int leftmargin, int topmargin){
  if(kpress < 0){
    return -1;
  }
  if(kpress == NCKEY_ESC){
    const esctrie* esc = nc->inputescapes;
    int candidate = 0;
    while(esc && esc->special == NCKEY_INVALID && nc->inputbuf_occupied){
      candidate = pop_input_keypress(nc);
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
        return handle_csi(nc, ni, leftmargin, topmargin);
      }
      return esc->special;
    }
    // interpret it as alt + candidate FIXME broken for first char matching
    // trie, second char not -- will read as alt+second char...
    if(candidate > 0 && candidate < 0x80){
      if(ni){
        ni->alt = true;
      }
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
  return -1;
}

// blocks up through ts (infinite with NULL ts), returning number of events
// (0 on timeout) or -1 on error/interruption.
static int
block_on_input(int fd, const struct timespec* ts, const sigset_t* sigmask){
  struct pollfd pfd = {
    .fd = fd,
    .events = POLLIN,
    .revents = 0,
  };
  // we don't want to persistently modify the provided sigmask
  sigset_t scratchmask;
  if(sigmask){
    memcpy(&scratchmask, sigmask, sizeof(*sigmask));
  }else{
    sigfillset(&scratchmask);
  }
  sigdelset(&scratchmask, SIGCONT);
  sigdelset(&scratchmask, SIGWINCH);
  sigdelset(&scratchmask, SIGILL);
  sigdelset(&scratchmask, SIGSEGV);
  sigdelset(&scratchmask, SIGABRT);
  // now add those which we don't want while writing
  sigaddset(&scratchmask, SIGINT);
  sigaddset(&scratchmask, SIGQUIT);
  sigaddset(&scratchmask, SIGTERM);
#ifdef POLLRDHUP
  pfd.events |= POLLRDHUP;
#endif
  int events;
  while((events = ppoll(&pfd, 1, ts, &scratchmask)) < 0){
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

static bool
input_queue_full(const ncinputlayer* nc){
  return nc->inputbuf_occupied == sizeof(nc->inputbuf) / sizeof(*nc->inputbuf);
}

static char32_t
handle_queued_input(ncinputlayer* nc, ncinput* ni, int leftmargin, int topmargin){
  // if there was some error in getc(), we still dole out the existing queue
  if(nc->inputbuf_occupied == 0){
    return -1;
  }
  int r = pop_input_keypress(nc);
  char32_t ret = handle_getc(nc, r, ni, leftmargin, topmargin);
  if(ret != (char32_t)-1 && ni){
    ni->id = ret;
  }
  return ret;
}

static char32_t
handle_input(ncinputlayer* nc, ncinput* ni, int leftmargin, int topmargin,
             const sigset_t* sigmask){
  unsigned char c;
  while(!input_queue_full(nc) && read(nc->infd, &c, 1) > 0){
    nc->inputbuf[nc->inputbuf_write_at] = c;
//fprintf(stderr, "OCCUPY: %u@%u read: %d\n", nc->inputbuf_occupied, nc->inputbuf_write_at, nc->inputbuf[nc->inputbuf_write_at]);
    if(++nc->inputbuf_write_at == sizeof(nc->inputbuf) / sizeof(*nc->inputbuf)){
      nc->inputbuf_write_at = 0;
    }
    ++nc->inputbuf_occupied;
    const struct timespec ts = {};
    if(block_on_input(nc->infd, &ts, sigmask) < 1){
      break;
    }
  }
  // highest priority is resize notifications, since they don't queue
  if(resize_seen){
    resize_seen = 0;
    return NCKEY_SIGNAL;
  }
  return handle_queued_input(nc, ni, leftmargin, topmargin);
}

static char32_t
handle_ncinput(ncinputlayer* nc, ncinput* ni, int leftmargin, int topmargin,
               const sigset_t* sigmask){
  if(ni){
    memset(ni, 0, sizeof(*ni));
  }
  char32_t r = handle_input(nc, ni, leftmargin, topmargin, sigmask);
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
static inline char32_t
ncinputlayer_prestamp(ncinputlayer* nc, const struct timespec *ts,
                      const sigset_t* sigmask, ncinput* ni, int leftmargin,
                      int topmargin){
//fprintf(stderr, "PRESTAMP OCCUPADO: %d\n", nc->inputbuf_occupied);
  if(nc->inputbuf_occupied){
    return handle_queued_input(nc, ni, leftmargin, topmargin);
  }
  errno = 0;
  if(block_on_input(nc->infd, ts, sigmask) > 0){
//fprintf(stderr, "%d events from input!\n", events);
    return handle_ncinput(nc, ni, leftmargin, topmargin, sigmask);
  }
//fprintf(stderr, "ERROR: %d events from input!\n", events);
  return -1;
}

// infp has already been set non-blocking
char32_t notcurses_getc(notcurses* nc, const struct timespec *ts,
                        const sigset_t* sigmask, ncinput* ni){
  char32_t r = ncinputlayer_prestamp(&nc->tcache.input, ts, sigmask, ni,
                                     nc->margin_l, nc->margin_t);
  if(r != (char32_t)-1){
    uint64_t stamp = nc->tcache.input.input_events++; // need increment even if !ni
    if(ni){
      ni->seqnum = stamp;
    }
  }
  return r;
}

char32_t ncdirect_getc(ncdirect* nc, const struct timespec *ts,
                       sigset_t* sigmask, ncinput* ni){
  char32_t r = ncinputlayer_prestamp(&nc->tcache.input, ts, sigmask, ni, 0, 0);
  if(r != (char32_t)-1){
    uint64_t stamp = nc->tcache.input.input_events++; // need increment even if !ni
    if(ni){
      ni->seqnum = stamp;
    }
  }
  return r;
}

// load all known special keys from terminfo, and build the input sequence trie
static int
prep_special_keys(ncinputlayer* nc){
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
//fprintf(stderr, "support for terminfo's %s: %s\n", k->tinfo, seq);
    if(ncinputlayer_add_input_escape(nc, seq, k->key)){
      fprintf(stderr, "Couldn't add support for %s\n", k->tinfo);
      return -1;
    }
  }
  if(ncinputlayer_add_input_escape(nc, CSIPREFIX, NCKEY_CSI)){
    fprintf(stderr, "Couldn't add support for %s\n", k->tinfo);
    return -1;
  }
  return 0;
}

void ncinputlayer_stop(ncinputlayer* nilayer){
  if(nilayer->ttyfd >= 0){
    close(nilayer->ttyfd);
  }
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
  STATE_DA,   // primary DA   (CSI ? ... c) OR XTSMGRAPHICS OR DECRPM
  STATE_DA_1, // got '1', XTSMGRAPHICS color registers or primary DA
  STATE_DA_1_SEMI, // got '1;'
  STATE_DA_1_0, // got '1;0', XTSMGRAPHICS color registers or VT101
  STATE_DA_6, // got '6', could be VT102 or VT220/VT320/VT420
  STATE_DA_DRAIN, // drain out the primary DA to 'c'
  STATE_SIXEL,// XTSMGRAPHICS about Sixel geometry (got '2')
  STATE_SIXEL_SEMI1,   // got first semicolon in sixel geometry, want Ps
  STATE_SIXEL_SUCCESS, // got Ps == 0, want second semicolon
  STATE_SIXEL_WIDTH,   // reading maximum sixel width until ';'
  STATE_SIXEL_HEIGHT,  // reading maximum sixel height until 'S'
  STATE_SIXEL_CREGS,   // reading max color registers until 'S'
  STATE_XTSMGRAPHICS_DRAIN, // drain out XTSMGRAPHICS to 'S'
  STATE_APPSYNC_REPORT, // got DECRPT ?2026
  STATE_APPSYNC_REPORT_DRAIN, // drain out decrpt to 'y'
  STATE_CURSOR, // reading row of cursor location to ';'
  STATE_CURSOR_COL, // reading col of cursor location to 'R'
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
  int cursor_y, cursor_x;// cursor location
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
    // FIXME replace these with some structured loop
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
        { .prefix = "iTerm2 [", .suffix = ']', .term = TERMINAL_ITERM, },
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
      break;
    }case STATE_XTGETTCAP_TERMNAME1:
      if(strcmp(inits->runstring, "xterm-kitty") == 0){
        inits->qterm = TERMINAL_KITTY;
      }else if(strcmp(inits->runstring, "mlterm") == 0){
        inits->qterm = TERMINAL_MLTERM;
      }
      break;
    case STATE_TDA1:
      if(strcmp(inits->runstring, "~VTE") == 0){
        inits->qterm = TERMINAL_VTE;
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
      fprintf(stderr, "invalid string stashed %d\n", inits->stringstate);
      break;
  }
  inits->runstring[0] = '\0';
  inits->stridx = 0;
  return 0;
}

// FIXME ought implement the full Williams automaton
// FIXME doesn't handle 8-bit controls (would need convert UTF-8)
// FIXME sloppy af in general
// returns 1 after handling the Device Attributes response, 0 if more input
// ought be fed to the machine, and -1 on an invalid state transition.
static int
pump_control_read(query_state* inits, unsigned char c){
//fprintf(stderr, "state: %2d char: %1c %3d %02x\n", inits->state, isprint(c) ? c : ' ', c, c);
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
        inits->state = STATE_SDA;
      }else if(isdigit(c)){
        inits->numeric = 0;
        if(ruts_numeric(&inits->numeric, c)){
          return -1;
        }
        inits->state = STATE_CURSOR;
      }else if(c >= 0x40 && c <= 0x7E){
        inits->state = STATE_NULL;
      }
      break;
    case STATE_CURSOR:
      if(isdigit(c)){
        if(ruts_numeric(&inits->numeric, c)){
          return -1;
        }
      }else if(c == ';'){
        inits->cursor_y = inits->numeric;
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
        inits->cursor_x = inits->numeric;
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
        inits->state = STATE_DCS_DRAIN;
      }
      inits->numeric = 0;
      break;
    case STATE_SDA:
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
      }else if(c == 'c'){
        inits->state = STATE_NULL;
        return 1;
      }
      break;
    case STATE_DA_1:
      if(c == 'c'){
        inits->state = STATE_NULL;
        return 1;
      }else if(c == ';'){
        inits->state = STATE_DA_1_SEMI;
      }else{
        // FIXME error?
      }
      break;
    case STATE_DA_1_SEMI:
      if(c == '2'){ // VT100 with Advanced Video Option
        inits->state = STATE_DA_DRAIN;
      }else if(c == '0'){
        inits->state = STATE_DA_1_0;
      }
      break;
    case STATE_DA_1_0:
      if(c == 'c'){ // VT101 with No Options
        inits->state = STATE_NULL;
      }else if(c == ';'){
        inits->state = STATE_SIXEL_CREGS;
      }else{
        // FIXME error?
      }
      break;
    case STATE_SIXEL_CREGS:
      if(c == 'S'){
        inits->tcache->color_registers = inits->numeric;
        inits->state = STATE_NULL;
      }else if(ruts_numeric(&inits->numeric, c)){
        return -1;
      }
      break;
    case STATE_DA_6:
      if(c == 'c'){
        inits->state = STATE_NULL;
        return 1;
      }
      // FIXME
      break;
    case STATE_DA_DRAIN:
      if(c == 'c'){
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
        inits->tcache->sixel_maxx = inits->numeric;
        inits->state = STATE_SIXEL_HEIGHT;
        inits->numeric = 0;
      }else if(ruts_numeric(&inits->numeric, c)){
        return -1;
      }
      break;
    case STATE_SIXEL_HEIGHT:
      if(c == 'S'){
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
err:
  fprintf(stderr, "Reading control replies failed on %d (%s)\n", ttyfd, strerror(errno));
  free(buf);
  return -1;
}

int ncinputlayer_init(tinfo* tcache, FILE* infp, queried_terminals_e* detected,
                      unsigned* appsync, int* cursor_y, int* cursor_x){
  ncinputlayer* nilayer = &tcache->input;
  setbuffer(infp, NULL, 0);
  nilayer->inputescapes = NULL;
  nilayer->infd = fileno(infp);
  nilayer->ttyfd = isatty(nilayer->infd) ? -1 : get_tty_fd(infp);
  if(prep_special_keys(nilayer)){
    return -1;
  }
  nilayer->inputbuf_occupied = 0;
  nilayer->inputbuf_valid_starts = 0;
  nilayer->inputbuf_write_at = 0;
  nilayer->input_events = 0;
  int csifd = nilayer->ttyfd >= 0 ? nilayer->ttyfd : nilayer->infd;
  if(isatty(csifd)){
    query_state inits = {
      .tcache = tcache,
      .state = STATE_NULL,
      .qterm = TERMINAL_UNKNOWN,
      .cursor_x = -1,
      .cursor_y = -1,
    };
    if(control_read(csifd, &inits)){
      input_free_esctrie(&nilayer->inputescapes);
      free(inits.version);
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
  }
  return 0;
}
