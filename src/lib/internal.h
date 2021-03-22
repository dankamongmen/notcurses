#ifndef NOTCURSES_INTERNAL
#define NOTCURSES_INTERNAL

#ifdef __cplusplus
extern "C" {
#endif

#include "version.h"
#include "builddef.h"

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
#include "compat/compat.h"
#include "egcpool.h"

#define API __attribute__((visibility("default")))
#define ALLOC __attribute__((malloc)) __attribute__((warn_unused_result))

struct esctrie;
struct ncvisual_details;

// Does this glyph completely obscure the background? If so, there's no need
// to emit a background when rasterizing, a small optimization.
#define CELL_NOBACKGROUND_MASK  0x8700000000000000ull

// Was this glyph drawn as part of an ncvisual? If so, we need to honor
// blitter stacking rather than the standard trichannel solver.
#define CELL_BLITTERSTACK_MASK  CELL_NOBACKGROUND_MASK

// we can't define multipart ncvisual here, because OIIO requires C++ syntax,
// and we can't go throwing C++ syntax into this header. so it goes.

typedef enum {
  SPRIXEL_NOCHANGE,
  SPRIXEL_INVALIDATED,
  SPRIXEL_HIDE,
} sprixel_e;

// there is a context-wide set of displayed pixel glyphs ("sprixels"); i.e.
// these are independent of particular piles. there should never be very many
// associated with a context (a dozen or so at max). with the kitty protocol,
// we can register them, and then manipulate them by id. with the sixel
// protocol, we just have to rewrite them.
typedef struct sprixel {
  char* glyph;       // glyph; can be quite large
  int id;            // embedded into glusters field of nccell
  struct ncplane* n; // associated ncplane
  sprixel_e invalidated;
  struct sprixel* next;
  int y, x;
  int dimy, dimx;    // cell geometry
  int pixy, pixx;    // pixel geometry (might be smaller than cell geo)
  int* tacache;       // transparency-annihilatin cache (dimy * dimx)
} sprixel;

// A plane is memory for some rectilinear virtual window, plus current cursor
// state for that window, and part of a pile. Each pile has a total order along
// its z-axis. Functions update these virtual planes over a series of API
// calls. Eventually, notcurses_render() is called. We then do a depth buffer
// blit of updated cells. A cell is updated if the topmost plane including that
// cell updates it, not simply if any plane updates it.
//
// A plane may be partially or wholly offscreen--this might occur if the
// screen is resized, for example. Offscreen portions will not be rendered.
// Accesses beyond the borders of a panel, however, are errors.
//
// The framebuffer 'fb' is a set of rows. For scrolling, we interpret it as a
// circular buffer of rows. 'logrow' is the index of the row at the logical top
// of the plane. It only changes from 0 if the plane is scrollable.
typedef struct ncplane {
  nccell* fb;            // "framebuffer" of character cells
  int logrow;            // logical top row, starts at 0, add one for each scroll
  int x, y;              // current cursor location within this plane
  // ncplane_yx() etc. use coordinates relative to the plane to which this
  // plane is bound, but absx/absy are always relative to the terminal origin.
  // they must thus be translated by any function which moves a parent plane.
  int absx, absy;        // origin of the plane relative to the pile's origin
  int lenx, leny;        // size of the plane, [0..len{x,y}) is addressable
  egcpool pool;          // attached storage pool for UTF-8 EGCs
  uint64_t channels;     // works the same way as cells

  // a notcurses context is made up of piles, each rooted by one or more root
  // planes. each pile has its own (totally ordered) z-axis.
  struct ncpile* pile;   // pile of which we are a part
  struct ncplane* above; // plane above us, NULL if we're on top
  struct ncplane* below; // plane below us, NULL if we're on bottom

  // every plane is bound to some other plane, unless it is a root plane of a
  // pile. a pile has a set of one or more root planes, all of them siblings.
  // root planes are bound to themselves. the standard plane is always a root
  // plane (since it cannot be reparented). a path exists to a root plane.
  struct ncplane* bnext;  // next in the blist
  struct ncplane** bprev; // blist link back to us
  struct ncplane* blist;  // head of list of bound planes
  struct ncplane* boundto;// plane to which we are bound (ourself for roots)

  sprixel* sprite;       // pointer into the sprixel cache

  void* userptr;         // slot for the user to stick some opaque pointer
  int (*resizecb)(struct ncplane*); // callback after parent is resized
  nccell basecell;       // cell written anywhere that fb[i].gcluster == 0
  char* name;            // used only for debugging
  ncalign_e align;       // relative to parent plane, for automatic realignment
  uint16_t stylemask;    // same deal as in a cell
  bool scrolling;        // is scrolling enabled? always disabled by default
} ncplane;

// current presentation state of the terminal. it is carried across render
// instances. initialize everything to 0 on a terminal reset / startup.
typedef struct rasterstate {
  // we assemble the encoded (rasterized) output in a POSIX memstream, and keep
  // it around between uses. this could be a problem if it ever tremendously
  // spiked, but that's a highly unlikely situation.
  char* mstream;  // buffer for rasterizing memstream, see open_memstream(3)
  FILE* mstreamfp;// FILE* for rasterizing memstream
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
  // we elide a color escape iff the color has not changed between two cells
  bool fgelidable;
  bool bgelidable;
  bool fgpalelidable;
  bool bgpalelidable;
  bool fgdefelidable;
  bool bgdefelidable;

  // need we do a hard cursor update (i.e. did we just emit a pixel graphic)?
  bool hardcursorpos;
} rasterstate;

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

typedef struct ncprogbar {
  ncplane* ncp;
  double progress;          // on the range [0, 1]
  uint32_t ulchannel, urchannel, blchannel, brchannel;
  bool retrograde;
} ncprogbar;

// terminfo cache
typedef struct tinfo {
  unsigned colors;// number of colors terminfo reported usable for this screen
  char* sgr;      // set many graphics properties at once
  char* sgr0;     // restore default presentation properties
  char* setaf;    // set foreground color (ANSI)
  char* setab;    // set background color (ANSI)
  char* op;       // set foreground and background color to default
  char* fgop;     // set foreground to default
  char* bgop;     // set background to default
  char* cup;      // move cursor
  char* cuu;      // move N cells up
  char* cub;      // move N cells left
  char* cuf;      // move N cells right
  char* cud;      // move N cells down
  char* cuf1;     // move 1 cell right
  char* home;     // home cursor
  char* civis;    // hide cursor
  char* cnorm;    // restore cursor to default state
  char* hpa;      // horizontal position adjusment (move cursor on row)
  char* vpa;      // vertical position adjustment (move cursor on column)
  char* standout; // NCSTYLE_STANDOUT
  char* uline;    // NCSTYLE_UNDERLINK
  char* reverse;  // NCSTYLE_REVERSE
  char* blink;    // NCSTYLE_BLINK
  char* dim;      // NCSTYLE_DIM
  char* bold;     // NCSTYLE_BOLD
  char* italics;  // NCSTYLE_ITALIC
  char* italoff;  // NCSTYLE_ITALIC (disable)
  char* struck;   // NCSTYLE_STRUCK
  char* struckoff;// NCSTYLE_STRUCK (disable)
  char* initc;    // set a palette entry's RGB value
  char* oc;       // restore original colors
  char* clearscr; // erase screen and home cursor
  char* sc;       // push the cursor location onto the stack
  char* rc;       // pop the cursor location off the stack
  char* smkx;     // enter keypad transmit mode (keypad_xmit)
  char* rmkx;     // leave keypad transmit mode (keypad_local)
  char* getm;     // get mouse events
  char* smcup;    // enter alternate mode
  char* rmcup;    // restore primary mode
  bool RGBflag;   // "RGB" flag for 24bpc truecolor
  bool CCCflag;   // "CCC" flag for palette set capability
  bool BCEflag;   // "BCE" flag for erases with background color
  bool AMflag;    // "AM" flag for automatic movement to next line
  bool utf8;      // are we using utf-8 encoding, as hoped?

  // we use the cell's size in pixels for pixel blitting. this information can
  // be acquired on all terminals with pixel support.
  int cellpixy;   // cell pixel height, might be 0
  int cellpixx;   // cell pixel width, might be 0

  // kitty interprets an RGB background that matches the default background
  // color *as* the default background, meaning it'll be translucent if
  // background_opaque is in use. detect this, and avoid the default if so.
  // bg_collides_default is either 0x0000000 or 0x1RRGGBB.
  uint32_t bg_collides_default;
  pthread_mutex_t pixel_query; // only query for pixel support once
  int color_registers; // sixel color registers (post pixel_query_done)
  int sixel_maxx, sixel_maxy; // sixel size maxima (post pixel_query_done)
  bool sixel_supported;  // do we support sixel (post pixel_query_done)?
  int sprixelnonce;      // next sprixel id
  int (*pixel_destroy)(const struct notcurses* nc, const struct ncpile* p, FILE* out, sprixel* s);
  // wipe out a cell's worth of pixels from within a sprixel. for sixel, this
  // means leaving out the pixels (and likely resizes the string). for kitty,
  // this means dialing down their alpha to 0 (in equivalent space).
  int (*pixel_cell_wipe)(const struct notcurses* nc, sprixel* s, int y, int x);
  int (*pixel_clear_all)(const struct notcurses* nc);
  bool pixel_query_done; // have we yet performed pixel query?
  bool sextants;  // do we have (good, vetted) Unicode 13 sextant support?
  bool braille;   // do we have Braille support? (linux console does not)
} tinfo;

typedef struct ncinputlayer {
  int ttyinfd;  // file descriptor for processing input
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
  uint64_t channels;         // current channels
  uint16_t stylemask;        // current styles
  ncinputlayer input;        // input layer; we're in cbreak mode
  struct termios tpreserved; // terminal state upon entry
  // some terminals (e.g. kmscon) return cursor coordinates inverted from the
  // typical order. we detect it the first time ncdirect_cursor_yx() is called.
  bool detected_cursor_inversion; // have we performed inversion testing?
  bool inverted_cursor;      // does the terminal return inverted coordinates?
  bool initialized_readline; // have we initialized Readline?
  uint64_t flags;            // copied in ncdirect_init() from param
} ncdirect;

typedef struct ncpile {
  ncplane* top;               // topmost plane, never NULL
  ncplane* bottom;            // bottommost plane, never NULL
  ncplane* roots;             // head of root plane list
  struct crender* crender;    // array (rows * cols crender objects)
  struct notcurses* nc;       // notcurses context
  struct ncpile *prev, *next; // circular list
  size_t crenderlen;          // size of crender vector
  int dimy, dimx;             // rows and cols at time of render
} ncpile;

// the standard pile can be reached through ->stdplane.
typedef struct notcurses {
  ncplane* stdplane; // standard plane, covers screen

  // the style state of the terminal is carried across render runs
  rasterstate rstate;

  // we keep a copy of the last rendered frame. this facilitates O(1)
  // notcurses_at_yx() and O(1) damage detection (at the cost of some memory).
  nccell* lastframe;// last rasterized framebuffer, NULL until first raster
  egcpool pool;   // egcpool for lastframe

  int lfdimx;     // dimensions of lastframe, unchanged by screen resize
  int lfdimy;     // lfdimx/lfdimy are 0 until first rasterization

  int cursory;    // desired cursor placement according to user.
  int cursorx;    // -1 is don't-care, otherwise moved here after each render.

  pthread_mutex_t statlock; // FIXME align on cacheline
  ncstats stats;  // some statistics across the lifetime of the notcurses ctx
  ncstats stashed_stats; // retain across a notcurses_stats_reset(), to print in closing banner

  FILE* ttyfp;    // FILE* for writing rasterized data
  int ttyfd;      // file descriptor for controlling tty
  ncinputlayer input; // input layer; we're in cbreak mode
  FILE* renderfp; // debugging FILE* to which renderings are written
  tinfo tcache;   // terminfo cache
  struct termios tpreserved; // terminal state upon entry
  pthread_mutex_t pilelock; // guards pile list, locks resize in render
  bool suppress_banner; // from notcurses_options

  sprixel* sprixelcache; // list of pixel graphics currently displayed

  // desired margins (best-effort only), copied in from notcurses_options
  int margin_t, margin_b, margin_r, margin_l;
  int loglevel;
  palette256 palette; // 256-indexed palette can be used instead of/with RGB
  bool palette_damage[NCPALETTESIZE];
  unsigned stdio_blocking_save; // was stdio blocking at entry? restore on stop.
} notcurses;

// cell vs pixel-specific arguments
typedef union {
  struct {
    int blendcolors;    // use CELL_ALPHA_BLEND
    int placey;           // placement within ncplane
    int placex;
  } cell;               // for cells
  struct {
    int celldimx;       // horizontal pixels per cell
    int celldimy;       // vertical pixels per cell
    int colorregs;      // number of color registers
    int sprixelid;      // unqie 24-bit id into sprixel cache
    int placey;         // placement within ncplane
    int placex;
  } pixel;              // for pixels
} blitterargs;

typedef int (*blitter)(struct ncplane* n, int linesize, const void* data,
                       int begy, int begx, int leny, int lenx,
                       const blitterargs* bargs);

// a system for rendering RGBA pixels as text glyphs
struct blitset {
  ncblitter_e geom;
  int width;
  int height;
  // the EGCs which form the various levels of a given plotset. if the geometry
  // is wide, things are arranged with the rightmost side increasing most
  // quickly, i.e. it can be indexed as height arrays of 1 + height glyphs. i.e.
  // the first five braille EGCs are all 0 on the left, [0..4] on the right.
  const wchar_t* egcs;
  blitter blit;
  const char* name;
  bool fill;
};

#include "blitset.h"

static inline int
ncfputs(const char* ext, FILE* out){
  int r;
#ifdef __USE_GNU
  r = fputs_unlocked(ext, out);
#else
  r = fputs(ext, out);
#endif
  return r;
}

static inline int
ncfputc(char c, FILE* out){
#ifdef __USE_GNU
  return fputc_unlocked(c, out);
#else
  return fputc(c, out);
#endif
}

void reset_stats(ncstats* stats);
void summarize_stats(notcurses* nc);

void sigwinch_handler(int signo);

void init_lang(notcurses* nc); // nc may be NULL, only used for logging
int terminfostr(char** gseq, const char* name);

// load |ti| from the terminfo database, which must already have been
// initialized. set |utf8| if we've verified UTF8 output encoding.
int interrogate_terminfo(tinfo* ti, int fd, const char* termname, unsigned utf8);

void free_terminfo_cache(tinfo* ti);

// perform queries that require writing to the terminal, and reading a
// response, rather than simply reading the terminfo database. can result
// in a lengthy delay or even block if the terminal doesn't respond.
int query_term(tinfo* ti, int fd);

// if there were missing elements we wanted from terminfo, bitch about them here
void warn_terminfo(const notcurses* nc, const tinfo* ti);

int resize_callbacks_children(ncplane* n);

static inline ncpile*
ncplane_pile(ncplane* n){
  return n->pile;
}

static inline const ncpile*
ncplane_pile_const(const ncplane* n){
  return n->pile;
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

// write(2) with retry on partial write or interrupted write
static inline ssize_t
writen(int fd, const void* buf, size_t len){
  ssize_t r;
  size_t w = 0;
  while(w < len){
    if((r = write(fd, (const char*)buf + w, len - w)) < 0){
      if(errno == EAGAIN || errno == EBUSY || errno == EINTR){
        continue;
      }
      return -1;
    }
    w += r;
  }
  return w;
}

static inline int
tty_emit(const char* seq, int fd){
  if(!seq){
    return -1;
  }
  size_t slen = strlen(seq);
  if(writen(fd, seq, slen) < 0){
    return -1;
  }
  return 0;
}

static inline int
term_emit(const char* seq, FILE* out, bool flush){
  if(!seq){
    return -1;
  }
  if(ncfputs(seq, out) == EOF){
//fprintf(stderr, "Error emitting %zub escape (%s)\n", strlen(seq), strerror(errno));
    return -1;
  }
  if(flush){
    while(fflush(out) == EOF){
      if(errno != EAGAIN && errno != EINTR && errno != EBUSY){
        fprintf(stderr, "Error flushing after %zub sequence (%s)\n", strlen(seq), strerror(errno));
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
  return term_emit(tiparm(nc->tcache.setab, pal), out, false);
}

static inline int
term_fg_palindex(const notcurses* nc, FILE* out, unsigned pal){
  if(nc->tcache.setaf == NULL){
    return 0;
  }
  return term_emit(tiparm(nc->tcache.setaf, pal), out, false);
}

static inline const char*
pool_extended_gcluster(const egcpool* pool, const nccell* c){
  if(cell_simple_p(c)){
    return (const char*)&c->gcluster;
  }
  return egcpool_extended_gcluster(pool, c);
}

static inline nccell*
ncplane_cell_ref_yx(ncplane* n, int y, int x){
  return &n->fb[nfbcellidx(n, y, x)];
}

static inline void
cell_debug(const egcpool* p, const nccell* c){
  fprintf(stderr, "gcluster: %08x %s style: 0x%04x chan: 0x%016jx\n",
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
        const nccell* c = &n->fb[fbcellidx(y, dimx, x)];
        fprintf(stderr, "[%03d/%03d] ", y, x);
        cell_debug(&n->pool, c);
      }
    }
  }
}

void sprixel_free(sprixel* s);
void sprixel_hide(sprixel* s);
// dimy and dimx are cell geometry, not pixel
sprixel* sprixel_create(ncplane* n, const char* s, int bytes, int placey, int placex,
                        int sprixelid, int dimy, int dimx, int pixy, int pixx);
API int sprite_wipe_cell(const notcurses* nc, sprixel* s, int y, int x);
int sprite_kitty_annihilate(const notcurses* nc, const ncpile* p, FILE* out, sprixel* s);
int sprite_kitty_clear_all(const notcurses* nc);
int sprite_sixel_annihilate(const notcurses* nc, const ncpile* p, FILE* out, sprixel* s);
int sprite_clear_all(const notcurses* nc);

static inline void
pool_release(egcpool* pool, nccell* c){
  if(cell_extended_p(c)){
    egcpool_release(pool, cell_egc_idx(c));
  }
  c->gcluster = 0; // don't subject ourselves to double-release problems
}

// set the nccell 'c' to point into the egcpool at location 'eoffset'
static inline void
set_gcluster_egc(nccell* c, int eoffset){
  c->gcluster = htole(0x01000000ul) + htole(eoffset);
}

// Duplicate one nccell onto another, possibly crossing ncplanes.
static inline int
cell_duplicate_far(egcpool* tpool, nccell* targ, const ncplane* splane, const nccell* c){
  pool_release(tpool, targ);
  targ->stylemask = c->stylemask;
  targ->channels = c->channels;
  targ->width = c->width;
  if(!cell_extended_p(c)){
    targ->gcluster = c->gcluster;
    return 0;
  }
  const char* egc = cell_extended_gcluster(splane, c);
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

int update_term_dimensions(int fd, int* rows, int* cols, tinfo* tcache);

ALLOC static inline void*
memdup(const void* src, size_t len){
  void* ret = malloc(len);
  if(ret){
    memcpy(ret, src, len);
  }
  return ret;
}

ALLOC void* bgra_to_rgba(const void* data, int rows, int rowstride, int cols);

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
ALLOC char* ncplane_vprintf_prep(const char* format, va_list ap);

// Resize the provided ncviusal to the specified 'rows' x 'cols', but do not
// change the internals of the ncvisual. Uses oframe.
int ncvisual_blit(struct ncvisual* ncv, int rows, int cols,
                  ncplane* n, const struct blitset* bset,
                  int begy, int begx, int leny, int lenx, const blitterargs* bargs);

void nclog(const char* fmt, ...);

bool is_linux_console(const notcurses* nc, unsigned no_font_changes);

// get a file descriptor for the controlling tty device, -1 on error
int get_controlling_tty(FILE* fp);

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
cell_nobackground_p(const nccell* c){
  return (c->channels & CELL_NOBACKGROUND_MASK) == CELL_NOBACKGROUND_MASK;
}

// Returns a number 0 <= n <= 15 representing the four quadrants, and which (if
// any) are occupied due to blitting with a transparent background. The mapping
// is {tl, tr, bl, br}.
static inline unsigned
cell_blittedquadrants(const nccell* c){
  return ((c->channels & 0x8000000000000000ull) ? 1 : 0) |
         ((c->channels & 0x0400000000000000ull) ? 2 : 0) |
         ((c->channels & 0x0200000000000000ull) ? 4 : 0) |
         ((c->channels & 0x0100000000000000ull) ? 8 : 0);
}

// Set this whenever blitting an ncvisual, when we have a transparent
// background. In such cases, ncvisuals underneath the cell must be rendered
// slightly differently.
static inline void
cell_set_blitquadrants(nccell* c, unsigned tl, unsigned tr, unsigned bl, unsigned br){
  // FIXME want a static assert that these four constants OR together to
  // equal CELL_BLITTERSTACK_MASK, bah
  uint64_t newval = (tl ? 0x8000000000000000ull : 0) |
                    (tr ? 0x0400000000000000ull : 0) |
                    (bl ? 0x0200000000000000ull : 0) |
                    (br ? 0x0100000000000000ull : 0);
  c->channels = ((c->channels & ~CELL_BLITTERSTACK_MASK) | newval);
}

// Destroy a plane and all its bound descendants.
int ncplane_genocide(ncplane *ncp);

// Extract the 32-bit background channel from a cell.
static inline uint32_t
cell_bchannel(const nccell* cl){
  return channels_bchannel(cl->channels);
}

// Extract those elements of the channel which are common to both foreground
// and background channel representations.
static inline uint32_t
channel_common(uint32_t channel){
  return channel & (CELL_BGDEFAULT_MASK | CELL_BG_RGB_MASK |
                    CELL_BG_PALETTE | CELL_BG_ALPHA_MASK);
}

// Extract those elements of the background channel which may be freely swapped
// with the foreground channel (alpha and coloring info).
static inline uint32_t
cell_bchannel_common(const nccell* cl){
  return channel_common(cell_bchannel(cl));
}

// Extract the 32-bit foreground channel from a cell.
static inline uint32_t
cell_fchannel(const nccell* cl){
  return channels_fchannel(cl->channels);
}

// Extract those elements of the foreground channel which may be freely swapped
// with the background channel (alpha and coloring info).
static inline uint32_t
cell_fchannel_common(const nccell* cl){
  return channel_common(cell_fchannel(cl));
}

// Set the 32-bit background channel of an nccell.
static inline uint64_t
cell_set_bchannel(nccell* cl, uint32_t channel){
  return channels_set_bchannel(&cl->channels, channel);
}

// Set the 32-bit foreground channel of an nccell.
static inline uint64_t
cell_set_fchannel(nccell* cl, uint32_t channel){
  return channels_set_fchannel(&cl->channels, channel);
}

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
  bool c2default = channel_default_p(c2);
  if(*blends == 0){
    // don't just return c2, or you set wide status and all kinds of crap
    if(channel_default_p(c2)){
      channel_set_default(&c1);
    }else{
      channel_set(&c1, c2 & CELL_BG_RGB_MASK);
    }
    channel_set_alpha(&c1, channel_alpha(c2));
  }else if(!c2default && !channel_default_p(c1)){
    unsigned rsum, gsum, bsum;
    channel_rgb8(c2, &rsum, &gsum, &bsum);
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
cell_blend_fchannel(nccell* cl, unsigned channel, unsigned* blends){
  return cell_set_fchannel(cl, channels_blend(cell_fchannel(cl), channel, blends));
}

static inline uint64_t
cell_blend_bchannel(nccell* cl, unsigned channel, unsigned* blends){
  return cell_set_bchannel(cl, channels_blend(cell_bchannel(cl), channel, blends));
}

// examine the UTF-8 EGC in the first |*bytes| bytes of |egc|. if the EGC is
// right-to-left, we make a copy, appending an U+200E to force left-to-right.
// only the first unicode char of the EGC is currently checked FIXME. if the
// EGC is not RTL, we return NULL.
ALLOC static inline char*
egc_rtl(const char* egc, int* bytes){
  wchar_t w;
  mbstate_t mbstate = { };
  size_t r = mbrtowc(&w, egc, *bytes, &mbstate);
  if(r == (size_t)-1 || r == (size_t)-2){
    return NULL;
  }
  const int bidic = uc_bidi_category(w);
//fprintf(stderr, "BIDI CAT (%lc): %d\n", w, bidic);
  if(bidic != UC_BIDI_R && bidic != UC_BIDI_AL && bidic != UC_BIDI_RLE && bidic != UC_BIDI_RLO){
    return NULL;
  }
  // insert U+200E, "LEFT-TO-RIGHT MARK". This ought reset the text direction
  // after emitting a potentially RTL EGC.
  const char LTRMARK[] = "\xe2\x80\x8e";
  char* s = (char*)malloc(*bytes + sizeof(LTRMARK)); // cast for C++ callers
  memcpy(s, egc, *bytes);
  memcpy(s + *bytes, LTRMARK, sizeof(LTRMARK));
  *bytes += strlen(LTRMARK);
  return s;
}

// a sprixel occupies the entirety of its associated plane. each cell contains
// a reference to the context-wide sprixel cache. this ought be an entirely
// new, purpose-specific plane.
static inline int
plane_blit_sixel(ncplane* n, const char* s, int bytes, int placey, int placex,
                 int leny, int lenx, int sprixelid, int dimy, int dimx){
  sprixel* spx = sprixel_create(n, s, bytes, placey, placex, sprixelid, leny, lenx, dimy, dimx);
  if(spx == NULL){
    return -1;
  }
  uint32_t gcluster = htole(0x02000000ul) + htole(spx->id);
  for(int y = placey ; y < placey + leny && y < ncplane_dim_y(n) ; ++y){
    for(int x = placex ; x < placex + lenx && x < ncplane_dim_x(n) ; ++x){
      nccell* c = ncplane_cell_ref_yx(n, y, x);
      memcpy(&c->gcluster, &gcluster, sizeof(gcluster));
      c->width = lenx;
    }
  }
  if(n->sprite){
    sprixel_hide(n->sprite);
  }
  n->sprite = spx;
  return 0;
}

// lowest level of cell+pool setup. if the EGC changes the output to RTL, it
// must be suffixed with a LTR-forcing character by now. The four bits of
// CELL_BLITTERSTACK_MASK ought already be initialized. If gcluster is four
// bytes or fewer, this function cannot fail.
static inline int
pool_blit_direct(egcpool* pool, nccell* c, const char* gcluster, int bytes, int cols){
  pool_release(pool, c);
  if(bytes < 0 || cols < 0){
    return -1;
  }
  c->width = cols;
  if(bytes <= 4){
    c->gcluster = 0;
    memcpy(&c->gcluster, gcluster, bytes);
  }else{
    int eoffset = egcpool_stash(pool, gcluster, bytes);
    if(eoffset < 0){
      return -1;
    }
    set_gcluster_egc(c, eoffset);
  }
  return bytes;
}

// Do an RTL-check, reset the quadrant occupancy bits, and pass the cell down to
// pool_blit_direct().
static inline int
pool_load_direct(egcpool* pool, nccell* c, const char* gcluster, int bytes, int cols){
  char* rtl = NULL;
  c->channels &= ~CELL_NOBACKGROUND_MASK;
  if(bytes >= 0){
    rtl = egc_rtl(gcluster, &bytes); // checks for RTL and adds U+200E if so
  }
  if(rtl){
    gcluster = rtl;
  }
  int r = pool_blit_direct(pool, c, gcluster, bytes, cols);
  free(rtl);
  return r;
}

static inline int
cell_load_direct(ncplane* n, nccell* c, const char* gcluster, int bytes, int cols){
  return pool_load_direct(&n->pool, c, gcluster, bytes, cols);
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

// the heart of damage detection. compare two nccells (from two different
// planes) for equality. if they are equal, return 0. otherwise, dup the second
// onto the first and return non-zero.
static inline int
cellcmp_and_dupfar(egcpool* dampool, nccell* damcell,
                   const ncplane* srcplane, const nccell* srccell){
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

int set_fd_nonblocking(int fd, unsigned state, unsigned* oldstate);

// Given the four channels arguments, verify that:
//
// - if any is default foreground, all are default foreground
// - if any is default background, all are default background
// - all foregrounds must have the same alpha
// - all backgrounds must have the same alpha
// - palette-indexed color must not be used
//
// If you only want to check n < 4 channels, just duplicate one.
bool check_gradient_args(uint64_t ul, uint64_t ur, uint64_t bl, uint64_t br);

int setup_signals(void* nc, bool no_quit_sigs, bool no_winch_sig,
                  int(*handler)(void*));
int drop_signals(void* nc);

void ncvisual_printbanner(const notcurses* nc);

// alpha comes to us 0--255, but we have only 3 alpha values to map them to.
// settled on experimentally.
static inline bool
rgba_trans_p(unsigned alpha){
  if(alpha < 192){
    return true;
  }
  return false;
}

// get a non-negative "manhattan distance" between two rgb values
static inline uint32_t
rgb_diff(unsigned r1, unsigned g1, unsigned b1, unsigned r2, unsigned g2, unsigned b2){
  uint32_t distance = 0;
  distance += r1 > r2 ? r1 - r2 : r2 - r1;
  distance += g1 > g2 ? g1 - g2 : g2 - g1;
  distance += b1 > b2 ? b1 - b2 : b2 - b1;
//fprintf(stderr, "RGBDIFF %u %u %u %u %u %u: %u\n", r1, g1, b1, r2, g2, b2, distance);
  return distance;
}

static inline uint64_t
ncdirect_channels(const ncdirect* nc){
  return nc->channels;
}

static inline bool
ncdirect_fg_default_p(const struct ncdirect* nc){
  return channels_fg_default_p(ncdirect_channels(nc));
}

static inline bool
ncdirect_bg_default_p(const struct ncdirect* nc){
  return channels_bg_default_p(ncdirect_channels(nc));
}

int sprite_sixel_cell_wipe(const notcurses* nc, sprixel* s, int y, int x);
int sprite_kitty_cell_wipe(const notcurses* nc, sprixel* s, int y, int x);

int sixel_blit(ncplane* nc, int linesize, const void* data, int begy, int begx,
               int leny, int lenx, const blitterargs* bargs);

int kitty_blit(ncplane* nc, int linesize, const void* data, int begy, int begx,
               int leny, int lenx, const blitterargs* bargs);

int term_fg_rgb8(bool RGBflag, const char* setaf, int colors, FILE* out,
                 unsigned r, unsigned g, unsigned b);

API const struct blitset* lookup_blitset(const tinfo* tcache, ncblitter_e setid, bool may_degrade);

static inline int
rgba_blit_dispatch(ncplane* nc, const struct blitset* bset,
                   int linesize, const void* data, int begy,
                   int begx, int leny, int lenx, const blitterargs* bargs){
  return bset->blit(nc, linesize, data, begy, begx, leny, lenx, bargs);
}

static inline const struct blitset*
rgba_blitter_low(const tinfo* tcache, ncscale_e scale, bool maydegrade,
                 ncblitter_e blitrec) {
  if(blitrec == NCBLIT_DEFAULT){
    blitrec = rgba_blitter_default(tcache, scale);
  }
  return lookup_blitset(tcache, blitrec, maydegrade);
}

// RGBA visuals all use NCBLIT_2x1 by default (or NCBLIT_1x1 if not in
// UTF-8 mode), but an alternative can be specified.
static inline const struct blitset*
rgba_blitter(const struct notcurses* nc, const struct ncvisual_options* opts) {
  const bool maydegrade = !(opts && (opts->flags & NCVISUAL_OPTION_NODEGRADE));
  const ncscale_e scale = opts ? opts->scaling : NCSCALE_NONE;
  return rgba_blitter_low(&nc->tcache, scale, maydegrade, opts ? opts->blitter : NCBLIT_DEFAULT);
}

typedef struct ncvisual_implementation {
  int (*visual_init)(int loglevel);
  void (*visual_printbanner)(const struct notcurses* nc);
  int (*visual_blit)(struct ncvisual* ncv, int rows, int cols, ncplane* n,
                     const struct blitset* bset, int begy, int begx,
                     int leny, int lenx, const blitterargs* barg);
  struct ncvisual* (*visual_create)(void);
  struct ncvisual* (*visual_from_file)(const char* fname);
  // ncv constructors other than ncvisual_from_file() need to set up the
  // AVFrame* 'frame' according to their own data, which is assumed to
  // have been prepared already in 'ncv'.
  void (*visual_details_seed)(struct ncvisual* ncv);
  int (*visual_decode)(struct ncvisual* nc);
  int (*visual_decode_loop)(struct ncvisual* nc);
  int (*visual_stream)(notcurses* nc, struct ncvisual* ncv, float timescale,
                       streamcb streamer, const struct ncvisual_options* vopts, void* curry);
  char* (*visual_subtitle)(const struct ncvisual* ncv);
  int (*visual_resize)(struct ncvisual* ncv, int rows, int cols);
  void (*visual_destroy)(struct ncvisual* ncv);
  bool canopen_images;
  bool canopen_videos;
} ncvisual_implementation;

// assigned by libnotcurses.so if linked with multimedia
API extern const ncvisual_implementation* visual_implementation;

#undef ALLOC
#undef API

#ifdef __cplusplus
}
#endif

#endif
