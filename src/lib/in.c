#include <stdio.h>
#include "automaton.h"
#include "internal.h"
#include "in.h"

// Notcurses takes over stdin, and if it is not connected to a terminal, also
// tries to make a connection to the controlling terminal. If such a connection
// is made, it will read from that source (in addition to stdin). We dump one or
// both into distinct buffers. We then try to lex structured elements out of
// the buffer(s). We can extract cursor location reports, mouse events, and
// UTF-8 characters. Completely extracted ones are placed in their appropriate
// queues, and removed from the depository buffer. We aim to consume the
// entirety of the deposit before going back to read more data, but let anyone
// blocking on data wake up as soon as we've processed any input.
//
// The primary goal is to react to terminal messages (mostly cursor location
// reports) as quickly as possible, and definitely not with unbounded latency,
// without unbounded allocation, and also without losing data. We'd furthermore
// like to reliably differentiate escapes and regular input, even when that
// latter contains escapes. Unbounded input will hopefully only be present when
// redirected from a file (NCOPTION_TOSS_INPUT)

// FIXME still need to:
//  integrate main specials trie with automaton, enable input_errors
//  probably want pipes/eventfds rather than SIGCONT

static sig_atomic_t resize_seen;

// called for SIGWINCH and SIGCONT, and causes block_on_input to return
void sigwinch_handler(int signo){
  if(signo == SIGWINCH){
    resize_seen = signo;
  }
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

// local state for the input thread. don't put this large struct on the stack.
typedef struct inputctx {
  int stdinfd;          // bulk in fd. always >= 0 (almost always 0). we do not
                        //  own this descriptor, and must not close() it.
  int termfd;           // terminal fd: -1 with no controlling terminal, or
                        //  if stdin is a terminal, or on MSFT Terminal.
#ifdef __MINGW64__
  HANDLE stdinhandle;   // handle to input terminal for MSFT Terminal
#endif

  int lmargin, tmargin; // margins in use at left and top

  automaton amata;

  // these two are not ringbuffers; we always move any leftover materia to the
  // front of the queue (it ought be a handful of bytes at most).
  unsigned char ibuf[BUFSIZ]; // might be intermingled bulk/control data
  unsigned char tbuf[BUFSIZ]; // only used if we have distinct terminal fd
  int ibufvalid;      // we mustn't read() if ibufvalid == sizeof(ibuf)
  int tbufvalid;      // only used if we have distinct terminal connection

  // ringbuffers for processed, structured input
  cursorloc* csrs;    // cursor reports are dumped here
  ncinput* inputs;    // processed input is dumped here
  int csize, isize;   // total number of slots in csrs/inputs
  int cvalid, ivalid; // population count of csrs/inputs
  int cwrite, iwrite; // slot where we'll write the next csr/input;
                      //  we cannot write if valid == size
  int cread, iread;   // slot from which clients read the next csr/input;
                      //  they cannot read if valid == 0
  pthread_mutex_t ilock; // lock for ncinput ringbuffer, also initial state
  pthread_cond_t icond;  // condvar for ncinput ringbuffer
  pthread_mutex_t clock; // lock for csrs ringbuffer
  pthread_cond_t ccond;  // condvar for csrs ringbuffer
  tinfo* ti;          // link back to tinfo
  pthread_t tid;      // tid for input thread

  unsigned midescape; // we're in the middle of a potential escape. we need
                      //  to do a nonblocking read and try to complete it.
  unsigned stdineof;  // have we seen an EOF on stdin?

  unsigned linesigs;  // are line discipline signals active?
  unsigned drain;     // drain away bulk input?
  ncsharedstats *stats; // stats shared with notcurses context

  struct initial_responses* initdata;
  struct initial_responses* initdata_complete;
} inputctx;

static inline void
inc_input_events(inputctx* ictx){
  pthread_mutex_lock(&ictx->stats->lock);
  ++ictx->stats->s.input_events;
  pthread_mutex_unlock(&ictx->stats->lock);
}

static inline void
inc_input_errors(inputctx* ictx){
  pthread_mutex_lock(&ictx->stats->lock);
  ++ictx->stats->s.input_errors;
  pthread_mutex_unlock(&ictx->stats->lock);
}

static void
send_synth_signal(int sig){
#ifndef __MINGW64__
  if(sig){
    raise(sig);
  }
#else
  (void)sig; // FIXME
#endif
}

// load all known special keys from terminfo, and build the input sequence trie
static int
prep_special_keys(inputctx* ictx){
  /*
#ifndef __MINGW64__
  static const struct {
    const char* tinfo;
    uint32_t key;
    bool shift, ctrl, alt;
  } keys[] = {
    { .tinfo = "kcbt",  .key = '\t', .shift = true, },
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
    { .tinfo = "kDC",   .key = NCKEY_DEL, .shift = 1, },
    { .tinfo = "kDC3",  .key = NCKEY_DEL, .alt = 1, },
    { .tinfo = "kDC4",  .key = NCKEY_DEL, .alt = 1, .shift = 1, },
    { .tinfo = "kDC5",  .key = NCKEY_DEL, .ctrl = 1, },
    { .tinfo = "kDC6",  .key = NCKEY_DEL, .ctrl = 1, .shift = 1, },
    { .tinfo = "kDC7",  .key = NCKEY_DEL, .alt = 1, .ctrl = 1, },
    { .tinfo = "kDN",   .key = NCKEY_DOWN, .shift = 1, },
    { .tinfo = "kDN3",  .key = NCKEY_DOWN, .alt = 1, },
    { .tinfo = "kDN4",  .key = NCKEY_DOWN, .alt = 1, .shift = 1, },
    { .tinfo = "kDN5",  .key = NCKEY_DOWN, .ctrl = 1, },
    { .tinfo = "kDN6",  .key = NCKEY_DOWN, .ctrl = 1, .shift = 1, },
    { .tinfo = "kDN7",  .key = NCKEY_DOWN, .alt = 1, .ctrl = 1, },
    { .tinfo = "kEND",  .key = NCKEY_END, .shift = 1, },
    { .tinfo = "kEND3", .key = NCKEY_END, .alt = 1, },
    { .tinfo = "kEND4", .key = NCKEY_END, .alt = 1, .shift = 1, },
    { .tinfo = "kEND5", .key = NCKEY_END, .ctrl = 1, },
    { .tinfo = "kEND6", .key = NCKEY_END, .ctrl = 1, .shift = 1, },
    { .tinfo = "kEND7", .key = NCKEY_END, .alt = 1, .ctrl = 1, },
    { .tinfo = "kHOM",  .key = NCKEY_HOME, .shift = 1, },
    { .tinfo = "kHOM3", .key = NCKEY_HOME, .alt = 1, },
    { .tinfo = "kHOM4", .key = NCKEY_HOME, .alt = 1, .shift = 1, },
    { .tinfo = "kHOM5", .key = NCKEY_HOME, .ctrl = 1, },
    { .tinfo = "kHOM6", .key = NCKEY_HOME, .ctrl = 1, .shift = 1, },
    { .tinfo = "kHOM7", .key = NCKEY_HOME, .alt = 1, .ctrl = 1, },
    { .tinfo = "kIC",   .key = NCKEY_INS, .shift = 1, },
    { .tinfo = "kIC3",  .key = NCKEY_INS, .alt = 1, },
    { .tinfo = "kIC4",  .key = NCKEY_INS, .alt = 1, .shift = 1, },
    { .tinfo = "kIC5",  .key = NCKEY_INS, .ctrl = 1, },
    { .tinfo = "kIC6",  .key = NCKEY_INS, .ctrl = 1, .shift = 1, },
    { .tinfo = "kIC7",  .key = NCKEY_INS, .alt = 1, .ctrl = 1, },
    { .tinfo = "kLFT",  .key = NCKEY_LEFT, .shift = 1, },
    { .tinfo = "kLFT3", .key = NCKEY_LEFT, .alt = 1, },
    { .tinfo = "kLFT4", .key = NCKEY_LEFT, .alt = 1, .shift = 1, },
    { .tinfo = "kLFT5", .key = NCKEY_LEFT, .ctrl = 1, },
    { .tinfo = "kLFT6", .key = NCKEY_LEFT, .ctrl = 1, .shift = 1, },
    { .tinfo = "kLFT7", .key = NCKEY_LEFT, .alt = 1, .ctrl = 1, },
    { .tinfo = "kNXT",  .key = NCKEY_PGDOWN, .shift = 1, },
    { .tinfo = "kNXT3", .key = NCKEY_PGDOWN, .alt = 1, },
    { .tinfo = "kNXT4", .key = NCKEY_PGDOWN, .alt = 1, .shift = 1, },
    { .tinfo = "kNXT5", .key = NCKEY_PGDOWN, .ctrl = 1, },
    { .tinfo = "kNXT6", .key = NCKEY_PGDOWN, .ctrl = 1, .shift = 1, },
    { .tinfo = "kNXT7", .key = NCKEY_PGDOWN, .alt = 1, .ctrl = 1, },
    { .tinfo = "kPRV",  .key = NCKEY_PGUP, .shift = 1, },
    { .tinfo = "kPRV3", .key = NCKEY_PGUP, .alt = 1, },
    { .tinfo = "kPRV4", .key = NCKEY_PGUP, .alt = 1, .shift = 1, },
    { .tinfo = "kPRV5", .key = NCKEY_PGUP, .ctrl = 1, },
    { .tinfo = "kPRV6", .key = NCKEY_PGUP, .ctrl = 1, .shift = 1, },
    { .tinfo = "kPRV7", .key = NCKEY_PGUP, .alt = 1, .ctrl = 1, },
    { .tinfo = "kRIT",  .key = NCKEY_RIGHT, .shift = 1, },
    { .tinfo = "kRIT3", .key = NCKEY_RIGHT, .alt = 1, },
    { .tinfo = "kRIT4", .key = NCKEY_RIGHT, .alt = 1, .shift = 1, },
    { .tinfo = "kRIT5", .key = NCKEY_RIGHT, .ctrl = 1, },
    { .tinfo = "kRIT6", .key = NCKEY_RIGHT, .ctrl = 1, .shift = 1, },
    { .tinfo = "kRIT7", .key = NCKEY_RIGHT, .alt = 1, .ctrl = 1, },
    { .tinfo = "kUP",   .key = NCKEY_UP, .shift = 1, },
    { .tinfo = "kUP3",  .key = NCKEY_UP, .alt = 1, },
    { .tinfo = "kUP4",  .key = NCKEY_UP, .alt = 1, .shift = 1, },
    { .tinfo = "kUP5",  .key = NCKEY_UP, .ctrl = 1, },
    { .tinfo = "kUP6",  .key = NCKEY_UP, .ctrl = 1, .shift = 1, },
    { .tinfo = "kUP7",  .key = NCKEY_UP, .alt = 1, .ctrl = 1, },
    { .tinfo = NULL,    .key = 0, }
  }, *k;
  for(k = keys ; k->tinfo ; ++k){
    char* seq = tigetstr(k->tinfo);
    if(seq == NULL || seq == (char*)-1){
      loginfo("no terminfo declaration for %s\n", k->tinfo);
      continue;
    }
    if(seq[0] != NCKEY_ESC || strlen(seq) < 2){ // assume ESC prefix + content
      logwarn("invalid escape: %s (0x%x)\n", k->tinfo, k->key);
      continue;
    }
    if(inputctx_add_input_escape(&ictx->amata, seq, k->key, k->shift, k->ctrl, k->alt)){
      return -1;
    }
    logdebug("support for terminfo's %s: %s\n", k->tinfo, seq);
  }
#endif
  */
  (void)ictx;
  return 0;
}

// get the CSI node from the trie
static inline struct esctrie*
csi_node(automaton *amata){
  struct esctrie* e = amata->escapes;
  return esctrie_trie(e)['['];
}

// get the DCS node from the trie
static inline struct esctrie*
dcs_node(automaton *amata){
  struct esctrie* e = amata->escapes;
  return esctrie_trie(e)['P'];
}

// ictx->numeric, ictx->p3, and ictx->p2 have the two parameters. we're using
// SGR (1006) mouse encoding, so use the final character to determine release
// ('M' for click, 'm' for release).
static void
mouse_click(inputctx* ictx, unsigned release){
  struct esctrie* e = csi_node(&ictx->amata);
  e = esctrie_trie(e)['<'];
  e = esctrie_trie(e)['0'];
  const int mods = esctrie_numeric(e);
  e = esctrie_trie(e)[';'];
  e = esctrie_trie(e)['0'];
  const int x = esctrie_numeric(e) - 1 - ictx->lmargin;
  e = esctrie_trie(e)[';'];
  e = esctrie_trie(e)['0'];
  const int y = esctrie_numeric(e) - 1 - ictx->tmargin;
  // convert from 1- to 0-indexing, and account for margins
  if(x < 0 || y < 0){ // click was in margins, drop it
    logwarn("dropping click in margins %d/%d\n", y, x);
    return;
  }
  pthread_mutex_lock(&ictx->ilock);
  if(ictx->ivalid == ictx->isize){
    pthread_mutex_unlock(&ictx->ilock);
    logerror("dropping mouse click 0x%02x %d %d\n", mods, y, x);
    inc_input_errors(ictx);
    return;
  }
  ncinput* ni = ictx->inputs + ictx->iwrite;
  if(mods >= 0 && mods < 64){
    ni->id = NCKEY_BUTTON1 + (mods % 4);
  }else if(mods >= 64 && mods < 128){
    ni->id = NCKEY_BUTTON4 + (mods % 4);
  }else if(mods >= 128 && mods < 192){
    ni->id = NCKEY_BUTTON8 + (mods % 4);
  }
  ni->ctrl = mods & 0x10;
  ni->alt = mods & 0x08;
  ni->shift = mods & 0x04;
  // mice don't send repeat events, so we know it's either release or press
  if(release){
    ni->evtype = NCTYPE_RELEASE;
  }else{
    ni->evtype = NCTYPE_PRESS;
  }
  ni->x = x;
  ni->y = y;
  if(++ictx->iwrite == ictx->isize){
    ictx->iwrite = 0;
  }
  ++ictx->ivalid;
  pthread_mutex_unlock(&ictx->ilock);
  pthread_cond_broadcast(&ictx->icond);
}

static int
mouse_press_cb(inputctx* ictx){
  mouse_click(ictx, 0);
  return 0;
}

static int
mouse_release_cb(inputctx* ictx){
  mouse_click(ictx, 1);
  return 0;
}

static int
cursor_location_cb(inputctx* ictx){
  struct esctrie* e = csi_node(&ictx->amata);
  e = esctrie_trie(e)['0'];
  int y = esctrie_numeric(e) - 1;
  e = esctrie_trie(e)[';'];
  e = esctrie_trie(e)['0'];
  int x = esctrie_numeric(e) - 1;
  pthread_mutex_lock(&ictx->clock);
  if(ictx->cvalid == ictx->csize){
    pthread_mutex_unlock(&ictx->clock);
    logwarn("dropping cursor location report %d/%d\n", y, x);
    inc_input_errors(ictx);
  }else{
    cursorloc* cloc = &ictx->csrs[ictx->cwrite];
    if(++ictx->cwrite == ictx->csize){
      ictx->cwrite = 0;
    }
    cloc->y = y;
    cloc->x = x;
    ++ictx->cvalid;
    pthread_mutex_unlock(&ictx->clock);
    pthread_cond_broadcast(&ictx->ccond);
    loginfo("cursor location: %d/%d\n", y, x);
  }
  return 0;
}

static int
geom_cb(inputctx* ictx){
  struct esctrie* e = csi_node(&ictx->amata);
  e = esctrie_trie(e)['0'];
  int kind = esctrie_numeric(e);
  e = esctrie_trie(e)[';'];
  e = esctrie_trie(e)['0'];
  int y = esctrie_numeric(e);
  e = esctrie_trie(e)[';'];
  e = esctrie_trie(e)['0'];
  int x = esctrie_numeric(e);
  if(kind == 4){ // pixel geometry
    if(ictx->initdata){
      ictx->initdata->pixy = y;
      ictx->initdata->pixx = x;
    }
    loginfo("pixel geom report %d/%d\n", y, x);
  }else if(kind == 8){ // cell geometry
    if(ictx->initdata){
      ictx->initdata->dimy = y;
      ictx->initdata->dimx = x;
    }
    loginfo("cell geom report %d/%d\n", y, x);
  }else{
    logerror("invalid geom report type: %d\n", kind);
    return -1;
  }
  return 0;
}

// ictx->numeric and ictx->p2 have the two parameters, where ictx->numeric was
// optional and indicates a special key with no modifiers.
static void
kitty_kbd(inputctx* ictx, int val, int mods){
  int synth = 0;
  assert(mods >= 0);
  assert(val > 0);
  ncinput tni = {
    .id = val == 0x7f ? NCKEY_BACKSPACE : val,
    .shift = !!((mods - 1) & 0x1),
    .alt = !!((mods - 1) & 0x2),
    .ctrl = !!((mods - 1) & 0x4),
  };
  // FIXME decode remaining modifiers through 128
  // standard keyboard protocol reports ctrl+ascii as the capital form,
  // so (for now) conform when using kitty protocol...
  if(tni.ctrl){
    if(tni.id < 128 && islower(tni.id)){
      tni.id = toupper(tni.id);
    }
    if(!tni.alt && !tni.shift){
      if(tni.id == 'C'){
        synth = SIGINT;
      }else if(tni.id == '\\'){
        synth = SIGQUIT;
      }
    }
  }
  tni.x = 0;
  tni.y = 0;
  pthread_mutex_lock(&ictx->ilock);
  if(ictx->ivalid == ictx->isize){
    pthread_mutex_unlock(&ictx->ilock);
    logerror("dropping input 0x%08x 0x%02x\n", val, mods);
    inc_input_errors(ictx);
    send_synth_signal(synth);
    return;
  }
  ncinput* ni = ictx->inputs + ictx->iwrite;
  memcpy(ni, &tni, sizeof(tni));
  if(++ictx->iwrite == ictx->isize){
    ictx->iwrite = 0;
  }
  ++ictx->ivalid;
  pthread_mutex_unlock(&ictx->ilock);
  pthread_cond_broadcast(&ictx->icond);
  send_synth_signal(synth);
}

static int
kitty_cb_simple(inputctx* ictx){
  struct esctrie* e = csi_node(&ictx->amata);
  e = esctrie_trie(e)['0'];
  int val = esctrie_numeric(e);
  kitty_kbd(ictx, val, 0);
  return 0;
}

static int
kitty_cb(inputctx* ictx){
  struct esctrie* e = csi_node(&ictx->amata);
  e = esctrie_trie(e)['0'];
  int val = esctrie_numeric(e);
  e = esctrie_trie(e)[';'];
  e = esctrie_trie(e)['0'];
  int mods = esctrie_numeric(e);
  kitty_kbd(ictx, val, mods);
  return 0;
}

// the only xtsmgraphics reply with a single Pv arg is color registers
static int
xtsmgraphics_cregs_cb(inputctx* ictx){
  struct esctrie* e = csi_node(&ictx->amata);
  e = esctrie_trie(e)['?'];
  e = esctrie_trie(e)['0'];
  int xtype = esctrie_numeric(e); // expect 1 for color registers
  e = esctrie_trie(e)[';'];
  e = esctrie_trie(e)['0'];
  int ps = esctrie_numeric(e);
  e = esctrie_trie(e)[';'];
  e = esctrie_trie(e)['0'];
  int pv = esctrie_numeric(e);
  if(xtype != 1){
    logerror("expected type 1 color registers got %d\n", xtype);
    return -1;
  }
  if(ps != 0){
    logerror("expected status 0 got %d\n", ps);
  }else if(ictx->initdata){
    ictx->initdata->color_registers = pv;
    loginfo("sixel color registers: %d\n", ictx->initdata->color_registers);
  }
  return 0;
}

// the only xtsmgraphics reply with a dual Pv arg we want is sixel geometry
static int
xtsmgraphics_sixel_cb(inputctx* ictx){
  struct esctrie* e = csi_node(&ictx->amata);
  e = esctrie_trie(e)['?'];
  e = esctrie_trie(e)['0'];
  int xtype = esctrie_numeric(e); // expect 2 for color registers
  e = esctrie_trie(e)[';'];
  e = esctrie_trie(e)['0'];
  int ps = esctrie_numeric(e);
  e = esctrie_trie(e)[';'];
  e = esctrie_trie(e)['0'];
  int width = esctrie_numeric(e);
  e = esctrie_trie(e)[';'];
  e = esctrie_trie(e)['0'];
  int height = esctrie_numeric(e);
  if(xtype != 2){
    logerror("expected type 2 sixel geom got %d\n", xtype);
    return -1;
  }
  if(ps != 0){
    logerror("expected status 0 got %d\n", ps);
  }else if(ictx->initdata){
    ictx->initdata->sixelx = width;
    ictx->initdata->sixely = height;
    loginfo("max sixel geometry: %dx%d\n", ictx->initdata->sixely, ictx->initdata->sixelx);
  }
  return 0;
}

static void
handoff_initial_responses(inputctx* ictx){
  pthread_mutex_lock(&ictx->ilock);
  ictx->initdata_complete = ictx->initdata;
  ictx->initdata = NULL;
  pthread_mutex_unlock(&ictx->ilock);
  pthread_cond_broadcast(&ictx->icond);
  loginfo("handing off initial responses\n");
}

static int
da1_cb(inputctx* ictx){
  loginfo("read device attributes\n");
  if(ictx->initdata){
    handoff_initial_responses(ictx);
  }
  return 2;
}

static int
kittygraph_cb(inputctx* ictx){
  loginfo("kitty graphics message\n");
  if(ictx->initdata){
    ictx->initdata->kitty_graphics = 1;
  }
  return 2;
}

static int
decrpm_cb(inputctx* ictx){
  struct esctrie* e = csi_node(&ictx->amata);
  e = esctrie_trie(e)['?'];
  e = esctrie_trie(e)['0'];
  int pd = esctrie_numeric(e); // expect 2 for color registers
  e = esctrie_trie(e)[';'];
  e = esctrie_trie(e)['0'];
  int ps = esctrie_numeric(e);
  loginfo("received decrpm %d %d\n", pd, ps);
  if(pd == 2026 && ps == 2){
    if(ictx->initdata){
      ictx->initdata->appsync_supported = 1;
    }
  }
  return 2;
}

static int
bgdef_cb(inputctx* ictx){
  if(ictx->initdata == NULL){
    return 2;
  }
  /*
      case STATE_BG1:{
        int r, g, b;
        if(sscanf(ictx->runstring, "rgb:%02x/%02x/%02x", &r, &g, &b) == 3){
          // great! =]
        }else if(sscanf(ictx->runstring, "rgb:%04x/%04x/%04x", &r, &g, &b) == 3){
          r /= 256;
          g /= 256;
          b /= 256;
        }else{
          break;
        }
        inits->bg = (r << 16u) | (g << 8u) | b;
        break;
        */
  return 2;
}

// use the version extracted from Secondary Device Attributes, assuming that
// it is Alacritty (we ought check the specified terminfo database entry).
// Alacritty writes its crate version with each more significant portion
// multiplied by 100^{portion ID}, where major, minor, patch are 2, 1, 0.
// what happens when a component exceeds 99? who cares. support XTVERSION.
/*
static char*
set_sda_version(inputctx* ictx){
  int maj, min, patch;
  if(ictx->numeric <= 0){
    return NULL;
  }
  maj = ictx->numeric / 10000;
  min = (ictx->numeric % 10000) / 100;
  patch = ictx->numeric % 100;
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
*/

static int
extract_xtversion(inputctx* ictx, const char* str, char suffix){
  size_t slen = strlen(str);
  if(slen == 0){
    logwarn("empty version in xtversion\n");
    return -1;
  }
  if(suffix){
    if(str[slen - 1] != suffix){
      return -1;
    }
    --slen;
  }
  if(slen == 0){
    logwarn("empty version in xtversion\n");
    return -1;
  }
  ictx->initdata->version = strndup(str, slen);
  return 0;
}

static int
xtversion_cb(inputctx* ictx){
  struct esctrie* e = dcs_node(&ictx->amata);
  e = esctrie_trie(e)['>'];
  e = esctrie_trie(e)['|'];
  e = esctrie_trie(e)['a'];
  const char* xtversion = esctrie_string(e);
  if(xtversion == NULL){
    logwarn("empty xtversion\n");
    return 2; // don't replay as input
  }
  if(ictx->initdata == NULL){
    return 1;
  }
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
    { .prefix = "mintty ", .suffix = 0, .term = TERMINAL_MINTTY, },
    { .prefix = NULL, .suffix = 0, .term = TERMINAL_UNKNOWN, },
  }, *xtv;
  for(xtv = xtvers ; xtv->prefix ; ++xtv){
    if(strncmp(xtversion, xtv->prefix, strlen(xtv->prefix)) == 0){
      if(extract_xtversion(ictx, xtversion + strlen(xtv->prefix), xtv->suffix) == 0){
        loginfo("found terminal type %d version %s\n", xtv->term, ictx->initdata->version);
        ictx->initdata->qterm = xtv->term;
      }else{
        return -1;
      }
      break;
    }
  }
  if(xtv->prefix == NULL){
    logwarn("unknown xtversion [%s]\n", xtversion);
  }
  return 2;
}

static int
tcap_cb(inputctx* ictx){
  struct esctrie* e = dcs_node(&ictx->amata);
  e = esctrie_trie(e)['1'];
  e = esctrie_trie(e)['+'];
  e = esctrie_trie(e)['r'];
  e = esctrie_trie(e)['0'];
  int cap = esctrie_numeric(e);
  e = esctrie_trie(e)['='];
  int val = esctrie_numeric(e);
  if(cap == 0x544e){ // 'TN' terminal name
    loginfo("got TN capability %d\n", val);
    /* FIXME
        if(strcmp(ictx->runstring, "xterm-kitty") == 0){
          inits->qterm = TERMINAL_KITTY;
        }else if(strcmp(ictx->runstring, "mlterm") == 0){
          // MLterm prior to late 3.9.1 only reports via XTGETTCAP
          inits->qterm = TERMINAL_MLTERM;
        }
        break;
        */
  }
  return 2;
}

static int
tda_cb(inputctx* ictx){
  struct esctrie* e = dcs_node(&ictx->amata);
  e = esctrie_trie(e)['!'];
  e = esctrie_trie(e)['|'];
  const char* str = esctrie_string(e);
  if(str == NULL){
    logwarn("empty ternary device attribute\n");
    return 2; // don't replay
  }
  // FIXME hex encoded
  loginfo("got TDA: %s\n", str);
  /*
        if(strcmp(ictx->runstring, "~VTE") == 0){
          inits->qterm = TERMINAL_VTE;
        }else if(strcmp(ictx->runstring, "~~TY") == 0){
          inits->qterm = TERMINAL_TERMINOLOGY;
        }else if(strcmp(ictx->runstring, "FOOT") == 0){
          inits->qterm = TERMINAL_FOOT;
        }
        */
  return 2;
}

static int
build_cflow_automaton(inputctx* ictx){
  // syntax: literals are matched. \N is a numeric. \D is a drain (Kleene
  // closure). \S is a ST-terminated string. \H is a hex-encoded string.
  const struct {
    const char* cflow;
    triefunc fxn;
  } csis[] = {
    // CSI (\e[)
    { "[<\\N;\\N;\\NM", mouse_press_cb, },
    { "[<\\N;\\N;\\Nm", mouse_release_cb, },
    { "[\\N;\\NR", cursor_location_cb, },
    // technically these must begin with "4" or "8"; enforce in callbacks
    { "[\\N;\\N;\\Nt", geom_cb, },
    { "[\\Nu", kitty_cb_simple, },
    { "[\\N;\\Nu", kitty_cb, },
    { "[?1;2c", da1_cb, }, // CSI ? 1 ; 2 c  ("VT100 with Advanced Video Option")
    { "[?1;0c", da1_cb, }, // CSI ? 1 ; 0 c  ("VT101 with No Options")
    { "[?4;6c", da1_cb, }, // CSI ? 4 ; 6 c  ("VT132 with Advanced Video and Graphics")
    { "[?6c", da1_cb, },   // CSI ? 6 c  ("VT102")
    { "[?7c", da1_cb, },   // CSI ? 7 c  ("VT131")
    { "[?12;\\Dc", da1_cb, }, // CSI ? 1 2 ; Ps c  ("VT125")
    { "[?62;\\Dc", da1_cb, }, // CSI ? 6 2 ; Ps c  ("VT220")
    { "[?63;\\Dc", da1_cb, }, // CSI ? 6 3 ; Ps c  ("VT320")
    { "[?64;\\Dc", da1_cb, }, // CSI ? 6 4 ; Ps c  ("VT420")
    { "[?\\N;\\N;\\NS", xtsmgraphics_cregs_cb, },
    { "[?\\N;\\N;\\N;\\NS", xtsmgraphics_sixel_cb, },
    { "[?\\N;\\N$y", decrpm_cb, },
    // DCS (\eP...ST)
    { "P1+r\\H=\\H", tcap_cb, }, // positive XTGETTCAP
    { "P0+r\\H", NULL, },        // negative XTGETTCAP
    { "P!|\\S", tda_cb, },
    { "P>|\\S", xtversion_cb, },
    // OSC (\e_...ST)
    { "_G\\S", kittygraph_cb, },
    // a mystery to everyone!
    { "11;\\S", bgdef_cb, },
    { NULL, NULL, },
  }, *csi;
  for(csi = csis ; csi->cflow ; ++csi){
    if(inputctx_add_cflow(&ictx->amata, csi->cflow, csi->fxn)){
      logerror("failed adding %p via %s\n", csi->fxn, csi->cflow);
      return -1;
    }
    loginfo("added %p via %s\n", csi->fxn, csi->cflow);
  }
  return 0;
}

static inline inputctx*
create_inputctx(tinfo* ti, FILE* infp, int lmargin, int tmargin,
                ncsharedstats* stats, unsigned drain,
                int linesigs_enabled){
  inputctx* i = malloc(sizeof(*i));
  if(i){
    i->csize = 64;
    if( (i->csrs = malloc(sizeof(*i->csrs) * i->csize)) ){
      i->isize = BUFSIZ;
      if( (i->inputs = malloc(sizeof(*i->inputs) * i->isize)) ){
        if(pthread_mutex_init(&i->ilock, NULL) == 0){
          if(pthread_cond_init(&i->icond, NULL) == 0){
            if(pthread_mutex_init(&i->clock, NULL) == 0){
              if(pthread_cond_init(&i->ccond, NULL) == 0){
                if((i->stdinfd = fileno(infp)) >= 0){
                  if( (i->initdata = malloc(sizeof(*i->initdata))) ){
                    memset(&i->amata, 0, sizeof(i->amata));
                    if(prep_special_keys(i) == 0){
                      if(set_fd_nonblocking(i->stdinfd, 1, &ti->stdio_blocking_save) == 0){
                        i->termfd = tty_check(i->stdinfd) ? -1 : get_tty_fd(infp);
                        memset(i->initdata, 0, sizeof(*i->initdata));
                        i->iread = i->iwrite = i->ivalid = 0;
                        i->cread = i->cwrite = i->cvalid = 0;
                        i->initdata_complete = NULL;
                        i->stats = stats;
                        i->ti = ti;
                        i->stdineof = 0;
#ifdef __MINGW64__
                        i->stdinhandle = ti->inhandle;
#endif
                        i->ibufvalid = 0;
                        i->linesigs = linesigs_enabled;
                        i->tbufvalid = 0;
                        i->midescape = 0;
                        i->lmargin = lmargin;
                        i->tmargin = tmargin;
                        i->drain = drain;
                        logdebug("input descriptors: %d/%d\n", i->stdinfd, i->termfd);
                        return i;
                      }
                    }
                    input_free_esctrie(&i->amata);
                  }
                  free(i->initdata);
                }
                pthread_cond_destroy(&i->ccond);
              }
              pthread_mutex_destroy(&i->clock);
            }
            pthread_cond_destroy(&i->icond);
          }
          pthread_mutex_destroy(&i->ilock);
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
    pthread_mutex_destroy(&i->ilock);
    pthread_cond_destroy(&i->icond);
    pthread_mutex_destroy(&i->clock);
    pthread_cond_destroy(&i->ccond);
    input_free_esctrie(&i->amata);
    // do not kill the thread here, either.
    if(i->initdata){
      free(i->initdata->version);
      free(i->initdata);
    }
    if(i->initdata_complete){
      free(i->initdata_complete->version);
      free(i->initdata_complete);
    }
    free(i->inputs);
    free(i->csrs);
    free(i);
  }
}

// https://sw.kovidgoyal.net/kitty/keyboard-protocol/#functional-key-definitions
static int
prep_kitty_special_keys(inputctx* ictx){
  // we do not list here those already handled by prep_windows_special_keys()
  static const struct {
    const char* esc;
    uint32_t key;
    bool shift, ctrl, alt;
  } keys[] = {
    { .esc = "\x1b[P", .key = NCKEY_F01, },
    { .esc = "\x1b[Q", .key = NCKEY_F02, },
    { .esc = "\x1b[R", .key = NCKEY_F03, },
    { .esc = "\x1b[S", .key = NCKEY_F04, },
    { .esc = "\x1b[127;2u", .key = NCKEY_BACKSPACE, .shift = 1, },
    { .esc = "\x1b[127;3u", .key = NCKEY_BACKSPACE, .alt = 1, },
    { .esc = "\x1b[127;5u", .key = NCKEY_BACKSPACE, .ctrl = 1, },
    { .esc = NULL, .key = 0, },
  }, *k;
  for(k = keys ; k->esc ; ++k){
    if(inputctx_add_input_escape(&ictx->amata, k->esc, k->key,
                                 k->shift, k->ctrl, k->alt)){
      return -1;
    }
  }
  loginfo("added all kitty special keys\n");
  return 0;
}

// add the hardcoded windows input sequences to ti->input. should only
// be called after verifying that this is TERMINAL_MSTERMINAL.
static int
prep_windows_special_keys(inputctx* ictx){
  // here, lacking terminfo, we hardcode the sequences. they can be found at
  // https://docs.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
  // under the "Input Sequences" heading.
  static const struct {
    const char* esc;
    uint32_t key;
    bool shift, ctrl, alt;
  } keys[] = {
    { .esc = "\x1b[A", .key = NCKEY_UP, },
    { .esc = "\x1b[B", .key = NCKEY_DOWN, },
    { .esc = "\x1b[C", .key = NCKEY_RIGHT, },
    { .esc = "\x1b[D", .key = NCKEY_LEFT, },
    { .esc = "\x1b[1;5A", .key = NCKEY_UP, .ctrl = 1, },
    { .esc = "\x1b[1;5B", .key = NCKEY_DOWN, .ctrl = 1, },
    { .esc = "\x1b[1;5C", .key = NCKEY_RIGHT, .ctrl = 1, },
    { .esc = "\x1b[1;5D", .key = NCKEY_LEFT, .ctrl = 1, },
    { .esc = "\x1b[H", .key = NCKEY_HOME, },
    { .esc = "\x1b[F", .key = NCKEY_END, },
    { .esc = "\x1b[2~", .key = NCKEY_INS, },
    { .esc = "\x1b[3~", .key = NCKEY_DEL, },
    { .esc = "\x1b[5~", .key = NCKEY_PGUP, },
    { .esc = "\x1b[6~", .key = NCKEY_PGDOWN, },
    { .esc = "\x1bOP", .key = NCKEY_F01, },
    { .esc = "\x1bOQ", .key = NCKEY_F02, },
    { .esc = "\x1bOR", .key = NCKEY_F03, },
    { .esc = "\x1bOS", .key = NCKEY_F04, },
    { .esc = "\x1b[15~", .key = NCKEY_F05, },
    { .esc = "\x1b[17~", .key = NCKEY_F06, },
    { .esc = "\x1b[18~", .key = NCKEY_F07, },
    { .esc = "\x1b[19~", .key = NCKEY_F08, },
    { .esc = "\x1b[20~", .key = NCKEY_F09, },
    { .esc = "\x1b[21~", .key = NCKEY_F10, },
    { .esc = "\x1b[23~", .key = NCKEY_F11, },
    { .esc = "\x1b[24~", .key = NCKEY_F12, },
    { .esc = NULL, .key = 0, },
  }, *k;
  for(k = keys ; k->esc ; ++k){
    if(inputctx_add_input_escape(&ictx->amata, k->esc, k->key,
                                 k->shift, k->ctrl, k->alt)){
      return -1;
    }
    logdebug("added %s %u\n", k->esc, k->key);
  }
  loginfo("added all windows special keys\n");
  return 0;
}

static int
prep_all_keys(inputctx* ictx){
  if(prep_windows_special_keys(ictx)){
    return -1;
  }
  if(prep_kitty_special_keys(ictx)){
    input_free_esctrie(&ictx->amata);
    return -1;
  }
  return 0;
}

// populate |buf| with any new data from the specified file descriptor |fd|.
static void
read_input_nblock(int fd, unsigned char* buf, size_t buflen, int *bufused,
                  unsigned* goteof){
  if(fd < 0){
    return;
  }
  size_t space = buflen - *bufused;
  if(space == 0){
    return;
  }
  ssize_t r = read(fd, buf + *bufused, space);
  if(r <= 0){
    if(r < 0){
      logwarn("couldn't read from %d (%s)\n", fd, strerror(errno));
    }else{
      logwarn("got EOF on %d\n", fd);
      if(goteof){
        *goteof = 1;
      }
    }
    return;
  }
  *bufused += r;
  space -= r;
  loginfo("read %lldB from %d (%lluB left)\n", (long long)r, fd, (unsigned long long)space);
}

// are terminal and stdin distinct for this inputctx?
static inline bool
ictx_independent_p(const inputctx* ictx){
  return ictx->termfd >= 0; // FIXME does this hold on MSFT Terminal?
}

// add a decoded, valid Unicode to the bulk output buffer, or drop it if no
// space is available.
static void
add_unicode(inputctx* ictx, uint32_t id){
  inc_input_events(ictx);
  if(ictx->drain){
    return;
  }
  pthread_mutex_lock(&ictx->ilock);
  if(ictx->ivalid == ictx->isize){
    pthread_mutex_unlock(&ictx->ilock);
    logerror("dropping input 0x%08xx\n", id);
    inc_input_errors(ictx);
    return;
  }
  ncinput* ni = ictx->inputs + ictx->iwrite;
  ni->id = id;
  ni->alt = false;
  ni->ctrl = false;
  ni->shift = false;
  ni->x = ni->y = 0;
  if(++ictx->iwrite == ictx->isize){
    ictx->iwrite = 0;
  }
  ++ictx->ivalid;
  pthread_mutex_unlock(&ictx->ilock);
  pthread_cond_broadcast(&ictx->icond);
}

static void
special_key(inputctx* ictx, const ncinput* inni){
  inc_input_events(ictx);
  pthread_mutex_lock(&ictx->ilock);
  if(ictx->ivalid == ictx->isize){
    pthread_mutex_unlock(&ictx->ilock);
    logerror("dropping input 0x%08xx\n", inni->id);
    inc_input_errors(ictx);
    return;
  }
  ncinput* ni = ictx->inputs + ictx->iwrite;
  memcpy(ni, inni, sizeof(*ni));
  if(++ictx->iwrite == ictx->isize){
    ictx->iwrite = 0;
  }
  ++ictx->ivalid;
  pthread_mutex_unlock(&ictx->ilock);
  pthread_cond_broadcast(&ictx->icond);
}

// try to lex a single control sequence off of buf. return the number of bytes
// consumed if we do so, and -1 otherwise. buf is almost certainly *not*
// NUL-terminated. if we are definitely *not* an escape, or we're unsure when
// we run out of input, return the negated relevant number of bytes, setting
// ictx->midescape if we're uncertain. we preserve a->used, a->state, etc.
// across runs to avoid reprocessing.
//
// our rule is: an escape must arrive as a single unit to be interpreted as
// an escape. this is most relevant for Alt+keypress (Esc followed by the
// character), which is ambiguous with regards to pressing 'Escape' followed
// by some other character. if they arrive together, we consider it to be
// the escape. we might need to allow more than one process_escape call,
// however, in case the escape ended the previous read buffer.
//  precondition: buflen >= 1. precondition: buf[0] == 0x1b.
static int
process_escape(inputctx* ictx, const unsigned char* buf, int buflen){
  assert(1 <= buflen);
  // FIXME given that we keep our state across invocations, we want buf offset
  //  by the amount we handled the last iteration, if any was left over...
  //  until then, we reset the state on entry. remove that once we preserve!
  if(buf[0] != '\x1b'){
    return -1;
  }
  while(ictx->amata.used < buflen){
    unsigned char candidate = buf[ictx->amata.used++];
    unsigned used = ictx->amata.used;
    if(candidate >= 0x80){
      ictx->amata.used = 0;
      return -(used - 1);
    }
    // an escape always resets the trie, as does a NULL transition
    if(candidate == NCKEY_ESC && !ictx->amata.instring){
      ictx->amata.state = ictx->amata.escapes;
      ictx->amata.used = 1;
      if(used > 1){ // we got reset; replay as input
        return -(used - 1);
      }
      // validated first byte as escape! keep going. otherwise, check trie.
      // we can safely check trie[candidate] above because we are either coming
      // off the initial node, which definitely has a valid ->trie, or we're
      // coming from a transition, where ictx->triepos->trie is checked below.
    }else{
      ncinput ni = {};
      logtrace("triepos: %p in: %u special: 0x%08x\n", ictx->amata.state,
               isprint(candidate) ? candidate : ' ',
               esctrie_id(ictx->amata.state));
      int w = walk_automaton(&ictx->amata, ictx, candidate, &ni);
      logdebug("walk result on %u (%c): %d %p\n", candidate,
               isprint(candidate) ? candidate : ' ', w, ictx->amata.state);
      if(w > 0){
        if(ni.id){
          special_key(ictx, &ni);
        }
        ictx->amata.used = 0;
        return used;
      }else if(w < 0){
        // all inspected characters are invalid; return full negative "used"
        ictx->amata.used = 0;
        return -used;
      }
    }
  }
  // we exhausted input without knowing whether or not this is a valid control
  // sequence; we're still on-trie, and need more (immediate) input.
  ictx->midescape = 1;
  return -ictx->amata.used;
}

// process as many control sequences from |buf|, having |bufused| bytes,
// as we can. anything not a valid control sequence is dropped. this text
// needn't be valid UTF-8. this is always called on tbuf; if we find bulk data
// here, we need replay it into ibuf (assuming that there's room).
static void
process_escapes(inputctx* ictx, unsigned char* buf, int* bufused){
  int offset = 0;
  while(*bufused){
    int consumed = process_escape(ictx, buf + offset, *bufused);
    // if we aren't certain, that's not a control sequence unless we're at
    // the end of the tbuf, in which case we really do try reading more. if
    // this was not a sequence, we'll catch it on the next read.
    if(consumed < 0){
      int tavailable = sizeof(ictx->tbuf) - (offset + *bufused - consumed);
      // if midescape is not set, the negative return menas invalid escape. if
      // there was space available, we needn't worry about this escape having
      // been broken across distinct reads. in either case, replay it to the
      // bulk input buffer; our automaton will have been reset.
      if(!ictx->midescape || tavailable){
        consumed = -consumed;
        int available = sizeof(ictx->ibuf) - ictx->ibufvalid;
        if(available){
          if(available > consumed){
            available = consumed;
          }
          logwarn("replaying %dB of %dB to ibuf\n", available, consumed);
          memcpy(ictx->ibuf + ictx->ibufvalid, buf + offset, available);
          ictx->ibufvalid += available;
        }
        *bufused -= consumed;
        offset += consumed;
        ictx->midescape = 0;
      }else{
        break;
      }
    }
    *bufused -= consumed;
    offset += consumed;
  }
  // move any leftovers to the front; only happens if we fill output queue,
  // or ran out of input data mid-escape
  if(*bufused){
    memmove(buf, buf + offset, *bufused);
  }
}

// precondition: buflen >= 1. attempts to consume UTF8 input from buf. the
// expected length of a UTF8 character can be determined from its first byte.
// if we don't have that much data, return 0 and read more. if we determine
// an error, return -1 to consume 1 byte, restarting the UTF8 lex on the next
// byte. on a valid UTF8 character, set up the ncinput and return its length.
static int
process_input(const unsigned char* buf, int buflen, ncinput* ni){
  assert(1 <= buflen);
  memset(ni, 0, sizeof(*ni));
  const int cpointlen = utf8_codepoint_length(*buf);
  if(cpointlen <= 0){
    logwarn("invalid UTF8 initiator on input (0x%02x)\n", *buf);
    return -1;
  }else if(cpointlen == 1){ // pure ascii can't show up mid-utf8-character
    if(buf[0] == 0x7f){ // ASCII del, treated as backspace
      ni->id = NCKEY_BACKSPACE;
    }else if(buf[0] == '\n' || buf[0] == '\r'){
      ni->id = NCKEY_ENTER;
    }else if(buf[0] > 0 && buf[0] <= 26 && buf[0] != '\t'){
      ni->id = buf[0] + 'A' - 1;
      ni->ctrl = true;
    }else{
      ni->id = buf[0];
    }
    return 1;
  }
  if(cpointlen > buflen){
    logwarn("utf8 character (%dB) broken across read\n", cpointlen);
    return 0; // need read more data; we don't have the complete character
  }
  wchar_t w;
  mbstate_t mbstate = {};
//fprintf(stderr, "CANDIDATE: %d cpointlen: %zu cpoint: %d\n", candidate, cpointlen, cpoint[cpointlen]);
  // FIXME how the hell does this work with 16-bit wchar_t?
  size_t r = mbrtowc(&w, (const char*)buf, cpointlen, &mbstate);
  if(r == (size_t)-1 || r == (size_t)-2){
    logerror("invalid utf8 prefix (%dB) on input\n", cpointlen);
    return -1;
  }
  ni->id = w;
  return cpointlen;
}

// precondition: buflen >= 1. gets an ncinput prepared by process_input, and
// sticks that into the bulk queue.
static int
process_ncinput(inputctx* ictx, const unsigned char* buf, int buflen){
  pthread_mutex_lock(&ictx->ilock);
  if(ictx->ivalid == ictx->isize){
    pthread_mutex_unlock(&ictx->ilock);
    logwarn("blocking on input output queue (%d+%d)\n", ictx->ivalid, buflen);
    return 0;
  }
  ncinput* ni = ictx->inputs + ictx->iwrite;
  int r = process_input(buf, buflen, ni);
  if(r > 0){
    inc_input_events(ictx);
    if(!ictx->drain){
      if(++ictx->iwrite == ictx->isize){
        ictx->iwrite = 0;
      }
      ++ictx->ivalid;
    }
  }else if(r < 0){
    inc_input_errors(ictx);
    r = 1; // we want to consume a single byte, upstairs
  }
  pthread_mutex_unlock(&ictx->ilock);
  pthread_cond_broadcast(&ictx->icond);
  return r;
}

// process as much bulk UTF-8 input as we can, knowing it to be free of control
// sequences. anything not a valid UTF-8 character is dropped. a control
// sequence will be chopped up and passed up (assuming it to be valid UTF-8).
static void
process_bulk(inputctx* ictx, unsigned char* buf, int* bufused){
  int offset = 0;
  while(*bufused){
    int consumed = process_ncinput(ictx, buf + offset, *bufused);
    if(consumed <= 0){
      break;
    }
    *bufused -= consumed;
    offset += consumed;
  }
  // move any leftovers to the front
  if(*bufused){
    memmove(buf, buf + offset, *bufused);
  }
}

// process as much mixed input as we can. we might find UTF-8 bulk input and
// control sequences mixed (though each individual character/sequence ought be
// contiguous). known control sequences are removed for internal processing.
// everything else will be handed up to the client (assuming it to be valid
// UTF-8).
static void
process_melange(inputctx* ictx, const unsigned char* buf, int* bufused){
  int offset = 0;
  while(*bufused){
    logdebug("input %d/%d [0x%02x] (%c)\n", offset, *bufused, buf[offset],
             isprint(buf[offset]) ? buf[offset] : ' ');
    int consumed = 0;
    if(buf[offset] == '\x1b'){
      consumed = process_escape(ictx, buf + offset, *bufused);
      if(consumed < 0){
        if(ictx->midescape){
          if(offset + *bufused - consumed != sizeof(ictx->ibuf)){
            // not at the end; treat it as input. no need to move between
            // buffers; simply ensure we process it as input, and don't mark
            // anything as consumed.
            ictx->midescape = 0;
          }
        }
      }
    }
    // don't process as input only if we just read a valid control character,
    // or if we need to read more to determine what it is.
    if(consumed <= 0 && !ictx->midescape){
      consumed = process_ncinput(ictx, buf + offset, *bufused);
    }
    if(consumed < 0){
      break;
    }
    *bufused -= consumed;
    offset += consumed;
  }
}

// walk the matching automaton from wherever we were.
static void
process_ibuf(inputctx* ictx){
  if(resize_seen){
    add_unicode(ictx, NCKEY_RESIZE);
    resize_seen = 0;
  }
  if(ictx->tbufvalid){
    // we could theoretically do this in parallel with process_bulk, but it
    // hardly seems worthwhile without breaking apart the fetches of input.
    process_escapes(ictx, ictx->tbuf, &ictx->tbufvalid);
  }
  if(ictx->ibufvalid){
    if(ictx_independent_p(ictx)){
      process_bulk(ictx, ictx->ibuf, &ictx->ibufvalid);
    }else{
      int valid = ictx->ibufvalid;
      process_melange(ictx, ictx->ibuf, &ictx->ibufvalid);
      // move any leftovers to the front
      if(ictx->ibufvalid){
        memmove(ictx->ibuf, ictx->ibuf + valid - ictx->ibufvalid, ictx->ibufvalid);
      }
    }
  }
}

int ncinput_shovel(inputctx* ictx, const void* buf, int len){
  process_melange(ictx, buf, &len);
  if(len){
    logwarn("dropping %d byte%s\n", len, len == 1 ? "" : "s");
    inc_input_errors(ictx);
  }
  return 0;
}

// here, we always block for an arbitrarily long time, or not at all,
// doing the latter only when ictx->midescape is set. |rtfd| and/or |rifd|
// are set high iff they are ready for reading, and otherwise cleared.
static int
block_on_input(inputctx* ictx, unsigned* rtfd, unsigned* rifd){
  *rtfd = *rifd = 0;
  unsigned nonblock = ictx->midescape;
  if(nonblock){
    loginfo("nonblocking read to check for completion\n");
    ictx->midescape = 0;
  }
#ifdef __MINGW64__
  int timeoutms = nonblock ? 0 : -1;
  DWORD d = WaitForSingleObject(ictx->stdinhandle, timeoutms);
  if(d == WAIT_TIMEOUT){
    return 0;
  }else if(d == WAIT_FAILED){
    return -1;
  }else if(d - WAIT_OBJECT_0 == 0){
    *rifd = 1;
    return 1;
  }
  return -1;
#else
  int inevents = POLLIN;
#ifdef POLLRDHUP
  inevents |= POLLRDHUP;
#endif
  struct pollfd pfds[2];
  int pfdcount = 0;
  if(!ictx->stdineof){
    if(ictx->ibufvalid != sizeof(ictx->ibuf)){
      pfds[pfdcount].fd = ictx->stdinfd;
      pfds[pfdcount].events = inevents;
      pfds[pfdcount].revents = 0;
      ++pfdcount;
    }
  }
  if(ictx->termfd >= 0){
    pfds[pfdcount].fd = ictx->termfd;
    pfds[pfdcount].events = inevents;
    pfds[pfdcount].revents = 0;
    ++pfdcount;
  }
  sigset_t smask;
  sigfillset(&smask);
  sigdelset(&smask, SIGCONT);
  sigdelset(&smask, SIGWINCH);
#ifdef SIGTHR
  // freebsd uses SIGTHR for thread cancellation; need this to ensure wakeup
  // on exit (in cancel_and_join()).
  sigdelset(&smask, SIGTHR);
#endif
  if(pfdcount == 0){
    loginfo("output queues full; blocking on signals\n");
    int signum;
    sigwait(&smask, &signum);
    return 0;
  }
  int events;
#if defined(__APPLE__) || defined(__MINGW64__)
  int timeoutms = nonblock ? 0 : -1;
  while((events = poll(pfds, pfdcount, timeoutms)) < 0){ // FIXME smask?
#else
  struct timespec ts = { .tv_sec = 0, .tv_nsec = 0, };
  struct timespec* pts = nonblock ? &ts : NULL;
  while((events = ppoll(pfds, pfdcount, pts, &smask)) < 0){
#endif
    if(errno != EAGAIN && errno != EBUSY && errno != EWOULDBLOCK){
      return -1;
    }else if(errno == EINTR){
      return resize_seen;
    }
  }
  pfdcount = 0;
  while(events){
    if(pfds[pfdcount].revents){
      if(pfds[pfdcount].fd == ictx->stdinfd){
        *rifd = 1;
      }else if(pfds[pfdcount].fd == ictx->termfd){
        *rtfd = 1;
      }
      --events;
    }
  }
loginfo("got events: %c%c %d\n", *rtfd ? 'T' : 't', *rifd ? 'I' : 'i', pfds[0].revents);
  return events;
#endif
}

// populate the ibuf with any new data, up through its size, but do not block.
// don't loop around this call without some kind of readiness notification.
static void
read_inputs_nblock(inputctx* ictx){
  unsigned rtfd, rifd;
  block_on_input(ictx, &rtfd, &rifd);
  // first we read from the terminal, if that's a distinct source.
  if(rtfd){
    read_input_nblock(ictx->termfd, ictx->tbuf, sizeof(ictx->tbuf),
                      &ictx->tbufvalid, NULL);
  }
  // now read bulk, possibly with term escapes intermingled within (if there
  // was not a distinct terminal source).
  if(rifd){
    read_input_nblock(ictx->stdinfd, ictx->ibuf, sizeof(ictx->ibuf),
                      &ictx->ibufvalid, &ictx->stdineof);
  }
}

static void*
input_thread(void* vmarshall){
  inputctx* ictx = vmarshall;
  if(build_cflow_automaton(ictx)){
    raise(SIGSEGV); // ? FIXME
  }
  dump_automaton(&ictx->amata);
  if(prep_all_keys(ictx)){
    raise(SIGSEGV); // ? FIXME
  }
  dump_automaton(&ictx->amata);
  for(;;){
    read_inputs_nblock(ictx);
    // process anything we've read
    process_ibuf(ictx);
  }
  return NULL;
}

int init_inputlayer(tinfo* ti, FILE* infp, int lmargin, int tmargin,
                    ncsharedstats* stats, unsigned drain,
                    int linesigs_enabled){
  inputctx* ictx = create_inputctx(ti, infp, lmargin, tmargin, stats, drain,
                                   linesigs_enabled);
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
    // FIXME cancellation on shutdown does not yet work on windows #2192
#ifndef __MINGW64__
    if(ti->ictx){
      loginfo("tearing down input thread\n");
      ret |= cancel_and_join("input", ti->ictx->tid, NULL);
      ret |= set_fd_nonblocking(ti->ictx->stdinfd, ti->stdio_blocking_save, NULL);
      free_inputctx(ti->ictx);
      ti->ictx = NULL;
    }
#endif
  }
  return ret;
}

int inputready_fd(const inputctx* ictx){
  return ictx->stdinfd;
}

static inline uint32_t
internal_get(inputctx* ictx, const struct timespec* ts, ncinput* ni){
  uint32_t id;
  if(ictx->drain){
    logerror("input is being drained\n");
    return (uint32_t)-1;
  }
  pthread_mutex_lock(&ictx->ilock);
  while(!ictx->ivalid){
    if(ictx->stdineof){
      pthread_mutex_unlock(&ictx->ilock);
      logwarn("read eof on stdin\n");
      if(ni){
        memset(ni, 0, sizeof(*ni));
        ni->id = NCKEY_EOF;
      }
      return NCKEY_EOF;
    }
    if(ts == NULL){
      pthread_cond_wait(&ictx->icond, &ictx->ilock);
    }else{
      int r = pthread_cond_timedwait(&ictx->icond, &ictx->ilock, ts);
      if(r == ETIMEDOUT){
        pthread_mutex_unlock(&ictx->ilock);
        return 0;
      }else if(r < 0){
        inc_input_errors(ictx);
        return (uint32_t)-1;
      }
    }
  }
  id = ictx->inputs[ictx->iread].id;
  if(ni){
    memcpy(ni, &ictx->inputs[ictx->iread], sizeof(*ni));
  }
  if(++ictx->iread == ictx->isize){
    ictx->iread = 0;
  }
  bool sendsignal = false;
  if(ictx->ivalid-- == ictx->isize){
    sendsignal = true;
  }
  pthread_mutex_unlock(&ictx->ilock);
  if(sendsignal){
    pthread_kill(ictx->tid, SIGCONT);
  }
  return id;
}

static void
delaybound_to_deadline(const struct timespec* ts, struct timespec* absdl){
  if(ts){
    // incoming ts is a delay bound, but we want an absolute deadline for
    // pthread_cond_timedwait(). convert it.
    struct timeval tv;
    gettimeofday(&tv, NULL);
    absdl->tv_sec = ts->tv_sec + tv.tv_sec;
    absdl->tv_nsec = ts->tv_nsec + tv.tv_usec * 1000;
    if(absdl->tv_nsec > 1000000000){
      ++absdl->tv_sec;
      absdl->tv_nsec -= 1000000000;
    }
  }
}

// infp has already been set non-blocking
uint32_t notcurses_get(notcurses* nc, const struct timespec* ts, ncinput* ni){
  struct timespec absdl;
  delaybound_to_deadline(ts, &absdl);
  return internal_get(nc->tcache.ictx, ts ? &absdl : NULL, ni);
}

// FIXME better performance if we move this within the locked area
int notcurses_getvec(notcurses* n, const struct timespec* ts,
                     ncinput* ni, int vcount){
  struct timespec absdl;
  delaybound_to_deadline(ts, &absdl);
  for(int v = 0 ; v < vcount ; ++v){
    uint32_t u = notcurses_get(n, ts ? &absdl : NULL, &ni[v]);
    if(u == (uint32_t)-1){
      if(v == 0){
        return -1;
      }
      return v;
    }else if(u == 0){
      return v;
    }
  }
  return vcount;
}

uint32_t ncdirect_get(ncdirect* n, const struct timespec* ts, ncinput* ni){
  return internal_get(n->tcache.ictx, ts, ni);
}

uint32_t notcurses_getc(notcurses* nc, const struct timespec* ts,
                        const void* unused, ncinput* ni){
  (void)unused; // FIXME remove for abi3
  return notcurses_get(nc, ts, ni);
}

uint32_t ncdirect_getc(ncdirect* nc, const struct timespec *ts,
                       const void* unused, ncinput* ni){
  (void)unused; // FIXME remove for abi3
  return ncdirect_get(nc, ts, ni);
}

int get_cursor_location(struct inputctx* ictx, int* y, int* x){
  pthread_mutex_lock(&ictx->clock);
  while(ictx->cvalid == 0){
    pthread_cond_wait(&ictx->ccond, &ictx->clock);
  }
  const cursorloc* cloc = &ictx->csrs[ictx->cread];
  if(++ictx->cread == ictx->csize){
    ictx->cread = 0;
  }
  --ictx->cvalid;
  if(y){
    *y = cloc->y;
  }
  if(x){
    *x = cloc->x;
  }
  pthread_mutex_unlock(&ictx->clock);
  return 0;
}

// Disable signals originating from the terminal's line discipline, i.e.
// SIGINT (^C), SIGQUIT (^\), and SIGTSTP (^Z). They are enabled by default.
static int
linesigs_disable(tinfo* ti){
  if(!ti->ictx->linesigs){
    logwarn("linedisc signals already disabled\n");
  }
#ifndef __MINGW64__
  if(ti->ttyfd < 0){
    return 0;
  }
  struct termios tios;
  if(tcgetattr(ti->ttyfd, &tios)){
    logerror("Couldn't preserve terminal state for %d (%s)\n", ti->ttyfd, strerror(errno));
    return -1;
  }
  tios.c_lflag &= ~ISIG;
  if(tcsetattr(ti->ttyfd, TCSANOW, &tios)){
    logerror("Error disabling signals on %d (%s)\n", ti->ttyfd, strerror(errno));
    return -1;
  }
#else
  DWORD mode;
  if(!GetConsoleMode(ti->inhandle, &mode)){
    logerror("error acquiring input mode\n");
    return -1;
  }
  mode &= ~ENABLE_PROCESSED_INPUT;
  if(!SetConsoleMode(ti->inhandle, mode)){
    logerror("error setting input mode\n");
    return -1;
  }
#endif
  ti->ictx->linesigs = 0;
  loginfo("disabled line discipline signals\n");
  return 0;
}

int notcurses_linesigs_disable(notcurses* nc){
  return linesigs_disable(&nc->tcache);
}

static int
linesigs_enable(tinfo* ti){
  if(ti->ictx->linesigs){
    logwarn("linedisc signals already enabled\n");
  }
#ifndef __MINGW64__
  if(ti->ttyfd < 0){
    return 0;
  }
  struct termios tios;
  if(tcgetattr(ti->ttyfd, &tios)){
    logerror("Couldn't preserve terminal state for %d (%s)\n", ti->ttyfd, strerror(errno));
    return -1;
  }
  tios.c_lflag |= ISIG;
  if(tcsetattr(ti->ttyfd, TCSANOW, &tios)){
    logerror("Error disabling signals on %d (%s)\n", ti->ttyfd, strerror(errno));
    return -1;
  }
#else
  DWORD mode;
  if(!GetConsoleMode(ti->inhandle, &mode)){
    logerror("error acquiring input mode\n");
    return -1;
  }
  mode |= ENABLE_PROCESSED_INPUT;
  if(!SetConsoleMode(ti->inhandle, mode)){
    logerror("error setting input mode\n");
    return -1;
  }
#endif
  ti->ictx->linesigs = 1;
  loginfo("enabled line discipline signals\n");
  return 0;
}

// Restore signals originating from the terminal's line discipline, i.e.
// SIGINT (^C), SIGQUIT (^\), and SIGTSTP (^Z), if disabled.
int notcurses_linesigs_enable(notcurses* n){
  return linesigs_enable(&n->tcache);
}

struct initial_responses* inputlayer_get_responses(inputctx* ictx){
  struct initial_responses* iresp;
  pthread_mutex_lock(&ictx->ilock);
  while(!ictx->initdata_complete){
    pthread_cond_wait(&ictx->icond, &ictx->ilock);
  }
  iresp = ictx->initdata_complete;
  ictx->initdata_complete = NULL;
  pthread_mutex_unlock(&ictx->ilock);
  return iresp;
}
