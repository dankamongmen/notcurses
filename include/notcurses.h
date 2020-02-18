#ifndef NOTCURSES_NOTCURSES
#define NOTCURSES_NOTCURSES

#include <time.h>
#include <uchar.h>
#include <ctype.h>
#include <wchar.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <limits.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#define RESTRICT
#else
#define RESTRICT restrict
#endif

#define API __attribute__((visibility("default")))

// Get a human-readable string describing the running notcurses version.
API const char* notcurses_version(void);

struct cell;      // a coordinate on an ncplane: an EGC plus styling
struct ncplane;   // a drawable notcurses surface, composed of cells
struct ncvisual;  // a visual bit of multimedia opened with LibAV
struct notcurses; // notcurses state for a given terminal, composed of ncplanes

// Initialize a direct-mode notcurses context on the connected terminal at 'fp'.
// 'fp' must be a tty. You'll usually want stdout. Direct mode supportes a
// limited subset of notcurses routines which directly affect 'fp', and neither
// supports nor requires notcurses_render(). This can be used to add color and
// styling to text in the standard output paradigm. Returns NULL on error,
// including any failure initializing terminfo.
API struct ncdirect* notcurses_directmode(const char* termtype, FILE* fp);

// Direct mode. This API can be used to colorize and stylize output generated
// outside of notcurses, without ever calling notcurses_render(). These should
// not be intermixed with standard notcurses rendering.
API int ncdirect_fg(struct ncdirect* nc, unsigned rgb);
API int ncdirect_bg(struct ncdirect* nc, unsigned rgb);

static inline int
ncdirect_bg_rgb8(struct ncdirect* nc, unsigned r, unsigned g, unsigned b){
  if(r > 255 || g > 255 || b > 255){
    return -1;
  }
  return ncdirect_bg(nc, (r << 16u) + (g << 8u) + b);
}

static inline int
ncdirect_fg_rgb8(struct ncdirect* nc, unsigned r, unsigned g, unsigned b){
  if(r > 255 || g > 255 || b > 255){
    return -1;
  }
  return ncdirect_fg(nc, (r << 16u) + (g << 8u) + b);
}

API int ncdirect_fg_default(struct ncdirect* nc);
API int ncdirect_bg_default(struct ncdirect* nc);

// Get the current number of columns/rows.
API int ncdirect_dim_x(const struct ncdirect* nc);
API int ncdirect_dim_y(const struct ncdirect* nc);

// ncplane_styles_*() analogues
API int ncdirect_styles_set(struct ncdirect* n, unsigned stylebits);
API int ncdirect_styles_on(struct ncdirect* n, unsigned stylebits);
API int ncdirect_styles_off(struct ncdirect* n, unsigned stylebits);

// Clear the screen.
API int ncdirect_clear(struct ncdirect* nc);

// Release 'nc' and any associated resources. 0 on success, non-0 on failure.
API int ncdirect_stop(struct ncdirect* nc);

// Returns the number of columns occupied by a multibyte (UTF-8) string, or
// -1 if a non-printable/illegal character is encountered.
static inline int
mbswidth(const char* mbs){
  int offset = 0; // offset into mbs
  int cols = 0;   // number of columns consumed thus far
  mbstate_t ps;
  memset(&ps, 0, sizeof(ps));
  do{
    wchar_t wcs;
    size_t r = mbrtowc(&wcs, mbs + offset, SIZE_MAX, &ps);
    if(r == (size_t)-1 || r == (size_t)-2){
      return -1;
    }
    int w = wcwidth(wcs);
    if(w < 0){
      return -1;
    }
    cols += w;
    offset += r;
  }while(mbs[offset]);
  return cols;
}

#define NCPALETTESIZE 256

// A cell corresponds to a single character cell on some plane, which can be
// occupied by a single grapheme cluster (some root spacing glyph, along with
// possible combining characters, which might span multiple columns). At any
// cell, we can have a theoretically arbitrarily long UTF-8 string, a foreground
// color, a background color, and an attribute set. Valid grapheme cluster
// contents include:
//
//  * A NUL terminator,
//  * A single control character, followed by a NUL terminator,
//  * At most one spacing character, followed by zero or more nonspacing
//    characters, followed by a NUL terminator.
//
// Multi-column characters can only have a single style/color throughout.
// Existence is suffering, and thus wcwidth() is not reliable. It's just
// quoting whether or not the EGC contains a "Wide Asian" double-width
// character. This is set for some things, like most emoji, and not set for
// other things, like cuneiform. Fucccccck. True display width is a *property
// of the font*. Fuccccccccckkkkk. Among the longest Unicode codepoints is
//
//    U+FDFD ARABIC LIGATURE BISMILLAH AR-RAHMAN AR-RAHEEM ﷽
//
// wcwidth() rather optimistically claims this suicide bomber of a glyph to
// occupy a single column, right before it explodes in your diner. BiDi text
// is too complicated for me to even get into here. It sucks ass. Be assured
// there are no easy answers. Allah, the All-Powerful, has fucked us again!
//
// Each cell occupies 16 static bytes (128 bits). The surface is thus ~1.6MB
// for a (pretty large) 500x200 terminal. At 80x43, it's less than 64KB.
// Dynamic requirements (the egcpool) can add up to 32MB to an ncplane, but
// such large pools are unlikely in common use.
//
// We implement some small alpha compositing. Foreground and background both
// have two bits of inverted alpha. The actual grapheme written to a cell is
// the topmost non-zero grapheme. If its alpha is 00, its foreground color is
// used unchanged. If its alpha is 10, its foreground color is derived entirely
// from cells underneath it. Otherwise, the result will be a composite.
// Likewise for the background. If the bottom of a coordinate's zbuffer is
// reached with a cumulative alpha of zero, the default is used. In this way,
// a terminal configured with transparent background can be supported through
// multiple occluding ncplanes. A foreground alpha of 11 requests high-contrast
// text (relative to the computed background). A background alpha of 11 is
// currently forbidden.
//
// Default color takes precedence over palette or RGB, and cannot be used with
// transparency. Indexed palette takes precedence over RGB. It cannot
// meaningfully set transparency, but it can be mixed into a cascading color.
// RGB is used if neither default terminal colors nor palette indexing are in
// play, and fully supports all transparency options.
typedef struct cell {
  // These 32 bits are either a single-byte, single-character grapheme cluster
  // (values 0--0x7f), or an offset into a per-ncplane attached pool of
  // varying-length UTF-8 grapheme clusters. This pool may thus be up to 32MB.
  uint32_t gcluster;          // 4B -> 4B
  // NCSTYLE_* attributes (16 bits) + 8 foreground palette index bits + 8
  // background palette index bits. palette index bits are used only if the
  // corresponding default color bit *is not* set, and the corresponding
  // palette index bit *is* set.
  uint32_t attrword;          // + 4B -> 8B
  // (channels & 0x8000000000000000ull): left half of wide character
  // (channels & 0x4000000000000000ull): foreground is *not* "default color"
  // (channels & 0x3000000000000000ull): foreground alpha (2 bits)
  // (channels & 0x0800000000000000ull): foreground uses palette index
  // (channels & 0x0700000000000000ull): reserved, must be 0
  // (channels & 0x00ffffff00000000ull): foreground in 3x8 RGB (rrggbb)
  // (channels & 0x0000000080000000ull): right half of wide character
  // (channels & 0x0000000040000000ull): background is *not* "default color"
  // (channels & 0x0000000030000000ull): background alpha (2 bits)
  // (channels & 0x0000000008000000ull): background uses palette index
  // (channels & 0x0000000007000000ull): reserved, must be 0
  // (channels & 0x0000000000ffffffull): background in 3x8 RGB (rrggbb)
  // At render time, these 24-bit values are quantized down to terminal
  // capabilities, if necessary. There's a clear path to 10-bit support should
  // we one day need it, but keep things cagey for now. "default color" is
  // best explained by color(3NCURSES). ours is the same concept. until the
  // "not default color" bit is set, any color you load will be ignored.
  uint64_t channels;          // + 8B == 16B
} cell;

// These log levels consciously map cleanly to those of libav; notcurses itself
// does not use this full granularity. The log level does not affect the opening
// and closing banners, which can be disabled via the notcurses_option struct's
// 'suppress_banner'. Note that if stderr is connected to the same terminal on
// which we're rendering, any kind of logging will disrupt the output.
typedef enum {
  NCLOGLEVEL_SILENT,  // default. print nothing once fullscreen service begins
  NCLOGLEVEL_PANIC,   // print diagnostics immediately related to crashing
  NCLOGLEVEL_FATAL,   // we're hanging around, but we've had a horrible fault
  NCLOGLEVEL_ERROR,   // we can't keep doin' this, but we can do other things
  NCLOGLEVEL_WARNING, // you probably don't want what's happening to happen
  NCLOGLEVEL_INFO,    // "standard information"
  NCLOGLEVEL_VERBOSE, // "detailed information"
  NCLOGLEVEL_DEBUG,   // this is honestly a bit much
  NCLOGLEVEL_TRACE,   // there's probably a better way to do what you want
} ncloglevel_e;

// Configuration for notcurses_init().
typedef struct notcurses_options {
  // The name of the terminfo database entry describing this terminal. If NULL,
  // the environment variable TERM is used. Failure to open the terminal
  // definition will result in failure to initialize notcurses.
  const char* termtype;
  // If smcup/rmcup capabilities are indicated, notcurses defaults to making
  // use of the "alternate screen". This flag inhibits use of smcup/rmcup.
  bool inhibit_alternate_screen;
  // By default, we hide the cursor if possible. This flag inhibits use of
  // the civis capability, retaining the cursor.
  bool retain_cursor;
  // Notcurses typically prints version info in notcurses_init() and performance
  // info in notcurses_stop(). This inhibits that output.
  bool suppress_banner;
  // We typically install a signal handler for SIG{INT, SEGV, ABRT, QUIT} that
  // restores the screen, and then calls the old signal handler. Set to inhibit
  // registration of these signal handlers.
  bool no_quit_sighandlers;
  // We typically install a signal handler for SIGWINCH that generates a resize
  // event in the notcurses_getc() queue. Set to inhibit this handler.
  bool no_winch_sighandler;
  // If non-NULL, notcurses_render() will write each rendered frame to this
  // FILE* in addition to outfp. This is used primarily for debugging.
  FILE* renderfp;
  // Progressively higher log levels result in more logging to stderr. By
  // default, nothing is printed to stderr once fullscreen service begins.
  ncloglevel_e loglevel;
} notcurses_options;

// Initialize a notcurses context on the connected terminal at 'fp'. 'fp' must
// be a tty. You'll usually want stdout. Returns NULL on error, including any
// failure initializing terminfo.
API struct notcurses* notcurses_init(const notcurses_options* opts, FILE* fp);

// Destroy a notcurses context.
API int notcurses_stop(struct notcurses* nc);

// Make the physical screen match the virtual screen. Changes made to the
// virtual screen (i.e. most other calls) will not be visible until after a
// successful call to notcurses_render().
API int notcurses_render(struct notcurses* nc);

// Return the topmost ncplane, of which there is always at least one.
API struct ncplane* notcurses_top(struct notcurses* n);

// Destroy any ncplanes other than the stdplane.
API void notcurses_drop_planes(struct notcurses* nc);

// All input is currently taken from stdin, though this will likely change. We
// attempt to read a single UTF8-encoded Unicode codepoint, *not* an entire
// Extended Grapheme Cluster. It is also possible that we will read a special
// keypress, i.e. anything that doesn't correspond to a Unicode codepoint (e.g.
// arrow keys, function keys, screen resize events, etc.). These are mapped
// into Unicode's Supplementary Private Use Area-B, starting at U+100000.
//
// notcurses_getc() and notcurses_getc_nblock() are both nonblocking.
// notcurses_getc_blocking() blocks until a codepoint or special key is read,
// or until interrupted by a signal.
//
// In the case of a valid read, a 32-bit Unicode codepoint is returned. 0 is
// returned to indicate that no input was available, but only by
// notcurses_getc(). Otherwise (including on EOF) (char32_t)-1 is returned.

#define suppuabize(w) ((w) + 0x100000)

// Special composed key defintions. These values are added to 0x100000.
#define NCKEY_INVALID suppuabize(0)
#define NCKEY_RESIZE  suppuabize(1) // generated interally in response to SIGWINCH
#define NCKEY_UP      suppuabize(2)
#define NCKEY_RIGHT   suppuabize(3)
#define NCKEY_DOWN    suppuabize(4)
#define NCKEY_LEFT    suppuabize(5)
#define NCKEY_INS     suppuabize(6)
#define NCKEY_DEL     suppuabize(7)
#define NCKEY_BACKSPACE suppuabize(8) // backspace (sometimes)
#define NCKEY_PGDOWN  suppuabize(9)
#define NCKEY_PGUP    suppuabize(10)
#define NCKEY_HOME    suppuabize(11)
#define NCKEY_END     suppuabize(12)
#define NCKEY_F00     suppuabize(20)
#define NCKEY_F01     suppuabize(21)
#define NCKEY_F02     suppuabize(22)
#define NCKEY_F03     suppuabize(23)
#define NCKEY_F04     suppuabize(24)
#define NCKEY_F05     suppuabize(25)
#define NCKEY_F06     suppuabize(26)
#define NCKEY_F07     suppuabize(27)
#define NCKEY_F08     suppuabize(28)
#define NCKEY_F09     suppuabize(29)
#define NCKEY_F10     suppuabize(30)
#define NCKEY_F11     suppuabize(31)
#define NCKEY_F12     suppuabize(32)
#define NCKEY_F13     suppuabize(33)
#define NCKEY_F14     suppuabize(34)
#define NCKEY_F15     suppuabize(35)
#define NCKEY_F16     suppuabize(36)
#define NCKEY_F17     suppuabize(37)
#define NCKEY_F18     suppuabize(38)
#define NCKEY_F19     suppuabize(39)
#define NCKEY_F20     suppuabize(40)
#define NCKEY_F21     suppuabize(41)
#define NCKEY_F22     suppuabize(42)
#define NCKEY_F23     suppuabize(43)
#define NCKEY_F24     suppuabize(44)
#define NCKEY_F25     suppuabize(45)
#define NCKEY_F26     suppuabize(46)
#define NCKEY_F27     suppuabize(47)
#define NCKEY_F28     suppuabize(48)
#define NCKEY_F29     suppuabize(49)
#define NCKEY_F30     suppuabize(50)
#define NCKEY_F31     suppuabize(51)
#define NCKEY_F32     suppuabize(52)
#define NCKEY_F33     suppuabize(53)
#define NCKEY_F34     suppuabize(54)
#define NCKEY_F35     suppuabize(55)
#define NCKEY_F36     suppuabize(56)
#define NCKEY_F37     suppuabize(57)
#define NCKEY_F38     suppuabize(58)
#define NCKEY_F39     suppuabize(59)
#define NCKEY_F40     suppuabize(60)
#define NCKEY_F41     suppuabize(61)
#define NCKEY_F42     suppuabize(62)
#define NCKEY_F43     suppuabize(63)
#define NCKEY_F44     suppuabize(64)
#define NCKEY_F45     suppuabize(65)
#define NCKEY_F46     suppuabize(66)
#define NCKEY_F47     suppuabize(67)
#define NCKEY_F48     suppuabize(68)
#define NCKEY_F49     suppuabize(69)
#define NCKEY_F50     suppuabize(70)
#define NCKEY_F51     suppuabize(71)
#define NCKEY_F52     suppuabize(72)
#define NCKEY_F53     suppuabize(73)
#define NCKEY_F54     suppuabize(74)
#define NCKEY_F55     suppuabize(75)
#define NCKEY_F56     suppuabize(76)
#define NCKEY_F57     suppuabize(77)
#define NCKEY_F58     suppuabize(78)
#define NCKEY_F59     suppuabize(79)
#define NCKEY_F60     suppuabize(80)
// ... leave room for up to 100 function keys, egads
#define NCKEY_ENTER   suppuabize(121)
#define NCKEY_CLS     suppuabize(122) // "clear-screen or erase"
#define NCKEY_DLEFT   suppuabize(123) // down + left on keypad
#define NCKEY_DRIGHT  suppuabize(124)
#define NCKEY_ULEFT   suppuabize(125) // up + left on keypad
#define NCKEY_URIGHT  suppuabize(126)
#define NCKEY_CENTER  suppuabize(127) // the most truly neutral of keypresses
#define NCKEY_BEGIN   suppuabize(128)
#define NCKEY_CANCEL  suppuabize(129)
#define NCKEY_CLOSE   suppuabize(130)
#define NCKEY_COMMAND suppuabize(131)
#define NCKEY_COPY    suppuabize(132)
#define NCKEY_EXIT    suppuabize(133)
#define NCKEY_PRINT   suppuabize(134)
#define NCKEY_REFRESH suppuabize(135)
// Mouse events. We try to encode some details into the char32_t (i.e. which
// button was pressed), but some is embedded in the ncinput event. The release
// event is generic across buttons; callers must maintain state, if they care.
#define NCKEY_BUTTON1  suppuabize(201)
#define NCKEY_BUTTON2  suppuabize(202)
#define NCKEY_BUTTON3  suppuabize(203)
#define NCKEY_BUTTON4  suppuabize(204)
#define NCKEY_BUTTON5  suppuabize(205)
#define NCKEY_BUTTON6  suppuabize(206)
#define NCKEY_BUTTON7  suppuabize(207)
#define NCKEY_BUTTON8  suppuabize(208)
#define NCKEY_BUTTON9  suppuabize(209)
#define NCKEY_BUTTON10 suppuabize(210)
#define NCKEY_BUTTON11 suppuabize(211)
#define NCKEY_RELEASE  suppuabize(212)

// Is this char32_t a Supplementary Private Use Area-B codepoint?
static inline bool
nckey_supppuab_p(char32_t w){
  return w >= 0x100000 && w <= 0x10fffd;
}

// Is the event a synthesized mouse event?
static inline bool
nckey_mouse_p(char32_t r){
  return r >= NCKEY_BUTTON1 && r <= NCKEY_RELEASE;
}

// An input event. Cell coordinates are currently defined only for mouse events.
typedef struct ncinput {
  char32_t id;     // identifier. Unicode codepoint or synthesized NCKEY event
  int y;           // y cell coordinate of event, -1 for undefined
  int x;           // x cell coordinate of event, -1 for undefined
  bool alt;        // was alt held?
  bool shift;      // was shift held?
  bool ctrl;       // was ctrl held?
} ncinput;

// See ppoll(2) for more detail. Provide a NULL 'ts' to block at length, a 'ts'
// of 0 for non-blocking operation, and otherwise a timespec to bound blocking.
// Signals in sigmask (less several we handle internally) will be atomically
// masked and unmasked per ppoll(2). It should generally contain all signals.
// Returns a single Unicode code point, or (char32_t)-1 on error. 'sigmask' may
// be NULL. Returns 0 on a timeout. If an event is processed, the return value
// is the 'id' field from that event. 'ni' may be NULL.
API char32_t notcurses_getc(struct notcurses* n, const struct timespec* ts,
                            sigset_t* sigmask, ncinput* ni);

// 'ni' may be NULL if the caller is uninterested in event details. If no event
// is ready, returns 0.
static inline char32_t
notcurses_getc_nblock(struct notcurses* n, ncinput* ni){
  sigset_t sigmask;
  sigfillset(&sigmask);
  struct timespec ts = { .tv_sec = 0, .tv_nsec = 0 };
  return notcurses_getc(n, &ts, &sigmask, ni);
}

// 'ni' may be NULL if the caller is uninterested in event details. Blocks
// until an event is processed or a signal is received.
static inline char32_t
notcurses_getc_blocking(struct notcurses* n, ncinput* ni){
  sigset_t sigmask;
  sigemptyset(&sigmask);
  return notcurses_getc(n, NULL, &sigmask, ni);
}

// Enable the mouse in "button-event tracking" mode with focus detection and
// UTF8-style extended coordinates. On failure, -1 is returned. On success, 0
// is returned, and mouse events will be published to notcurses_getc().
API int notcurses_mouse_enable(struct notcurses* n);

// Disable mouse events. Any events in the input queue can still be delivered.
API int notcurses_mouse_disable(struct notcurses* n);

// Refresh our idea of the terminal's dimensions, reshaping the standard plane
// if necessary. Without a call to this function following a terminal resize
// (as signaled via SIGWINCH), notcurses_render() might not function properly.
// References to ncplanes (and the egcpools underlying cells) remain valid
// following a resize operation, but the cursor might have changed position.
API int notcurses_resize(struct notcurses* n, int* RESTRICT y, int* RESTRICT x);

// Refresh the physical screen to match what was last rendered (i.e., without
// reflecting any changes since the last call to notcurses_render()). This is
// primarily useful if the screen is externally corrupted.
API int notcurses_refresh(struct notcurses* n);

// Get a reference to the standard plane (one matching our current idea of the
// terminal size) for this terminal. The standard plane always exists, and its
// origin is always at the uppermost, leftmost cell of the terminal.
API struct ncplane* notcurses_stdplane(struct notcurses* nc);

// Retrieve the contents of the specified cell as last rendered. The EGC is
// returned, or NULL on error. This EGC must be free()d by the caller. The cell
// 'c' is not bound to a plane, and thus its gcluster value must not be used--
// use the return value only.
API char* notcurses_at_yx(struct notcurses* nc, int yoff, int xoff, cell* c);

// Alignment within the ncplane. Left/right-justified, or centered.
typedef enum {
  NCALIGN_LEFT,
  NCALIGN_CENTER,
  NCALIGN_RIGHT,
} ncalign_e;

// Create a new ncplane at the specified offset (relative to the standard plane)
// and the specified size. The number of rows and columns must both be positive.
// This plane is initially at the top of the z-buffer, as if ncplane_move_top()
// had been called on it. The void* 'opaque' can be retrieved (and reset) later.
API struct ncplane* ncplane_new(struct notcurses* nc, int rows, int cols,
                                int yoff, int xoff, void* opaque);

API struct ncplane* ncplane_aligned(struct ncplane* n, int rows, int cols,
                                    int yoff, ncalign_e align, void* opaque);

// Duplicate an existing ncplane. The new plane will have the same geometry,
// will duplicate all content, and will start with the same rendering state.
// The new plane will be immediately above the old one on the z axis.
API struct ncplane* ncplane_dup(struct ncplane* n, void* opaque);

// Returns a 16-bit bitmask of supported curses-style attributes
// (NCSTYLE_UNDERLINE, NCSTYLE_BOLD, etc.) The attribute is only
// indicated as supported if the terminal can support it together with color.
// For more information, see the "ncv" capability in terminfo(5).
API unsigned notcurses_supported_styles(const struct notcurses* nc);

// Returns the number of simultaneous colors claimed to be supported, or 1 if
// there is no color support. Note that several terminal emulators advertise
// more colors than they actually support, downsampling internally.
API int notcurses_palette_size(const struct notcurses* nc);

// Capabilities

// Can we fade? Fading requires either the "rgb" or "ccc" terminfo capability.
API bool notcurses_canfade(const struct notcurses* nc);

// Can we set the "hardware" palette? Requires the "ccc" terminfo capability.
API bool notcurses_canchangecolor(const struct notcurses* nc);

// Can we load images/videos? This requires being built against FFmpeg.
API bool notcurses_canopen(const struct notcurses* nc);

typedef struct ncstats {
  // purely increasing stats
  uint64_t renders;          // number of successful notcurses_render() runs
  uint64_t failed_renders;   // number of aborted renders, should be 0
  uint64_t render_bytes;     // bytes emitted to ttyfp
  int64_t render_max_bytes;  // max bytes emitted for a frame
  int64_t render_min_bytes;  // min bytes emitted for a frame
  uint64_t render_ns;        // nanoseconds spent in notcurses_render()
  int64_t render_max_ns;     // max ns spent in notcurses_render()
  int64_t render_min_ns;     // min ns spent in successful notcurses_render()
  uint64_t cellelisions;     // cells we elided entirely thanks to damage maps
  uint64_t cellemissions;    // cells we emitted due to inferred damage
  uint64_t fgelisions;       // RGB fg elision count
  uint64_t fgemissions;      // RGB fg emissions
  uint64_t bgelisions;       // RGB bg elision count
  uint64_t bgemissions;      // RGB bg emissions
  uint64_t defaultelisions;  // default color was emitted
  uint64_t defaultemissions; // default color was elided

  // current state -- these can decrease
  uint64_t fbbytes;          // total bytes devoted to all active framebuffers
  unsigned planes;           // number of planes currently in existence
} ncstats;

// Acquire an atomic snapshot of the notcurses object's stats.
API void notcurses_stats(struct notcurses* nc, ncstats* stats);

// Reset all cumulative stats (immediate ones, such as fbbytes, are not reset).
API void notcurses_reset_stats(struct notcurses* nc, ncstats* stats);

// Return the dimensions of this ncplane.
API void ncplane_dim_yx(struct ncplane* n, int* RESTRICT rows, int* RESTRICT cols);

static inline int
ncplane_dim_y(struct ncplane* n){
  int dimy;
  ncplane_dim_yx(n, &dimy, NULL);
  return dimy;
}

static inline int
ncplane_dim_x(struct ncplane* n){
  int dimx;
  ncplane_dim_yx(n, NULL, &dimx);
  return dimx;
}

// Return our current idea of the terminal dimensions in rows and cols.
static inline void
notcurses_term_dim_yx(struct notcurses* n, int* RESTRICT rows, int* RESTRICT cols){
  ncplane_dim_yx(notcurses_stdplane(n), rows, cols);
}

// Resize the specified ncplane. The four parameters 'keepy', 'keepx',
// 'keepleny', and 'keeplenx' define a subset of the ncplane to keep,
// unchanged. This may be a section of size 0, though none of these four
// parameters may be negative. 'keepx' and 'keepy' are relative to the ncplane.
// They must specify a coordinate within the ncplane's totality. 'yoff' and
// 'xoff' are relative to 'keepy' and 'keepx', and place the upper-left corner
// of the resized ncplane. Finally, 'ylen' and 'xlen' are the dimensions of the
// ncplane after resizing. 'ylen' must be greater than or equal to 'keepleny',
// and 'xlen' must be greater than or equal to 'keeplenx'. It is an error to
// attempt to resize the standard plane. If either of 'keepleny' or 'keeplenx'
// is non-zero, both must be non-zero.
//
// Essentially, the kept material does not move. It serves to anchor the
// resized plane. If there is no kept material, the plane can move freely.
API int ncplane_resize(struct ncplane* n, int keepy, int keepx, int keepleny,
                       int keeplenx, int yoff, int xoff, int ylen, int xlen);

// Resize the plane, retaining what data we can (everything, unless we're
// shrinking in some dimension). Keep the origin where it is.
static inline int
ncplane_resize_simple(struct ncplane* n, int ylen, int xlen){
  int oldy, oldx;
  ncplane_dim_yx(n, &oldy, &oldx); // current dimensions of 'n'
  int keepleny = oldy > ylen ? ylen : oldy;
  int keeplenx = oldx > xlen ? xlen : oldx;
  return ncplane_resize(n, 0, 0, keepleny, keeplenx, 0, 0, ylen, xlen);
}

// Destroy the specified ncplane. None of its contents will be visible after
// the next call to notcurses_render(). It is an error to attempt to destroy
// the standard plane.
API int ncplane_destroy(struct ncplane* ncp);

// Set the ncplane's base cell to this cell. It will be used for purposes of
// rendering anywhere that the ncplane's gcluster is 0. Erasing the ncplane
// does not reset the base cell; this function must be called with a zero 'c'.
API int ncplane_set_base_cell(struct ncplane* ncp, const cell* c);

// Set the ncplane's base cell to this cell. It will be used for purposes of
// rendering anywhere that the ncplane's gcluster is 0. Erasing the ncplane
// does not reset the base cell; this function must be called with an empty
// 'egc'. 'egc' must be a single extended grapheme cluster.
API int ncplane_set_base(struct ncplane* ncp, uint64_t channels,
                         uint32_t attrword, const char* egc);

// Extract the ncplane's base cell into 'c'. The reference is invalidated if
// 'ncp' is destroyed.
API int ncplane_base(struct ncplane* ncp, cell* c);

// Move this plane relative to the standard plane. It is an error to attempt to
// move the standard plane.
API int ncplane_move_yx(struct ncplane* n, int y, int x);

// Get the origin of this plane relative to the standard plane.
API void ncplane_yx(const struct ncplane* n, int* RESTRICT y, int* RESTRICT x);

// Splice ncplane 'n' out of the z-buffer, and reinsert it at the top or bottom.
API int ncplane_move_top(struct ncplane* n);
API int ncplane_move_bottom(struct ncplane* n);

// Splice ncplane 'n' out of the z-buffer, and reinsert it above 'above'.
API int ncplane_move_above_unsafe(struct ncplane* RESTRICT n,
                                  struct ncplane* RESTRICT above);

static inline int
ncplane_move_above(struct ncplane* n, struct ncplane* above){
  if(n == above){
    return -1;
  }
  return ncplane_move_above_unsafe(n, above);
}

// Splice ncplane 'n' out of the z-buffer, and reinsert it below 'below'.
API int ncplane_move_below_unsafe(struct ncplane* RESTRICT n,
                                  struct ncplane* RESTRICT below);

static inline int
ncplane_move_below(struct ncplane* n, struct ncplane* below){
  if(n == below){
    return -1;
  }
  return ncplane_move_below_unsafe(n, below);
}

// Return the plane above this one, or NULL if this is at the top.
API struct ncplane* ncplane_below(struct ncplane* n);

// Retrieve the cell at the cursor location on the specified plane, returning
// it in 'c'. This copy is safe to use until the ncplane is destroyed/erased.
API int ncplane_at_cursor(struct ncplane* n, cell* c);

// Retrieve the cell at the specified location on the specified plane, returning
// it in 'c'. This copy is safe to use until the ncplane is destroyed/erased.
// Returns the length of the EGC in bytes.
API int ncplane_at_yx(struct ncplane* n, int y, int x, cell* c);

// Manipulate the opaque user pointer associated with this plane.
// ncplane_set_userptr() returns the previous userptr after replacing
// it with 'opaque'. the others simply return the userptr.
API void* ncplane_set_userptr(struct ncplane* n, void* opaque);
API void* ncplane_userptr(struct ncplane* n);

// Return the column at which 'c' cols ought start in order to be aligned
// according to 'align' within ncplane 'n'. Returns INT_MAX on invalid 'align'.
// Undefined behavior on negative 'c'.
static inline int
ncplane_align(struct ncplane* n, ncalign_e align, int c){
  if(align == NCALIGN_LEFT){
    return 0;
  }
  int cols = ncplane_dim_x(n);
  if(align == NCALIGN_CENTER){
    return (cols - c) / 2;
  }else if(align == NCALIGN_RIGHT){
    return cols - c;
  }
  return INT_MAX;
}

// Move the cursor to the specified position (the cursor needn't be visible).
// Returns -1 on error, including negative parameters, or ones exceeding the
// plane's dimensions.
API int ncplane_cursor_move_yx(struct ncplane* n, int y, int x);

// Get the current position of the cursor within n. y and/or x may be NULL.
API void ncplane_cursor_yx(struct ncplane* n, int* RESTRICT y, int* RESTRICT x);

// Replace the cell at the specified coordinates with the provided cell 'c',
// and advance the cursor by the width of the cell (but not past the end of the
// plane). On success, returns the number of columns the cursor was advanced.
// On failure, -1 is returned.
API int ncplane_putc_yx(struct ncplane* n, int y, int x, const cell* c);

// Call ncplane_putc_yx() for the current cursor location.
static inline int
ncplane_putc(struct ncplane* n, const cell* c){
  return ncplane_putc_yx(n, -1, -1, c);
}

// Replace the cell at the specified coordinates with the provided 7-bit char
// 'c'. Advance the cursor by 1. On success, returns 1. On failure, returns -1.
// This works whether the underlying char is signed or unsigned.
API int ncplane_putsimple_yx(struct ncplane* n, int y, int x, char c);

// Call ncplane_putsimple_yx() at the current cursor location.
static inline int
ncplane_putsimple(struct ncplane* n, char c){
  return ncplane_putsimple_yx(n, -1, -1, c);
}

// Replace the cell at the specified coordinates with the provided EGC, and
// advance the cursor by the width of the cluster (but not past the end of the
// plane). On success, returns the number of columns the cursor was advanced.
// On failure, -1 is returned. The number of bytes converted from gclust is
// written to 'sbytes' if non-NULL.
API int ncplane_putegc_yx(struct ncplane* n, int y, int x, const char* gclust, int* sbytes);

// Call ncplane_putegc() at the current cursor location.
static inline int
ncplane_putegc(struct ncplane* n, const char* gclust, int* sbytes){
  return ncplane_putegc_yx(n, -1, -1, gclust, sbytes);
}

#define WCHAR_MAX_UTF8BYTES 6

// ncplane_putegc(), but following a conversion from wchar_t to UTF-8 multibyte.
static inline int
ncplane_putwegc(struct ncplane* n, const wchar_t* gclust, int* sbytes){
  // maximum of six UTF8-encoded bytes per wchar_t
  const size_t mbytes = (wcslen(gclust) * WCHAR_MAX_UTF8BYTES) + 1;
  char* mbstr = (char*)malloc(mbytes); // need cast for c++ callers
  if(mbstr == NULL){
    return -1;
  }
  size_t s = wcstombs(mbstr, gclust, mbytes);
  if(s == (size_t)-1){
    free(mbstr);
    return -1;
  }
  int ret = ncplane_putegc(n, mbstr, sbytes);
  free(mbstr);
  return ret;
}

// Call ncplane_putwegc() after successfully moving to y, x.
static inline int
ncplane_putwegc_yx(struct ncplane* n, int y, int x, const wchar_t* gclust,
                   int* sbytes){
  if(ncplane_cursor_move_yx(n, y, x)){
    return -1;
  }
  return ncplane_putwegc(n, gclust, sbytes);
}

// Write a series of EGCs to the current location, using the current style.
// They will be interpreted as a series of columns (according to the definition
// of ncplane_putc()). Advances the cursor by some positive number of cells
// (though not beyond the end of the plane); this number is returned on success.
// On error, a non-positive number is returned, indicating the number of cells
// which were written before the error.
API int ncplane_putstr_yx(struct ncplane* n, int y, int x, const char* gclustarr);

static inline int
ncplane_putstr(struct ncplane* n, const char* gclustarr){
  return ncplane_putstr_yx(n, -1, -1, gclustarr);
}

API int ncplane_putstr_aligned(struct ncplane* n, int y, ncalign_e align,
                               const char* s);

// ncplane_putstr(), but following a conversion from wchar_t to UTF-8 multibyte.
static inline int
ncplane_putwstr_yx(struct ncplane* n, int y, int x, const wchar_t* gclustarr){
  // maximum of six UTF8-encoded bytes per wchar_t
  const size_t mbytes = (wcslen(gclustarr) * WCHAR_MAX_UTF8BYTES) + 1;
  char* mbstr = (char*)malloc(mbytes); // need cast for c++ callers
  if(mbstr == NULL){
    return -1;
  }
  size_t s = wcstombs(mbstr, gclustarr, mbytes);
  if(s == (size_t)-1){
    free(mbstr);
    return -1;
  }
  int ret = ncplane_putstr_yx(n, y, x, mbstr);
  free(mbstr);
  return ret;
}

static inline int
ncplane_putwstr_aligned(struct ncplane* n, int y, ncalign_e align,
                        const wchar_t* gclustarr){
  int width = wcswidth(gclustarr, INT_MAX);
  int xpos = ncplane_align(n, align, width);
  return ncplane_putwstr_yx(n, y, xpos, gclustarr);
}

static inline int
ncplane_putwstr(struct ncplane* n, const wchar_t* gclustarr){
  return ncplane_putwstr_yx(n, -1, -1, gclustarr);
}

// Replace the cell at the specified coordinates with the provided wide char
// 'w'. Advance the cursor by the character's width as reported by wcwidth().
// On success, returns 1. On failure, returns -1.
static inline int
ncplane_putwc_yx(struct ncplane* n, int y, int x, wchar_t w){
  wchar_t warr[2] = { w, L'\0' };
  return ncplane_putwstr_yx(n, y, x, warr);
}

// Call ncplane_putwc() at the current cursor position.
static inline int
ncplane_putwc(struct ncplane* n, wchar_t w){
  return ncplane_putwc_yx(n, -1, -1, w);
}

// The ncplane equivalents of printf(3) and vprintf(3).
API int ncplane_vprintf_aligned(struct ncplane* n, int y, ncalign_e align,
                                const char* format, va_list ap);

API int ncplane_vprintf_yx(struct ncplane* n, int y, int x,
                           const char* format, va_list ap);

static inline int
ncplane_vprintf(struct ncplane* n, const char* format, va_list ap){
  return ncplane_vprintf_yx(n, -1, -1, format, ap);
}

static inline int
ncplane_printf(struct ncplane* n, const char* format, ...)
  __attribute__ ((format (printf, 2, 3)));

static inline int
ncplane_printf(struct ncplane* n, const char* format, ...){
  va_list va;
  va_start(va, format);
  int ret = ncplane_vprintf(n, format, va);
  va_end(va);
  return ret;
}

static inline int
ncplane_printf_yx(struct ncplane* n, int y, int x, const char* format, ...)
  __attribute__ ((format (printf, 4, 5)));

static inline int
ncplane_printf_yx(struct ncplane* n, int y, int x, const char* format, ...){
  va_list va;
  va_start(va, format);
  int ret = ncplane_vprintf_yx(n, y, x, format, va);
  va_end(va);
  return ret;
}

static inline int
ncplane_printf_aligned(struct ncplane* n, int y, ncalign_e align,
                       const char* format, ...)
  __attribute__ ((format (printf, 4, 5)));

static inline int
ncplane_printf_aligned(struct ncplane* n, int y, ncalign_e align, const char* format, ...){
  va_list va;
  va_start(va, format);
  int ret = ncplane_vprintf_aligned(n, y, align, format, va);
  va_end(va);
  return ret;
}

// Draw horizontal or vertical lines using the specified cell, starting at the
// current cursor position. The cursor will end at the cell following the last
// cell output (even, perhaps counter-intuitively, when drawing vertical
// lines), just as if ncplane_putc() was called at that spot. Return the
// number of cells drawn on success. On error, return the negative number of
// cells drawn.
API int ncplane_hline_interp(struct ncplane* n, const cell* c, int len,
                             uint64_t c1, uint64_t c2);

static inline int
ncplane_hline(struct ncplane* n, const cell* c, int len){
  return ncplane_hline_interp(n, c, len, c->channels, c->channels);
}

API int ncplane_vline_interp(struct ncplane* n, const cell* c, int len,
                             uint64_t c1, uint64_t c2);

static inline int
ncplane_vline(struct ncplane* n, const cell* c, int len){
  return ncplane_vline_interp(n, c, len, c->channels, c->channels);
}

#define NCBOXMASK_TOP    0x0001
#define NCBOXMASK_RIGHT  0x0002
#define NCBOXMASK_BOTTOM 0x0004
#define NCBOXMASK_LEFT   0x0008
#define NCBOXGRAD_TOP    0x0010
#define NCBOXGRAD_RIGHT  0x0020
#define NCBOXGRAD_BOTTOM 0x0040
#define NCBOXGRAD_LEFT   0x0080
#define NCBOXCORNER_MASK 0x0300
#define NCBOXCORNER_SHIFT 8u

// Draw a box with its upper-left corner at the current cursor position, and its
// lower-right corner at 'ystop'x'xstop'. The 6 cells provided are used to draw the
// upper-left, ur, ll, and lr corners, then the horizontal and vertical lines.
// 'ctlword' is defined in the least significant byte, where bits [7, 4] are a
// gradient mask, and [3, 0] are a border mask:
//  * 7, 3: top
//  * 6, 2: right
//  * 5, 1: bottom
//  * 4, 0: left
// If the gradient bit is not set, the styling from the hl/vl cells is used for
// the horizontal and vertical lines, respectively. If the gradient bit is set,
// the color is linearly interpolated between the two relevant corner cells.
//
// By default, vertexes are drawn whether their connecting edges are drawn or
// not. The value of the bits corresponding to NCBOXCORNER_MASK control this,
// and are interpreted as the number of connecting edges necessary to draw a
// given corner. At 0 (the default), corners are always drawn. At 3, corners
// are never drawn (as at most 2 edges can touch a box's corner).
API int ncplane_box(struct ncplane* n, const cell* ul, const cell* ur,
                    const cell* ll, const cell* lr, const cell* hline,
                    const cell* vline, int ystop, int xstop,
                    unsigned ctlword);

// Draw a box with its upper-left corner at the current cursor position, having
// dimensions 'ylen'x'xlen'. See ncplane_box() for more information. The
// minimum box size is 2x2, and it cannot be drawn off-screen.
static inline int
ncplane_box_sized(struct ncplane* n, const cell* ul, const cell* ur,
                  const cell* ll, const cell* lr, const cell* hline,
                  const cell* vline, int ylen, int xlen, unsigned ctlword){
  int y, x;
  ncplane_cursor_yx(n, &y, &x);
  return ncplane_box(n, ul, ur, ll, lr, hline, vline, y + ylen - 1,
                     x + xlen - 1, ctlword);
}

static inline int
ncplane_perimeter(struct ncplane* n, const cell* ul, const cell* ur,
                  const cell* ll, const cell* lr, const cell* hline,
                  const cell* vline, unsigned ctlword){
  if(ncplane_cursor_move_yx(n, 0, 0)){
    return -1;
  }
  int dimy, dimx;
  ncplane_dim_yx(n, &dimy, &dimx);
  return ncplane_box_sized(n, ul, ur, ll, lr, hline, vline, dimy, dimx, ctlword);
}

// Starting at the specified coordinate, if it has no glyph, 'c' is copied into
// it. We do the same to all cardinally-connected glyphless cells, filling in
// everything behind a boundary. Returns the number of cells polyfilled. An
// invalid initial y, x is an error.
API int ncplane_polyfill_yx(struct ncplane* n, int y, int x, const cell* c);

// Draw a gradient with its upper-left corner at the current cursor position,
// stopping at 'ystop'x'xstop'. The glyph composed of 'egc' and 'attrword' is
// used for all cells. The channels specified by 'ul', 'ur', 'll', and 'lr'
// are composed into foreground and background gradients. To do a vertical
// gradient, 'ul' ought equal 'ur' and 'll' ought equal 'lr'. To do a
// horizontal gradient, 'ul' ought equal 'll' and 'ur' ought equal 'ul'. To
// color everything the same, all four channels should be equivalent.
API int ncplane_gradient(struct ncplane* n, const char* egc, uint32_t attrword,
                         uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr,
                         int ystop, int xstop);

// Draw a gradient with its upper-left corner at the current cursor position,
// having dimensions 'ylen'x'xlen'. See ncplane_gradient for more information.
static inline int
ncplane_gradient_sized(struct ncplane* n, const char* egc, uint32_t attrword,
                       uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr,
                       int ylen, int xlen){
  int y, x;
  ncplane_cursor_yx(n, &y, &x);
  return ncplane_gradient(n, egc, attrword, ul, ur, ll, lr, y + ylen - 1, x + xlen - 1);
}

// Erase every cell in the ncplane, resetting all attributes to normal, all
// colors to the default color, and all cells to undrawn. All cells associated
// with this ncplane is invalidated, and must not be used after the call,
// excluding the base cell.
API void ncplane_erase(struct ncplane* n);

#define CELL_WIDEASIAN_MASK     0x8000000080000000ull
#define CELL_BGDEFAULT_MASK     0x0000000040000000ull
#define CELL_FGDEFAULT_MASK     (CELL_BGDEFAULT_MASK << 32u)
#define CELL_BG_MASK            0x0000000000ffffffull
#define CELL_FG_MASK            (CELL_BG_MASK << 32u)
#define CELL_BG_PALETTE         0x0000000008000000ull
#define CELL_FG_PALETTE         (CELL_BG_PALETTE << 32u)
#define CELL_ALPHA_MASK         0x0000000030000000ull
#define CELL_ALPHA_SHIFT        28u
#define CELL_ALPHA_HIGHCONTRAST 3
#define CELL_ALPHA_TRANSPARENT  2
#define CELL_ALPHA_BLEND        1
#define CELL_ALPHA_OPAQUE       0

// These lowest-level functions manipulate a 64-bit channel encoding directly.
// Users will typically manipulate ncplane and cell channels through those APIs,
// rather than calling these directly.

// Extract the 8-bit red component from a 32-bit channel.
static inline unsigned
channel_r(unsigned channel){
  return (channel & 0xff0000u) >> 16u;
}

// Extract the 8-bit green component from a 32-bit channel.
static inline unsigned
channel_g(unsigned channel){
  return (channel & 0x00ff00u) >> 8u;
}

// Extract the 8-bit blue component from a 32-bit channel.
static inline unsigned
channel_b(unsigned channel){
  return (channel & 0x0000ffu);
}

// Extract the three 8-bit R/G/B components from a 32-bit channel.
static inline unsigned
channel_rgb(unsigned channel, unsigned* RESTRICT r, unsigned* RESTRICT g,
                unsigned* RESTRICT b){
  *r = channel_r(channel);
  *g = channel_g(channel);
  *b = channel_b(channel);
  return channel;
}

// Set the three 8-bit components of a 32-bit channel, and mark it as not using
// the default color. Retain the other bits unchanged.
static inline int
channel_set_rgb(unsigned* channel, int r, int g, int b){
  if(r >= 256 || g >= 256 || b >= 256){
    return -1;
  }
  if(r < 0 || g < 0 || b < 0){
    return -1;
  }
  unsigned c = (r << 16u) | (g << 8u) | b;
  *channel = (*channel & ~CELL_BG_MASK) | CELL_BGDEFAULT_MASK | c;
  return 0;
}

// Set the three 8-bit components of a 32-bit channel, and mark it as not using
// the default color. Retain the other bits unchanged. r, g, and b will be
// clipped to the range [0..255].
static inline void
channel_set_rgb_clipped(unsigned* channel, int r, int g, int b){
  if(r >= 256){
    r = 255;
  }
  if(g >= 256){
    g = 255;
  }
  if(b >= 256){
    b = 255;
  }
  if(r <= -1){
    r = 0;
  }
  if(g <= -1){
    g = 0;
  }
  if(b <= -1){
    b = 0;
  }
  unsigned c = (r << 16u) | (g << 8u) | b;
  *channel = (*channel & ~CELL_BG_MASK) | CELL_BGDEFAULT_MASK | c;
}

// Same, but provide an assembled, packed 24 bits of rgb.
static inline int
channel_set(unsigned* channel, unsigned rgb){
  if(rgb > 0xffffffu){
    return -1;
  }
  *channel = (*channel & ~CELL_BG_MASK) | CELL_BGDEFAULT_MASK | rgb;
  return 0;
}

// Extract the 2-bit alpha component from a 32-bit channel.
static inline unsigned
channel_alpha(unsigned channel){
  return (channel & CELL_ALPHA_MASK) >> CELL_ALPHA_SHIFT;
}

// Set the 2-bit alpha component of the 32-bit channel.
static inline int
channel_set_alpha(unsigned* channel, int alpha){
  if(alpha < CELL_ALPHA_OPAQUE || alpha > CELL_ALPHA_HIGHCONTRAST){
    return -1;
  }
  *channel = (alpha << CELL_ALPHA_SHIFT) | (*channel & ~CELL_ALPHA_MASK);
  if(alpha == CELL_ALPHA_HIGHCONTRAST){
    *channel |= CELL_BGDEFAULT_MASK;
  }
  return 0;
}

// Is this channel using the "default color" rather than RGB/palette-indexed?
static inline bool
channel_default_p(unsigned channel){
  return !(channel & CELL_BGDEFAULT_MASK);
}

// Is this channel using palette-indexed color rather than RGB?
static inline bool
channel_palindex_p(unsigned channel){
  return !channel_default_p(channel) && (channel & CELL_BG_PALETTE);
}

// Mark the channel as using its default color, which also marks it opaque.
static inline unsigned
channel_set_default(unsigned* channel){
  return *channel &= ~(CELL_BGDEFAULT_MASK | CELL_ALPHA_HIGHCONTRAST);
}

// Extract the 32-bit background channel from a channel pair.
static inline uint32_t
channels_bchannel(uint64_t channels){
  return channels & 0xfffffffflu;
}

// Extract the 32-bit foreground channel from a channel pair.
static inline uint32_t
channels_fchannel(uint64_t channels){
  return channels_bchannel(channels >> 32u);
}

// Set the 32-bit background channel of a channel pair.
static inline uint64_t
channels_set_bchannel(uint64_t* channels, uint32_t channel){
  return *channels = (*channels & 0xffffffff00000000llu) | channel;
}

// Set the 32-bit foreground channel of a channel pair.
static inline uint64_t
channels_set_fchannel(uint64_t* channels, uint32_t channel){
  return *channels = (*channels & 0xfffffffflu) | ((uint64_t)channel << 32u);
}

// Extract 24 bits of foreground RGB from 'channels', shifted to LSBs.
static inline unsigned
channels_fg(uint64_t channels){
  return channels_fchannel(channels) & CELL_BG_MASK;
}

// Extract 24 bits of background RGB from 'channels', shifted to LSBs.
static inline unsigned
channels_bg(uint64_t channels){
  return channels_bchannel(channels) & CELL_BG_MASK;
}

// Extract 2 bits of foreground alpha from 'channels', shifted to LSBs.
static inline unsigned
channels_fg_alpha(uint64_t channels){
  return channel_alpha(channels_fchannel(channels));
}

// Extract 2 bits of background alpha from 'channels', shifted to LSBs.
static inline unsigned
channels_bg_alpha(uint64_t channels){
  return channel_alpha(channels_bchannel(channels));
}

// Extract 24 bits of foreground RGB from 'channels', split into subchannels.
static inline unsigned
channels_fg_rgb(uint64_t channels, unsigned* r, unsigned* g, unsigned* b){
  return channel_rgb(channels_fchannel(channels), r, g, b);
}

// Extract 24 bits of background RGB from 'channels', split into subchannels.
static inline unsigned
channels_bg_rgb(uint64_t channels, unsigned* r, unsigned* g, unsigned* b){
  return channel_rgb(channels_bchannel(channels), r, g, b);
}

// Set the r, g, and b channels for the foreground component of this 64-bit
// 'channels' variable, and mark it as not using the default color.
static inline int
channels_set_fg_rgb(uint64_t* channels, int r, int g, int b){
  unsigned channel = channels_fchannel(*channels);
  if(channel_set_rgb(&channel, r, g, b) < 0){
    return -1;
  }
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
  return 0;
}

// Same, but clips to [0..255].
static inline void
channels_set_fg_rgb_clipped(uint64_t* channels, int r, int g, int b){
  unsigned channel = channels_fchannel(*channels);
  channel_set_rgb_clipped(&channel, r, g, b);
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
}

// Set the r, g, and b channels for the background component of this 64-bit
// 'channels' variable, and mark it as not using the default color.
static inline int
channels_set_bg_rgb(uint64_t* channels, int r, int g, int b){
  unsigned channel = channels_bchannel(*channels);
  if(channel_set_rgb(&channel, r, g, b) < 0){
    return -1;
  }
  channels_set_bchannel(channels, channel);
  return 0;
}

// Same, but clips to [0..255].
static inline void
channels_set_bg_rgb_clipped(uint64_t* channels, int r, int g, int b){
  unsigned channel = channels_bchannel(*channels);
  channel_set_rgb_clipped(&channel, r, g, b);
  channels_set_bchannel(channels, channel);
}

// Same, but set an assembled 32 bit channel at once.
static inline int
channels_set_fg(uint64_t* channels, unsigned rgb){
  unsigned channel = channels_fchannel(*channels);
  if(channel_set(&channel, rgb) < 0){
    return -1;
  }
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
  return 0;
}

static inline int
channels_set_bg(uint64_t* channels, unsigned rgb){
  unsigned channel = channels_bchannel(*channels);
  if(channel_set(&channel, rgb) < 0){
    return -1;
  }
  channels_set_bchannel(channels, channel);
  return 0;
}

// Set the 2-bit alpha component of the foreground channel.
static inline int
channels_set_fg_alpha(uint64_t* channels, int alpha){
  unsigned channel = channels_fchannel(*channels);
  if(channel_set_alpha(&channel, alpha) < 0){
    return -1;
  }
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
  return 0;
}

// Set the 2-bit alpha component of the background channel.
static inline int
channels_set_bg_alpha(uint64_t* channels, int alpha){
  if(alpha == CELL_ALPHA_HIGHCONTRAST){ // forbidden for background alpha
    return -1;
  }
  unsigned channel = channels_bchannel(*channels);
  if(channel_set_alpha(&channel, alpha) < 0){
    return -1;
  }
  channels_set_bchannel(channels, channel);
  return 0;
}

// Is the foreground using the "default foreground color"?
static inline bool
channels_fg_default_p(uint64_t channels){
  return channel_default_p(channels_fchannel(channels));
}

// Is the foreground using indexed palette color?
static inline bool
channels_fg_palindex_p(uint64_t channels){
  return channel_palindex_p(channels_fchannel(channels));
}

// Is the background using the "default background color"? The "default
// background color" must generally be used to take advantage of
// terminal-effected transparency.
static inline bool
channels_bg_default_p(uint64_t channels){
  return channel_default_p(channels_bchannel(channels));
}

// Is the background using indexed palette color?
static inline bool
channels_bg_palindex_p(uint64_t channels){
  return channel_palindex_p(channels_bchannel(channels));
}

// Mark the foreground channel as using its default color.
static inline uint64_t
channels_set_fg_default(uint64_t* channels){
  unsigned channel = channels_fchannel(*channels);
  channel_set_default(&channel);
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
  return *channels;
}

// Mark the foreground channel as using its default color.
static inline uint64_t
channels_set_bg_default(uint64_t* channels){
  unsigned channel = channels_bchannel(*channels);
  channel_set_default(&channel);
  channels_set_bchannel(channels, channel);
  return *channels;
}

// Returns the result of blending two channels. 'blends' indicates how heavily
// 'c1' ought be weighed. If 'blends' is 0, 'c1' will be entirely replaced by
// 'c2'. If 'c1' is otherwise the default color, 'c1' will not be touched,
// since we can't blend default colors. Likewise, if 'c2' is a default color,
// it will not be used (unless 'blends' is 0).
//
// Palette-indexed colors do not blend, and since we need the attrword to store
// them, we just don't fuck wit' 'em here. Do not pass me palette-indexed
// channels! I will eat them.
static inline unsigned
channels_blend(unsigned c1, unsigned c2, unsigned* blends){
  if(channel_alpha(c2) == CELL_ALPHA_TRANSPARENT){
    return c1; // do *not* increment *blends
  }
  unsigned rsum, gsum, bsum;
  channel_rgb(c2, &rsum, &gsum, &bsum);
  bool c2default = channel_default_p(c2);
  if(*blends == 0){
    // don't just return c2, or you set wide status and all kinds of crap
    if(channel_default_p(c2)){
      channel_set_default(&c1);
    }else{
      channel_set_rgb(&c1, rsum, gsum, bsum);
    }
    channel_set_alpha(&c1, channel_alpha(c2));
  }else if(!c2default && !channel_default_p(c1)){
    rsum = (channel_r(c1) * *blends + rsum) / (*blends + 1);
    gsum = (channel_g(c1) * *blends + gsum) / (*blends + 1);
    bsum = (channel_b(c1) * *blends + bsum) / (*blends + 1);
    channel_set_rgb(&c1, rsum, gsum, bsum);
    channel_set_alpha(&c1, channel_alpha(c2));
  }
  ++*blends;
  return c1;
}

// Extract the 32-bit background channel from a cell.
static inline unsigned
cell_bchannel(const cell* cl){
  return channels_bchannel(cl->channels);
}

// Extract the 32-bit foreground channel from a cell.
static inline unsigned
cell_fchannel(const cell* cl){
  return channels_fchannel(cl->channels);
}

// Set the 32-bit background channel of a cell.
static inline uint64_t
cell_set_bchannel(cell* cl, uint32_t channel){
  return channels_set_bchannel(&cl->channels, channel);
}

// Set the 32-bit foreground channel of a cell.
static inline uint64_t
cell_set_fchannel(cell* cl, uint32_t channel){
  return channels_set_fchannel(&cl->channels, channel);
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

// Extract 24 bits of foreground RGB from 'cell', shifted to LSBs.
static inline unsigned
cell_fg(const cell* cl){
  return channels_fg(cl->channels);
}

// Extract 24 bits of background RGB from 'cell', shifted to LSBs.
static inline unsigned
cell_bg(const cell* cl){
  return channels_bg(cl->channels);
}

// Extract 2 bits of foreground alpha from 'cell', shifted to LSBs.
static inline unsigned
cell_fg_alpha(const cell* cl){
  return channels_fg_alpha(cl->channels);
}

// Extract 2 bits of background alpha from 'cell', shifted to LSBs.
static inline unsigned
cell_bg_alpha(const cell* cl){
  return channels_bg_alpha(cl->channels);
}

// Extract 24 bits of foreground RGB from 'cell', split into subcell.
static inline unsigned
cell_fg_rgb(const cell* cl, unsigned* r, unsigned* g, unsigned* b){
  return channels_fg_rgb(cl->channels, r, g, b);
}

// Extract 24 bits of background RGB from 'cell', split into subcell.
static inline unsigned
cell_bg_rgb(const cell* cl, unsigned* r, unsigned* g, unsigned* b){
  return channels_bg_rgb(cl->channels, r, g, b);
}

// Set the r, g, and b cell for the foreground component of this 64-bit
// 'cell' variable, and mark it as not using the default color.
static inline int
cell_set_fg_rgb(cell* cl, int r, int g, int b){
  return channels_set_fg_rgb(&cl->channels, r, g, b);
}

// Same, but clipped to [0..255].
static inline void
cell_set_fg_rgb_clipped(cell* cl, int r, int g, int b){
  channels_set_fg_rgb_clipped(&cl->channels, r, g, b);
}

// Same, but with an assembled 32-bit channel.
static inline int
cell_set_fg(cell* c, uint32_t channel){
  return channels_set_fg(&c->channels, channel);
}

// Set the cell's foreground palette index, set the foreground palette index
// bit, set it foreground-opaque, and clear the foreground default color bit.
static inline int
cell_set_fg_palindex(cell* cl, int idx){
  if(idx < 0 || idx >= NCPALETTESIZE){
    return -1;
  }
  cl->channels |= CELL_FGDEFAULT_MASK;
  cl->channels |= CELL_FG_PALETTE;
  cl->channels &= ~(CELL_ALPHA_MASK << 32u);
  cl->attrword &= 0xffff00ff;
  cl->attrword |= (idx << 8u);
  return 0;
}

static inline unsigned
cell_fg_palindex(const cell* cl){
  return (cl->attrword & 0x0000ff00) >> 8u;
}

// Set the r, g, and b cell for the background component of this 64-bit
// 'cell' variable, and mark it as not using the default color.
static inline int
cell_set_bg_rgb(cell* cl, int r, int g, int b){
  return channels_set_bg_rgb(&cl->channels, r, g, b);
}

// Same, but clipped to [0..255].
static inline void
cell_set_bg_rgb_clipped(cell* cl, int r, int g, int b){
  channels_set_bg_rgb_clipped(&cl->channels, r, g, b);
}

// Same, but with an assembled 32-bit channel.
static inline int
cell_set_bg(cell* c, uint32_t channel){
  return channels_set_bg(&c->channels, channel);
}

// Set the cell's background palette index, set the background palette index
// bit, set it background-opaque, and clear the background default color bit.
static inline int
cell_set_bg_palindex(cell* cl, int idx){
  if(idx < 0 || idx >= NCPALETTESIZE){
    return -1;
  }
  cl->channels |= CELL_BGDEFAULT_MASK;
  cl->channels |= CELL_BG_PALETTE;
  cl->channels &= ~CELL_ALPHA_MASK;
  cl->attrword &= 0xffffff00;
  cl->attrword |= idx;
  return 0;
}

static inline unsigned
cell_bg_palindex(const cell* cl){
  return cl->attrword & 0x000000ff;
}

// Is the foreground using the "default foreground color"?
static inline bool
cell_fg_default_p(const cell* cl){
  return channels_fg_default_p(cl->channels);
}

static inline bool
cell_fg_palindex_p(const cell* cl){
  return channels_fg_palindex_p(cl->channels);
}

// Is the background using the "default background color"? The "default
// background color" must generally be used to take advantage of
// terminal-effected transparency.
static inline bool
cell_bg_default_p(const cell* cl){
  return channels_bg_default_p(cl->channels);
}

static inline bool
cell_bg_palindex_p(const cell* cl){
  return channels_bg_palindex_p(cl->channels);
}

// Get the current channels or attribute word for ncplane 'n'.
API uint64_t ncplane_channels(const struct ncplane* n);
API uint32_t ncplane_attr(const struct ncplane* n);

// Extract the 32-bit working background channel from an ncplane.
static inline unsigned
ncplane_bchannel(const struct ncplane* nc){
  return channels_bchannel(ncplane_channels(nc));
}

// Extract the 32-bit working foreground channel from an ncplane.
static inline unsigned
ncplane_fchannel(const struct ncplane* nc){
  return channels_fchannel(ncplane_channels(nc));
}

// Extract 24 bits of working foreground RGB from an ncplane, shifted to LSBs.
static inline unsigned
ncplane_fg(const struct ncplane* nc){
  return channels_fg(ncplane_channels(nc));
}

// Extract 24 bits of working background RGB from an ncplane, shifted to LSBs.
static inline unsigned
ncplane_bg(const struct ncplane* nc){
  return channels_bg(ncplane_channels(nc));
}

// Extract 2 bits of foreground alpha from 'struct ncplane', shifted to LSBs.
static inline unsigned
ncplane_fg_alpha(const struct ncplane* nc){
  return channels_fg_alpha(ncplane_channels(nc));
}

// Extract 2 bits of background alpha from 'struct ncplane', shifted to LSBs.
static inline unsigned
ncplane_bg_alpha(const struct ncplane* nc){
  return channels_bg_alpha(ncplane_channels(nc));
}

// Extract 24 bits of foreground RGB from 'n', split into subcomponents.
static inline unsigned
ncplane_fg_rgb(const struct ncplane* n, unsigned* r, unsigned* g, unsigned* b){
  return channels_fg_rgb(ncplane_channels(n), r, g, b);
}

// Extract 24 bits of background RGB from 'n', split into subcomponents.
static inline unsigned
ncplane_bg_rgb(const struct ncplane* n, unsigned* r, unsigned* g, unsigned* b){
  return channels_bg_rgb(ncplane_channels(n), r, g, b);
}

// Set the current fore/background color using RGB specifications. If the
// terminal does not support directly-specified 3x8b cells (24-bit "Direct
// Color", indicated by the "RGB" terminfo capability), the provided values
// will be interpreted in some lossy fashion. None of r, g, or b may exceed 255.
// "HP-like" terminals require setting foreground and background at the same
// time using "color pairs"; notcurses will manage color pairs transparently.
API int ncplane_set_fg_rgb(struct ncplane* n, int r, int g, int b);
API int ncplane_set_bg_rgb(struct ncplane* n, int r, int g, int b);

// Same, but clipped to [0..255].
API void ncplane_set_bg_rgb_clipped(struct ncplane* n, int r, int g, int b);
API void ncplane_set_fg_rgb_clipped(struct ncplane* n, int r, int g, int b);

// Same, but with rgb assembled into a channel (i.e. lower 24 bits).
API int ncplane_set_fg(struct ncplane* n, unsigned channel);
API int ncplane_set_bg(struct ncplane* n, unsigned channel);

// Use the default color for the foreground/background.
API void ncplane_set_fg_default(struct ncplane* n);
API void ncplane_set_bg_default(struct ncplane* n);

// Set the ncplane's foreground palette index, set the foreground palette index
// bit, set it foreground-opaque, and clear the foreground default color bit.
API int ncplane_set_fg_palindex(struct ncplane* n, int idx);
API int ncplane_set_bg_palindex(struct ncplane* n, int idx);

// Set the alpha parameters for ncplane 'n'.
API int ncplane_set_fg_alpha(struct ncplane* n, int alpha);
API int ncplane_set_bg_alpha(struct ncplane* n, int alpha);

// Set the specified style bits for the ncplane 'n', whether they're actively
// supported or not.
API void ncplane_styles_set(struct ncplane* n, unsigned stylebits);

// Add the specified styles to the ncplane's existing spec.
API void ncplane_styles_on(struct ncplane* n, unsigned stylebits);

// Remove the specified styles from the ncplane's existing spec.
API void ncplane_styles_off(struct ncplane* n, unsigned stylebits);

// Return the current styling for this ncplane.
API unsigned ncplane_styles(struct ncplane* n);

// Called for each delta performed in a fade on ncp. If anything but 0 is returned,
// the fading operation ceases immediately, and that value is propagated out. If provided
// and not NULL, the faders will not themselves call notcurses_render().
typedef int (*fadecb)(struct notcurses* nc, struct ncplane* ncp, void* curry);

// Fade the ncplane out over the provided time, calling the specified function
// when done. Requires a terminal which supports direct color, or at least
// palette modification (if the terminal uses a palette, our ability to fade
// planes is limited, and affected by the complexity of the rest of the screen).
// It is not safe to resize or destroy the plane during the fadeout FIXME.
API int ncplane_fadeout(struct ncplane* n, const struct timespec* ts, fadecb fader, void* curry);

// Fade the ncplane in over the specified time. Load the ncplane with the
// target cells without rendering, then call this function. When it's done, the
// ncplane will have reached the target levels, starting from zeroes.
API int ncplane_fadein(struct ncplane* n, const struct timespec* ts, fadecb fader, void* curry);

// Pulse the plane in and out until the callback returns non-zero, relying on
// the callback 'fader' to initiate rendering. 'ts' defines the half-period
// (i.e. the transition from black to full brightness, or back again). Proper
// use involves preparing (but not rendering) an ncplane, then calling
// ncplane_pulse(), which will fade in from black to the specified colors.
API int ncplane_pulse(struct ncplane* n, const struct timespec* ts, fadecb fader, void* curry);

// Working with cells

#define CELL_TRIVIAL_INITIALIZER { .gcluster = '\0', .attrword = 0, .channels = 0, }
#define CELL_SIMPLE_INITIALIZER(c) { .gcluster = (c), .attrword = 0, .channels = 0, }
#define CELL_INITIALIZER(c, a, chan) { .gcluster = (c), .attrword = (a), .channels = (chan), }

static inline void
cell_init(cell* c){
  memset(c, 0, sizeof(*c));
}

// Breaks the UTF-8 string in 'gcluster' down, setting up the cell 'c'. Returns
// the number of bytes copied out of 'gcluster', or -1 on failure. The styling
// of the cell is left untouched, but any resources are released.
API int cell_load(struct ncplane* n, cell* c, const char* gcluster);

// cell_load(), plus blast the styling with 'attr' and 'channels'.
static inline int
cell_prime(struct ncplane* n, cell* c, const char* gcluster,
           uint32_t attr, uint64_t channels){
  c->attrword = attr;
  c->channels = channels;
  int ret = cell_load(n, c, gcluster);
  return ret;
}

// Duplicate 'c' into 'targ'. Not intended for external use; exposed for the
// benefit of unit tests.
API int cell_duplicate(struct ncplane* n, cell* targ, const cell* c);

// Release resources held by the cell 'c'.
API void cell_release(struct ncplane* n, cell* c);

#define NCSTYLE_MASK      0xffff0000ul
#define NCSTYLE_STANDOUT  0x00800000ul
#define NCSTYLE_UNDERLINE 0x00400000ul
#define NCSTYLE_REVERSE   0x00200000ul
#define NCSTYLE_BLINK     0x00100000ul
#define NCSTYLE_DIM       0x00080000ul
#define NCSTYLE_BOLD      0x00040000ul
#define NCSTYLE_INVIS     0x00020000ul
#define NCSTYLE_PROTECT   0x00010000ul
#define NCSTYLE_ITALIC    0x01000000ul

// Set the specified style bits for the cell 'c', whether they're actively
// supported or not.
static inline void
cell_styles_set(cell* c, unsigned stylebits){
  c->attrword = (c->attrword & ~NCSTYLE_MASK) | ((stylebits & NCSTYLE_MASK));
}

// Extract the style bits from the cell's attrword.
static inline unsigned
cell_styles(const cell* c){
  return c->attrword & NCSTYLE_MASK;
}

// Add the specified styles (in the LSBs) to the cell's existing spec, whether
// they're actively supported or not.
static inline void
cell_styles_on(cell* c, unsigned stylebits){
  c->attrword |= (stylebits & NCSTYLE_MASK);
}

// Remove the specified styles (in the LSBs) from the cell's existing spec.
static inline void
cell_styles_off(cell* c, unsigned stylebits){
  c->attrword &= ~(stylebits & NCSTYLE_MASK);
}

// Use the default color for the foreground.
static inline void
cell_set_fg_default(cell* c){
  channels_set_fg_default(&c->channels);
}

// Use the default color for the background.
static inline void
cell_set_bg_default(cell* c){
  channels_set_bg_default(&c->channels);
}

static inline int
cell_set_fg_alpha(cell* c, int alpha){
  return channels_set_fg_alpha(&c->channels, alpha);
}

static inline int
cell_set_bg_alpha(cell* c, int alpha){
  return channels_set_bg_alpha(&c->channels, alpha);
}

// Does the cell contain an East Asian Wide codepoint?
static inline bool
cell_double_wide_p(const cell* c){
  return (c->channels & CELL_WIDEASIAN_MASK);
}

// Is this the right half of a wide character?
static inline bool
cell_wide_right_p(const cell* c){
  return cell_double_wide_p(c) && c->gcluster == 0;
}

// Is this the left half of a wide character?
static inline bool
cell_wide_left_p(const cell* c){
  return cell_double_wide_p(c) && c->gcluster;
}

// Is the cell simple (a lone ASCII character, encoded as such)?
static inline bool
cell_simple_p(const cell* c){
  return c->gcluster < 0x80;
}

// return a pointer to the NUL-terminated EGC referenced by 'c'. this pointer
// is invalidated by any further operation on the plane 'n', so...watch out!
API const char* cell_extended_gcluster(const struct ncplane* n, const cell* c);

// True if the cell does not generate foreground pixels (i.e., the cell is
// entirely whitespace or special characters).
// FIXME do this at cell prep time and set a bit in the channels
static inline bool
cell_noforeground_p(const cell* c){
  return cell_simple_p(c) && isspace(c->gcluster);
}

static inline int
cell_load_simple(struct ncplane* n, cell* c, char ch){
  cell_release(n, c);
  c->channels &= ~CELL_WIDEASIAN_MASK;
  c->gcluster = ch;
  if(cell_simple_p(c)){
    return 1;
  }
  return -1;
}

// get the offset into the egcpool for this cell's EGC. returns meaningless and
// unsafe results if called on a simple cell.
static inline uint32_t
cell_egc_idx(const cell* c){
  return c->gcluster - 0x80;
}

// load up six cells with the EGCs necessary to draw a box. returns 0 on
// success, -1 on error. on error, any cells this function might
// have loaded before the error are cell_release()d. There must be at least
// six EGCs in gcluster.
static inline int
cells_load_box(struct ncplane* n, uint32_t attrs, uint64_t channels,
               cell* ul, cell* ur, cell* ll, cell* lr,
               cell* hl, cell* vl, const char* gclusters){
  int ulen;
  if((ulen = cell_prime(n, ul, gclusters, attrs, channels)) > 0){
    if((ulen = cell_prime(n, ur, gclusters += ulen, attrs, channels)) > 0){
      if((ulen = cell_prime(n, ll, gclusters += ulen, attrs, channels)) > 0){
        if((ulen = cell_prime(n, lr, gclusters += ulen, attrs, channels)) > 0){
          if((ulen = cell_prime(n, hl, gclusters += ulen, attrs, channels)) > 0){
            if((ulen = cell_prime(n, vl, gclusters += ulen, attrs, channels)) > 0){
              return 0;
            }
            cell_release(n, hl);
          }
          cell_release(n, lr);
        }
        cell_release(n, ll);
      }
      cell_release(n, ur);
    }
    cell_release(n, ul);
  }
  return -1;
}

static inline int
cells_rounded_box(struct ncplane* n, uint32_t attr, uint64_t channels,
                  cell* ul, cell* ur, cell* ll, cell* lr, cell* hl, cell* vl){
  return cells_load_box(n, attr, channels, ul, ur, ll, lr, hl, vl, "╭╮╰╯─│");
}

static inline int
ncplane_rounded_box(struct ncplane* n, uint32_t attr, uint64_t channels,
                    int ystop, int xstop, unsigned ctlword){
  int ret = 0;
  cell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  cell ll = CELL_TRIVIAL_INITIALIZER, lr = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if((ret = cells_rounded_box(n, attr, channels, &ul, &ur, &ll, &lr, &hl, &vl)) == 0){
    ret = ncplane_box(n, &ul, &ur, &ll, &lr, &hl, &vl, ystop, xstop, ctlword);
  }
  cell_release(n, &ul); cell_release(n, &ur);
  cell_release(n, &ll); cell_release(n, &lr);
  cell_release(n, &hl); cell_release(n, &vl);
  return ret;
}

static inline int
ncplane_rounded_box_sized(struct ncplane* n, uint32_t attr, uint64_t channels,
                          int ylen, int xlen, unsigned ctlword){
  int y, x;
  ncplane_cursor_yx(n, &y, &x);
  return ncplane_rounded_box(n, attr, channels, y + ylen - 1,
                             x + xlen - 1, ctlword);
}

static inline int
cells_double_box(struct ncplane* n, uint32_t attr, uint64_t channels,
                 cell* ul, cell* ur, cell* ll, cell* lr, cell* hl, cell* vl){
  return cells_load_box(n, attr, channels, ul, ur, ll, lr, hl, vl, "╔╗╚╝═║");
}

static inline int
ncplane_double_box(struct ncplane* n, uint32_t attr, uint64_t channels,
                   int ystop, int xstop, unsigned ctlword){
  int ret = 0;
  cell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  cell ll = CELL_TRIVIAL_INITIALIZER, lr = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if((ret = cells_double_box(n, attr, channels, &ul, &ur, &ll, &lr, &hl, &vl)) == 0){
    ret = ncplane_box(n, &ul, &ur, &ll, &lr, &hl, &vl, ystop, xstop, ctlword);
  }
  cell_release(n, &ul); cell_release(n, &ur);
  cell_release(n, &ll); cell_release(n, &lr);
  cell_release(n, &hl); cell_release(n, &vl);
  return ret;
}

static inline int
ncplane_double_box_sized(struct ncplane* n, uint32_t attr, uint64_t channels,
                         int ylen, int xlen, unsigned ctlword){
  int y, x;
  ncplane_cursor_yx(n, &y, &x);
  return ncplane_double_box(n, attr, channels, y + ylen - 1,
                            x + xlen - 1, ctlword);
}

// multimedia functionality
struct AVFrame;

// Open a visual (image or video), associating it with the specified ncplane.
// Returns NULL on any error, writing the AVError to 'averr'.
// FIXME this ought also take an ncscale_e!
API struct ncvisual* ncplane_visual_open(struct ncplane* nc, const char* file,
                                         int* averr);

// How to scale the visual in ncvisual_open_plane(). NCSCALE_NONE will open a
// plane tailored to the visual's exact needs, which is probably larger than the
// visible screen (but might be smaller). NCSCALE_SCALE scales a visual larger
// than the visible screen down, maintaining aspect ratio. NCSCALE_STRETCH
// stretches and scales the image in an attempt to fill the visible screen.
typedef enum {
  NCSCALE_NONE,
  NCSCALE_SCALE,
  NCSCALE_STRETCH,
} ncscale_e;

// Open a visual, extract a codec and parameters, and create a new plane
// suitable for its display at 'y','x'. If there is sufficient room to display
// the visual in its native size, or if NCSCALE_NONE is passed for 'style', the
// new plane will be exactly that large. Otherwise, the plane will be as large
// as possble (given the visible screen), either maintaining aspect ratio
// (NCSCALE_SCALE) or abandoning it (NCSCALE_STRETCH).
API struct ncvisual* ncvisual_open_plane(struct notcurses* nc, const char* file,
                                         int* averr, int y, int x,
                                         ncscale_e style);

// Return the plane to which this ncvisual is bound.
API struct ncplane* ncvisual_plane(struct ncvisual* ncv);

// Destroy an ncvisual. Rendered elements will not be disrupted, but the visual
// can be neither decoded nor rendered any further.
API void ncvisual_destroy(struct ncvisual* ncv);

// extract the next frame from an ncvisual. returns NULL on end of file,
// writing AVERROR_EOF to 'averr'. returns NULL on a decoding or allocation
// error, placing the AVError in 'averr'. this frame is invalidated by a
// subsequent call to ncvisual_decode(), and should not be freed by the caller.
API struct AVFrame* ncvisual_decode(struct ncvisual* nc, int* averr);

// Render the decoded frame to the associated ncplane. The frame will be scaled
// to the size of the ncplane per the ncscale_e style. A subregion of the
// frame can be specified using 'begx', 'begy', 'lenx', and 'leny'. To render
// the rectangle formed by begy x begx and the lower-right corner, zero can be
// supplied to 'leny' and 'lenx'. Zero for all four values will thus render the
// entire visual. Negative values for any of the four parameters are an error.
// It is an error to specify any region beyond the boundaries of the frame.
API int ncvisual_render(const struct ncvisual* ncv, int begy, int begx,
                        int leny, int lenx);

// If a subtitle ought be displayed at this time, return a heap-allocated copy
// of the UTF8 text.
API char* ncvisual_subtitle(const struct ncvisual* ncv);

// Called for each frame rendered from 'ncv'. If anything but 0 is returned,
// the streaming operation ceases immediately, and that value is propagated out.
typedef int (*streamcb)(struct notcurses* nc, struct ncvisual* ncv, void*);

// Shut up and display my frames! Provide as an argument to ncvisual_stream().
// If you'd like subtitles to be decoded, provide a ncplane as the curry. If the
// curry is NULL, subtitles will not be displayed.
static inline int
ncvisual_simple_streamer(struct notcurses* nc, struct ncvisual* ncv, void* curry){
  if(notcurses_render(nc)){
    return -1;
  }
  int ret = 0;
  if(curry){
    // need a cast for C++ callers
    struct ncplane* subncp = (struct ncplane*)curry;
    char* subtitle = ncvisual_subtitle(ncv);
    if(subtitle){
      if(ncplane_putstr_yx(subncp, 0, 0, subtitle) < 0){
        ret = -1;
      }
      free(subtitle);
    }
  }
  return ret;
}

// Stream the entirety of the media, according to its own timing. Blocking,
// obviously. streamer may be NULL; it is otherwise called for each frame, and
// its return value handled as outlined for stream cb. Pretty raw; beware.
// If streamer() returns non-zero, the stream is aborted, and that value is
// returned. By convention, return a positive number to indicate intentional
// abort from within streamer(). 'timescale' allows the frame duration time to
// be scaled. For a visual naturally running at 30FPS, a 'timescale' of 0.1
// will result in 300FPS, and a 'timescale' of 10 will result in 3FPS. It is an
// error to supply 'timescale' less than or equal to 0.
API int ncvisual_stream(struct notcurses* nc, struct ncvisual* ncv,
                        int* averr, float timescale, streamcb streamer,
                        void* curry);

// Blit a flat array 'data' of BGRx 32-bit values to the ncplane 'nc', offset
// from the upper left by 'placey' and 'placex'. Each row ought occupy
// 'linesize' bytes (this might be greater than lenx * 4 due to padding). A
// subregion of the input can be specified with 'begy'x'begx' and 'leny'x'lenx'.
API int ncblit_bgrx(struct ncplane* nc, int placey, int placex, int linesize,
                    const unsigned char* data, int begy, int begx,
                    int leny, int lenx);

// Blit a flat array 'data' of RGBA 32-bit values to the ncplane 'nc', offset
// from the upper left by 'placey' and 'placex'. Each row ought occupy
// 'linesize' bytes (this might be greater than lenx * 4 due to padding). A
// subregion of the input can be specified with 'begy'x'begx' and 'leny'x'lenx'.
API int ncblit_rgba(struct ncplane* nc, int placey, int placex, int linesize,
                    const unsigned char* data, int begy, int begx,
	                  int leny, int lenx);

// An ncreel is an notcurses region devoted to displaying zero or more
// line-oriented, contained panels between which the user may navigate. If at
// least one panel exists, there is an active panel. As much of the active
// panel as is possible is always displayed. If there is space left over, other
// panels are included in the display. Panels can come and go at any time, and
// can grow or shrink at any time.
//
// This structure is amenable to line- and page-based navigation via keystrokes,
// scrolling gestures, trackballs, scrollwheels, touchpads, and verbal commands.

typedef struct ncreel_options {
  // require this many rows and columns (including borders). otherwise, a
  // message will be displayed stating that a larger terminal is necessary, and
  // input will be queued. if 0, no minimum will be enforced. may not be
  // negative. note that ncreel_create() does not return error if given a
  // WINDOW smaller than these minima; it instead patiently waits for the
  // screen to get bigger.
  int min_supported_cols;
  int min_supported_rows;

  // use no more than this many rows and columns (including borders). may not be
  // less than the corresponding minimum. 0 means no maximum.
  int max_supported_cols;
  int max_supported_rows;

  // desired offsets within the surrounding WINDOW (top right bottom left) upon
  // creation / resize. an ncreel_move() operation updates these.
  int toff, roff, boff, loff;
  // is scrolling infinite (can one move down or up forever, or is an end
  // reached?). if true, 'circular' specifies how to handle the special case of
  // an incompletely-filled reel.
  bool infinitescroll;
  // is navigation circular (does moving down from the last panel move to the
  // first, and vice versa)? only meaningful when infinitescroll is true. if
  // infinitescroll is false, this must be false.
  bool circular;
  // notcurses can draw a border around the ncreel, and also around the
  // component tablets. inhibit borders by setting all valid bits in the masks.
  // partially inhibit borders by setting individual bits in the masks. the
  // appropriate attr and pair values will be used to style the borders.
  // focused and non-focused tablets can have different styles. you can instead
  // draw your own borders, or forgo borders entirely.
  unsigned bordermask; // bitfield; 1s will not be drawn (see bordermaskbits)
  uint64_t borderchan; // attributes used for ncreel border
  unsigned tabletmask; // bitfield; same as bordermask but for tablet borders
  uint64_t tabletchan; // tablet border styling channel
  uint64_t focusedchan;// focused tablet border styling channel
  uint64_t bgchannel;  // background colors
} ncreel_options;

struct nctablet;
struct ncreel;

// Create an ncreel according to the provided specifications. Returns NULL on
// failure. w must be a valid WINDOW*, to which offsets are relative. Note that
// there might not be enough room for the specified offsets, in which case the
// ncreel will be clipped on the bottom and right. A minimum number of rows
// and columns can be enforced via popts. efd, if non-negative, is an eventfd
// that ought be written to whenever ncreel_touch() updates a tablet (this
// is useful in the case of nonblocking input).
API struct ncreel* ncreel_create(struct ncplane* nc,
                                       const ncreel_options* popts,
                                       int efd);

// Returns the ncplane on which this ncreel lives.
API struct ncplane* ncreel_plane(struct ncreel* pr);

// Tablet draw callback, provided a tablet (from which the ncplane and userptr
// may be extracted), the first column that may be used, the first row that may
// be used, the first column that may not be used, the first row that may not
// be used, and a bool indicating whether output ought be clipped at the top
// (true) or bottom (false). Rows and columns are zero-indexed, and both are
// relative to the tablet's plane.
//
// Regarding clipping: it is possible that the tablet is only partially
// displayed on the screen. If so, it is either partially present on the top of
// the screen, or partially present at the bottom. In the former case, the top
// is clipped (cliptop will be true), and output ought start from the end. In
// the latter case, cliptop is false, and output ought start from the beginning.
//
// Returns the number of lines of output, which ought be less than or equal to
// maxy - begy, and non-negative (negative values might be used in the future).
typedef int (*tabletcb)(struct nctablet* t, int begx, int begy, int maxx,
                        int maxy, bool cliptop);

// Add a new nctablet to the provided ncreel, having the callback object
// opaque. Neither, either, or both of after and before may be specified. If
// neither is specified, the new tablet can be added anywhere on the reel. If
// one or the other is specified, the tablet will be added before or after the
// specified tablet. If both are specifid, the tablet will be added to the
// resulting location, assuming it is valid (after->next == before->prev); if
// it is not valid, or there is any other error, NULL will be returned.
API struct nctablet* ncreel_add(struct ncreel* pr, struct nctablet* after,
                                 struct nctablet* before, tabletcb cb,
                                 void* opaque);

// Return the number of nctablets in the ncreel.
API int ncreel_tabletcount(const struct ncreel* pr);

// Indicate that the specified nctablet has been updated in a way that would
// change its display. This will trigger some non-negative number of callbacks
// (though not in the caller's context).
API int ncreel_touch(struct ncreel* pr, struct nctablet* t);

// Delete the tablet specified by t from the ncreel specified by pr. Returns
// -1 if the tablet cannot be found.
API int ncreel_del(struct ncreel* pr, struct nctablet* t);

// Delete the active tablet. Returns -1 if there are no tablets.
API int ncreel_del_focused(struct ncreel* pr);

// Move to the specified location within the containing WINDOW.
API int ncreel_move(struct ncreel* pr, int x, int y);

// Redraw the ncreel in its entirety, for instance after
// clearing the screen due to external corruption, or a SIGWINCH.
API int ncreel_redraw(struct ncreel* pr);

// Return the focused tablet, if any tablets are present. This is not a copy;
// be careful to use it only for the duration of a critical section.
API struct nctablet* ncreel_focused(struct ncreel* pr);

// Change focus to the next tablet, if one exists
API struct nctablet* ncreel_next(struct ncreel* pr);

// Change focus to the previous tablet, if one exists
API struct nctablet* ncreel_prev(struct ncreel* pr);

// Destroy an ncreel allocated with ncreel_create(). Does not destroy the
// underlying WINDOW. Returns non-zero on failure.
API int ncreel_destroy(struct ncreel* pr);

// Returns a pointer to an user pointer associated with this nctablet.
API void* nctablet_userptr(struct nctablet* t);

// Access the ncplane associated with this nctablet, if one exists.
API struct ncplane* nctablet_ncplane(struct nctablet* t);

#define PREFIXSTRLEN 7  // Does not include a '\0' (xxx.xxU)
#define IPREFIXSTRLEN 8 //  Does not include a '\0' (xxxx.xxU)
#define BPREFIXSTRLEN 9  // Does not include a '\0' (xxxx.xxUi), i == prefix

// A bit of the nasties here to stringize our preprocessor tokens just now
// #defined, making them usable as printf(3) specifiers.
#define STRHACK1(x) #x
#define STRHACK2(x) STRHACK1(x)
#define PREFIXFMT "%" STRHACK2(PREFIXSTRLEN) "s"
#define IPREFIXFMT "%" STRHACK2(IPREFIXSTRLEN) "s"
#define BPREFIXFMT "%" STRHACK2(BPREFIXSTRLEN) "s"

// Takes an arbitrarily large number, and prints it into a fixed-size buffer by
// adding the necessary SI suffix. Usually, pass a |[B]PREFIXSTRLEN+1|-sized
// buffer to generate up to [B]PREFIXSTRLEN characters. The characteristic can
// occupy up through |mult-1| characters (3 for 1000, 4 for 1024). The mantissa
// can occupy either zero or two characters.
//
// Floating-point is never used, because an IEEE758 double can only losslessly
// represent integers through 2^53-1.
//
// 2^64-1 is 18446744073709551615, 18.45E(xa). KMGTPEZY thus suffice to handle
// a 89-bit uintmax_t. Beyond Z(etta) and Y(otta) lie lands unspecified by SI.
//
// val: value to print
// decimal: scaling. '1' if none has taken place.
// buf: buffer in which string will be generated
// omitdec: inhibit printing of all-0 decimal portions
// mult: base of suffix system (almost always 1000 or 1024)
// uprefix: character to print following suffix ('i' for kibibytes basically).
//   only printed if suffix is actually printed (input >= mult).
API const char* ncmetric(uintmax_t val, unsigned decimal, char* buf,
                         int omitdec, unsigned mult, int uprefix);

// Mega, kilo, gigafoo. Use PREFIXSTRLEN + 1.
static inline const char*
qprefix(uintmax_t val, unsigned decimal, char* buf, int omitdec){
  return ncmetric(val, decimal, buf, omitdec, 1000, '\0');
}

// Mibi, kebi, gibibytes. Use BPREFIXSTRLEN + 1.
static inline const char*
bprefix(uintmax_t val, unsigned decimal, char* buf, int omitdec){
  return ncmetric(val, decimal, buf, omitdec, 1024, 'i');
}

API void notcurses_cursor_enable(struct notcurses* nc);
API void notcurses_cursor_disable(struct notcurses* nc);

// Palette API. Some terminals only support 256 colors, but allow the full
// palette to be specified with arbitrary RGB colors. In all cases, it's more
// performant to use indexed colors, since it's much less data to write to the
// terminal. If you can limit yourself to 256 colors, that' probably best.

typedef struct palette256 {
  // We store the RGB values as a regular ol' channel
  uint32_t chans[NCPALETTESIZE];
} palette256;

// Create a new palette store. It will be initialized with notcurses's best
// knowledge of the currently configured palette.
API palette256* palette256_new(struct notcurses* nc);

// Attempt to configure the terminal with the provided palette 'p'. Does not
// transfer ownership of 'p'; palette256_free() can still be called.
API int palette256_use(struct notcurses* nc, const palette256* p);

// Manipulate entries in the palette store 'p'. These are *not* locked.
static inline int
palette256_set_rgb(palette256* p, int idx, int r, int g, int b){
  if(idx < 0 || (size_t)idx > sizeof(p->chans) / sizeof(*p->chans)){
    return -1;
  }
  return channel_set_rgb(&p->chans[idx], r, g, b);
}

static inline int
palette256_set(palette256* p, int idx, unsigned rgb){
  if(idx < 0 || (size_t)idx > sizeof(p->chans) / sizeof(*p->chans)){
    return -1;
  }
  return channel_set(&p->chans[idx], rgb);
}

static inline int
palette256_get_rgb(const palette256* p, int idx, unsigned* RESTRICT r, unsigned* RESTRICT g, unsigned* RESTRICT b){
  if(idx < 0 || (size_t)idx > sizeof(p->chans) / sizeof(*p->chans)){
    return -1;
  }
  return channel_rgb(p->chans[idx], r, g, b);
}

// Free the palette store 'p'.
API void palette256_free(palette256* p);

// Convert the plane's content to greyscale.
API void ncplane_greyscale(struct ncplane* n);

// selection widget -- an ncplane with a title header and a body section. the
// body section supports infinite scrolling up and down. the widget looks like:
//                                 ╭──────────────────────────╮
//                                 │This is the primary header│
//   ╭──────────────────────this is the secondary header──────╮
//   │                                                        │
//   │ option1   Long text #1                                 │
//   │ option2   Long text #2                                 │
//   │ option3   Long text #3                                 │
//   │ option4   Long text #4                                 │
//   │ option5   Long text #5                                 │
//   │ option6   Long text #6                                 │
//   │                                                        │
//   ╰────────────────────────────────────here's the footer───╯
//
// At all times, exactly one item is selected.

struct selector_item {
  char* option;
  char* desc;
  size_t opcolumns;   // filled in by library
  size_t desccolumns; // filled in by library
};

typedef struct selector_options {
  char* title; // title may be NULL, inhibiting riser, saving two rows.
  char* secondary; // secondary may be NULL
  char* footer; // footer may be NULL
  struct selector_item* items; // initial items and descriptions
  unsigned itemcount; // number of initial items and descriptions
  // default item (selected at start), must be < itemcount unless 'itemcount'
  // is 0, in which case 'defidx' must also be 0
  unsigned defidx;
  // maximum number of options to display at once, 0 to use all available space
  unsigned maxdisplay;
  // exhaustive styling options
  uint64_t opchannels;   // option channels
  uint64_t descchannels; // description channels
  uint64_t titlechannels;// title channels
  uint64_t footchannels; // secondary and footer channels
  uint64_t boxchannels;  // border channels
  uint64_t bgchannels;   // background channels, used only in body
} selector_options;

struct ncselector;

API struct ncselector* ncselector_create(struct ncplane* n, int y, int x,
                                         const selector_options* opts);

API int ncselector_additem(struct ncselector* n, const struct selector_item* item);
API int ncselector_delitem(struct ncselector* n, const char* item);

// Return reference to the selected option, or NULL if there are no items.
API const char* ncselector_selected(const struct ncselector* n);

// Return a reference to the ncselector's underlying ncplane.
API struct ncplane* ncselector_plane(struct ncselector* n);

// Move up or down in the list. A reference to the newly-selected item is
// returned, or NULL if there are no items in the list.
API const char* ncselector_previtem(struct ncselector* n);
API const char* ncselector_nextitem(struct ncselector* n);

// Offer the input to the ncselector. If it's relevant, this function returns
// true, and the input ought not be processed further. If it's irrelevant to
// the selector, false is returned. Relevant inputs include:
//  * a mouse click on an item
//  * a mouse scrollwheel event
//  * a mouse click on the scrolling arrows
//  * a mouse click outside of an unrolled menu (the menu is rolled up)
//  * up, down, pgup, or pgdown on an unrolled menu (navigates among items)
API bool ncselector_offer_input(struct ncselector* n, const struct ncinput* nc);

// Destroy the ncselector. If 'item' is not NULL, the last selected option will
// be strdup()ed and assigned to '*item' (and must be free()d by the caller).
API void ncselector_destroy(struct ncselector* n, char** item);

// Menus. Horizontal menu bars are supported, on the top and/or bottom rows.
// If the menu bar is longer than the screen, it will be only partially
// visible. Menus may be either visible or invisible by default. In the event of
// a screen resize, menus will be automatically moved/resized. Elements can be
// dynamically enabled or disabled at all levels (menu, section, and item),

struct ncmenu_item {
  char* desc;           // utf-8 menu item, NULL for horizontal separator
  ncinput shortcut;     // shortcut, all should be distinct
};

struct ncmenu_section {
  char* name;             // utf-8 c string
  int itemcount;
  struct ncmenu_item* items;
  ncinput shortcut;       // shortcut, will be underlined if present in name
};

typedef struct ncmenu_options {
  bool bottom;              // on the bottom row, as opposed to top row
  bool hiding;              // hide the menu when not being used
  struct ncmenu_section* sections; // array of 'sectioncount' menu_sections
  int sectioncount;         // must be positive
  uint64_t headerchannels;  // styling for header
  uint64_t sectionchannels; // styling for sections
} ncmenu_options;

// Create a menu with the specified options. Menus are currently bound to an
// overall notcurses object (as opposed to a particular plane), and are
// implemented as ncplanes kept atop other ncplanes.
API struct ncmenu* ncmenu_create(struct notcurses* nc, const ncmenu_options* opts);

// Unroll the specified menu section, making the menu visible if it was
// invisible, and rolling up any menu section that is already unrolled.
API int ncmenu_unroll(struct ncmenu* n, int sectionidx);

// Roll up any unrolled menu section, and hide the menu if using hiding.
API int ncmenu_rollup(struct ncmenu* n);

// Unroll the previous/next section (relative to current unrolled). If no
// section is unrolled, the first section will be unrolled.
API int ncmenu_nextsection(struct ncmenu* n);
API int ncmenu_prevsection(struct ncmenu* n);

// Move to the previous/next item within the currently unrolled section. If no
// section is unrolled, the first section will be unrolled.
API int ncmenu_nextitem(struct ncmenu* n);
API int ncmenu_previtem(struct ncmenu* n);

// Return the selected item description, or NULL if no section is unrolled. If
// 'ni' is not NULL, and the selected item has a shortcut, 'ni' will be filled
// in with that shortcut--this can allow faster matching.
API const char* ncmenu_selected(const struct ncmenu* n, struct ncinput* ni);

// Return the ncplane backing this ncmenu.
API struct ncplane* ncmenu_plane(struct ncmenu* n);

// Offer the input to the ncmenu. If it's relevant, this function returns true,
// and the input ought not be processed further. If it's irrelevant to the
// menu, false is returned. Relevant inputs include:
//  * mouse movement over a hidden menu
//  * a mouse click on a menu section (the section is unrolled)
//  * a mouse click outside of an unrolled menu (the menu is rolled up)
//  * left or right on an unrolled menu (navigates among sections)
//  * up or down on an unrolled menu (navigates among items)
//  * escape on an unrolled menu (the menu is rolled up)
API bool ncmenu_offer_input(struct ncmenu* n, const struct ncinput* nc);

// Destroy a menu created with ncmenu_create().
API int ncmenu_destroy(struct ncmenu* n);

// Plots. Given a rectilinear area, an ncplot can graph samples along some axis.
// There is some underlying independent variable--this could be measurement
// number, or measurement time. Samples are tagged with this variable, which
// should never fall, but may grow non-monotonically. The desired range in terms
// of the underlying independent variable is provided at creation time. The
// desired domain can be specified, or can be autosolved. Granularity of the
// dependent variable depends on glyph selection.
//
// For instance, perhaps we're sampling load as a time series. We want to
// display an hour's worth of samples in 40 columns and 5 rows. We define the
// x-axis to be the independent variable, time. We'll stamp at second
// granularity. In this case, there are 60 * 60 == 3600 total elements in the
// range. Each column will thus cover a 90s span. Using vertical blocks (the
// most granular glyph), we have 8 * 5 == 40 levels of domain. If we report the
// following samples, starting at 0, using autosolving, we will observe:
//
// 60   -- 1%       |domain:   1--1, 0: 20 levels
// 120  -- 50%      |domain:  1--50, 0: 0 levels, 1: 40 levels
// 180  -- 50%      |domain:  1--50, 0: 0 levels, 1: 40 levels, 2: 40 levels
// 240  -- 100%     |domain:  1--75, 0: 1, 1: 27, 2: 40
// 271  -- 100%     |domain: 1--100, 0: 0, 1: 20, 2: 30, 3: 40
// 300  -- 25%      |domain:  1--75, 0: 0, 1: 27, 2: 40, 3: 33
//
// At the end, we have data in 4 90s spans: [0--89], [90--179], [180--269], and
// [270--359]. The first two spans have one sample each, while the second two
// have two samples each. Samples within a span are averaged (FIXME we could
// probably do better), so the results are 0, 50, 75, and 62.5. Scaling each of
// these out of 90 and multiplying by 40 gets our resulting levels. The final
// domain is 75 rather than 100 due to the averaging of 100+25/2->62.5 in the
// third span, at which point the maximum span value is once again 75.
//
// The 20 levels at first is a special case. When the domain is only 1 unit,
// and autoscaling is in play, assign 50%.
typedef struct ncplot_options {
  // styling of the maximum and minimum levels. linear interpolation will be
  // applied across the domain between these two.
  uint64_t maxchannel;
  uint64_t minchannel;
  // FIXME select braille, block, etc
  // FIXME give parameters for variables
} ncplot_options;

#undef API

#ifdef __cplusplus
} // extern "C"
#endif

#endif
