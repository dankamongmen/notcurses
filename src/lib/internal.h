#ifndef NOTCURSES_INTERNAL
#define NOTCURSES_INTERNAL

#include <term.h>
#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include "notcurses.h"
#include "version.h"
#include "egcpool.h"

#ifndef DISABLE_FFMPEG
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/pixdesc.h>
#include <libavutil/version.h>
#include <libswscale/version.h>
#include <libavformat/version.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

struct AVFormatContext;
struct AVCodecContext;
struct AVFrame;
struct AVCodec;
struct AVCodecParameters;
struct AVPacket;
struct SwsContext;
struct esctrie;

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
typedef struct ncplane {
  cell* fb;             // "framebuffer" of character cells
  int x, y;             // current location within this plane
  int absx, absy;       // origin of the plane relative to the screen
  int lenx, leny;       // size of the plane, [0..len{x,y}) is addressable
  struct ncplane* z;    // plane below us
  egcpool pool;         // attached storage pool for UTF-8 EGCs
  uint64_t channels;    // works the same way as cells
  uint32_t attrword;    // same deal as in a cell
  void* userptr;        // slot for the user to stick some opaque pointer
  cell basecell;        // cell written anywhere that fb[i].gcluster == 0
  struct notcurses* nc; // notcurses object of which we are a part
} ncplane;

typedef struct ncvisual {
  struct AVFormatContext* fmtctx;
  struct AVCodecContext* codecctx;
  struct AVFrame* frame;
  struct AVFrame* oframe;
  struct AVCodec* codec;
  struct AVCodec* subtcodec;
  struct AVCodecParameters* cparams;
  struct AVPacket* packet;
  struct AVPacket* subtitle;
  struct SwsContext* swsctx;
  int packet_outstanding;
  int dstwidth, dstheight;
  int stream_index;        // match against this following av_read_frame()
  float timescale;         // scale frame duration by this value
  ncplane* ncp;
  // if we're creating the plane based off the first frame's dimensions, these
  // describe where the plane ought be placed, and how it ought be sized. this
  // path sets ncobj. ncvisual_destroy() ought in that case kill the ncplane.
  int placex, placey;
  ncscale_e style;         // none, scale, or stretch
  struct notcurses* ncobj; // set iff this ncvisual "owns" its ncplane
} ncvisual;

// current presentation state of the terminal. it is carried across render
// instances. initialize everything to 0 on a terminal reset / startup.
typedef struct renderstate {
  // we assemble the encoded output in a POSIX memstream, and keep it around
  // between uses. this could be a problem if it ever tremendously spiked, but
  // that's a highly unlikely situation.
  char* mstream;  // buffer for rendering memstream, see open_memstream(3)
  FILE* mstreamfp;// FILE* for rendering memstream
  size_t mstrsize;// size of rendering memstream

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

typedef struct notcurses {
  pthread_mutex_t lock;
  ncplane* top;   // the contents of our topmost plane (initially entire screen)
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

  // We verify that some terminfo capabilities exist. These needn't be checked
  // before further use; just use tiparm() directly.
  int colors;     // number of colors terminfo reported usable for this screen
  char* cup;      // move cursor
  char* cuf;      // move n cells right
  char* cuf1;     // move 1 cell right
  char* cub1;     // move 1 cell left
  char* civis;    // hide cursor
  // These might be NULL, and we can more or less work without them. Check!
  char* clearscr; // erase screen and home cursor
  char* cleareol; // clear to end of line
  char* clearbol; // clear to beginning of line
  char* cnorm;    // restore cursor to default state
  char* sgr;      // set many graphics properties at once
  char* sgr0;     // restore default presentation properties
  char* smcup;    // enter alternate mode
  char* rmcup;    // restore primary mode
  char* setaf;    // set foreground color (ANSI)
  char* setab;    // set background color (ANSI)
  char* op;       // set foreground and background color to default
  char* standout; // CELL_STYLE_STANDOUT
  char* uline;    // CELL_STYLE_UNDERLINK
  char* reverse;  // CELL_STYLE_REVERSE
  char* blink;    // CELL_STYLE_BLINK
  char* dim;      // CELL_STYLE_DIM
  char* bold;     // CELL_STYLE_BOLD
  char* italics;  // CELL_STYLE_ITALIC
  char* italoff;  // CELL_STYLE_ITALIC (disable)
  char* smkx;     // enter keypad transmit mode (keypad_xmit)
  char* rmkx;     // leave keypad transmit mode (keypad_local)
  char* getm;     // get mouse events
  char* initc;    // set a palette entry's RGB value
  char* oc;       // restore original colors
  bool RGBflag;   // terminfo-reported "RGB" flag for 24bpc directcolor
  bool CCCflag;   // terminfo-reported "CCC" flag for palette set capability

  int ttyfd;      // file descriptor for controlling tty, from opts->ttyfp
  FILE* ttyfp;    // FILE* for controlling tty, from opts->ttyfp
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
  palette256 palette; // 256-indexed palette can be used instead of/with RGB
  bool palette_damage[NCPALETTESIZE];
  struct esctrie* inputescapes; // trie of input escapes -> ncspecial_keys
} notcurses;

void sigwinch_handler(int signo);

// load all known special keys from terminfo, and build the input sequence trie
int prep_special_keys(notcurses* nc);

// free up the input escapes trie
void input_free_esctrie(struct esctrie** trie);

// initialize libav
int ncvisual_init(int loglevel);

static inline void
ncplane_lock(const ncplane* n){
  pthread_mutex_lock(&n->nc->lock);
}

static inline void
ncplane_unlock(const ncplane* n){
  pthread_mutex_unlock(&n->nc->lock);
}

static inline int
fbcellidx(int row, int rowlen, int col){
  return row * rowlen + col;
}

static inline int
nfbcellidx(const ncplane* n, int row, int col){
  return fbcellidx(row, n->lenx, col);
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
// otherwise, c / 42.7 to map to 6 values. this never generates pure black
// nor white, though, lame...FIXME
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

static inline int
term_emit(const char* name __attribute__ ((unused)), const char* seq,
          FILE* out, bool flush){
  if(!seq){
    return -1;
  }
  int ret = fprintf(out, "%s", seq);
  if(ret < 0){
// fprintf(stderr, "Error emitting %zub %s escape (%s)\n", strlen(seq), name, strerror(errno));
    return -1;
  }
  if((size_t)ret != strlen(seq)){
// fprintf(stderr, "Short write (%db) for %zub %s sequence\n", ret, strlen(seq), name);
    return -1;
  }
  if(flush && fflush(out)){
// fprintf(stderr, "Error flushing after %db %s sequence (%s)\n", ret, name, strerror(errno));
    return -1;
  }
  return 0;
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
		fprintf(stderr, "gcluster: %u %c attr: 0x%08x chan: 0x%016lx\n",
				    c->gcluster, c->gcluster, c->attrword, c->channels);
	}else{
		fprintf(stderr, "gcluster: %u %s attr: 0x%08x chan: 0x%016lx\n",
				    c->gcluster, egcpool_extended_gcluster(p, c), c->attrword, c->channels);
	}
}

// True if the cell does not generate background pixels. Only the FULL BLOCK
// glyph has this property, AFAIK.
// FIXME set a bit, doing this at load time
static inline bool
cell_nobackground_p(const egcpool* e, const cell* c){
  return !cell_simple_p(c) && !strcmp(egcpool_extended_gcluster(e, c), "\xe2\x96\x88");
}

// no CLOCK_MONOTONIC_RAW on FreeBSD as of 12.0 :/
#ifndef CLOCK_MONOTONIC_RAW
#define CLOCK_MONOTONIC_RAW CLOCK_MONOTONIC
#endif

#ifdef __cplusplus
}
#endif

#endif
