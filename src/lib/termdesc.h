#ifndef NOTCURSES_TERM
#define NOTCURSES_TERM

#ifdef __cplusplus
extern "C" {
#endif

// internal header, not installed

#include <stdbool.h>

struct ncpile;
struct sprixel;
struct notcurses;

// we store all our escape sequences in a single large block, and use
// 16-bit one-biased byte-granularity indices to get the location in said
// block. we'd otherwise be using 32 or 64-bit pointers to get locations
// scattered all over memory. this way the lookup elements require two or four
// times fewer cachelines total, and the actual escape sequences are packed
// tightly into minimal cachelines. if an escape is not defined, that index
// is 0. the first escape defined has an index of 1, and so on. an escape
// thus cannot actually start at byte 65535.

// indexes into the table of fixed-width (16-bit) indices
typedef enum {
  ESCAPE_CUP,      // "cup" move cursor to absolute x, y position
  ESCAPE_HPA,      // "hpa" move cursor to absolute horizontal position
  ESCAPE_VPA,      // "vpa" move cursor to absolute vertical position
  ESCAPE_SETAF,    // "setaf" set foreground color
  ESCAPE_SETAB,    // "setab" set background color
  ESCAPE_OP,       // "op" set foreground and background color to defaults
  ESCAPE_FGOP,     // set foreground only to default
  ESCAPE_BGOP,     // set background only to default
  ESCAPE_SGR,      // "sgr" set graphics rendering (styles)
  ESCAPE_SGR0,     // "sgr0" turn off all styles
  ESCAPE_CIVIS,    // "civis" make the cursor invisiable
  ESCAPE_CNORM,    // "cnorm" restore the cursor to normal
  ESCAPE_OC,       // "oc" restore original colors
  ESCAPE_SITM,     // "sitm" start italics
  ESCAPE_RITM,     // "ritm" end italics
  ESCAPE_CUU,      // "cuu" move n cells up
  ESCAPE_CUB,      // "cub" move n cells back (left)
  ESCAPE_CUF,      // "cuf" move n cells forward (right)
  ESCAPE_CUD,      // "cud" move n cells down
  ESCAPE_CUF1,     // "cuf1" move 1 cell forward (right)
  ESCAPE_SMKX,     // "smkx" keypad_xmit (keypad transmit mode)
  ESCAPE_RMKX,     // "rmkx" keypad_local
  ESCAPE_SMCUP,    // "smcup" enter alternate screen
  ESCAPE_RMCUP,    // "rmcup" leave alternate screen
  ESCAPE_SMXX,     // "smxx" start struckout
  ESCAPE_SMUL,     // "smul" start underline
  ESCAPE_RMUL,     // "rmul" end underline
  ESCAPE_SMULX,    // "Smulx" deparameterized: start extended underline
  ESCAPE_SMULNOX,  // "Smulx" deparameterized: kill underline
  ESCAPE_RMXX,     // "rmxx" end struckout
  ESCAPE_SC,       // "sc" push the cursor onto the stack
  ESCAPE_RC,       // "rc" pop the cursor off the stack
  ESCAPE_CLEAR,    // "clear" clear screen and home cursor
  ESCAPE_HOME,     // "home" home cursor
  ESCAPE_INITC,    // "initc" set up palette entry
  ESCAPE_GETM,     // "getm" get mouse events
  ESCAPE_DSRCPR,   // "u7" cursor position report
  // Application synchronized updates, not present in terminfo
  // (https://gitlab.com/gnachman/iterm2/-/wikis/synchronized-updates-spec)
  ESCAPE_BSU,      // Begin Synchronized Update
  ESCAPE_ESU,      // End Synchronized Update
  ESCAPE_MAX
} escape_e;

// we read input from one or two places. if stdin is connected to our
// controlling tty, we read only from that file descriptor. if it is
// connected to something else, and we have a controlling tty, we will
// read data only from stdin and control only from the tty. if we have
// no connected tty, only data is available.
typedef struct ncinputlayer {
  // ttyfd is only valid if we are connected to a tty, *and* stdin is not
  // connected to that tty. in that case, we read control sequences only
  // from ttyfd.
  int ttyfd; // file descriptor for connected tty
  int infd;  // file descriptor for processing input, from stdin
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

// terminal interface description. most of these are acquired from terminfo(5)
// (using a database entry specified by TERM). some are determined via
// heuristics based off terminal interrogation or the TERM environment
// variable. some are determined via ioctl(2). treat all of them as if they
// can change over the program's life (don't cache them locally).
typedef struct tinfo {
  uint16_t escindices[ESCAPE_MAX]; // table of 1-biased indices into esctable
  char* esctable;                  // packed table of escape sequences
  nccapabilities caps;             // exported to the user, when requested
  // we use the cell's size in pixels for pixel blitting. this information can
  // be acquired on all terminals with pixel support.
  int cellpixy;   // cell pixel height, might be 0
  int cellpixx;   // cell pixel width, might be 0

  unsigned supported_styles; // bitmask over NCSTYLE_* driven via sgr/ncv

  // kitty interprets an RGB background that matches the default background
  // color *as* the default background, meaning it'll be translucent if
  // background_opaque is in use. detect this, and avoid the default if so.
  // bg_collides_default is either 0x0000000 or (if in use) 0x1RRGGBB.
  uint32_t bg_collides_default;

  // sprixel support. there are several different sprixel protocols, of
  // which we support sixel and kitty. the kitty protocol is used based
  // on TERM heuristics. otherwise, we attempt to detect sixel support, and
  // query the details of the implementation.
  int color_registers; // sixel color registers (post pixel_query_done)
  int sixel_maxx;      // maximum theoretical sixel width
  // in sixel, we can't render to the bottom row, lest we force a one-line
  // scroll. we thus clamp sixel_maxy_pristine to the minimum of
  // sixel_maxy_pristine (the reported sixel_maxy), and the number of rows
  // less one times the cell height. sixel_maxy is thus recomputed whenever
  // we get a resize event. it is only defined if we have sixel_maxy_pristine,
  // so kitty graphics (which don't force a scroll) never deal with this.
  int sixel_maxy;          // maximum working sixel height
  int sixel_maxy_pristine; // maximum theoretical sixel height, as queried
  int (*pixel_destroy)(const struct notcurses* nc, const struct ncpile* p, FILE* out, struct sprixel* s);
  // wipe out a cell's worth of pixels from within a sprixel. for sixel, this
  // means leaving out the pixels (and likely resizes the string). for kitty,
  // this means dialing down their alpha to 0 (in equivalent space).
  int (*pixel_wipe)(struct sprixel* s, int y, int x);
  // perform the inverse of pixel_wipe, restoring an annihilated sprixcell.
  int (*pixel_rebuild)(struct sprixel* s, int y, int x, uint8_t* auxvec);
  int (*pixel_remove)(int id, FILE* out); // kitty only, issue actual delete command
  int (*pixel_init)(const struct tinfo*, int fd); // called when support is detected
  int (*pixel_draw)(const struct ncpile* p, struct sprixel* s, FILE* out);
  int (*pixel_shutdown)(int fd);  // called during context shutdown
  int (*pixel_clear_all)(int fd); // called during startup, kitty only
  int sprixel_scale_height; // sprixel must be a multiple of this many rows
  const char* termname;     // terminal name from environment variables/init
  struct termios tpreserved; // terminal state upon entry
  ncinputlayer input;       // input layer
  bool bitmap_supported;    // do we support bitmaps (post pixel_query_done)?

  // mlterm resets the cursor (i.e. makes it visible) any time you print
  // a sprixel. we work around this spiritedly unorthodox decision. it
  // furthermore interprets DECSDM in the reverse sense of most terminals
  // (though possibly in conformance with the actual VT340).
  bool sprixel_cursor_hack; // mlterm fixes
} tinfo;

// retrieve the terminfo(5)-style escape 'e' from tdesc (NULL if undefined).
static inline __attribute__ ((pure)) const char*
get_escape(const tinfo* tdesc, escape_e e){
  unsigned idx = tdesc->escindices[e];
  if(idx){
    return tdesc->esctable + idx - 1;
  }
  return NULL;
}

static inline int
term_supported_styles(const tinfo* ti){
  return ti->supported_styles;
}

// if the terminal unambiguously identifies itself in response to our
// queries, go ahead and trust that, overriding TERM.
typedef enum {
  TERMINAL_UNKNOWN,       // no useful information from queries; use termname
  TERMINAL_LINUX,         // ioctl()s
  TERMINAL_XTERM,         // XTVERSION == 'XTerm(ver)'
  TERMINAL_VTE,           // TDA: "~VTE"
  TERMINAL_KITTY,         // XTGETTCAP['TN'] == 'xterm-kitty'
  TERMINAL_FOOT,          // TDA: "\EP!|464f4f54\E\\"
  TERMINAL_MLTERM,        // XTGETTCAP['TN'] == 'mlterm'
  TERMINAL_WEZTERM,       // XTVERSION == 'WezTerm *'
  TERMINAL_ALACRITTY,     // can't be detected; match TERM
  TERMINAL_CONTOUR,       // XTVERSION == 'console *'
} queried_terminals_e;

#ifdef __cplusplus
}
#endif

#endif
