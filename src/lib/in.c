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
} initstates_e;

// local state for the input thread. don't put this large struct on the stack.
typedef struct inputctx {
  int stdinfd;          // bulk in fd. always >= 0 (almost always 0). we do not
                        //  own this descriptor, and must not close() it.
  int termfd;           // terminal fd: -1 with no controlling terminal, or
                        //  if stdin is a terminal, or on MSFT Terminal.
#ifdef __MINGW64__
  HANDLE stdinhandle;   // handle to input terminal for MSFT Terminal
#endif

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

  struct initial_responses* initdata;
  struct initial_responses* initdata_complete;
} inputctx;

static inline inputctx*
create_inputctx(tinfo* ti, FILE* infp){
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
                    if(set_fd_nonblocking(i->stdinfd, 1, &ti->stdio_blocking_save) == 0){
                      memset(i->initdata, 0, sizeof(*i->initdata));
                      i->termfd = tty_check(i->stdinfd) ? -1 : get_tty_fd(infp);
                      i->ti = ti;
                      i->cvalid = i->ivalid = 0;
                      i->cwrite = i->iwrite = 0;
                      i->cread = i->iread = 0;
                      i->ibufvalid = 0;
                      i->tbufvalid = 0;
                      i->state = i->stringstate = STATE_NULL;
                      i->numeric = 0;
                      i->stridx = 0;
                      i->initdata_complete = NULL;
                      i->runstring[i->stridx] = '\0';
                      logdebug("input descriptors: %d/%d\n", i->stdinfd, i->termfd);
                      return i;
                    }
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

// ictx->numeric and ictx->p2 have the two parameters
static void
kitty_kbd(inputctx* ictx){
  assert(ictx->numeric > 0);
  assert(ictx->p2 > 0);
  pthread_mutex_lock(&ictx->ilock);
  if(ictx->ivalid == ictx->isize){
    pthread_mutex_unlock(&ictx->ilock);
    logerror("dropping input 0x%08x 0x%02x\n", ictx->p2, ictx->numeric);
    return;
  }
  ncinput* ni = ictx->inputs + ictx->iwrite;
  ni->id = ictx->p2;
  ni->shift = !!((ictx->numeric - 1) & 0x1);
  ni->alt = !!((ictx->numeric - 1) & 0x2);
  ni->ctrl = !!((ictx->numeric - 1) & 0x4);
  // FIXME decode remaining modifiers through 128
  // standard keyboard protocol reports ctrl+ascii as the capital form,
  // so (for now) conform with kitty protocol...
  if(ni->id < 128 && islower(ni->id) && ni->ctrl){
    ni->id = toupper(ni->id);
  }
  ni->x = 0;
  ni->y = 0;
  if(++ictx->iwrite == ictx->isize){
    ictx->iwrite = 0;
  }
  ++ictx->ivalid;
  pthread_mutex_unlock(&ictx->ilock);
  pthread_cond_broadcast(&ictx->icond);
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
        // FIXME
      }
      break;
    case STATE_BG2:
      if(c == ';'){
        ictx->state = STATE_BGSEMI;
        ictx->stridx = 0;
        ictx->runstring[0] = '\0';
      }else{
        // FIXME
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
    case STATE_CURSOR_COL:
      if(isdigit(c)){
        if(ruts_numeric(&ictx->numeric, c)){
          return -1;
        }
      }else if(c == 'R'){
//fprintf(stderr, "CURSOR X: %d\n", ictx->numeric);
        if(ictx->initdata){
          ictx->initdata->cursorx = ictx->numeric - 1;
          ictx->initdata->cursory = ictx->p2 - 1;
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
        // FIXME error?
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
        // FIXME malformed
      }
      break;
    case STATE_XTGETTCAP2:
      if(c == 'r'){
        ictx->state = STATE_XTGETTCAP3;
      }else{
        // FIXME malformed
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
        // FIXME
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
      fprintf(stderr, "Reached invalid init state %d\n", ictx->state);
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
    ni->id = buf[0];
    return 1;
  }
  int cpointlen = 0;
  wchar_t w;
  mbstate_t mbstate;
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
  // FIXME extract modifiers, mice
  return 0;
}

// precondition: buflen >= 1. gets an ncinput prepared by process_input, and
// sticks that into the bulk queue.
static int
process_ncinput(inputctx* ictx, const unsigned char* buf, int buflen){
  pthread_mutex_lock(&ictx->ilock);
  if(ictx->ivalid == sizeof(ictx->ivalid)){
    pthread_mutex_unlock(&ictx->ilock);
    logwarn("blocking on input output queue (%d+%d)\n", ictx->ivalid, buflen);
    return 0;
  }
  ncinput* ni = ictx->inputs + ictx->iwrite;
  int r = process_input(buf, buflen, ni);
  if(r > 0){
    if(++ictx->iwrite == ictx->isize){
      ictx->iwrite = 0;
    }
    ++ictx->ivalid;
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
	DWORD d = WaitForMultipleObjects(1, &ti->inhandle, FALSE, timeoutms);
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
    if(errno != EINTR && errno != EAGAIN && errno != EBUSY && errno != EWOULDBLOCK){
      return -1;
    }
    if(resize_seen){
      return 1;
    }
  }
  return events;
#endif
}

// populate the ibuf with any new data, up through its size, but do not block.
// don't loop around this call without some kind of readiness notification.
static void
read_inputs_nblock(inputctx* ictx){
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

static inline uint32_t
internal_get(inputctx* ictx, const struct timespec* ts, ncinput* ni,
             int lmargin, int tmargin){
  pthread_mutex_lock(&ictx->ilock);
  while(!ictx->ivalid){
    pthread_cond_wait(&ictx->icond, &ictx->ilock);
  }
  memcpy(ni, &ictx->inputs[ictx->iread], sizeof(*ni));
  if(++ictx->iread == ictx->isize){
    ictx->iread = 0;
  }
  --ictx->ivalid;
  pthread_mutex_unlock(&ictx->ilock);
  // FIXME adjust mouse coordinates for margins
  return ni->id;
  /*
  uint32_t r = ncinputlayer_prestamp(&nc->tcache, ts, ni,
                                     nc->margin_l, nc->margin_t);
  if(r != (uint32_t)-1){
    uint64_t stamp = nc->tcache.input.input_events++; // need increment even if !ni
    if(ni){
      ni->seqnum = stamp;
    }
  }
  return r;
  */
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

// infp has already been set non-blocking
uint32_t notcurses_get(notcurses* nc, const struct timespec* ts, ncinput* ni){
  uint32_t r = internal_get(nc->tcache.ictx, ts, ni,
                            nc->margin_l, nc->margin_t);
  if(r != (uint32_t)-1){
    ++nc->stats.s.input_events;
  }
  return r;
}

// FIXME better performance if we move this within the locked area
int notcurses_getvec(notcurses* n, const struct timespec* ts,
                     ncinput* ni, int vcount){
  for(int v = 0 ; v < vcount ; ++v){
    // FIXME need to manage ts; right now, we could delay up to ts * vcount!
    uint32_t u = notcurses_get(n, ts, &ni[v]);
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
  return internal_get(n->tcache.ictx, ts, ni, 0, 0);
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
  *y = cloc->y;
  *x = cloc->x;
  pthread_mutex_unlock(&ictx->clock);
  return 0;
}

// Disable signals originating from the terminal's line discipline, i.e.
// SIGINT (^C), SIGQUIT (^\), and SIGTSTP (^Z). They are enabled by default.
int notcurses_linesigs_disable(notcurses* n){
#ifndef __MINGW64__
  if(n->tcache.ttyfd < 0){
    return 0;
  }
  struct termios tios;
  if(tcgetattr(n->tcache.ttyfd, &tios)){
    logerror("Couldn't preserve terminal state for %d (%s)\n", n->tcache.ttyfd, strerror(errno));
    return -1;
  }
  tios.c_lflag &= ~ISIG;
  if(tcsetattr(n->tcache.ttyfd, TCSANOW, &tios)){
    logerror("Error disabling signals on %d (%s)\n", n->tcache.ttyfd, strerror(errno));
    return -1;
  }
#else
  DWORD mode;
  if(!GetConsoleMode(n->tcache.inhandle, &mode)){
    logerror("error acquiring input mode\n");
    return -1;
  }
  mode &= ~ENABLE_PROCESSED_INPUT;
  if(!SetConsoleMode(n->tcache.inhandle, mode)){
    logerror("error setting input mode\n");
    return -1;
  }
#endif
  return 0;
}

// Restore signals originating from the terminal's line discipline, i.e.
// SIGINT (^C), SIGQUIT (^\), and SIGTSTP (^Z), if disabled.
int notcurses_linesigs_enable(notcurses* n){
#ifndef __MINGW64__
  if(n->tcache.ttyfd < 0){
    return 0;
  }
  struct termios tios;
  if(tcgetattr(n->tcache.ttyfd, &tios)){
    logerror("Couldn't preserve terminal state for %d (%s)\n", n->tcache.ttyfd, strerror(errno));
    return -1;
  }
  tios.c_lflag |= ~ISIG;
  if(tcsetattr(n->tcache.ttyfd, TCSANOW, &tios)){
    logerror("Error disabling signals on %d (%s)\n", n->tcache.ttyfd, strerror(errno));
    return -1;
  }
#else
  DWORD mode;
  if(!GetConsoleMode(n->tcache.inhandle, &mode)){
    logerror("error acquiring input mode\n");
    return -1;
  }
  mode |= ENABLE_PROCESSED_INPUT;
  if(!SetConsoleMode(n->tcache.inhandle, mode)){
    logerror("error setting input mode\n");
    return -1;
  }
#endif
  return 0;
}
