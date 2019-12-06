#ifndef NOTCURSES_INTERNAL
#define NOTCURSES_INTERNAL

#include <term.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <pthread.h>
#include "notcurses.h"
#include "egcpool.h"

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
  cell background;      // cell written anywhere that fb[i].gcluster == 0
  struct notcurses* nc; // notcurses object of which we are a part
} ncplane;

typedef struct ncvisual {
  struct AVFormatContext* fmtctx;
  struct AVCodecContext* codecctx;
  struct AVFrame* frame;
  struct AVFrame* oframe;
  struct AVCodec* codec;
  struct AVCodecParameters* cparams;
  struct AVPacket* packet;
  struct SwsContext* swsctx;
  int packet_outstanding;
  int dstwidth, dstheight;
  int stream_index;     // match against this following av_read_frame()
  ncplane* ncp;
} ncvisual;

typedef struct notcurses {
  pthread_mutex_t lock;
  int ttyfd;      // file descriptor for controlling tty, from opts->ttyfp
  FILE* ttyfp;    // FILE* for controlling tty, from opts->ttyfp
  FILE* ttyinfp;  // FILE* for processing input
  int colors;     // number of colors usable for this screen
  ncstats stats;  // some statistics across the lifetime of the notcurses ctx
  // We verify that some terminfo capabilities exist. These needn't be checked
  // before further use; just use tiparm() directly.
  char* cup;      // move cursor
  char* civis;    // hide cursor
  // These might be NULL, and we can more or less work without them. Check!
  char* clearscr; // erase screen and home cursor
  char* cleareol; // clear to end of line
  char* clearbol; // clear to beginning of line
  char* cnorm;    // restore cursor to default state
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

  struct termios tpreserved; // terminal state upon entry
  bool RGBflag;   // terminfo-reported "RGB" flag for 24bpc directcolor
  bool CCCflag;   // terminfo-reported "CCC" flag for palette set capability
  ncplane* top;   // the contents of our topmost plane (initially entire screen)
  ncplane* stdscr;// aliases some plane from the z-buffer, covers screen
  FILE* renderfp; // debugging FILE* to which renderings are written
  unsigned char inputbuf[BUFSIZ];
  // we keep a wee ringbuffer of input queued up for delivery. if
  // inputbuf_occupied == sizeof(inputbuf), there is no room. otherwise, data
  // can be read to inputbuf_write_at until we fill up. the first datum
  // available for the app is at inputbuf_valid_starts iff inputbuf_occupied is
  // not 0. the main purpose is working around bad predictions of escapes.
  unsigned inputbuf_occupied;
  unsigned inputbuf_valid_starts;
  unsigned inputbuf_write_at;
  struct esctrie* inputescapes; // trie of input escapes -> ncspecial_keys
} notcurses;

extern sig_atomic_t resize_seen;

// load all known special keys from terminfo, and build the input sequence trie
int prep_special_keys(notcurses* nc);

// free up the input escapes trie
void input_free_esctrie(struct esctrie** trie);

#ifdef __cplusplus
}
#endif

#endif
