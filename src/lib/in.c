#include <stdio.h>
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
  STATE_APC,        // application programming command, starts with \x1b_
  STATE_APC_DRAIN,  // looking for \x1b
  STATE_APC_ST,     // looking for ST
  STATE_BG1,        // got '1'
  STATE_BG2,        // got second '1'
  STATE_BGSEMI,     // got '11;', draining string to ESC ST
  STATE_TDA1, // tertiary DA, got '!'
  STATE_TDA2, // tertiary DA, got '|', first hex nibble
  STATE_TDA3, // tertiary DA, second hex nibble
  STATE_SDA,  // secondary DA (CSI > Pp ; Pv ; Pc c)
  STATE_SDA_VER,  // secondary DA, got semi, reading to next semi
  STATE_SDA_DRAIN, // drain secondary DA to 'c'
  STATE_DA,   // primary DA   (CSI ? ... c) OR XTSMGRAPHICS OR DECRPM or kittykbd
  STATE_DA_DRAIN, // drain out the primary DA to an alpha
  STATE_DA_SEMI,    // got first semicolon following numeric
  STATE_DA_SEMI2,   // got second semicolon following numeric ; numeric
  STATE_DA_SEMI3,   // got third semicolon following numeric ; numeric ; numeric
  STATE_APPSYNC_REPORT, // got DECRPT ?2026
  STATE_APPSYNC_REPORT_DRAIN, // drain out decrpt to 'y'
  // cursor location report: CSI row ; col R
  // text area pixel geometry: CSI 4 ; rows ; cols t
  // text area cell geometry: CSI 8 ; rows ; cols t
  // so we handle them the same until we hit either a second semicolon or an
  // 'R', 't', or 'u'. at the second ';', we verify that the first variable was
  // '4' or '8', and continue to 't' via STATE_{PIXELS,CELLS}_WIDTH.
  STATE_CURSOR_COL,    // reading numeric to 'R', 't', 'u', or ';'
  STATE_PIXELS_WIDTH,  // reading text area width in pixels to ';'
  STATE_CELLS_WIDTH,   // reading text area width in cells to ';'
  STATE_MOUSE,         // got '<', looking for mouse coordinates
  STATE_MOUSE2,        // got mouse click modifiers
  STATE_MOUSE3,        // got first mouse coordinate
} initstates_e;

// we assumed escapes can only be composed of 7-bit chars
typedef struct esctrie {
  struct esctrie** trie;  // if non-NULL, next level of radix-128 trie
  uint32_t special;       // composed key terminating here
  bool shift, ctrl, alt;
} esctrie;

static esctrie*
create_esctrie_node(int special){
  esctrie* e = malloc(sizeof(*e));
  if(e){
    e->trie = NULL;
    e->special = special;
    e->shift = 0;
    e->ctrl = 0;
    e->alt = 0;
  }
  return e;
}

// local state for the input thread. don't put this large struct on the stack.
typedef struct inputctx {
  int stdinfd;          // bulk in fd. always >= 0 (almost always 0). we do not
                        //  own this descriptor, and must not close() it.
  int termfd;           // terminal fd: -1 with no controlling terminal, or
                        //  if stdin is a terminal, or on MSFT Terminal.
#ifdef __MINGW64__
  HANDLE stdinhandle;   // handle to input terminal for MSFT Terminal
#endif

  uint64_t seqnum;      // process-scope sequence number
  int lmargin, tmargin; // margins in use at left and top

  struct esctrie* inputescapes;

  // these two are not ringbuffers; we always move any leftover materia to the
  // front of the queue (it ought be a handful of bytes at most).
  unsigned char ibuf[BUFSIZ]; // might be intermingled bulk/control data
  unsigned char tbuf[BUFSIZ]; // only used if we have distinct terminal fd
  int ibufvalid;      // we mustn't read() if ibufvalid == sizeof(ibuf)
  int tbufvalid;      // only used if we have distinct terminal connection

  // transient state for processing control sequences
  // stringstate is the state at which this string was initialized, and can be
  // one of STATE_XTVERSION1, STATE_XTGETTCAP_TERMNAME1, STATE_TDA1, and STATE_BG1
  initstates_e state, stringstate;
  int numeric;            // currently-lexed numeric
  char runstring[BUFSIZ]; // running string (when stringstate != STATE_NULL)
  int stridx;             // length of runstring
  int p2, p3, p4;         // holders for numeric params
  struct esctrie* triepos;// position in escapes automaton

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

  unsigned linesigs;  // are line discipline signals active?
  unsigned drain;     // drain away bulk input?
  ncsharedstats *stats; // stats shared with notcurses context

  struct initial_responses* initdata;
  struct initial_responses* initdata_complete;
} inputctx;

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

// multiple input escapes might map to the same input
static int
inputctx_add_input_escape(inputctx* ictx, const char* esc, uint32_t special,
                          unsigned shift, unsigned ctrl, unsigned alt){
  if(esc[0] != NCKEY_ESC || strlen(esc) < 2){ // assume ESC prefix + content
    logerror("not an escape (0x%x)\n", special);
    return -1;
  }
  if(ictx->inputescapes == NULL){
    if((ictx->inputescapes = create_esctrie_node(NCKEY_INVALID)) == NULL){
      return -1;
    }
  }
  esctrie* cur = ictx->inputescapes;
  ++esc; // don't encode initial escape as a transition
  do{
    int valid = *esc;
    if(valid <= 0 || valid >= 0x80 || valid == NCKEY_ESC){
      logerror("invalid character %d in escape\n", valid);
      return -1;
    }
    if(cur->trie == NULL){
      const size_t tsize = sizeof(cur->trie) * 0x80;
      if((cur->trie = malloc(tsize)) == NULL){
        return -1;
      }
      memset(cur->trie, 0, tsize);
    }
    if(cur->trie[valid] == NULL){
      if((cur->trie[valid] = create_esctrie_node(NCKEY_INVALID)) == NULL){
        return -1;
      }
    }
    cur = cur->trie[valid];
    ++esc;
  }while(*esc);
  // it appears that multiple keys can be mapped to the same escape string. as
  // an example, see "kend" and "kc1" in st ("simple term" from suckless) :/.
  if(cur->special != NCKEY_INVALID){ // already had one here!
    if(cur->special != special){
      logwarn("already added escape (got 0x%x, wanted 0x%x)\n", cur->special, special);
    }
  }else{
    cur->special = special;
    cur->shift = shift;
    cur->ctrl = ctrl;
    cur->alt = alt;
  }
  return 0;
}

// load all known special keys from terminfo, and build the input sequence trie
static int
prep_special_keys(inputctx* ictx){
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
    { .tinfo = NULL,    .key = NCKEY_INVALID, }
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
    if(inputctx_add_input_escape(ictx, seq, k->key, k->shift, k->ctrl, k->alt)){
      return -1;
    }
    logdebug("support for terminfo's %s: %s\n", k->tinfo, seq);
  }
#endif
  (void)ictx;
  return 0;
}

static inline inputctx*
create_inputctx(tinfo* ti, FILE* infp, int lmargin, int tmargin,
                ncsharedstats* stats, unsigned drain){
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
                    i->inputescapes = NULL;
                    if(prep_special_keys(i) == 0){
                      if(set_fd_nonblocking(i->stdinfd, 1, &ti->stdio_blocking_save) == 0){
                        i->termfd = tty_check(i->stdinfd) ? -1 : get_tty_fd(infp);
                        memset(i->initdata, 0, sizeof(*i->initdata));
                        i->state = i->stringstate = STATE_NULL;
                        i->iread = i->iwrite = i->ivalid = 0;
                        i->cread = i->cwrite = i->cvalid = 0;
                        i->initdata_complete = NULL;
                        i->stats = stats;
                        i->ti = ti;
                        i->ibufvalid = 0;
                        // FIXME need to get this out of the initial termios
                        // (as stored in tpreserved)
                        i->linesigs = 1;
                        i->tbufvalid = 0;
                        i->numeric = 0;
                        i->stridx = 0;
                        i->runstring[i->stridx] = '\0';
                        i->lmargin = lmargin;
                        i->tmargin = tmargin;
                        i->seqnum = 0;
                        i->drain = drain;
                        logdebug("input descriptors: %d/%d\n", i->stdinfd, i->termfd);
                        return i;
                      }
                    }
                    input_free_esctrie(&i->inputescapes);
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
    input_free_esctrie(&i->inputescapes);
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
prep_kitty_special_keys(inputctx* nc){
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
    { .esc = NULL, .key = NCKEY_INVALID, },
  }, *k;
  for(k = keys ; k->esc ; ++k){
    if(inputctx_add_input_escape(nc, k->esc, k->key, k->shift, k->ctrl, k->alt)){
      return -1;
    }
  }
  return 0;
}

// add the hardcoded windows input sequences to ti->input. should only
// be called after verifying that this is TERMINAL_MSTERMINAL.
static int
prep_windows_special_keys(inputctx* nc){
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
    { .esc = NULL, .key = NCKEY_INVALID, },
  }, *k;
  for(k = keys ; k->esc ; ++k){
    if(inputctx_add_input_escape(nc, k->esc, k->key, k->shift, k->ctrl, k->alt)){
      return -1;
    }
  }
  return 0;
}

static int
prep_all_keys(inputctx* ictx){
  if(prep_windows_special_keys(ictx)){
    return -1;
  }
  if(prep_kitty_special_keys(ictx)){
    input_free_esctrie(&ictx->inputescapes);
    return -1;
  }
  ictx->triepos = ictx->inputescapes;
  return 0;
}

// populate |buf| with any new data from the specified file descriptor |fd|.
static void
read_input_nblock(int fd, unsigned char* buf, size_t buflen, int *bufused){
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
ruts_string(inputctx* ictx, initstates_e state){
  if(ictx->stridx == sizeof(ictx->runstring)){
    return -1; // overflow, too long
  }
  if(ictx->numeric > 255){
    return -1;
  }
  unsigned char c = ictx->numeric;
  if(!isprint(c)){
    return -1;
  }
  ictx->stringstate = state;
  ictx->runstring[ictx->stridx] = c;
  ictx->runstring[++ictx->stridx] = '\0';
  return 0;
}

// extract the terminal version from the running string, following 'prefix'
static int
extract_version(inputctx* ictx, size_t slen){
  size_t bytes = strlen(ictx->runstring + slen) + 1;
  ictx->initdata->version = malloc(bytes);
  if(ictx->initdata->version == NULL){
    return -1;
  }
  memcpy(ictx->initdata->version, ictx->runstring + slen, bytes);
  return 0;
}

static int
extract_xtversion(inputctx* ictx, size_t slen, char suffix){
  if(suffix){
    if(ictx->runstring[ictx->stridx - 1] != suffix){
      return -1;
    }
    ictx->runstring[ictx->stridx - 1] = '\0';
  }
  return extract_version(ictx, slen);
}

static int
stash_string(inputctx* ictx){
  struct initial_responses* inits = ictx->initdata;
//fprintf(stderr, "string terminator after %d [%s]\n", inits->stringstate, inits->runstring);
  if(inits){
    switch(ictx->stringstate){
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
          { .prefix = "mintty ", .suffix = 0, .term = TERMINAL_MINTTY, },
          { .prefix = NULL, .suffix = 0, .term = TERMINAL_UNKNOWN, },
        }, *xtv;
        for(xtv = xtvers ; xtv->prefix ; ++xtv){
          if(strncmp(ictx->runstring, xtv->prefix, strlen(xtv->prefix)) == 0){
            if(extract_xtversion(ictx, strlen(xtv->prefix), xtv->suffix) == 0){
              inits->qterm = xtv->term;
            }
            break;
          }
        }
        if(xtv->prefix == NULL){
          logwarn("Unrecognizable XTVERSION [%s]\n", ictx->runstring);
        }
        break;
      }case STATE_XTGETTCAP_TERMNAME1:
        if(strcmp(ictx->runstring, "xterm-kitty") == 0){
          inits->qterm = TERMINAL_KITTY;
        }else if(strcmp(ictx->runstring, "mlterm") == 0){
          // MLterm prior to late 3.9.1 only reports via XTGETTCAP
          inits->qterm = TERMINAL_MLTERM;
        }
        break;
      case STATE_TDA1:
        if(strcmp(ictx->runstring, "~VTE") == 0){
          inits->qterm = TERMINAL_VTE;
        }else if(strcmp(ictx->runstring, "~~TY") == 0){
          inits->qterm = TERMINAL_TERMINOLOGY;
        }else if(strcmp(ictx->runstring, "FOOT") == 0){
          inits->qterm = TERMINAL_FOOT;
        }
        break;
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
      }default:
  // don't generally enable this -- XTerm terminates TDA with ST
  //fprintf(stderr, "invalid string [%s] stashed %d\n", inits->runstring, inits->stringstate);
        break;
    }
  }
  ictx->runstring[0] = '\0';
  ictx->stridx = 0;
  return 0;
}

// use the version extracted from Secondary Device Attributes, assuming that
// it is Alacritty (we ought check the specified terminfo database entry).
// Alacritty writes its crate version with each more significant portion
// multiplied by 100^{portion ID}, where major, minor, patch are 2, 1, 0.
// what happens when a component exceeds 99? who cares. support XTVERSION.
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

// ictx->numeric, ictx->p3, and ictx->p2 have the two parameters
static void
mouse_click(inputctx* ictx){
  pthread_mutex_lock(&ictx->ilock);
  if(ictx->ivalid == ictx->isize){
    pthread_mutex_unlock(&ictx->ilock);
    logerror("dropping mouse click 0x%02x %d %d\n", ictx->p2, ictx->p3, ictx->numeric);
    return;
  }
  ncinput* ni = ictx->inputs + ictx->iwrite;
  if(ictx->p2 >= 0 && ictx->p2 < 64){
    if(ictx->p2 % 4 == 3){
      ni->id = NCKEY_RELEASE;
    }else{
      ni->id = NCKEY_BUTTON1 + (ictx->p2 % 4);
    }
  }else if(ictx->p2 >= 64 && ictx->p2 < 128){
    ni->id = NCKEY_BUTTON4 + (ictx->p2 % 4);
  }else if(ictx->p2 >= 128 && ictx->p2 < 192){
    ni->id = NCKEY_BUTTON8 + (ictx->p2 % 4);
  }
  ni->ctrl = ictx->p2 & 0x10;
  ni->alt = ictx->p2 & 0x08;
  ni->shift = ictx->p2 & 0x04;
  // convert from 1- to 0-indexing and account for margins
  ni->x = ictx->p3 - 1 - ictx->lmargin;
  ni->y = ictx->numeric - 1 - ictx->tmargin;
  if(++ictx->iwrite == ictx->isize){
    ictx->iwrite = 0;
  }
  ++ictx->ivalid;
  pthread_mutex_unlock(&ictx->ilock);
  pthread_cond_broadcast(&ictx->icond);
}

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
alt_key(inputctx* ictx, unsigned id){
  inc_input_events(ictx);
  if(ictx->drain){
    return;
  }
  pthread_mutex_lock(&ictx->ilock);
  if(ictx->ivalid == ictx->isize){
    pthread_mutex_unlock(&ictx->ilock);
    logerror("dropping input 0x%08xx\n", ictx->triepos->special);
    return;
  }
  ncinput* ni = ictx->inputs + ictx->iwrite;
  ni->id = id;
  ni->alt = true;
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
special_key(inputctx* ictx){
  assert(ictx->triepos);
  assert(NCKEY_INVALID != ictx->triepos->special);
  pthread_mutex_lock(&ictx->ilock);
  if(ictx->ivalid == ictx->isize){
    pthread_mutex_unlock(&ictx->ilock);
    logerror("dropping input 0x%08xx\n", ictx->triepos->special);
    return;
  }
  ncinput* ni = ictx->inputs + ictx->iwrite;
  ni->id = ictx->triepos->special;
  ni->alt = ictx->triepos->alt;
  ni->ctrl = ictx->triepos->ctrl;
  ni->shift = ictx->triepos->shift;
  ni->x = ni->y = 0;
  if(++ictx->iwrite == ictx->isize){
    ictx->iwrite = 0;
  }
  ++ictx->ivalid;
  pthread_mutex_unlock(&ictx->ilock);
  pthread_cond_broadcast(&ictx->icond);
}

// ictx->numeric and ictx->p2 have the two parameters
static void
kitty_kbd(inputctx* ictx){
  enum { // synthesized events derived from keypresses
    SYNTH_NOTHING,
    SYNTH_SIGINT,
    SYNTH_SIGQUIT,
  } synth = SYNTH_NOTHING;
  assert(ictx->numeric > 0);
  assert(ictx->p2 > 0);
  pthread_mutex_lock(&ictx->ilock);
  if(ictx->ivalid == ictx->isize){
    pthread_mutex_unlock(&ictx->ilock);
    logerror("dropping input 0x%08x 0x%02x\n", ictx->p2, ictx->numeric);
    return;
  }
  ncinput* ni = ictx->inputs + ictx->iwrite;
  if((ni->id = ictx->p2) == 0x7f){
    ni->id = NCKEY_BACKSPACE;
  }
  ni->shift = !!((ictx->numeric - 1) & 0x1);
  ni->alt = !!((ictx->numeric - 1) & 0x2);
  ni->ctrl = !!((ictx->numeric - 1) & 0x4);
  // FIXME decode remaining modifiers through 128
  // standard keyboard protocol reports ctrl+ascii as the capital form,
  // so (for now) conform when using kitty protocol...
  if(ni->id < 128 && islower(ni->id) && ni->ctrl){
    ni->id = toupper(ni->id);
  }
  ni->x = 0;
  ni->y = 0;
  if(++ictx->iwrite == ictx->isize){
    ictx->iwrite = 0;
  }
  ++ictx->ivalid;
  if(ni->ctrl && !ni->alt && !ni->shift){
    if(ni->id == 'C'){
      synth = SYNTH_SIGINT;
    }else if(ni->id == '\\'){
      synth = SYNTH_SIGQUIT;
    }
  }
  pthread_mutex_unlock(&ictx->ilock);
  pthread_cond_broadcast(&ictx->icond);
#ifndef __MINGW64__
  if(synth == SYNTH_SIGINT){
    raise(SIGINT);
  }else if(synth == SYNTH_SIGQUIT){
    raise(SIGQUIT);
  }
#else
  (void)synth; // FIXME
#endif
}

// FIXME ought implement the full Williams automaton
// FIXME sloppy af in general
// returns 1 after handling the Device Attributes response, 0 if more input
// ought be fed to the machine, and -1 on an invalid state transition.
static int
pump_control_read(inputctx* ictx, unsigned char c){
  logdebug("state: %2d char: %1c %3d %02x\n", ictx->state, isprint(c) ? c : ' ', c, c);
  if(c == NCKEY_ESC){
    ictx->state = STATE_ESC;
    return 0;
  }
  switch(ictx->state){
    case STATE_NULL:
      // not an escape -- throw into user queue
      break;
    case STATE_ESC:
      ictx->numeric = 0;
      if(c == '['){
        ictx->state = STATE_CSI;
      }else if(c == 'P'){
        ictx->state = STATE_DCS;
      }else if(c == '\\'){
        if(stash_string(ictx)){
          return -1;
        }
        ictx->state = STATE_NULL;
      }else if(c == '1'){
        ictx->state = STATE_BG1;
      }else if(c == '_'){
        ictx->state = STATE_APC;
      }
      break;
    case STATE_APC:
      if(c == 'G'){
        if(ictx->initdata){
          ictx->initdata->kitty_graphics = true;
        }
      }
      ictx->state = STATE_APC_DRAIN;
      break;
    case STATE_APC_DRAIN:
      if(c == '\x1b'){
        ictx->state = STATE_APC_ST;
      }
      break;
    case STATE_APC_ST:
      if(c == '\\'){
        ictx->state = STATE_NULL;
      }else{
        ictx->state = STATE_APC_DRAIN;
      }
      break;
    case STATE_BG1:
      if(c == '1'){
        ictx->state = STATE_BG2;
      }else{
        ictx->state = STATE_NULL;
      }
      break;
    case STATE_BG2:
      if(c == ';'){
        ictx->state = STATE_BGSEMI;
        ictx->stridx = 0;
        ictx->runstring[0] = '\0';
      }else{
        ictx->state = STATE_NULL;
      }
      break;
    case STATE_BGSEMI: // drain string
      if(c == '\x07'){ // contour sends this at the end for some unknown reason
        if(stash_string(ictx)){
          return -1;
        }
        ictx->state = STATE_NULL;
        break;
      }
      ictx->numeric = c;
      if(ruts_string(ictx, STATE_BG1)){
        return -1;
      }
      break;
    case STATE_CSI: // terminated by 0x40--0x7E ('@'--'~')
      if(c == '?'){
        ictx->state = STATE_DA; // could also be DECRPM/XTSMGRAPHICS/kittykbd
      }else if(c == '>'){
        // SDA yields up Alacritty's crate version, but it doesn't unambiguously
        // identify Alacritty. If we've got any other version information, skip
        // directly to STATE_SDA_DRAIN, rather than doing STATE_SDA_VER.
        if(ictx->initdata){
          if(ictx->initdata->qterm || ictx->initdata->version){
            loginfo("Identified terminal already; ignoring DA2\n");
            ictx->state = STATE_SDA_DRAIN;
          }else{
            ictx->state = STATE_SDA;
          }
        }
      }else if(c == '<'){
        ictx->state = STATE_MOUSE;
      }else if(isdigit(c)){
        if(ruts_numeric(&ictx->numeric, c)){
          return -1;
        }
      }else if(c == ';'){
        ictx->p2 = ictx->numeric;
        ictx->state = STATE_CURSOR_COL;
        ictx->numeric = 0;
      }else if(c >= 0x40 && c <= 0x7E){
        ictx->state = STATE_NULL;
      }
      break;
    case STATE_MOUSE:
      if(isdigit(c)){
        if(ruts_numeric(&ictx->numeric, c)){
          return -1;
        }
      }else if(c == ';'){
        ictx->state = STATE_MOUSE2;
        ictx->p2 = ictx->numeric;
        ictx->numeric = 0;
      }else{
        ictx->state = STATE_NULL;
      }
      break;
    case STATE_MOUSE2:
      if(isdigit(c)){
        if(ruts_numeric(&ictx->numeric, c)){
          return -1;
        }
      }else if(c == ';'){
        ictx->state = STATE_MOUSE3;
        ictx->p3 = ictx->numeric;
        ictx->numeric = 0;
      }else{
        ictx->state = STATE_NULL;
      }
      break;
    case STATE_MOUSE3:
      if(isdigit(c)){
        if(ruts_numeric(&ictx->numeric, c)){
          return -1;
        }
      }else if(c == 'M'){
        mouse_click(ictx);
        ictx->state = STATE_NULL;
      }else{
        ictx->state = STATE_NULL;
      }
      break;
    case STATE_CURSOR_COL:
      if(isdigit(c)){
        if(ruts_numeric(&ictx->numeric, c)){
          return -1;
        }
      }else if(c == 'R'){
//fprintf(stderr, "CURSOR X: %d\n", ictx->numeric);
        if(ictx->initdata){
          // convert from 1- to 0-indexing, and account for margins
          ictx->initdata->cursorx = ictx->numeric - 1 - ictx->lmargin;
          ictx->initdata->cursory = ictx->p2 - 1 - ictx->tmargin;
        }else{
          pthread_mutex_lock(&ictx->clock);
          if(ictx->cvalid == ictx->csize){
            pthread_mutex_unlock(&ictx->clock);
            logwarn("dropping cursor location report\n");
          }else{
            cursorloc* cloc = &ictx->csrs[ictx->cwrite];
            cloc->x = ictx->numeric - 1;
            cloc->y = ictx->p2 - 1;
            if(++ictx->cwrite == ictx->csize){
              ictx->cwrite = 0;
            }
            ++ictx->cvalid;
            pthread_mutex_unlock(&ictx->clock);
            pthread_cond_broadcast(&ictx->ccond);
          }
        }
        ictx->state = STATE_NULL;
      }else if(c == 't'){
//fprintf(stderr, "CELLS X: %d\n", ictx->numeric);
        if(ictx->initdata){
          ictx->initdata->dimx = ictx->numeric;
          ictx->initdata->dimy = ictx->p2;
        }
        ictx->state = STATE_NULL;
      }else if(c == 'u'){
        // kitty keyboard protocol
        kitty_kbd(ictx);
        ictx->state = STATE_NULL;
      }else if(c == ';'){
        if(ictx->p2 == 4){
          if(ictx->initdata){
            ictx->initdata->pixy = ictx->numeric;
            ictx->state = STATE_PIXELS_WIDTH;
          }
          ictx->numeric = 0;
        }else if(ictx->p2 == 8){
          if(ictx->initdata){
            ictx->initdata->dimy = ictx->numeric;
          }
          ictx->state = STATE_CELLS_WIDTH;
          ictx->numeric = 0;
        }else{
          logerror("expected 4 to lead pixel report, got %d\n", ictx->p2);
          return -1;
        }
      }else{
        ictx->state = STATE_NULL;
      }
      break;
    case STATE_PIXELS_WIDTH:
      if(isdigit(c)){
        if(ruts_numeric(&ictx->numeric, c)){
          return -1;
        }
      }else if(c == 't'){
        if(ictx->initdata){
          ictx->initdata->pixx = ictx->numeric;
          loginfo("got pixel geometry: %d/%d\n", ictx->initdata->pixy, ictx->initdata->pixx);
        }
        ictx->state = STATE_NULL;
      }else{
        ictx->state = STATE_NULL;
      }
      break;
    case STATE_CELLS_WIDTH:
      if(isdigit(c)){
        if(ruts_numeric(&ictx->numeric, c)){
          return -1;
        }
      }else if(c == 't'){
        if(ictx->initdata){
          ictx->initdata->dimx = ictx->numeric;
          loginfo("got cell geometry: %d/%d\n", ictx->initdata->dimy, ictx->initdata->dimx);
        }
        ictx->state = STATE_NULL;
      }else{
        ictx->state = STATE_NULL;
      }
      break;
    case STATE_DCS: // terminated by ST
      if(c == '\\'){
//fprintf(stderr, "terminated DCS\n");
        ictx->state = STATE_NULL;
      }else if(c == '1'){
        ictx->state = STATE_XTGETTCAP1; // we have tcap
      }else if(c == '0'){
        ictx->state = STATE_XTGETTCAP1; // no tcap for us
      }else if(c == '>'){
        ictx->state = STATE_XTVERSION1;
      }else if(c == '!'){
        ictx->state = STATE_TDA1;
      }else{
        ictx->state = STATE_DCS_DRAIN;
      }
      break;
    case STATE_DCS_DRAIN:
      // we drain to ST, which is an escape, and thus already handled, so...
      break;
    case STATE_XTVERSION1:
      if(c == '|'){
        ictx->state = STATE_XTVERSION2;
        ictx->stridx = 0;
        ictx->runstring[0] = '\0';
      }else{
        ictx->state = STATE_NULL;
      }
      break;
    case STATE_XTVERSION2:
      ictx->numeric = c;
      if(ruts_string(ictx, STATE_XTVERSION1)){
        return -1;
      }
      break;
    case STATE_XTGETTCAP1:
      if(c == '+'){
        ictx->state = STATE_XTGETTCAP2;
      }else{
        ictx->state = STATE_NULL;
      }
      break;
    case STATE_XTGETTCAP2:
      if(c == 'r'){
        ictx->state = STATE_XTGETTCAP3;
      }else{
        ictx->state = STATE_NULL;
      }
      break;
    case STATE_XTGETTCAP3:
      if(c == '='){
        if(ictx->numeric == 0x544e){
          ictx->state = STATE_XTGETTCAP_TERMNAME1;
          ictx->stridx = 0;
          ictx->numeric = 0;
          ictx->runstring[0] = '\0';
        }else{
          ictx->state = STATE_DCS_DRAIN;
        }
      }else if(ruts_hex(&ictx->numeric, c)){
        return -1;
      }
      break;
    case STATE_XTGETTCAP_TERMNAME1:
      if(ruts_hex(&ictx->numeric, c)){
        return -1;
      }
      ictx->state = STATE_XTGETTCAP_TERMNAME2;
      break;
    case STATE_XTGETTCAP_TERMNAME2:
      if(ruts_hex(&ictx->numeric, c)){
        return -1;
      }
      ictx->state = STATE_XTGETTCAP_TERMNAME1;
      if(ruts_string(ictx, STATE_XTGETTCAP_TERMNAME1)){
        return -1;
      }
      ictx->numeric = 0;
      break;
    case STATE_TDA1:
      if(c == '|'){
        ictx->state = STATE_TDA2;
        ictx->stridx = 0;
        ictx->runstring[0] = '\0';
      }else{
        ictx->state = STATE_NULL;
      }
      break;
    case STATE_TDA2:
      if(ruts_hex(&ictx->numeric, c)){
        return -1;
      }
      ictx->state = STATE_TDA3;
      break;
    case STATE_TDA3:
      if(ruts_hex(&ictx->numeric, c)){
        return -1;
      }
      ictx->state = STATE_TDA2;
      if(ruts_string(ictx, STATE_TDA1)){
        ictx->state = STATE_DCS_DRAIN; // FIXME return -1?
      }
      ictx->numeric = 0;
      break;
    case STATE_SDA:
      if(c == ';'){
        ictx->state = STATE_SDA_VER;
        ictx->numeric = 0;
      }else if(c == 'c'){
        ictx->state = STATE_NULL;
      }
      break;
    case STATE_SDA_VER:
      if(c == ';'){
        ictx->state = STATE_SDA_DRAIN;
        if(ictx->initdata){
          loginfo("Got DA2 Pv: %u\n", ictx->numeric);
          // if a version was set, we couldn't have arrived here. alacritty
          // writes its crate version here, in an encoded form. nothing else
          // necessarily does, though, so allow failure. this value will be
          // interpreted as the version only if TERM indicates alacritty.
          ictx->initdata->version = set_sda_version(ictx);
        }
      }else if(ruts_numeric(&ictx->numeric, c)){
        return -1;
      }
      break;
    case STATE_SDA_DRAIN:
      if(c == 'c'){
        ictx->state = STATE_NULL;
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
    // KITTYKBD: CSI ? flags u
    case STATE_DA: // return success on end of DA
//fprintf(stderr, "DA: %c\n", c);
      // FIXME several of these numbers could be DECRPM/XTSM/kittykbd. probably
      // just want to read number, *then* make transition on non-number.
      if(isdigit(c)){
        if(ruts_numeric(&ictx->numeric, c)){ // stash for DECRPM/XTSM/kittykbd
          return -1;
        }
      }else if(c == 'u'){ // kitty keyboard
        loginfo("keyboard protocol level 0x%x\n", ictx->numeric);
        ictx->state = STATE_NULL;
      }else if(c == ';'){
        ictx->p2 = ictx->numeric;
        ictx->numeric = 0;
        ictx->state = STATE_DA_SEMI;
      }else if(c >= 0x40 && c <= 0x7E){
        ictx->state = STATE_NULL;
        if(c == 'c'){
          return 1;
        }
      }
      break;
    case STATE_DA_SEMI:
      if(c == ';'){
        ictx->p3 = ictx->numeric;
        ictx->numeric = 0;
        ictx->state = STATE_DA_SEMI2;
      }else if(isdigit(c)){
        if(ruts_numeric(&ictx->numeric, c)){
          return -1;
        }
      }else if(c == '$'){
        if(ictx->p2 == 2026){
          ictx->state = STATE_APPSYNC_REPORT;
          loginfo("terminal reported SUM support\n");
        }else{
          ictx->state = STATE_APPSYNC_REPORT_DRAIN;
        }
      }else if(c >= 0x40 && c <= 0x7E){
        ictx->state = STATE_NULL;
        if(c == 'c'){
          return 1;
        }
      }
      break;
    case STATE_DA_SEMI2:
      if(c == ';'){
        ictx->p4 = ictx->numeric;
        ictx->numeric = 0;
        ictx->state = STATE_DA_SEMI3;
      }else if(isdigit(c)){
        if(ruts_numeric(&ictx->numeric, c)){
          return -1;
        }
      }else if(c == 'S'){
        if(ictx->p2 == 1){
          if(ictx->initdata){
            ictx->initdata->color_registers = ictx->numeric;
            loginfo("sixel color registers: %d\n", ictx->initdata->color_registers);
          }
          ictx->numeric = 0;
        }
        ictx->state = STATE_NULL;
      }else if(c >= 0x40 && c <= 0x7E){
        ictx->state = STATE_NULL;
        if(c == 'c'){
          return 1;
        }
      }
      break;
    case STATE_DA_DRAIN:
      if(c >= 0x40 && c <= 0x7E){
        ictx->state = STATE_NULL;
        if(c == 'c'){
          return 1;
        }
      }
      break;
    case STATE_DA_SEMI3:
      if(c == ';'){
        ictx->numeric = 0;
        ictx->state = STATE_DA_DRAIN;
      }else if(isdigit(c)){
        if(ruts_numeric(&ictx->numeric, c)){
          return -1;
        }
      }else if(c == 'S'){
        if(ictx->initdata){
          ictx->initdata->sixelx = ictx->p4;
          ictx->initdata->sixely = ictx->numeric;
          loginfo("max sixel geometry: %dx%d\n", ictx->initdata->sixely, ictx->initdata->sixelx);
        }
      }else if(c >= 0x40 && c <= 0x7E){
        ictx->state = STATE_NULL;
        if(c == 'c'){
          return 1;
        }
      }
      break;
    case STATE_APPSYNC_REPORT:
      if(ictx->numeric == '2'){
        if(ictx->initdata){
          ictx->initdata->appsync_supported = 1;
        }
        ictx->state = STATE_APPSYNC_REPORT_DRAIN;
      }
      break;
    case STATE_APPSYNC_REPORT_DRAIN:
      if(c == 'y'){
        ictx->state = STATE_NULL;
      }
      break;
    default:
      logerror("Reached invalid init state %d\n", ictx->state);
      return -1;
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
}

// try to lex a control sequence off of buf. return the number of bytes
// consumed if we do so, and -1 otherwise. buf is *not* necessarily
// NUL-terminated. precondition: buflen >= 1.
// FIXME we ought play complete failures into the general input buffer?
static int
process_escape(inputctx* ictx, const unsigned char* buf, int buflen){
  int used = 0;
  while(used < buflen){
    int r = pump_control_read(ictx, buf[used]);
    if(r == 1){
      handoff_initial_responses(ictx);
    }
    // FIXME this drops alt+ characters intermingled with responses
    if(ictx->initdata == NULL){
      // an escape always resets the trie, as does a NULL transition
      unsigned char candidate = buf[used];
      if(candidate == NCKEY_ESC){
        ictx->triepos = ictx->inputescapes;
      }else if(ictx->triepos->trie[candidate] == NULL){
        if(ictx->state == STATE_ESC){
          if(candidate && candidate <= 0x80){ // FIXME what about supraASCII utf8?
            alt_key(ictx, candidate);
          }
        }
        ictx->triepos = ictx->inputescapes;
      }else{
        ictx->triepos = ictx->triepos->trie[candidate];
        logtrace("triepos: %p in: %u special: 0x%08x\n", ictx->triepos, candidate, ictx->triepos->special);
        if(ictx->triepos->special != NCKEY_INVALID){ // match! mark and reset
          special_key(ictx);
          ictx->triepos = ictx->inputescapes;
        }else if(ictx->triepos->trie == NULL){
          ictx->triepos = ictx->inputescapes;
        }
      }
    }
    ++used;
  }
  return used;
}

// process as many control sequences from |buf|, having |bufused| bytes,
// as we can. anything not a valid control sequence is dropped. this text
// needn't be valid UTF-8.
static void
process_escapes(inputctx* ictx, unsigned char* buf, int* bufused){
  int offset = 0;
  while(*bufused){
    int consumed = process_escape(ictx, buf + offset, *bufused);
    if(consumed < 0){
      break;
    }
    *bufused -= consumed;
    offset += consumed;
  }
  // move any leftovers to the front; only happens if we fill output queue
  if(*bufused){
    memmove(buf, buf + offset, *bufused);
  }
}

// precondition: buflen >= 1.
static int
process_input(const unsigned char* buf, int buflen, ncinput* ni){
  memset(ni, 0, sizeof(*ni));
  if(buf[0] == 0x7f){ // ASCII del, treated as backspace
    ni->id = NCKEY_BACKSPACE;
    return 1;
  }
  if(buf[0] < 0x80){ // pure ascii can't show up mid-utf8
    if(buf[0] == '\n' || buf[0] == '\r'){
      ni->id = NCKEY_ENTER;
    }else if(buf[0] > 0 && buf[0] <= 26 && buf[0] != '\t'){
      ni->id = buf[0] + 'A' - 1;
      ni->ctrl = true;
    }else{
      ni->id = buf[0];
    }
    return 1;
  }
  int cpointlen = 0;
  wchar_t w;
  mbstate_t mbstate;
  // FIXME verify that we have enough length based off first char
  while(++cpointlen <= (int)MB_CUR_MAX && cpointlen < buflen){
//fprintf(stderr, "CANDIDATE: %d cpointlen: %zu cpoint: %d\n", candidate, cpointlen, cpoint[cpointlen]);
    // FIXME how the hell does this work with 16-bit wchar_t?
    memset(&mbstate, 0, sizeof(mbstate));
    size_t r;
    if((r = mbrtowc(&w, (const char*)buf, cpointlen + 1, &mbstate)) != (size_t)-1 &&
        r != (size_t)-2){
      ni->id = w;
      return cpointlen + 1;
    }
  }
  // FIXME input error stat
  return 0;
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
    logdebug("input %d/%d [0x%02x]\n", offset, *bufused, buf[offset]);
    int consumed = 0;
    if(buf[offset] == '\x1b'){
      consumed = process_escape(ictx, buf + offset, *bufused);
    }else{
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
  }
  return 0;
}

static int
block_on_input(inputctx* ictx){
  struct timespec* ts = NULL; // FIXME
#ifdef __MINGW64__
	int timeoutms = ts ? ts->tv_sec * 1000 + ts->tv_nsec / 1000000 : -1;
	DWORD d = WaitForMultipleObjects(1, &ictx->stdinhandle, FALSE, timeoutms);
	if(d == WAIT_TIMEOUT){
		return 0;
	}else if(d == WAIT_FAILED){
		return -1;
	}else if(d - WAIT_OBJECT_0 == 0){
		return 1;
	}
	return -1;
#else
  int inevents = POLLIN;
#ifdef POLLRDHUP
  inevents |= POLLRDHUP;
#endif
  struct pollfd pfds[2] = {
    {
			.fd = ictx->stdinfd,
			.events = inevents,
			.revents = 0,
    }
  };
  int pfdcount = 1;
  if(ictx->termfd >= 0){
    pfds[pfdcount].fd = ictx->termfd;
    pfds[pfdcount].events = inevents;
    pfds[pfdcount].revents = 0;
    ++pfdcount;
  }
  int events;
#if defined(__APPLE__) || defined(__MINGW64__)
  int timeoutms = ts ? ts->tv_sec * 1000 + ts->tv_nsec / 1000000 : -1;
  while((events = poll(pfds, pfdcount, timeoutms)) < 0){ // FIXME smask?
#else
  sigset_t smask;
  sigfillset(&smask);
  sigdelset(&smask, SIGCONT);
  sigdelset(&smask, SIGWINCH);
  while((events = ppoll(pfds, pfdcount, ts, &smask)) < 0){
#endif
    if(errno != EAGAIN && errno != EBUSY && errno != EWOULDBLOCK){
      return -1;
    }else if(errno == EINTR){
      return resize_seen;
    }
  }
  return events;
#endif
}

// populate the ibuf with any new data, up through its size, but do not block.
// don't loop around this call without some kind of readiness notification.
static void
read_inputs_nblock(inputctx* ictx){
  // FIXME also need to wake up if our output queues have space that has
  // opened up for us to write into, lest we deadlock with full buffers...
  block_on_input(ictx);
  // first we read from the terminal, if that's a distinct source.
  read_input_nblock(ictx->termfd, ictx->tbuf, sizeof(ictx->tbuf),
                    &ictx->tbufvalid);
  // now read bulk, possibly with term escapes intermingled within (if there
  // was not a distinct terminal source).
  read_input_nblock(ictx->stdinfd, ictx->ibuf, sizeof(ictx->ibuf),
                    &ictx->ibufvalid);
}

static void*
input_thread(void* vmarshall){
  inputctx* ictx = vmarshall;
  prep_all_keys(ictx);
  for(;;){
    read_inputs_nblock(ictx);
    // process anything we've read
    process_ibuf(ictx);
  }
  return NULL;
}

int init_inputlayer(tinfo* ti, FILE* infp, int lmargin, int tmargin,
                    ncsharedstats* stats, unsigned drain){
  inputctx* ictx = create_inputctx(ti, infp, lmargin, tmargin, stats, drain);
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

static inline uint32_t
internal_get(inputctx* ictx, const struct timespec* ts, ncinput* ni){
  if(ictx->drain){
    logerror("input is being drained\n");
    return (uint32_t)-1;
  }
  pthread_mutex_lock(&ictx->ilock);
  while(!ictx->ivalid){
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
  memcpy(ni, &ictx->inputs[ictx->iread], sizeof(*ni));
  if(++ictx->iread == ictx->isize){
    ictx->iread = 0;
  }
  bool sendsignal = false;
  if(ictx->ivalid-- == ictx->isize){
    sendsignal = true;
  }
  ni->seqnum = ++ictx->seqnum;
  pthread_mutex_unlock(&ictx->ilock);
  if(sendsignal){
    pthread_kill(ictx->tid, SIGCONT);
  }
  return ni->id;
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
  }
}

// infp has already been set non-blocking
uint32_t notcurses_get(notcurses* nc, const struct timespec* ts, ncinput* ni){
  struct timespec absdl;
  delaybound_to_deadline(ts, &absdl);
  uint32_t r = internal_get(nc->tcache.ictx, ts ? &absdl : NULL, ni);
  if(r != (uint32_t)-1){
    ++nc->stats.s.input_events;
  }
  return r;
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
