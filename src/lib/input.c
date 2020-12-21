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

void sigwinch_handler(int signo){
  resize_seen = signo;
}

int cbreak_mode(int ttyfd, const struct termios* tpreserved){
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
    logerror(n, "Couldn't preserve terminal state for %d (%s)\n", n->ttyfd, strerror(errno));
    return -1;
  }
  tios.c_lflag &= ~ISIG;
  if(tcsetattr(n->ttyfd, TCSANOW, &tios)){
    logerror(n, "Error disabling signals on %d (%s)\n", n->ttyfd, strerror(errno));
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
    logerror(n, "Couldn't preserve terminal state for %d (%s)\n", n->ttyfd, strerror(errno));
    return -1;
  }
  tios.c_lflag |= ~ISIG;
  if(tcsetattr(n->ttyfd, TCSANOW, &tios)){
    logerror(n, "Error disabling signals on %d (%s)\n", n->ttyfd, strerror(errno));
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
block_on_input(FILE* fp, const struct timespec* ts, sigset_t* sigmask){
  struct pollfd pfd = {
    .fd = fileno(fp),
    .events = POLLIN,
    .revents = 0,
  };
  // we don't want to persistently modify the provided sigmask
  sigset_t scratchmask;
  if(sigmask){
    memcpy(&scratchmask, sigmask, sizeof(*sigmask));
  }else{
    pthread_sigmask(0, NULL, &scratchmask);
  }
  sigmask = &scratchmask;
  sigdelset(sigmask, SIGWINCH);
  sigdelset(sigmask, SIGINT);
  sigdelset(sigmask, SIGQUIT);
  sigdelset(sigmask, SIGSEGV);
  sigdelset(sigmask, SIGABRT);
  sigdelset(sigmask, SIGTERM);
#ifdef POLLRDHUP
  pfd.events |= POLLRDHUP;
#endif
  int events;
  while((events = ppoll(&pfd, 1, ts, sigmask)) < 0){
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
handle_input(ncinputlayer* nc, ncinput* ni, int leftmargin, int topmargin, sigset_t* sigmask){
  int r;
  // getc() returns unsigned chars cast to ints
  while(!input_queue_full(nc) && (r = getc(nc->ttyinfp)) >= 0){
    nc->inputbuf[nc->inputbuf_write_at] = (unsigned char)r;
//fprintf(stderr, "OCCUPY: %u@%u read: %d\n", nc->inputbuf_occupied, nc->inputbuf_write_at, nc->inputbuf[nc->inputbuf_write_at]);
    if(++nc->inputbuf_write_at == sizeof(nc->inputbuf) / sizeof(*nc->inputbuf)){
      nc->inputbuf_write_at = 0;
    }
    ++nc->inputbuf_occupied;
    const struct timespec ts = {};
    if(block_on_input(nc->ttyinfp, &ts, sigmask) < 1){
      break;
    }
  }
  // highest priority is resize notifications, since they don't queue
  if(resize_seen){
    resize_seen = 0;
    return NCKEY_RESIZE;
  }
  return handle_queued_input(nc, ni, leftmargin, topmargin);
}

static char32_t
handle_ncinput(ncinputlayer* nc, ncinput* ni, int leftmargin, int topmargin,
               sigset_t* sigmask){
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
                      sigset_t* sigmask, ncinput* ni, int leftmargin,
                      int topmargin){
//fprintf(stderr, "PRESTAMP OCCUPADO: %d\n", nc->inputbuf_occupied);
  if(nc->inputbuf_occupied){
    return handle_queued_input(nc, ni, leftmargin, topmargin);
  }
  errno = 0;
  if(block_on_input(nc->ttyinfp, ts, sigmask) > 0){
//fprintf(stderr, "%d events from input!\n", events);
    return handle_ncinput(nc, ni, leftmargin, topmargin, sigmask);
  }
//fprintf(stderr, "ERROR: %d events from input!\n", events);
  return -1;
}

// infp has already been set non-blocking
char32_t notcurses_getc(notcurses* nc, const struct timespec *ts,
                        sigset_t* sigmask, ncinput* ni){
  char32_t r = ncinputlayer_prestamp(&nc->input, ts, sigmask, ni,
                                     nc->margin_l, nc->margin_t);
  if(r != (char32_t)-1){
    uint64_t stamp = nc->input.input_events++; // need increment even if !ni
    if(ni){
      ni->seqnum = stamp;
    }
  }
  return r;
}

char32_t ncdirect_getc(ncdirect* nc, const struct timespec *ts,
                       sigset_t* sigmask, ncinput* ni){
  char32_t r = ncinputlayer_prestamp(&nc->input, ts, sigmask, ni, 0, 0);
  if(r != (char32_t)-1){
    uint64_t stamp = nc->input.input_events++; // need increment even if !ni
    if(ni){
      ni->seqnum = stamp;
    }
  }
  return r;
}

int prep_special_keys(ncinputlayer* nc){
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
