#ifndef NOTCURSES_NOTCURSES
#define NOTCURSES_NOTCURSES

#include <time.h>
#include <wchar.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#define RESTRICT
#include <libavutil/pixdesc.h>
#include <libavutil/avconfig.h>
#include <libavcodec/avcodec.h> // ffmpeg doesn't reliably "C"-guard itself
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
// used unchanged. If its alpha is 11, its foreground color is derived entirely
// from cells underneath it. Otherwise, the result will be a composite.
// Likewise for the background. If the bottom of a coordinate's zbuffer is
// reached with a cumulative alpha of zero, the default is used. In this way,
// a terminal configured with transparent background can be supported through
// multiple occluding ncplanes.
typedef struct cell {
  // These 32 bits are either a single-byte, single-character grapheme cluster
  // (values 0--0x7f), or an offset into a per-ncplane attached pool of
  // varying-length UTF-8 grapheme clusters. This pool may thus be up to 32MB.
  uint32_t gcluster;          // 1 * 4b -> 4b
  // CELL_STYLE_* attributes (16 bits) + 16 reserved bits
  uint32_t attrword;          // + 4b -> 8b
  // (channels & 0x8000000000000000ull): left half of wide character
  // (channels & 0x4000000000000000ull): foreground is *not* "default color"
  // (channels & 0x3000000000000000ull): foreground alpha (2 bits)
  // (channels & 0x0f00000000000000ull): reserved, must be 0
  // (channels & 0x00ffffff00000000ull): foreground in 3x8 RGB (rrggbb)
  // (channels & 0x0000000080000000ull): right half of wide character
  // (channels & 0x0000000040000000ull): background is *not* "default color"
  // (channels & 0x0000000030000000ull): background alpha (2 bits)
  // (channels & 0x000000000f000000ull): reserved, must be 0
  // (channels & 0x0000000000ffffffull): background in 3x8 RGB (rrggbb)
  // At render time, these 24-bit values are quantized down to terminal
  // capabilities, if necessary. There's a clear path to 10-bit support should
  // we one day need it, but keep things cagey for now. "default color" is
  // best explained by color(3NCURSES). ours is the same concept. until the
  // "not default color" bit is set, any color you load will be ignored.
  uint64_t channels;          // + 8b == 16b
} cell;

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
} notcurses_options;

// Initialize a notcurses context on the connected terminal at 'fp'. 'fp' must
// be a tty. You'll usually want stdout. Returns NULL on error, including any
// failure to initialize terminfo.
API struct notcurses* notcurses_init(const notcurses_options* opts, FILE* fp);

// Destroy a notcurses context.
API int notcurses_stop(struct notcurses* nc);

// Make the physical screen match the virtual screen. Changes made to the
// virtual screen (i.e. most other calls) will not be visible until after a
// successful call to notcurses_render().
API int notcurses_render(struct notcurses* nc);

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
// In the case of a valid read, a positive value is returned corresponding to
// the number of bytes in the UTF-8 character, or '1' for all special keys.
// 0 is returned to indicate that no input was available, but only by
// notcurses_getc(). Otherwise (including on EOF) -1 is returned.

#define suppuabize(w) ((w) + 0x100000)

// is this wide character a Supplementary Private Use Area-B codepoint?
static inline bool
wchar_supppuab_p(wchar_t w){
  return w >= 0x100000 && w <= 0x10fffd;
}

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

// See ppoll(2) for more detail. Provide a NULL 'ts' to block at length, a 'ts'
// of 0 for non-blocking operation, and otherwise a timespec to bound blocking.
// Signals in sigmask (less several we handle internally) will be atomically
// masked and unmasked per ppoll(2). It should generally contain all signals.
// Returns a single Unicode code point, or (wchar_t)-1 on error. 'sigmask' may
// be NULL.
API int notcurses_getc(struct notcurses* n, const struct timespec* ts, sigset_t* sigmask);

static inline int
notcurses_getc_nblock(struct notcurses* n){
  sigset_t sigmask;
  sigfillset(&sigmask);
  struct timespec ts = { .tv_sec = 0, .tv_nsec = 0 };
  return notcurses_getc(n, &ts, &sigmask);
}

static inline wchar_t
notcurses_getc_blocking(struct notcurses* n){
  sigset_t sigmask;
  sigemptyset(&sigmask);
  return notcurses_getc(n, NULL, &sigmask);
}

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
// origin is always at the uppermost, leftmost cell of the screen.
API struct ncplane* notcurses_stdplane(struct notcurses* nc);
API const struct ncplane* notcurses_stdplane_const(const struct notcurses* nc);

// Create a new ncplane at the specified offset (relative to the standard plane)
// and the specified size. The number of rows and columns must both be positive.
// This plane is initially at the top of the z-buffer, as if ncplane_move_top()
// had been called on it. The void* 'opaque' can be retrieved (and reset) later.
API struct ncplane* notcurses_newplane(struct notcurses* nc, int rows, int cols,
                                       int yoff, int xoff, void* opaque);

// Returns a 16-bit bitmask of supported curses-style attributes
// (CELL_STYLE_UNDERLINE, CELL_STYLE_BOLD, etc.) The attribute is only
// indicated as supported if the terminal can support it together with color.
// For more information, see the "ncv" capability in terminfo(5).
API unsigned notcurses_supported_styles(const struct notcurses* nc);

// Returns the number of simultaneous colors claimed to be supported, or 1 if
// there is no color support. Note that several terminal emulators advertise
// more colors than they actually support, downsampling internally.
API int notcurses_palette_size(const struct notcurses* nc);

typedef struct ncstats {
  uint64_t renders;          // number of notcurses_render() runs
  uint64_t render_bytes;     // bytes emitted to ttyfp
  int64_t render_max_bytes;  // max bytes emitted for a frame
  int64_t render_min_bytes;  // min bytes emitted for a frame
  uint64_t render_ns;        // nanoseconds spent in notcurses_render()
  int64_t render_max_ns;     // max ns spent in notcurses_render()
  int64_t render_min_ns;     // min ns spent in successful notcurses_render()
  uint64_t cellelisions;     // cells we elided entirely thanks to damage maps
  uint64_t cellemissions;    // cells we emitted due to inferred damage
  uint64_t fbbytes;          // total bytes devoted to all active framebuffers
  uint64_t fgelisions;       // RGB fg elision count
  uint64_t fgemissions;      // RGB fg emissions
  uint64_t bgelisions;       // RGB bg elision count
  uint64_t bgemissions;      // RGB bg emissions
  uint64_t defaultelisions;  // default color was emitted
  uint64_t defaultemissions; // default color was elided
} ncstats;

// Acquire a snapshot of the notcurses object's stats.
API void notcurses_stats(const struct notcurses* nc, ncstats* stats);

// Resize the specified ncplane. The four parameters 'keepy', 'keepx',
// 'keepleny', and 'keeplenx' define a subset of the ncplane to keep,
// unchanged. This may be a section of size 0, though none of these four
// parameters may be negative. 'keepx' and 'keepy' are relative to the ncplane.
// They must specify a coordinate within the ncplane's totality. 'yoff' and
// 'xoff' are relative to 'keepy' and 'keepx', and place the upper-left corner
// of the resized ncplane. Finally, 'ylen' and 'xlen' are the dimensions of the
// ncplane after resizing. 'ylen' must be greater than or equal to 'keepleny',
// and 'xlen' must be greater than or equal to 'keeplenx'. It is an error to
// attempt to resize the standard plane. If either of 'keepy' or 'keepx' is
// non-zero, both must be non-zero.
//
// Essentially, the kept material does not move. It serves to anchor the
// resized plane. If there is no kept material, the plane can move freely:
// it is possible to implement ncplane_move() in terms of ncplane_resize().
API int ncplane_resize(struct ncplane* n, int keepy, int keepx, int keepleny,
                       int keeplenx, int yoff, int xoff, int ylen, int xlen);

// Destroy the specified ncplane. None of its contents will be visible after
// the next call to notcurses_render(). It is an error to attempt to destroy
// the standard plane.
API int ncplane_destroy(struct ncplane* ncp);

// Set the ncplane's default cell to this cell. If defined, it will be rendered
// anywhere that the ncplane's gcluster is 0. Erasing the ncplane does not
// reset the default cell; this function must instead be called with a zero c.
API int ncplane_set_default(struct ncplane* ncp, const cell* c);

// Extract the ncplane's default cell into 'c'.
API int ncplane_default(struct ncplane* ncp, cell* c);

// Move this plane relative to the standard plane. It is an error to attempt to
// move the standard plane.
API int ncplane_move_yx(struct ncplane* n, int y, int x);

// Get the origin of this plane relative to the standard plane.
API void ncplane_yx(const struct ncplane* n, int* RESTRICT y, int* RESTRICT x);

// Splice ncplane 'n' out of the z-buffer, and reinsert it at the top or bottom.
API int ncplane_move_top(struct ncplane* n);
API int ncplane_move_bottom(struct ncplane* n);

// Splice ncplane 'n' out of the z-buffer, and reinsert it below 'below'.
API int ncplane_move_below(struct ncplane* RESTRICT n, struct ncplane* RESTRICT below);

// Splice ncplane 'n' out of the z-buffer, and reinsert it above 'above'.
API int ncplane_move_above(struct ncplane* RESTRICT n, struct ncplane* RESTRICT above);

// Retrieve the cell at the cursor location on the specified plane, returning
// it in 'c'. This copy is safe to use until the ncplane is destroyed/erased.
API int ncplane_at_cursor(struct ncplane* n, cell* c);

// Manipulate the opaque user pointer associated with this plane.
// ncplane_set_userptr() returns the previous userptr after replacing
// it with 'opaque'. the others simply return the userptr.
API void* ncplane_set_userptr(struct ncplane* n, void* opaque);
API void* ncplane_userptr(struct ncplane* n);
API const void* ncplane_userptr_const(const struct ncplane* n);

// Returns the dimensions of this ncplane.
API void ncplane_dim_yx(const struct ncplane* n, int* RESTRICT rows,
                        int* RESTRICT cols);

// Return our current idea of the terminal dimensions in rows and cols.
static inline void
notcurses_term_dim_yx(const struct notcurses* n, int* RESTRICT rows,
                      int* RESTRICT cols){
  ncplane_dim_yx(notcurses_stdplane_const(n), rows, cols);
}

// Move the cursor to the specified position (the cursor needn't be visible).
// Returns -1 on error, including negative parameters, or ones exceeding the
// plane's dimensions.
API int ncplane_cursor_move_yx(struct ncplane* n, int y, int x);

// Get the current position of the cursor within n. y and/or x may be NULL.
API void ncplane_cursor_yx(const struct ncplane* n, int* RESTRICT y,
                           int* RESTRICT x);

// Replace the cell underneath the cursor with the provided cell 'c', and
// advance the cursor by the width of the cell (but not past the end of the
// plane). On success, returns the number of columns the cursor was advanced.
// On failure, -1 is returned.
API int ncplane_putc(struct ncplane* n, const cell* c);

// Call ncplane_putc() after successfully moving to y, x on the specified plane.
static inline int
ncplane_putc_yx(struct ncplane* n, int y, int x, const cell* c){
  if(ncplane_cursor_move_yx(n, y, x)){
    return -1;
  }
  return ncplane_putc(n, c);
}

// Replace the cell underneath the cursor with the provided 7-bit char 'c'.
// Advance the cursor by 1. On success, returns 1. On failure, returns -1.
// This works whether the underlying char is signed or unsigned.
API int ncplane_putsimple(struct ncplane* n, char c);

// Call ncplane_simple() after successfully moving to y, x.
static inline int
ncplane_putsimple_yx(struct ncplane* n, int y, int x, char c){
  if(ncplane_cursor_move_yx(n, y, x)){
    return -1;
  }
  return ncplane_putsimple(n, c);
}

// Replace the cell underneath the cursor with the provided wide char 'w'.
// Advance the cursor by the character's width as reported by wcwidth(). On
// success, returns 1. On failure, returns -1.
API int ncplane_putwc(struct ncplane* n, wchar_t w);

// Call ncplane_putwc() after successfully moving to y, x.
static inline int
ncplane_putwc_yx(struct ncplane* n, int y, int x, wchar_t w){
  if(ncplane_cursor_move_yx(n, y, x)){
    return -1;
  }
  return ncplane_putwc(n, w);
}

// Replace the cell underneath the cursor with the provided EGC, using the
// specified 'attr' and 'channels' for styling, and advance the cursor by the
// width of the cluster (but not past the end of the plane). On success, returns
// the number of columns the cursor was advanced. On failure, -1 is returned.
// The number of bytes converted from gclust is written to 'sbytes' if non-NULL.
API int ncplane_putegc(struct ncplane* n, const char* gclust, uint32_t attr,
                       uint64_t channels, int* sbytes);

// Call ncplane_putegc() after successfully moving to y, x.
static inline int
ncplane_putegc_yx(struct ncplane* n, int y, int x, const char* gclust, uint32_t attr,
                  uint64_t channels, int* sbytes){
  if(ncplane_cursor_move_yx(n, y, x)){
    return -1;
  }
  return ncplane_putegc(n, gclust, attr, channels, sbytes);
}

#define WCHAR_MAX_UTF8BYTES 6

// ncplane_putegc(), but following a conversion from wchar_t to UTF-8 multibyte.
static inline int
ncplane_putwegc(struct ncplane* n, const wchar_t* gclust, uint32_t attr,
                uint64_t channels, int* sbytes){
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
  int ret = ncplane_putegc(n, mbstr, attr, channels, sbytes);
  free(mbstr);
  return ret;
}

// Call ncplane_putwegc() after successfully moving to y, x.
static inline int
ncplane_putwegc_yx(struct ncplane* n, int y, int x, const wchar_t* gclust,
                   uint32_t attr, uint64_t channels, int* sbytes){
  if(ncplane_cursor_move_yx(n, y, x)){
    return -1;
  }
  return ncplane_putwegc(n, gclust, attr, channels, sbytes);
}

// Write a series of EGCs to the current location, using the current style.
// They will be interpreted as a series of columns (according to the definition
// of ncplane_putc()). Advances the cursor by some positive number of cells
// (though not beyond the end of the plane); this number is returned on success.
// On error, a non-positive number is returned, indicating the number of cells
// which were written before the error.
API int ncplane_putstr(struct ncplane* n, const char* gclustarr);

// Alignment within the ncplane. Left/right-justified, or centered.
typedef enum {
  NCALIGN_LEFT,
  NCALIGN_CENTER,
  NCALIGN_RIGHT,
} ncalign_e;

API int ncplane_putstr_aligned(struct ncplane* n, int y, const char* s,
                               ncalign_e atype);

static inline int
ncplane_putstr_yx(struct ncplane* n, int y, int x, const char* gclustarr){
  if(ncplane_cursor_move_yx(n, y, x)){
    return -1;
  }
  return ncplane_putstr(n, gclustarr);
}

// ncplane_putstr(), but following a conversion from wchar_t to UTF-8 multibyte.
static inline int
ncplane_putwstr(struct ncplane* n, const wchar_t* gclustarr){
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
  int ret = ncplane_putstr(n, mbstr);
  free(mbstr);
  return ret;
}

API int ncplane_putwstr_aligned(struct ncplane* n, int y,
                                const wchar_t* gclustarr, ncalign_e atype);

static inline int
ncplane_putwstr_yx(struct ncplane* n, int y, int x, const wchar_t* gclustarr){
  if(ncplane_cursor_move_yx(n, y, x)){
    return -1;
  }
  return ncplane_putwstr(n, gclustarr);
}

// The ncplane equivalents of printf(3) and vprintf(3).
API int ncplane_printf(struct ncplane* n, const char* format, ...)
  __attribute__ ((format (printf, 2, 3)));

API int ncplane_vprintf(struct ncplane* n, const char* format, va_list ap);

static inline int
ncplane_vprintf_yx(struct ncplane* n, int y, int x, const char* format, va_list ap){
  if(ncplane_cursor_move_yx(n, y, x)){
    return -1;
  }
  return ncplane_vprintf(n, format, ap);
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

// Erase every cell in the ncplane, resetting all attributes to normal, all
// colors to the default color, and all cells to undrawn. All cells associated
// with this ncplane is invalidated, and must not be used after the call,
// excluding the default cell.
API void ncplane_erase(struct ncplane* n);

#define CELL_WIDEASIAN_MASK    0x8000000080000000ull
#define CELL_FGDEFAULT_MASK    0x4000000000000000ull
#define CELL_FG_MASK           0x00ffffff00000000ull
#define CELL_BGDEFAULT_MASK    0x0000000040000000ull
#define CELL_BG_MASK           0x0000000000ffffffull
#define CELL_ALPHA_MASK        0x0000000030000000ull
#define CELL_ALPHA_SHIFT       28u
#define CELL_ALPHA_TRANS       3
#define CELL_ALPHA_OPAQUE      0

// These lowest-level functions manipulate a 64-bit channel encoding directly.
// Users will typically manipulate ncplane and cell channels through those APIs,
// rather than calling these directly.

// Extract the 8-bit red component from a 32-bit channel.
static inline unsigned
channel_get_r(unsigned channel){
  return (channel & 0xff0000u) >> 16u;
}

// Extract the 8-bit green component from a 32-bit channel.
static inline unsigned
channel_get_g(unsigned channel){
  return (channel & 0x00ff00u) >> 8u;
}

// Extract the 8-bit blue component from a 32-bit channel.
static inline unsigned
channel_get_b(unsigned channel){
  return (channel & 0x0000ffu);
}

// Extract the three 8-bit R/G/B components from a 32-bit channel.
static inline unsigned
channel_get_rgb(unsigned channel, unsigned* RESTRICT r, unsigned* RESTRICT g,
                unsigned* RESTRICT b){
  *r = channel_get_r(channel);
  *g = channel_get_g(channel);
  *b = channel_get_b(channel);
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
  c |= CELL_BGDEFAULT_MASK;
  const uint64_t mask = CELL_BGDEFAULT_MASK | CELL_BG_MASK;
  *channel = (*channel & ~mask) | c;
  return 0;
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
channel_get_alpha(unsigned channel){
  return (channel & CELL_ALPHA_MASK) >> CELL_ALPHA_SHIFT;
}

// Set the 2-bit alpha component of the 32-bit channel.
static inline int
channel_set_alpha(unsigned* channel, int alpha){
  if(alpha < CELL_ALPHA_OPAQUE || alpha > CELL_ALPHA_TRANS){
    return -1;
  }
  *channel = (alpha << CELL_ALPHA_SHIFT) | (*channel & ~CELL_ALPHA_MASK);
  return 0;
}

// Is this channel using the "default color" rather than its RGB?
static inline bool
channel_default_p(unsigned channel){
  return !(channel & CELL_BGDEFAULT_MASK);
}

// Mark the channel as using its default color.
static inline unsigned
channel_set_default(unsigned* channel){
  return *channel &= ~CELL_BGDEFAULT_MASK;
}

// Extract the 32-bit background channel from a channel pair.
static inline unsigned
channels_get_bchannel(uint64_t channels){
  return channels & 0xfffffffflu;
}

// Extract the 32-bit foreground channel from a channel pair.
static inline unsigned
channels_get_fchannel(uint64_t channels){
  return channels_get_bchannel(channels >> 32u);
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
channels_get_fg(uint64_t channels){
  return channels_get_fchannel(channels) & CELL_BG_MASK;
}

// Extract 24 bits of background RGB from 'channels', shifted to LSBs.
static inline unsigned
channels_get_bg(uint64_t channels){
  return channels_get_bchannel(channels) & CELL_BG_MASK;
}

// Extract 2 bits of foreground alpha from 'channels', shifted to LSBs.
static inline unsigned
channels_get_fg_alpha(uint64_t channels){
  return channel_get_alpha(channels_get_fchannel(channels));
}

// Extract 2 bits of background alpha from 'channels', shifted to LSBs.
static inline unsigned
channels_get_bg_alpha(uint64_t channels){
  return channel_get_alpha(channels_get_bchannel(channels));
}

// Extract 24 bits of foreground RGB from 'channels', split into subchannels.
static inline unsigned
channels_get_fg_rgb(uint64_t channels, unsigned* r, unsigned* g, unsigned* b){
  return channel_get_rgb(channels_get_fchannel(channels), r, g, b);
}

// Extract 24 bits of background RGB from 'channels', split into subchannels.
static inline unsigned
channels_get_bg_rgb(uint64_t channels, unsigned* r, unsigned* g, unsigned* b){
  return channel_get_rgb(channels_get_bchannel(channels), r, g, b);
}

// Set the r, g, and b channels for the foreground component of this 64-bit
// 'channels' variable, and mark it as not using the default color.
static inline int
channels_set_fg_rgb(uint64_t* channels, int r, int g, int b){
  unsigned channel = channels_get_fchannel(*channels);
  if(channel_set_rgb(&channel, r, g, b) < 0){
    return -1;
  }
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
  return 0;
}

// Set the r, g, and b channels for the background component of this 64-bit
// 'channels' variable, and mark it as not using the default color.
static inline int
channels_set_bg_rgb(uint64_t* channels, int r, int g, int b){
  unsigned channel = channels_get_bchannel(*channels);
  if(channel_set_rgb(&channel, r, g, b) < 0){
    return -1;
  }
  *channels = (*channels & 0xffffffff00000000llu) | channel;
  return 0;
}

// Same, but set an assembled 24 bits of rgb at once.
static inline int
channels_set_fg(uint64_t* channels, unsigned rgb){
  unsigned channel = channels_get_fchannel(*channels);
  if(channel_set(&channel, rgb) < 0){
    return -1;
  }
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
  return 0;
}

static inline int
channels_set_bg(uint64_t* channels, unsigned rgb){
  unsigned channel = channels_get_bchannel(*channels);
  if(channel_set(&channel, rgb) < 0){
    return -1;
  }
  *channels = (*channels & 0xffffffff00000000llu) | channel;
  return 0;
}

// Set the 2-bit alpha component of the foreground channel.
static inline int
channels_set_fg_alpha(uint64_t* channels, int alpha){
  unsigned channel = channels_get_fchannel(*channels);
  if(channel_set_alpha(&channel, alpha) < 0){
    return -1;
  }
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
  return 0;
}

// Set the 2-bit alpha component of the background channel.
static inline int
channels_set_bg_alpha(uint64_t* channels, int alpha){
  unsigned channel = channels_get_bchannel(*channels);
  if(channel_set_alpha(&channel, alpha) < 0){
    return -1;
  }
  *channels = (*channels & 0xffffffff00000000llu) | channel;
  return 0;
}

// Is the foreground using the "default foreground color"?
static inline bool
channels_fg_default_p(uint64_t channels){
  return channel_default_p(channels_get_fchannel(channels));
}

// Is the background using the "default background color"? The "default
// background color" must generally be used to take advantage of
// terminal-effected transparency.
static inline bool
channels_bg_default_p(uint64_t channels){
  return channel_default_p(channels_get_bchannel(channels));
}

// Mark the foreground channel as using its default color.
static inline uint64_t
channels_set_fg_default(uint64_t* channels){
  unsigned channel = channels_get_fchannel(*channels);
  channel_set_default(&channel);
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
  return *channels;
}

// Mark the foreground channel as using its default color.
static inline uint64_t
channels_set_bg_default(uint64_t* channels){
  unsigned channel = channels_get_bchannel(*channels);
  channel_set_default(&channel);
  *channels = (*channels & 0xffffffff00000000llu) | channel;
  return *channels;
}

// Extract the 32-bit background channel from a cell.
static inline unsigned
cell_get_bchannel(const cell* cl){
  return channels_get_bchannel(cl->channels);
}

// Extract the 32-bit foreground channel from a cell.
static inline unsigned
cell_get_fchannel(const cell* cl){
  return channels_get_fchannel(cl->channels);
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

// Extract 24 bits of foreground RGB from 'cell', shifted to LSBs.
static inline unsigned
cell_get_fg(const cell* cl){
  return channels_get_fg(cl->channels);
}

// Extract 24 bits of background RGB from 'cell', shifted to LSBs.
static inline unsigned
cell_get_bg(const cell* cl){
  return channels_get_bg(cl->channels);
}

// Extract 2 bits of foreground alpha from 'cell', shifted to LSBs.
static inline unsigned
cell_get_fg_alpha(const cell* cl){
  return channels_get_fg_alpha(cl->channels);
}

// Extract 2 bits of background alpha from 'cell', shifted to LSBs.
static inline unsigned
cell_get_bg_alpha(const cell* cl){
  return channels_get_bg_alpha(cl->channels);
}

// Extract 24 bits of foreground RGB from 'cell', split into subcell.
static inline unsigned
cell_get_fg_rgb(const cell* cl, unsigned* r, unsigned* g, unsigned* b){
  return channels_get_fg_rgb(cl->channels, r, g, b);
}

// Extract 24 bits of background RGB from 'cell', split into subcell.
static inline unsigned
cell_get_bg_rgb(const cell* cl, unsigned* r, unsigned* g, unsigned* b){
  return channels_get_bg_rgb(cl->channels, r, g, b);
}

// Set the r, g, and b cell for the foreground component of this 64-bit
// 'cell' variable, and mark it as not using the default color.
static inline int
cell_set_fg_rgb(cell* cl, int r, int g, int b){
  return channels_set_fg_rgb(&cl->channels, r, g, b);
}

// Set the r, g, and b cell for the background component of this 64-bit
// 'cell' variable, and mark it as not using the default color.
static inline int
cell_set_bg_rgb(cell* cl, int r, int g, int b){
  return channels_set_bg_rgb(&cl->channels, r, g, b);
}

// Same, but with rgb assembled into a channel (i.e. lower 24 bits).
static inline int
cell_set_fg(cell* c, uint32_t channel){
  return channels_set_fg(&c->channels, channel);
}

static inline int
cell_set_bg(cell* c, uint32_t channel){
  return channels_set_bg(&c->channels, channel);
}

// Is the foreground using the "default foreground color"?
static inline bool
cell_fg_default_p(const cell* cl){
  return channels_fg_default_p(cl->channels);
}

// Is the background using the "default background color"? The "default
// background color" must generally be used to take advantage of
// terminal-effected transparency.
static inline bool
cell_bg_default_p(const cell* cl){
  return channels_bg_default_p(cl->channels);
}

// Get the current channels or attribute word for ncplane 'n'.
API uint64_t ncplane_get_channels(const struct ncplane* n);
API uint32_t ncplane_get_attr(const struct ncplane* n);

// Extract the 32-bit working background channel from an ncplane.
static inline unsigned
ncplane_get_bchannel(const struct ncplane* nc){
  return channels_get_bchannel(ncplane_get_channels(nc));
}

// Extract the 32-bit working foreground channel from an ncplane.
static inline unsigned
ncplane_get_fchannel(const struct ncplane* nc){
  return channels_get_fchannel(ncplane_get_channels(nc));
}

// Extract 24 bits of working foreground RGB from an ncplane, shifted to LSBs.
static inline unsigned
ncplane_get_fg(const struct ncplane* nc){
  return channels_get_fg(ncplane_get_channels(nc));
}

// Extract 24 bits of working background RGB from an ncplane, shifted to LSBs.
static inline unsigned
ncplane_get_bg(const struct ncplane* nc){
  return channels_get_bg(ncplane_get_channels(nc));
}

// Extract 2 bits of foreground alpha from 'struct ncplane', shifted to LSBs.
static inline unsigned
ncplane_get_fg_alpha(const struct ncplane* nc){
  return channels_get_fg_alpha(ncplane_get_channels(nc));
}

// Extract 2 bits of background alpha from 'struct ncplane', shifted to LSBs.
static inline unsigned
ncplane_get_bg_alpha(const struct ncplane* nc){
  return channels_get_bg_alpha(ncplane_get_channels(nc));
}

// Extract 24 bits of foreground RGB from 'n', split into subcomponents.
static inline unsigned
ncplane_get_fg_rgb(const struct ncplane* n, unsigned* r, unsigned* g, unsigned* b){
  return channels_get_fg_rgb(ncplane_get_channels(n), r, g, b);
}

// Extract 24 bits of background RGB from 'n', split into subcomponents.
static inline unsigned
ncplane_get_bg_rgb(const struct ncplane* n, unsigned* r, unsigned* g, unsigned* b){
  return channels_get_bg_rgb(ncplane_get_channels(n), r, g, b);
}

// Set the current fore/background color using RGB specifications. If the
// terminal does not support directly-specified 3x8b cells (24-bit "Direct
// Color", indicated by the "RGB" terminfo capability), the provided values
// will be interpreted in some lossy fashion. None of r, g, or b may exceed 255.
// "HP-like" terminals require setting foreground and background at the same
// time using "color pairs"; notcurses will manage color pairs transparently.
API int ncplane_set_fg_rgb(struct ncplane* n, int r, int g, int b);
API int ncplane_set_bg_rgb(struct ncplane* n, int r, int g, int b);

// Same, but with rgb assembled into a channel (i.e. lower 24 bits).
API void ncplane_set_fg(struct ncplane* n, uint32_t channel);
API void ncplane_set_bg(struct ncplane* n, uint32_t channel);

// Use the default color for the foreground/background.
API void ncplane_set_fg_default(struct ncplane* n);
API void ncplane_set_bg_default(struct ncplane* n);

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
API unsigned ncplane_styles(const struct ncplane* n);

// Fade the ncplane out over the provided time, calling the specified function
// when done. Requires a terminal which supports direct color, or at least
// palette modification (if the terminal uses a palette, our ability to fade
// planes is limited, and affected by the complexity of the rest of the screen).
// It is not safe to resize or destroy the plane during the fadeout FIXME.
API int ncplane_fadeout(struct ncplane* n, const struct timespec* ts);

// Fade the ncplane in over the specified time. Load the ncplane with the
// target cells without rendering, then call this function. When it's done, the
// ncplane will have reached the target levels, starting from zeroes.
API int ncplane_fadein(struct ncplane* n, const struct timespec* ts);

// Working with cells

#define CELL_TRIVIAL_INITIALIZER { .gcluster = '\0', .attrword = 0, .channels = 0, }
#define CELL_SIMPLE_INITIALIZER(c) { .gcluster = c, .attrword = 0, .channels = 0, }

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
cell_prime(struct ncplane* n, cell* c, const char *gcluster,
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

#define CELL_STYLE_MASK      0xffff0000ul
#define CELL_STYLE_STANDOUT  0x00800000ul
#define CELL_STYLE_UNDERLINE 0x00400000ul
#define CELL_STYLE_REVERSE   0x00200000ul
#define CELL_STYLE_BLINK     0x00100000ul
#define CELL_STYLE_DIM       0x00080000ul
#define CELL_STYLE_BOLD      0x00040000ul
#define CELL_STYLE_INVIS     0x00020000ul
#define CELL_STYLE_PROTECT   0x00010000ul
#define CELL_STYLE_ITALIC    0x01000000ul

// Set the specified style bits for the cell 'c', whether they're actively
// supported or not.
static inline void
cell_styles_set(cell* c, unsigned stylebits){
  c->attrword = (c->attrword & ~CELL_STYLE_MASK) | ((stylebits & CELL_STYLE_MASK));
}

// Extract the style bits from the cell's attrword.
static inline unsigned
cell_styles(const cell* c){
  return c->attrword & CELL_STYLE_MASK;
}

// Add the specified styles (in the LSBs) to the cell's existing spec, whether
// they're actively supported or not.
static inline void
cell_styles_on(cell* c, unsigned stylebits){
  c->attrword |= (stylebits & CELL_STYLE_MASK);
}

// Remove the specified styles (in the LSBs) from the cell's existing spec.
static inline void
cell_styles_off(cell* c, unsigned stylebits){
  c->attrword &= ~(stylebits & CELL_STYLE_MASK);
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

// does the cell contain an East Asian Wide codepoint?
static inline bool
cell_double_wide_p(const cell* c){
  return (c->channels & CELL_WIDEASIAN_MASK);
}

// is the cell simple (a lone ASCII character, encoded as such)?
static inline bool
cell_simple_p(const cell* c){
  return c->gcluster < 0x80;
}

// get the offset into the egcpool for this cell's EGC. returns meaningless and
// unsafe results if called on a simple cell.
static inline uint32_t
cell_egc_idx(const cell* c){
  return c->gcluster - 0x80;
}

// return a pointer to the NUL-terminated EGC referenced by 'c'. this pointer
// is invalidated by any further operation on the plane 'n', so...watch out!
API const char* cell_extended_gcluster(const struct ncplane* n, const cell* c);

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
  cell_release(n, &ul);
  cell_release(n, &ur);
  cell_release(n, &ll);
  cell_release(n, &lr);
  cell_release(n, &hl);
  cell_release(n, &vl);
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
  cell_release(n, &ul);
  cell_release(n, &ur);
  cell_release(n, &ll);
  cell_release(n, &lr);
  cell_release(n, &hl);
  cell_release(n, &vl);
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
API struct ncvisual* ncplane_visual_open(struct ncplane* nc, const char* file,
                                         int* averr);

// Open a visual, extract a codec and parameters, and create a new plane
// suitable for its display at 'y','x'. If there is sufficient room to display
// the visual in its native size, the new plane will be exactly that large.
// Otherwise, the visual will be scaled to the available space. If 'stretch' is
// false, its aspect ratio will be maintained. Otherwise, the visual will be
// scaled to fill the maximum possible new plane.
API struct ncvisual* ncvisual_open_plane(struct notcurses* nc, const char* file,
                                         int* averr, int y, int x, bool stretch);

// Destroy an ncvisual. Rendered elements will not be disrupted, but the visual
// can be neither decoded nor rendered any further.
API void ncvisual_destroy(struct ncvisual* ncv);

// extract the next frame from an ncvisual. returns NULL on end of file,
// writing AVERROR_EOF to 'averr'. returns NULL on a decoding or allocation
// error, placing the AVError in 'averr'. this frame is invalidated by a
// subsequent call to ncvisual_decode(), and should not be freed by the caller.
API struct AVFrame* ncvisual_decode(struct ncvisual* nc, int* averr);

// render the decoded frame to the associated ncplane. the frame will be scaled
// to the size of the ncplane at ncplane_visual_open() time.
API int ncvisual_render(const struct ncvisual* ncv);

// Called for each frame rendered from 'ncv'. If anything but 0 is returned,
// the streaming operation ceases immediately, and that value is propagated out.
typedef int (*streamcb)(struct notcurses* nc, struct ncvisual* ncv);

// Shut up and display my frames! Provide as an argument to ncvisual_stream().
static inline int
ncvisual_simple_streamer(struct notcurses* nc, struct ncvisual* ncv __attribute__ ((unused))){
  return notcurses_render(nc);
}

// Stream the entirety of the media, according to its own timing. Blocking,
// obviously. streamer may be NULL; it is otherwise called for each frame, and
// its return value handled as outlined for stream cb. Pretty raw; beware.
API int ncvisual_stream(struct notcurses* nc, struct ncvisual* ncv,
                        int* averr, streamcb streamer);

// A panelreel is an notcurses region devoted to displaying zero or more
// line-oriented, contained panels between which the user may navigate. If at
// least one panel exists, there is an active panel. As much of the active
// panel as is possible is always displayed. If there is space left over, other
// panels are included in the display. Panels can come and go at any time, and
// can grow or shrink at any time.
//
// This structure is amenable to line- and page-based navigation via keystrokes,
// scrolling gestures, trackballs, scrollwheels, touchpads, and verbal commands.

typedef struct panelreel_options {
  // require this many rows and columns (including borders). otherwise, a
  // message will be displayed stating that a larger terminal is necessary, and
  // input will be queued. if 0, no minimum will be enforced. may not be
  // negative. note that panelreel_create() does not return error if given a
  // WINDOW smaller than these minima; it instead patiently waits for the
  // screen to get bigger.
  int min_supported_cols;
  int min_supported_rows;

  // use no more than this many rows and columns (including borders). may not be
  // less than the corresponding minimum. 0 means no maximum.
  int max_supported_cols;
  int max_supported_rows;

  // desired offsets within the surrounding WINDOW (top right bottom left) upon
  // creation / resize. a panelreel_move() operation updates these.
  int toff, roff, boff, loff;
  // is scrolling infinite (can one move down or up forever, or is an end
  // reached?). if true, 'circular' specifies how to handle the special case of
  // an incompletely-filled reel.
  bool infinitescroll;
  // is navigation circular (does moving down from the last panel move to the
  // first, and vice versa)? only meaningful when infinitescroll is true. if
  // infinitescroll is false, this must be false.
  bool circular;
  // outcurses can draw a border around the panelreel, and also around the
  // component tablets. inhibit borders by setting all valid bits in the masks.
  // partially inhibit borders by setting individual bits in the masks. the
  // appropriate attr and pair values will be used to style the borders.
  // focused and non-focused tablets can have different styles. you can instead
  // draw your own borders, or forgo borders entirely.
  unsigned bordermask; // bitfield; 1s will not be drawn (see bordermaskbits)
  cell borderattr;     // attributes used for panelreel border
  unsigned tabletmask; // bitfield; same as bordermask but for tablet borders
  // FIXME should be attrword + channels, as we don't use gcluster here
  cell tabletattr;     // attributes used for tablet borders
  cell focusedattr;    // attributes used for focused tablet borders, no color!
  uint64_t bgchannel;  // background colors
} panelreel_options;

struct tablet;
struct panelreel;

// Create a panelreel according to the provided specifications. Returns NULL on
// failure. w must be a valid WINDOW*, to which offsets are relative. Note that
// there might not be enough room for the specified offsets, in which case the
// panelreel will be clipped on the bottom and right. A minimum number of rows
// and columns can be enforced via popts. efd, if non-negative, is an eventfd
// that ought be written to whenever panelreel_touch() updates a tablet (this
// is useful in the case of nonblocking input).
API struct panelreel* panelreel_create(struct ncplane* nc,
                                       const panelreel_options* popts,
                                       int efd);

// Returns the ncplane on which this panelreel lives.
API struct ncplane* panelreel_plane(struct panelreel* pr);

// Tablet draw callback, provided a ncplane the first column that may be used,
// the first row that may be used, the first column that may not be used, the
// first row that may not be used, and a bool indicating whether output ought
// be clipped at the top (true) or bottom (false). Rows and columns are
// zero-indexed, and both are relative to the panel.
//
// Regarding clipping: it is possible that the tablet is only partially
// displayed on the screen. If so, it is either partially present on the top of
// the screen, or partially present at the bottom. In the former case, the top
// is clipped (cliptop will be true), and output ought start from the end. In
// the latter case, cliptop is false, and output ought start from the beginning.
//
// Returns the number of lines of output, which ought be less than or equal to
// maxy - begy, and non-negative (negative values might be used in the future).
typedef int (*tabletcb)(struct ncplane* p, int begx, int begy, int maxx,
                        int maxy, bool cliptop, void* curry);

// Add a new tablet to the provided panelreel, having the callback object
// opaque. Neither, either, or both of after and before may be specified. If
// neither is specified, the new tablet can be added anywhere on the reel. If
// one or the other is specified, the tablet will be added before or after the
// specified tablet. If both are specifid, the tablet will be added to the
// resulting location, assuming it is valid (after->next == before->prev); if
// it is not valid, or there is any other error, NULL will be returned.
API struct tablet* panelreel_add(struct panelreel* pr, struct tablet* after,
                                 struct tablet *before, tabletcb cb,
                                 void* opaque);

// Return the number of tablets.
API int panelreel_tabletcount(const struct panelreel* pr);

// Indicate that the specified tablet has been updated in a way that would
// change its display. This will trigger some non-negative number of callbacks
// (though not in the caller's context).
API int panelreel_touch(struct panelreel* pr, struct tablet* t);

// Delete the tablet specified by t from the panelreel specified by pr. Returns
// -1 if the tablet cannot be found.
API int panelreel_del(struct panelreel* pr, struct tablet* t);

// Delete the active tablet. Returns -1 if there are no tablets.
API int panelreel_del_focused(struct panelreel* pr);

// Move to the specified location within the containing WINDOW.
API int panelreel_move(struct panelreel* pr, int x, int y);

// Redraw the panelreel in its entirety, for instance after
// clearing the screen due to external corruption, or a SIGWINCH.
API int panelreel_redraw(struct panelreel* pr);

// Return the focused tablet, if any tablets are present. This is not a copy;
// be careful to use it only for the duration of a critical section.
API struct tablet* panelreel_focused(struct panelreel* pr);

// Change focus to the next tablet, if one exists
API struct tablet* panelreel_next(struct panelreel* pr);

// Change focus to the previous tablet, if one exists
API struct tablet* panelreel_prev(struct panelreel* pr);

// Destroy a panelreel allocated with panelreel_create(). Does not destroy the
// underlying WINDOW. Returns non-zero on failure.
API int panelreel_destroy(struct panelreel* pr);

API void* tablet_userptr(struct tablet* t);
API const void* tablet_userptr_const(const struct tablet* t);

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
API const char *enmetric(uintmax_t val, unsigned decimal, char *buf,
                         int omitdec, unsigned mult, int uprefix);

// Mega, kilo, gigabytes. Use PREFIXSTRLEN + 1.
static inline const char *
qprefix(uintmax_t val, unsigned decimal, char *buf, int omitdec){
  return enmetric(val, decimal, buf, omitdec, 1000, '\0');
}

// Mibi, kebi, gibibytes. Use BPREFIXSTRLEN + 1.
static inline const char *
bprefix(uintmax_t val, unsigned decimal, char *buf, int omitdec){
  return enmetric(val, decimal, buf, omitdec, 1024, 'i');
}

API void notcurses_cursor_enable(struct notcurses* nc);
API void notcurses_cursor_disable(struct notcurses* nc);

#undef API

#ifdef __cplusplus
} // extern "C"
#endif

#endif
