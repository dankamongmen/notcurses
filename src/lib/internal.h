#ifndef NOTCURSES_INTERNAL
#define NOTCURSES_INTERNAL

#ifdef __cplusplus
extern "C" {
#endif

#include "version.h"
#include "builddef.h"

#ifdef USE_FFMPEG
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
#include <libavutil/version.h>
#include <libswscale/version.h>
#include <libavformat/version.h>
#else
#ifdef USE_OIIO
const char* oiio_version(void);
#endif
#endif

#include <term.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <wctype.h>
#include <pthread.h>
#include <termios.h>
#include <stdbool.h>
#include <unictype.h>
#include <langinfo.h>
#include <netinet/in.h>
#include "notcurses/notcurses.h"
#include "egcpool.h"

struct esctrie;

// we can't define multipart ncvisual here, because OIIO requires C++ syntax,
// and we can't go throwing C++ syntax into this header. so it goes.

// A plane is memory for some rectilinear virtual window, plus current cursor
// state for that window. A notcurses context describes a single terminal, and
// has a z-order of planes (I see no advantage to maintaining a poset, and we
// instead just use a list, top-to-bottom). Every cell on the terminal is part
// of at least one plane, and at least one plane covers the entirety of the
// terminal (this plane is created during initialization).
//
// Functions update these virtual planes over a series of API calls. Eventually,
// notcurses_render() is called. We then do a depth buffer blit of updated
// cells. A cell is updated if the topmost plane including that cell updates it,
// not simply if any plane updates it.
//
// A plane may be partially or wholly offscreen--this might occur if the
// screen is resized, for example. Offscreen portions will not be rendered.
// Accesses beyond the borders of a panel, however, are errors.
//
// The framebuffer 'fb' is a set of rows. For scrolling, we interpret it as a
// circular buffer of rows. 'logrow' is the index of the row at the logical top
// of the plane.
typedef struct ncplane {
  cell* fb;              // "framebuffer" of character cells
  int logrow;            // logical top row, starts at 0, add one for each scroll
  int x, y;              // current cursor location within this plane
  // ncplane_yx() etc. use coordinates relative to the plane to which this
  // plane is bound, but absx/absy are always relative to the terminal origin.
  // they must thus be translated by any function which moves a parent plane.
  int absx, absy;        // origin of the plane relative to the screen
  int lenx, leny;        // size of the plane, [0..len{x,y}) is addressable
  // a notcurses context is made up of stacks, each rooted by a plane which is
  // bound to no other plane. the main stack is rooted by the standard plane,
  // and is the only stack which is rendered. each stack has its own z-axis.
  struct ncplane* above; // plane above us, NULL if we're on top
  struct ncplane* below; // plane below us, NULL if we're on bottom
  struct ncplane* bnext; // next in the bound list of plane to which we are bound
  struct ncplane** bprev;// link to us iff we're bound, NULL otherwise
  struct ncplane* blist; // head of list of bound planes
  // a root plane is bound to itself. every other plane has a path to its
  // stack's root via boundto. the standard plane is always bound to itself.
  struct ncplane* boundto;
  egcpool pool;          // attached storage pool for UTF-8 EGCs
  uint64_t channels;     // works the same way as cells
  void* userptr;         // slot for the user to stick some opaque pointer
  int (*resizecb)(struct ncplane*); // callback after parent is resized
  cell basecell;         // cell written anywhere that fb[i].gcluster == 0
  struct notcurses* nc;  // notcurses object of which we are a part
  char* name;            // used only for debugging
  ncalign_e align;       // relative to parent plane, for automatic realignment
  uint16_t stylemask;    // same deal as in a cell
  bool scrolling;        // is scrolling enabled? always disabled by default
} ncplane;

#include "blitset.h"

// current presentation state of the terminal. it is carried across render
// instances. initialize everything to 0 on a terminal reset / startup.
typedef struct renderstate {
  // we assemble the encoded output in a POSIX memstream, and keep it around
  // between uses. this could be a problem if it ever tremendously spiked, but
  // that's a highly unlikely situation.
  char* mstream;  // buffer for rendering memstream, see open_memstream(3)
  FILE* mstreamfp;// FILE* for rendering memstream
  size_t mstrsize;// size of rendering memstream

  // the current cursor position. this is independent of whether the cursor is
  // visible. it is the cell at which the next write will take place. this is
  // modified by: output, cursor moves, clearing the screen (during refresh).
  int y, x;

  uint32_t curattr;// current attributes set (does not include colors)
  unsigned lastr;  // foreground rgb, overloaded for palindexed fg
  unsigned lastg;
  unsigned lastb;
  unsigned lastbr; // background rgb, overloaded for palindexed bg
  unsigned lastbg;
  unsigned lastbb;
  // we can elide a color escape iff the color has not changed between the two
  // cells and the current cell uses no defaults, or if both the current and
  // the last used both defaults.
  bool fgelidable;
  bool bgelidable;
  bool fgpalelidable;
  bool bgpalelidable;
  bool defaultelidable;
} renderstate;

// Tablets are the toplevel entitites within an ncreel. Each corresponds to
// a single, distinct ncplane.
typedef struct nctablet {
  ncplane* p;                  // border plane, NULL when offscreen
  ncplane* cbp;                // data plane, NULL when offscreen
  struct nctablet* next;
  struct nctablet* prev;
  tabletcb cbfxn;              // application callback to draw cbp
  void* curry;                 // application data provided to cbfxn
} nctablet;

typedef struct ncreel {
  ncplane* p;           // ncplane this ncreel occupies, under tablets
  // doubly-linked list, a circular one when infinity scrolling is in effect.
  // points at the focused tablet (when at least one tablet exists, one must be
  // focused). it will be visibly focused following the next redraw.
  nctablet* tablets;
  nctablet* vft;        // the visibly-focused tablet
  enum {
    LASTDIRECTION_UP,
    LASTDIRECTION_DOWN,
  } direction;          // last direction of travel
  int tabletcount;      // could be derived, but we keep it o(1)
  ncreel_options ropts; // copied in ncreel_create()
} ncreel;

// ncmenu_item and ncmenu_section have internal and (minimal) external forms
typedef struct ncmenu_int_item {
  char* desc;           // utf-8 menu item, NULL for horizontal separator
  ncinput shortcut;     // shortcut, all should be distinct
  int shortcut_offset;  // column offset with desc of shortcut EGC
  char* shortdesc;      // description of shortcut, can be NULL
  int shortdesccols;    // columns occupied by shortcut description
  bool disabled;        // disabled?
} ncmenu_int_item;

typedef struct ncmenu_int_section {
  char* name;             // utf-8 c string
  int itemcount;
  ncmenu_int_item* items; // items, NULL iff itemcount == 0
  ncinput shortcut;       // shortcut, will be underlined if present in name
  int xoff;               // column offset from beginning of menu bar
  int bodycols;           // column width of longest item
  int itemselected;       // current item selected, -1 for no selection
  int shortcut_offset;    // column offset within name of shortcut EGC
  int enabled_item_count; // number of enabled items: section is disabled iff 0
} ncmenu_int_section;

typedef struct ncfdplane {
  ncfdplane_callback cb;      // invoked with fresh hot data
  ncfdplane_done_cb donecb;   // invoked on EOF (if !follow) or error
  void* curry;                // passed to the callbacks
  int fd;                     // we take ownership of the fd, and close it
  bool follow;                // keep trying to read past the end (event-based)
  ncplane* ncp;               // bound ncplane
  pthread_t tid;              // thread servicing this i/o
  bool destroyed;             // set in ncfdplane_destroy() in our own context
} ncfdplane;

typedef struct ncsubproc {
  ncfdplane* nfp;
  pid_t pid;                  // subprocess
  int pidfd;                  // for signalling/watching the subprocess
  pthread_t waittid;          // wait()ing thread if pidfd is not available
  pthread_mutex_t lock;       // guards waited
  bool waited;                // we've wait()ed on it, don't kill/wait further
} ncsubproc;

typedef struct ncreader {
  ncplane* ncp;               // always owned by ncreader
  uint64_t tchannels;         // channels for input text
  uint32_t tattrs;            // attributes for input text
  ncplane* textarea;          // grows as needed iff scrolling is enabled
  int xproject;               // virtual x location of ncp origin on textarea
  bool horscroll;             // is there horizontal panning?
  bool no_cmd_keys;           // are shortcuts disabled?
  bool manage_cursor;         // enable and place the terminal cursor
} ncreader;

typedef struct ncmenu {
  ncplane* ncp;
  int sectioncount;         // must be positive
  ncmenu_int_section* sections; // NULL iff sectioncount == 0
  int unrolledsection;      // currently unrolled section, -1 if none
  int headerwidth;          // minimum space necessary to display all sections
  uint64_t headerchannels;  // styling for header
  uint64_t dissectchannels; // styling for disabled section headers
  uint64_t sectionchannels; // styling for sections
  uint64_t disablechannels; // styling for disabled entries
  bool bottom;              // are we on the bottom (vs top)?
} ncmenu;

// terminfo cache
typedef struct tinfo {
  unsigned colors;// number of colors terminfo reported usable for this screen
  char* sgr;      // set many graphics properties at once
  char* sgr0;     // restore default presentation properties
  char* setaf;    // set foreground color (ANSI)
  char* setab;    // set background color (ANSI)
  char* op;       // set foreground and background color to default
  char* cup;      // move cursor
  char* cuu;      // move N cells up
  char* cub;      // move N cells left
  char* cuf;      // move N cells right
  char* cud;      // move N cells down
  char* cuf1;     // move 1 cell right
  char* cub1;     // move 1 cell left
  char* home;     // home cursor
  char* civis;    // hide cursor
  char* cnorm;    // restore cursor to default state
  char* hpa;      // horizontal position adjusment (move cursor on row)
  char* vpa;      // vertical position adjustment (move cursor on column)
  char* standout; // CELL_STYLE_STANDOUT
  char* uline;    // CELL_STYLE_UNDERLINK
  char* reverse;  // CELL_STYLE_REVERSE
  char* blink;    // CELL_STYLE_BLINK
  char* dim;      // CELL_STYLE_DIM
  char* bold;     // CELL_STYLE_BOLD
  char* italics;  // CELL_STYLE_ITALIC
  char* italoff;  // CELL_STYLE_ITALIC (disable)
  char* initc;    // set a palette entry's RGB value
  char* oc;       // restore original colors
  char* clearscr; // erase screen and home cursor
  char* cleareol; // clear to end of line
  char* clearbol; // clear to beginning of line
  char* sc;       // push the cursor location onto the stack
  char* rc;       // pop the cursor location off the stack
  char* smkx;     // enter keypad transmit mode (keypad_xmit)
  char* rmkx;     // leave keypad transmit mode (keypad_local)
  char* getm;     // get mouse events
  bool RGBflag;   // "RGB" flag for 24bpc truecolor
  bool CCCflag;   // "CCC" flag for palette set capability
  bool BCEflag;   // "BCE" flag for erases with background color
  bool AMflag;    // "AM" flag for automatic movement to next line
  char* smcup;    // enter alternate mode
  char* rmcup;    // restore primary mode
} tinfo;

typedef struct ncinputlayer {
  FILE* ttyinfp;  // FILE* for processing input
  unsigned char inputbuf[BUFSIZ];
  // we keep a wee ringbuffer of input queued up for delivery. if
  // inputbuf_occupied == sizeof(inputbuf), there is no room. otherwise, data
  // can be read to inputbuf_write_at until we fill up. the first datum
  // available for the app is at inputbuf_valid_starts iff inputbuf_occupied is
  // not 0. the main purpose is working around bad predictions of escapes.
  unsigned inputbuf_occupied;
  unsigned inputbuf_valid_starts;
  unsigned inputbuf_write_at;
  // number of input events seen. does not belong in ncstats, since it must not
  // be reset (semantics are relied upon by widgets for mouse click detection).
  uint64_t input_events;
  struct esctrie* inputescapes; // trie of input escapes -> ncspecial_keys
} ncinputlayer;

typedef struct ncdirect {
  palette256 palette;        // 256-indexed palette can be used instead of/with RGB
  FILE* ttyfp;               // FILE* for output tty
  int ctermfd;               // fd for controlling terminal
  tinfo tcache;              // terminfo cache
  unsigned fgrgb, bgrgb;     // last RGB values of foreground/background
  uint16_t stylemask;        // current styles
  ncinputlayer input;        // input layer; we're in cbreak mode
  bool fgdefault, bgdefault; // are FG/BG currently using default colors?
  bool utf8;                 // are we using utf-8 encoding, as hoped?
  struct termios tpreserved; // terminal state upon entry
  // some terminals (e.g. kmscon) return cursor coordinates inverted from the
  // typical order. we detect it the first time ncdirect_cursor_yx() is called.
  bool detected_cursor_inversion; // have we performed inversion testing?
  bool inverted_cursor;      // does the terminal return inverted coordinates?
  uint64_t flags;            // copied in ncdirect_init() from param
} ncdirect;

typedef struct notcurses {
  ncplane* top;     // topmost plane, never NULL
  ncplane* bottom;  // bottommost plane, never NULL 
  ncplane* stdplane;// standard plane, covers screen

  // the style state of the terminal is carried across render runs
  renderstate rstate;

  // we keep a copy of the last rendered frame. this facilitates O(1)
  // notcurses_at_yx() and O(1) damage detection (at the cost of some memory).
  cell* lastframe;// last rendered framebuffer, NULL until first render
  int lfdimx;     // dimensions of lastframe, unchanged by screen resize
  int lfdimy;     // lfdimx/lfdimy are 0 until first render
  egcpool pool;   // duplicate EGCs into this pool

  int cursory;    // desired cursor placement according to user. -1 is a don't-
  int cursorx;    //  care, otherwise moved here after each render.

  ncstats stats;  // some statistics across the lifetime of the notcurses ctx
  ncstats stashstats; // cumulative stats, unaffected by notcurses_stats_reset()

  int truecols;   // true number of columns in the physical rendering area.
                  // used only to see if output motion takes us to the next
                  // line thanks to terminal action alone.
  FILE* ttyfp;    // FILE* for writing rasterized data
  int ttyfd;      // file descriptor for controlling tty
  ncinputlayer input; // input layer; we're in cbreak mode
  FILE* renderfp; // debugging FILE* to which renderings are written
  tinfo tcache;   // terminfo cache
  struct termios tpreserved; // terminal state upon entry
  bool suppress_banner; // from notcurses_options

  // desired margins (best-effort only), copied in from notcurses_options
  int margin_t, margin_b, margin_r, margin_l;
  int loglevel;
  palette256 palette; // 256-indexed palette can be used instead of/with RGB
  bool palette_damage[NCPALETTESIZE];
  bool utf8;      // are we using utf-8 encoding, as hoped?
  bool libsixel;  // do we have Sixel support?
} notcurses;

void sigwinch_handler(int signo);

void init_lang(struct notcurses* nc); // nc may be NULL, only used for logging
int terminfostr(char** gseq, const char* name);
int interrogate_terminfo(tinfo* ti);

// Search the provided multibyte (UTF8) string 's' for the provided unicode
// codepoint 'cp'. If found, return the column offset of the EGC in which the
// codepoint appears in 'col', and the byte offset as the return value. If not
// found, -1 is returned, and 'col' is meaningless.
static inline int
mbstr_find_codepoint(const char* s, char32_t cp, int* col){
  mbstate_t ps;
  memset(&ps, 0, sizeof(ps));
  size_t bytes = 0;
  size_t r;
  wchar_t w;
  *col = 0;
  while((r = mbrtowc(&w, s + bytes, MB_CUR_MAX, &ps)) != (size_t)-1 && r != (size_t)-2){
    if(r == 0){
      break;
    }
    if(towlower(cp) == towlower(w)){
      return bytes;
    }
    *col += wcwidth(w);
    bytes += r;
  }
  return -1;
}

static inline ncplane*
ncplane_stdplane(ncplane* n){
  return notcurses_stdplane(ncplane_notcurses(n));
}

static inline const ncplane*
ncplane_stdplane_const(const ncplane* n){
  return notcurses_stdplane_const(ncplane_notcurses_const(n));
}

// load all known special keys from terminfo, and build the input sequence trie
int prep_special_keys(ncinputlayer* nc);

// free up the input escapes trie
void input_free_esctrie(struct esctrie** trie);

// initialize libav
int ncvisual_init(int loglevel);

static inline int
fbcellidx(int row, int rowlen, int col){
//fprintf(stderr, "row: %d rowlen: %d col: %d\n", row, rowlen, col);
  return row * rowlen + col;
}

// take a logical 'y' and convert it to the virtual 'y'. see HACKING.md.
static inline int
logical_to_virtual(const ncplane* n, int y){
//fprintf(stderr, "y: %d n->logrow: %d n->leny: %d\n", y, n->logrow, n->leny);
  return (y + n->logrow) % n->leny;
}

static inline int
nfbcellidx(const ncplane* n, int row, int col){
  return fbcellidx(logical_to_virtual(n, row), n->lenx, col);
}

// For our first attempt, O(1) uniform conversion from 8-bit r/g/b down to
// ~2.4-bit 6x6x6 cube + greyscale (assumed on entry; I know no way to
// even semi-portably recover the palette) proceeds via: map each 8-bit to
// a 5-bit target grey. if all 3 components match, select that grey.
// otherwise, c / 42.7 to map to 6 values.
static inline int
rgb_quantize_256(unsigned r, unsigned g, unsigned b){
  const unsigned GREYMASK = 0xf8;
  // if all 5 MSBs match, return grey from 24-member grey ramp or pure
  // black/white from original 16 (0 and 15, respectively)
  if((r & GREYMASK) == (g & GREYMASK) && (g & GREYMASK) == (b & GREYMASK)){
    // 256 / 26 == 9.846
    int gidx = r * 5 / 49 - 1;
    if(gidx < 0){
      return 0;
    }
    if(gidx >= 24){
      return 15;
    }
    return 232 + gidx;
  }
  r /= 43;
  g /= 43;
  b /= 43;
  return r * 36 + g * 6 + b + 16;
}

// We get black, red, green, yellow, blue, magenta, cyan, and white. Ugh. Under
// an additive model: G + R -> Y, G + B -> C, R + B -> M, R + G + B -> W
static inline int
rgb_quantize_8(unsigned r, unsigned g, unsigned b){
  static const int BLACK = 0;
  static const int RED = 1;
  static const int GREEN = 2;
  static const int YELLOW = 3;
  static const int BLUE = 4;
  static const int MAGENTA = 5;
  static const int CYAN = 6;
  static const int WHITE = 7;
  if(r < 128){ // we have no red
    if(g < 128){ // we have no green
      if(b < 128){
        return BLACK;
      }
      return BLUE;
    } // we have green
    if(b < 128){
      return GREEN;
    }
    return CYAN;
  }else if(g < 128){ // we have red, but not green
    if(b < 128){
      return RED;
    }
    return MAGENTA;
  }else if(b < 128){ // we have red and green
    return YELLOW;
  }
  return WHITE;
}

// Given r, g, and b values 0..255, do a weighted average per Rec. 601, and
// return the 8-bit greyscale value (this value will be the r, g, and b value
// for the new color).
static inline int
rgb_greyscale(int r, int g, int b){
  if(r < 0 || r > 255){
    return -1;
  }
  if(g < 0 || g > 255){
    return -1;
  }
  if(b < 0 || b > 255){
    return -1;
  }
  // Use Rec. 601 scaling plus linear approximation of gamma decompression
  float fg = (0.299 * (r / 255.0) + 0.587 * (g / 255.0) + 0.114 * (b / 255.0));
  return fg * 255;
}

static inline int
tty_emit(const char* name __attribute__ ((unused)), const char* seq, int fd){
  if(!seq){
    return -1;
  }
  size_t slen = strlen(seq);
  size_t written = 0;
  do{
    ssize_t ret = write(fd, seq, slen);
    if(ret > 0){
      written += ret;
    }
    if(ret < 0){
      if(errno != EAGAIN){
        break;
      }
    }
  }while(written < slen);
  if(written < slen){
//fprintf(stderr, "Error emitting %zub %s escape (%s)\n", strlen(seq), name, strerror(errno));
    return -1;
  }
  return 0;
}

static inline int
term_emit(const char* name __attribute__ ((unused)), const char* seq,
          FILE* out, bool flush){
  if(!seq){
    return -1;
  }
  if(fputs(seq, out) == EOF){
//fprintf(stderr, "Error emitting %zub %s escape (%s)\n", strlen(seq), name, strerror(errno));
    return -1;
  }
  if(flush){
    while(fflush(out) == EOF){
      if(errno != EAGAIN){
        fprintf(stderr, "Error flushing after %zub %s sequence (%s)\n",
                strlen(seq), name, strerror(errno));
        return -1;
      }
    }
  }
  return 0;
}

static inline int
term_bg_palindex(const notcurses* nc, FILE* out, unsigned pal){
  if(nc->tcache.setab == NULL){
    return 0;
  }
  return term_emit("setab", tiparm(nc->tcache.setab, pal), out, false);
}

static inline int
term_fg_palindex(const notcurses* nc, FILE* out, unsigned pal){
  if(nc->tcache.setaf == NULL){
    return 0;
  }
  return term_emit("setaf", tiparm(nc->tcache.setaf, pal), out, false);
}

static inline const char*
pool_extended_gcluster(const egcpool* pool, const cell* c){
  if(cell_simple_p(c)){
    return (const char*)&c->gcluster;
  }
  return egcpool_extended_gcluster(pool, c);
}

cell* ncplane_cell_ref_yx(ncplane* n, int y, int x);

static inline void
cell_set_wide(cell* c){
  c->channels |= CELL_WIDEASIAN_MASK;
}

#define NANOSECS_IN_SEC 1000000000

static inline uint64_t
timespec_to_ns(const struct timespec* t){
  return t->tv_sec * NANOSECS_IN_SEC + t->tv_nsec;
}

static inline struct timespec*
ns_to_timespec(uint64_t ns, struct timespec* ts){
  ts->tv_sec = ns / NANOSECS_IN_SEC;
  ts->tv_nsec = ns % NANOSECS_IN_SEC;
  return ts;
}

static inline void
cell_debug(const egcpool* p, const cell* c){
  fprintf(stderr, "gcluster: %u %s style: 0x%04x chan: 0x%016jx\n",
				  c->gcluster, egcpool_extended_gcluster(p, c), c->stylemask, c->channels);
}

static inline void
plane_debug(const ncplane* n, bool details){
  int dimy, dimx;
  ncplane_dim_yx(n, &dimy, &dimx);
  fprintf(stderr, "p: %p dim: %d/%d poolsize: %d\n", n, dimy, dimx, n->pool.poolsize);
  if(details){
    for(int y = 0 ; y < 1 ; ++y){
      for(int x = 0 ; x < 10 ; ++x){
        const cell* c = &n->fb[fbcellidx(y, dimx, x)];
        fprintf(stderr, "[%03d/%03d] ", y, x);
        cell_debug(&n->pool, c);
      }
    }
  }
}

static inline void
pool_release(egcpool* pool, cell* c){
  if(!cell_simple_p(c)){
    egcpool_release(pool, cell_egc_idx(c));
  }
  c->gcluster = 0; // don't subject ourselves to double-release problems
}

// set the cell 'c' to point into the egcpool at location 'eoffset'
static inline void
set_gcluster_egc(cell* c, int eoffset){
  c->gcluster = ntole(0x01000000ul) + eoffset;
}

// Duplicate one cell onto another, possibly crossing ncplanes.
static inline int
cell_duplicate_far(egcpool* tpool, cell* targ, const ncplane* splane, const cell* c){
  pool_release(tpool, targ);
  targ->stylemask = c->stylemask;
  targ->channels = c->channels;
  if(cell_simple_p(c)){
    targ->gcluster = c->gcluster;
    return 0;
  }
  assert(splane);
  const char* egc = cell_extended_gcluster(splane, c);
  // FIXME we could eliminate this strlen() with a cell_extended_gcluster_len()
  // that returned the length, combined with O(1) length for inlined EGCs...
  size_t ulen = strlen(egc);
  int eoffset = egcpool_stash(tpool, egc, ulen);
  if(eoffset < 0){
    return -1;
  }
  set_gcluster_egc(targ, eoffset);
  return 0;
}

int ncplane_resize_internal(ncplane* n, int keepy, int keepx,
                            int keepleny, int keeplenx, int yoff, int xoff,
                            int ylen, int xlen);

int update_term_dimensions(int fd, int* rows, int* cols);

static inline void*
memdup(const void* src, size_t len){
  void* ret = malloc(len);
  if(ret){
    memcpy(ret, src, len);
  }
  return ret;
}

void* bgra_to_rgba(const void* data, int rows, int rowstride, int cols);

int rgba_blit_dispatch(ncplane* nc, const struct blitset* bset, int placey,
                       int placex, int linesize, const void* data, int begy,
                       int begx, int leny, int lenx, bool blendcolors);

// find the "center" cell of two lengths. in the case of even rows/columns, we
// place the center on the top/left. in such a case there will be one more
// cell to the bottom/right of the center.
static inline void
center_box(int* RESTRICT y, int* RESTRICT x){
  if(y){
    *y = (*y - 1) / 2;
  }
  if(x){
    *x = (*x - 1) / 2;
  }
}

// find the "center" cell of a plane. in the case of even rows/columns, we
// place the center on the top/left. in such a case there will be one more
// cell to the bottom/right of the center.
static inline void
ncplane_center(const ncplane* n, int* RESTRICT y, int* RESTRICT x){
  *y = n->leny;
  *x = n->lenx;
  center_box(y, x);
}

int ncvisual_bounding_box(const struct ncvisual* ncv, int* leny, int* lenx,
                          int* offy, int* offx);

// Our gradient is a 2d lerp among the four corners of the region. We start
// with the observation that each corner ought be its exact specified corner,
// and the middle ought be the exact average of all four corners' components.
// Another observation is that if all four corners are the same, every cell
// ought be the exact same color. From this arises the observation that a
// perimeter element is not affected by the other three sides:
//
//  a corner element is defined by itself
//  a perimeter element is defined by the two points on its side
//  an internal element is defined by all four points
//
// 2D equation of state: solve for each quadrant's contribution (min 2x2):
//
//  X' = (xlen - 1) - X
//  Y' = (ylen - 1) - Y
//  TLC: X' * Y' * TL
//  TRC: X * Y' * TR
//  BLC: X' * Y * BL
//  BRC: X * Y * BR
//  steps: (xlen - 1) * (ylen - 1) [maximum steps away from origin]
//
// Then add TLC + TRC + BLC + BRC + steps / 2, and divide by steps (the
//  steps / 2 is to work around truncate-towards-zero).
static int
calc_gradient_component(unsigned tl, unsigned tr, unsigned bl, unsigned br,
                        int y, int x, int ylen, int xlen){
  assert(y >= 0);
  assert(y < ylen);
  assert(x >= 0);
  assert(x < xlen);
  const int avm = (ylen - 1) - y;
  const int ahm = (xlen - 1) - x;
  if(xlen < 2){
    if(ylen < 2){
      return tl;
    }
    return (tl * avm + bl * y) / (ylen - 1);
  }
  if(ylen < 2){
    return (tl * ahm + tr * x) / (xlen - 1);
  }
  const int tlc = ahm * avm * tl;
  const int blc = ahm * y * bl;
  const int trc = x * avm * tr;
  const int brc = y * x * br;
  const int divisor = (ylen - 1) * (xlen - 1);
  return ((tlc + blc + trc + brc) + divisor / 2) / divisor;
}

// calculate one of the channels of a gradient at a particular point.
static inline uint32_t
calc_gradient_channel(uint32_t ul, uint32_t ur, uint32_t ll, uint32_t lr,
                      int y, int x, int ylen, int xlen){
  uint32_t chan = 0;
  channel_set_rgb8_clipped(&chan,
                           calc_gradient_component(channel_r(ul), channel_r(ur),
                                                   channel_r(ll), channel_r(lr),
                                                   y, x, ylen, xlen),
                           calc_gradient_component(channel_g(ul), channel_g(ur),
                                                   channel_g(ll), channel_g(lr),
                                                   y, x, ylen, xlen),
                           calc_gradient_component(channel_b(ul), channel_b(ur),
                                                   channel_b(ll), channel_b(lr),
                                                   y, x, ylen, xlen));
  channel_set_alpha(&chan, channel_alpha(ul)); // precondition: all αs are equal
  return chan;
}

// calculate both channels of a gradient at a particular point, storing them
// into `channels'. x and y ought be the location within the gradient.
static inline void
calc_gradient_channels(uint64_t* channels, uint64_t ul, uint64_t ur,
                       uint64_t ll, uint64_t lr, int y, int x,
                       int ylen, int xlen){
  if(!channels_fg_default_p(ul)){
    channels_set_fchannel(channels,
                          calc_gradient_channel(channels_fchannel(ul),
                                                channels_fchannel(ur),
                                                channels_fchannel(ll),
                                                channels_fchannel(lr),
                                                y, x, ylen, xlen));
  }else{
    channels_set_fg_default(channels);
  }
  if(!channels_bg_default_p(ul)){
    channels_set_bchannel(channels,
                          calc_gradient_channel(channels_bchannel(ul),
                                                channels_bchannel(ur),
                                                channels_bchannel(ll),
                                                channels_bchannel(lr),
                                                y, x, ylen, xlen));
  }else{
    channels_set_bg_default(channels);
  }
}

// ncdirect needs to "fake" an isolated ncplane as a drawing surface for
// ncvisual_render(), and thus calls these low-level internal functions.
// they are not for general use -- check ncplane_new() and ncplane_destroy().
ncplane* ncplane_new_internal(notcurses* nc, ncplane* n, const ncplane_options* nopts);

void free_plane(ncplane* p);

// heap-allocated formatted output
char* ncplane_vprintf_prep(const char* format, va_list ap);

// Resize the provided ncviusal to the specified 'rows' x 'cols', but do not
// change the internals of the ncvisual. Uses oframe.
int ncvisual_blit(struct ncvisual* ncv, int rows, int cols,
                  ncplane* n, const struct blitset* bset,
                  int placey, int placex, int begy, int begx,
                  int leny, int lenx, bool blendcolors);

void nclog(const char* fmt, ...);

bool is_linux_console(const notcurses* nc, unsigned no_font_changes);

// get a file descriptor for the controlling tty device, -1 on error
int get_controlling_tty(void);

// logging
#define logerror(nc, fmt, ...) do{ \
  if(nc){ if((nc)->loglevel >= NCLOGLEVEL_ERROR){ \
    nclog("%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__); } \
  }else{ fprintf(stderr, "%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__); } \
}while(0);

#define logwarn(nc, fmt, ...) do{ \
  if(nc){ if((nc)->loglevel >= NCLOGLEVEL_WARNING){ \
    nclog("%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__); } \
  }else{ fprintf(stderr, "%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__); } \
}while(0);

#define loginfo(nc, fmt, ...) do{ \
  if(nc){ if((nc)->loglevel >= NCLOGLEVEL_INFO){ \
    nclog("%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__); } \
  } }while(0);

#define logverbose(nc, fmt, ...) do{ \
  if(nc){ if((nc)->loglevel >= NCLOGLEVEL_VERBOSE){ \
    nclog("%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__); } \
  } }while(0);

#define logdebug(nc, fmt, ...) do{ \
  if(nc){ if((nc)->loglevel >= NCLOGLEVEL_DEBUG){ \
    nclog("%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__); } \
  } }while(0);

#define logtrace(nc, fmt, ...) do{ \
  if(nc){ if((nc)->loglevel >= NCLOGLEVEL_TRACE){ \
    nclog("%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__); } \
  } }while(0);

// Convert a notcurses log level to some multimedia library equivalent.
int ffmpeg_log_level(ncloglevel_e level);

int term_setstyle(FILE* out, unsigned cur, unsigned targ, unsigned stylebit,
                  const char* ton, const char* toff);

// how many edges need touch a corner for it to be printed?
static inline unsigned
box_corner_needs(unsigned ctlword){
  return (ctlword & NCBOXCORNER_MASK) >> NCBOXCORNER_SHIFT;
}

// True if the cell does not generate background pixels (i.e., the cell is a
// solid or shaded block, or certain emoji).
static inline bool
cell_nobackground_p(const cell* c){
  return c->channels & CELL_NOBACKGROUND_MASK;
}

// Destroy a plane and all its bound descendants.
int ncplane_genocide(ncplane *ncp);

// Returns the result of blending two channels. 'blends' indicates how heavily
// 'c1' ought be weighed. If 'blends' is 0, 'c1' will be entirely replaced by
// 'c2'. If 'c1' is otherwise the default color, 'c1' will not be touched,
// since we can't blend default colors. Likewise, if 'c2' is a default color,
// it will not be used (unless 'blends' is 0).
//
// Palette-indexed colors do not blend. Do not pass me palette-indexed channels!
static inline unsigned
channels_blend(unsigned c1, unsigned c2, unsigned* blends){
  if(channel_alpha(c2) == CELL_ALPHA_TRANSPARENT){
    return c1; // do *not* increment *blends
  }
  unsigned rsum, gsum, bsum;
  channel_rgb8(c2, &rsum, &gsum, &bsum);
  bool c2default = channel_default_p(c2);
  if(*blends == 0){
    // don't just return c2, or you set wide status and all kinds of crap
    if(channel_default_p(c2)){
      channel_set_default(&c1);
    }else{
      channel_set_rgb8(&c1, rsum, gsum, bsum);
    }
    channel_set_alpha(&c1, channel_alpha(c2));
  }else if(!c2default && !channel_default_p(c1)){
    rsum = (channel_r(c1) * *blends + rsum) / (*blends + 1);
    gsum = (channel_g(c1) * *blends + gsum) / (*blends + 1);
    bsum = (channel_b(c1) * *blends + bsum) / (*blends + 1);
    channel_set_rgb8(&c1, rsum, gsum, bsum);
    channel_set_alpha(&c1, channel_alpha(c2));
  }
  ++*blends;
  return c1;
}

// do not pass palette-indexed channels!
static inline uint64_t
cell_blend_fchannel(cell* cl, unsigned channel, unsigned* blends){
  return cell_set_fchannel(cl, channels_blend(cell_fchannel(cl), channel, blends));
}

static inline uint64_t
cell_blend_bchannel(cell* cl, unsigned channel, unsigned* blends){
  return cell_set_bchannel(cl, channels_blend(cell_bchannel(cl), channel, blends));
}

// examine the UTF-8 EGC in the first |*bytes| bytes of |egc|. if the EGC is
// right-to-left, we make a copy, appending an U+200E to force left-to-right.
// only the first unicode char of the EGC is currently checked FIXME. if the
// EGC is not RTL, we return NULL.
static char*
egc_rtl(const char* egc, int* bytes){
  wchar_t w;
  mbstate_t mbstate = { };
  size_t r = mbrtowc(&w, egc, *bytes, &mbstate);
  if(r == (size_t)-1 || r == (size_t)-2){
    return NULL;
  }
  const int bidic = uc_bidi_category(w);
  if(bidic != UC_BIDI_R && bidic != UC_BIDI_RLE && bidic != UC_BIDI_RLO){
    return NULL;
  }
  // insert U+200E, "LEFT-TO-RIGHT MARK". This ought reset the text direction
  // after emitting a potentially RTL EGC.
  const char LTRMARK[] = "\xe2\x80\x8e";
  char* s = (char*)malloc(*bytes + sizeof(LTRMARK)); // cast for C++ callers
  memcpy(s, egc, *bytes);
  memcpy(s + *bytes, LTRMARK, sizeof(LTRMARK));
  return s;
}

// lowest level of cell+pool setup. if the EGC changes the output to RTL, it
// must be suffixed with a LTR-forcing character by now, and both
// CELL_WIDEASIAN_MASK and CELL_NOBACKGROUND_MASK ought be set however they're
// going to be set.
static inline int
pool_blit_direct(egcpool* pool, cell* c, const char* gcluster, int bytes, int cols){
  pool_release(pool, c);
  if(bytes < 0 || cols < 0){
    return -1;
  }
  if(bytes <= 1){
    assert(cols < 2);
    c->gcluster = 0;
    ((unsigned char*)&c->gcluster)[0] = *gcluster;
    return bytes;
  }
  if(bytes <= 4){
    if(strcmp(gcluster, (const char*)&c->gcluster)){
      c->gcluster = 0;
      memcpy(&c->gcluster, gcluster, bytes);
    }
    return bytes;
  }
  int eoffset = egcpool_stash(pool, gcluster, bytes);
  if(eoffset < 0){
    return -1;
  }
  set_gcluster_egc(c, eoffset);
  return bytes;
}

static inline int
pool_load_direct(egcpool* pool, cell* c, const char* gcluster, int bytes, int cols){
  if(bytes <= 1){
    c->channels &= ~(CELL_WIDEASIAN_MASK | CELL_NOBACKGROUND_MASK);
  }else if(cols < 2){
    c->channels &= ~CELL_WIDEASIAN_MASK;
    // FIXME also shaded blocks! ░ etc. are there combined EGCs involving these?
    if(bytes == 3 && memcmp(gcluster, "\xe2\x96\x88", 4) == 0){
      c->channels |= CELL_NOBACKGROUND_MASK;
    }else{
      c->channels &= ~CELL_NOBACKGROUND_MASK;
    }
  }else{
    c->channels |= CELL_WIDEASIAN_MASK;
    c->channels &= ~CELL_NOBACKGROUND_MASK;
  }
  char* rtl = egc_rtl(gcluster, &bytes); // checks for RTL and adds U+200E if so
  if(rtl){
    gcluster = rtl;
  }
  int r = pool_blit_direct(pool, c, gcluster, bytes, cols);
  free(rtl);
  return r;
}

static inline int
cell_load_direct(ncplane* n, cell* c, const char* gcluster, int bytes, int cols){
  return pool_load_direct(&n->pool, c, gcluster, bytes, cols);
}

static inline int
pool_load(egcpool* pool, cell* c, const char* gcluster){
  int cols;
  int bytes = utf8_egc_len(gcluster, &cols);
  return pool_load_direct(pool, c, gcluster, bytes, cols);
}

// increment y by 1 and rotate the framebuffer up one line. x moves to 0.
void scroll_down(ncplane* n);

static inline bool
islinebreak(wchar_t wchar){
  // UC_LINE_SEPARATOR + UC_PARAGRAPH_SEPARATOR
  if(wchar == L'\n' || wchar == L'\v' || wchar == L'\f'){
    return true;
  }
  const uint32_t mask = UC_CATEGORY_MASK_Zl | UC_CATEGORY_MASK_Zp;
  return uc_is_general_category_withtable(wchar, mask);
}

static inline bool
iswordbreak(wchar_t wchar){
  const uint32_t mask = UC_CATEGORY_MASK_Z |
                        UC_CATEGORY_MASK_Zs;
  return uc_is_general_category_withtable(wchar, mask);
}

// the heart of damage detection. compare two cells (from two different planes)
// for equality. if they are equal, return 0. otherwise, dup the second onto
// the first and return non-zero.
static inline int
cellcmp_and_dupfar(egcpool* dampool, cell* damcell,
                   const ncplane* srcplane, const cell* srccell){
  if(damcell->stylemask == srccell->stylemask){
    if(damcell->channels == srccell->channels){
      const char* srcegc = cell_extended_gcluster(srcplane, srccell);
      const char* damegc = pool_extended_gcluster(dampool, damcell);
      if(strcmp(damegc, srcegc) == 0){
        return 0; // EGC match
      }
    }
  }
  cell_duplicate_far(dampool, damcell, srcplane, srccell);
  return 1;
}

int ncinputlayer_init(ncinputlayer* nilayer, FILE* infp);

// FIXME absorb into ncinputlayer_init()
int cbreak_mode(int ttyfd, const struct termios* tpreserved);

#ifdef __cplusplus
}
#endif

#endif
