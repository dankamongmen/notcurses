#ifndef NOTCURSES_INTERNAL
#define NOTCURSES_INTERNAL

#ifdef __cplusplus
extern "C" {
#endif

#include "version.h"

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
#include <langinfo.h>
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
  int absx, absy;        // origin of the plane relative to the screen
  int lenx, leny;        // size of the plane, [0..len{x,y}) is addressable
  struct ncplane* above; // plane above us, NULL if we're on top
  struct ncplane* below; // plane below us, NULL if we're on bottom
  struct ncplane* bnext; // next in the bound list of plane to which we are bound
  struct ncplane** bprev;// link to us iff we're bound, NULL otherwise
  struct ncplane* blist; // head of list of bound planes
  struct ncplane* boundto; // plane to which we are bound, if any
  egcpool pool;          // attached storage pool for UTF-8 EGCs
  uint64_t channels;     // works the same way as cells
  uint32_t attrword;     // same deal as in a cell
  void* userptr;         // slot for the user to stick some opaque pointer
  cell basecell;         // cell written anywhere that fb[i].gcluster == 0
  struct notcurses* nc;  // notcurses object of which we are a part
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
  // modified by: output, cursor moves, clearing the screen (during refresh)
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

// ncmenu_item and ncmenu_section have internal and (minimal) external forms
typedef struct ncmenu_int_item {
  char* desc;           // utf-8 menu item, NULL for horizontal separator
  ncinput shortcut;     // shortcut, all should be distinct
  int shortcut_offset;  // column offset with desc of shortcut EGC
  char* shortdesc;      // description of shortcut, can be NULL
  int shortdesccols;    // columns occupied by shortcut description
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
} ncreader;

typedef struct ncmenu {
  ncplane* ncp;
  int sectioncount;         // must be positive
  ncmenu_int_section* sections; // NULL iff sectioncount == 0
  int unrolledsection;      // currently unrolled section, -1 if none
  int headerwidth;          // minimum space necessary to display all sections
  uint64_t headerchannels;  // styling for header
  uint64_t sectionchannels; // styling for sections
  bool bottom;              // are we on the bottom (vs top)?
} ncmenu;

typedef struct ncselector {
  ncplane* ncp;                  // backing ncplane
  unsigned selected;             // index of selection
  unsigned startdisp;            // index of first option displayed
  unsigned maxdisplay;           // max number of items to display, 0 -> no limit
  int longop;                    // columns occupied by longest option
  int longdesc;                  // columns occupied by longest description
  struct ncselector_item* items; // list of items and descriptions, heap-copied
  unsigned itemcount;            // number of pairs in 'items'
  char* title;                   // can be NULL, in which case there's no riser
  int titlecols;                 // columns occupied by title
  char* secondary;               // can be NULL
  int secondarycols;             // columns occupied by secondary
  char* footer;                  // can be NULL
  int footercols;                // columns occupied by footer
  cell background;               // background, used in body only
  uint64_t opchannels;           // option channels
  uint64_t descchannels;         // description channels
  uint64_t titlechannels;        // title channels
  uint64_t footchannels;         // secondary and footer channels
  uint64_t boxchannels;          // border channels
  int uarrowy, darrowy, arrowx;// location of scrollarrows, even if not present
} ncselector;

typedef struct ncmultiselector {
  ncplane* ncp;                   // backing ncplane
  unsigned current;               // index of highlighted item
  unsigned startdisp;             // index of first option displayed
  unsigned maxdisplay;            // max number of items to display, 0 -> no limit
  int longitem;                   // columns occupied by longest item
  struct ncmselector_item* items; // items, descriptions, and statuses, heap-copied
  unsigned itemcount;             // number of pairs in 'items'
  char* title;                    // can be NULL, in which case there's no riser
  int titlecols;                  // columns occupied by title
  char* secondary;                // can be NULL
  int secondarycols;              // columns occupied by secondary
  char* footer;                   // can be NULL
  int footercols;                 // columns occupied by footer
  cell background;                // background, used in body only
  uint64_t opchannels;            // option channels
  uint64_t descchannels;          // description channels
  uint64_t titlechannels;         // title channels
  uint64_t footchannels;          // secondary and footer channels
  uint64_t boxchannels;           // border channels
  int uarrowy, darrowy, arrowx;   // location of scrollarrows, even if not present
} ncmultiselector;

// terminfo cache
typedef struct tinfo {
  int colors;     // number of colors terminfo reported usable for this screen
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
  bool RGBflag;   // ti-reported "RGB" flag for 24bpc truecolor
  bool CCCflag;   // ti-reported "CCC" flag for palette set capability
  bool AMflag;    // ti-reported "AM" flag for automatic movement to next line
  char* smcup;    // enter alternate mode
  char* rmcup;    // restore primary mode
} tinfo;

typedef struct ncdirect {
  int attrword;              // current styles
  palette256 palette;        // 256-indexed palette can be used instead of/with RGB
  FILE* ttyfp;               // FILE* for output tty
  int ctermfd;               // fd for controlling terminal
  tinfo tcache;              // terminfo cache
  unsigned fgrgb, bgrgb;     // last RGB values of foreground/background
  bool fgdefault, bgdefault; // are FG/BG currently using default colors?
  bool utf8;                 // are we using utf-8 encoding, as hoped?
} ncdirect;

typedef struct notcurses {
  ncplane* top;   // topmost plane, never NULL
  ncplane* bottom;// bottommost plane, never NULL 
  ncplane* stdscr;// aliases some plane from the z-buffer, covers screen

  // the style state of the terminal is carried across render runs
  renderstate rstate;

  // we keep a copy of the last rendered frame. this facilitates O(1)
  // notcurses_at_yx() and O(1) damage detection (at the cost of some memory).
  cell* lastframe;// last rendered framebuffer, NULL until first render
  int lfdimx;     // dimensions of lastframe, unchanged by screen resize
  int lfdimy;     // lfdimx/lfdimy are 0 until first render
  egcpool pool;   // duplicate EGCs into this pool

  ncstats stats;  // some statistics across the lifetime of the notcurses ctx
  ncstats stashstats; // cumulative stats, unaffected by notcurses_reset_stats()

  int truecols;   // true number of columns in the physical rendering area.
                  // used only to see if output motion takes us to the next
                  // line thanks to terminal action alone.

  tinfo tcache;   // terminfo cache

  FILE* ttyfp;    // FILE* for controlling tty
  int ttyfd;      // file descriptor for controlling tty
  FILE* ttyinfp;  // FILE* for processing input
  FILE* renderfp; // debugging FILE* to which renderings are written
  struct termios tpreserved; // terminal state upon entry
  bool suppress_banner; // from notcurses_options
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

  // desired margins (best-effort only), copied in from notcurses_options
  int margin_t, margin_b, margin_r, margin_l;
  int loglevel;
  palette256 palette; // 256-indexed palette can be used instead of/with RGB
  bool palette_damage[NCPALETTESIZE];
  struct esctrie* inputescapes; // trie of input escapes -> ncspecial_keys
  bool utf8;      // are we using utf-8 encoding, as hoped?
  bool libsixel;  // do we have Sixel support?
} notcurses;

void sigwinch_handler(int signo);

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
  return notcurses_stdplane(n->nc);
}

static inline const ncplane*
ncplane_stdplane_const(const ncplane* n){
  return notcurses_stdplane_const(n->nc);
}

// load all known special keys from terminfo, and build the input sequence trie
int prep_special_keys(notcurses* nc);

// free up the input escapes trie
void input_free_esctrie(struct esctrie** trie);

// initialize libav
int ncvisual_init(int loglevel);

static inline int
fbcellidx(int row, int rowlen, int col){
  return row * rowlen + col;
}

// take a logical 'y' and convert it to the virtual 'y'. see HACKING.md.
static inline int
logical_to_virtual(const ncplane* n, int y){
  return (y + n->logrow) % n->leny;
}

static inline int
nfbcellidx(const ncplane* n, int row, int col){
  return fbcellidx(logical_to_virtual(n, row), n->lenx, col);
}

// copy the UTF8-encoded EGC out of the cell, whether simple or complex. the
// result is not tied to the ncplane, and persists across erases / destruction.
static inline char*
pool_egc_copy(const egcpool* e, const cell* c){
  char* ret;
  if(cell_simple_p(c)){
    if( (ret = (char*)malloc(2)) ){
      ret[0] = c->gcluster;
      ret[1] = '\0';
    }
  }else{
    ret = strdup(egcpool_extended_gcluster(e, c));
  }
  return ret;
}

// copy the UTF8-encoded EGC out of the cell, whether simple or complex. the
// result is not tied to the ncplane, and persists across erases / destruction.
static inline char*
cell_egc_copy(const ncplane* n, const cell* c){
  return pool_egc_copy(&n->pool, c);
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
  ssize_t ret = write(fd, seq, strlen(seq));
  if(ret < 0){
//fprintf(stderr, "Error emitting %zub %s escape (%s)\n", strlen(seq), name, strerror(errno));
    return -1;
  }
  if((size_t)ret != strlen(seq)){
//fprintf(stderr, "Short write (%db) for %zub %s sequence\n", ret, strlen(seq), name);
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
  return term_emit("setab", tiparm(nc->tcache.setab, pal), out, false);
}

static inline int
term_fg_palindex(const notcurses* nc, FILE* out, unsigned pal){
  return term_emit("setaf", tiparm(nc->tcache.setaf, pal), out, false);
}

static inline const char*
extended_gcluster(const ncplane* n, const cell* c){
  return egcpool_extended_gcluster(&n->pool, c);
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
	if(cell_simple_p(c)){
		fprintf(stderr, "gcluster: %u %c attr: 0x%08x chan: 0x%016jx\n",
				    c->gcluster, c->gcluster, c->attrword, c->channels);
	}else{
		fprintf(stderr, "gcluster: %u %s attr: 0x%08x chan: 0x%016jx\n",
				    c->gcluster, egcpool_extended_gcluster(p, c), c->attrword, c->channels);
	}
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

// True if the cell does not generate background pixels. Only the FULL BLOCK
// glyph has this property, AFAIK.
// FIXME set a bit, doing this at load time
static inline bool
cell_nobackground_p(const egcpool* e, const cell* c){
  return !cell_simple_p(c) && !strcmp(egcpool_extended_gcluster(e, c), "\xe2\x96\x88");
}

static inline void
pool_release(egcpool* pool, cell* c){
  if(!cell_simple_p(c)){
    egcpool_release(pool, cell_egc_idx(c));
  }
  c->gcluster = 0; // don't subject ourselves to double-release problems
}

// Duplicate one cell onto another, possibly crossing ncplanes.
static inline int
cell_duplicate_far(egcpool* tpool, cell* targ, const ncplane* splane, const cell* c){
  pool_release(tpool, targ);
  targ->attrword = c->attrword;
  targ->channels = c->channels;
  if(cell_simple_p(c)){
    targ->gcluster = c->gcluster;
    return !!c->gcluster;
  }
  assert(splane);
  const char* egc = extended_gcluster(splane, c);
  size_t ulen = strlen(egc);
  int eoffset = egcpool_stash(tpool, egc, ulen);
  if(eoffset < 0){
    return -1;
  }
  targ->gcluster = eoffset + 0x80;
  return ulen;
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
  channel_set_rgb_clipped(&chan,
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
ncplane* ncplane_create(notcurses* nc, ncplane* n, int rows, int cols,
                        int yoff, int xoff, void* opaque);
void free_plane(ncplane* p);

// heap-allocated formatted output
char* ncplane_vprintf_prep(const char* format, va_list ap);

// Resize the provided ncviusal to the specified 'rows' x 'cols', but do not
// change the internals of the ncvisual. Uses oframe.
nc_err_e ncvisual_blit(struct ncvisual* ncv, int rows, int cols,
                       ncplane* n, const struct blitset* bset,
                       int placey, int placex, int begy, int begx,
                       int leny, int lenx, bool blendcolors);

void nclog(const char* fmt, ...);

// get a file descriptor for the controlling tty device, -1 on error
int get_controlling_tty(void);

// logging
#define logerror(nc, fmt, ...) do{ \
  if((nc)->loglevel >= NCLOGLEVEL_ERROR){ \
    nclog("%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__); } }while(0);

#define logwarning(nc, fmt, ...) do{ \
  if((nc)->loglevel >= NCLOGLEVEL_WARNING){ \
    nclog("%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__); } }while(0);

#define loginfo(nc, fmt, ...) do{ \
  if((nc)->loglevel >= NCLOGLEVEL_INFO){ \
    nclog("%s:%d:" fmt, __func__, __LINE__, ##__VA_ARGS__); } }while(0);

#ifdef __cplusplus
}
#endif

#endif
