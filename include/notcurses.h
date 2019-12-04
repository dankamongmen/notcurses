#ifndef NOTCURSES_NOTCURSES
#define NOTCURSES_NOTCURSES

#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
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
// Dynamic requirements can add up to 16MB to an ncplane, but such large pools
// are unlikely in common use.
typedef struct cell {
  // These 32 bits are either a single-byte, single-character grapheme cluster
  // (values 0--0x7f), or a pointer into a per-ncplane attached pool of
  // varying-length UTF-8 grapheme clusters. This pool may thus be up to 16MB.
  uint32_t gcluster;          // 1 * 4b -> 4b
  // The CELL_STYLE_* attributes (16 bits), plus 16 bits of alpha.
  uint32_t attrword;          // + 4b -> 8b
  // (channels & 0x8000000000000000ull): inherit styling from prior cell
  // (channels & 0x4000000000000000ull): foreground is *not* "default color"
  // (channels & 0x2000000000000000ull): wide character (left or right side)
  // (channels & 0x1f00000000000000ull): reserved, must be 0
  // (channels & 0x00ffffff00000000ull): foreground in 3x8 RGB (rrggbb)
  // (channels & 0x0000000080000000ull): reserved, must be 0
  // (channels & 0x0000000040000000ull): background is *not* "default color"
  // (channels & 0x000000003f000000ull): reserved, must be 0
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
  // An open FILE* for this terminal, on which we will generate output. If
  // not attached to a sufficiently capable terminal, notcurses will refuse
  // to start. You'll usually want stdout.
  FILE* outfp;
  // If smcup/rmcup capabilities are indicated, notcurses defaults to making
  // use of the "alternate screen". This flag inhibits use of smcup/rmcup.
  bool inhibit_alternate_screen;
  // By default, we hide the cursor if possible. This flag inhibits use of
  // the civis capability, retaining the cursor.
  bool retain_cursor;
  // By default, we handle escape sequences and turn them into special keys.
  // This is necessary for e.g. arrow keys. This can cause notcurses_getc() to
  // block for a short time when Escape is pressed. Disable with this bool.
  bool pass_through_esc;
  // We typically install a signal handler for SIGINT and SIGQUIT that restores
  // the screen, and then calls the old signal handler. Set this to inhibit
  // registration of any signal handlers.
  bool no_quit_sighandlers;
  // We typically install a signal handler for SIGWINCH that generates a resize
  // event in the notcurses_getc() queue. Set this to inhibit the handler.
  bool no_winch_sighandler;
  // If non-NULL, notcurses_render() will write each rendered frame to this
  // FILE* in addition to outfp. This is used primarily for debugging.
  FILE* renderfp;
} notcurses_options;

// Initialize a notcurses context, corresponding to a connected terminal.
// Returns NULL on error, including any failure to initialize terminfo.
API struct notcurses* notcurses_init(const notcurses_options* opts);

// Destroy a notcurses context.
API int notcurses_stop(struct notcurses* nc);

// Make the physical screen match the virtual screen. Changes made to the
// virtual screen (i.e. most other calls) will not be visible until after a
// successful call to notcurses_render().
API int notcurses_render(struct notcurses* nc);

// All input is currently taken from stdin, though this will likely change. We
// attempt to read a single UTF8-encoded Unicode codepoint, *not* an entire
// Extended Grapheme Cluster (despite use of the cell object, which encodes an
// entire EGC). It is also possible that we will read a special keypress, i.e.
// anything that doesn't correspond to a Unicode codepoint (e.g. arrow keys,
// function keys, screen resize events, etc.). On return, 'special' is a valid
// special key if and only if c.gcluster is 0 AND the return value is positive.
//
// Many special keys arrive as an escape sequence. It can thus be necessary for
// notcurses_getc() to wait a short time following receipt of an escape. If no
// further input is received, it is assumed that the actual Escape key was
// pressed. Otherwise, the input will be checked against the terminfo database
// to see if it indicates a special key. In all other cases, notcurses_getc()
// is non-blocking. notcurses_getc_blocking() blocks until a codepoint or
// special key is read (though it can be interrupted by a signal).
//
// In the case of a valid read, a positive value is returned corresponding to
// the number of bytes in the UTF-8 character, or '1' for all specials keys.
// 0 is returned to indicate that no input was available, but only by
// notcurses_getc(). Otherwise (including on EOF) -1 is returned.
typedef enum {
  NCKEY_INVALID,
  NCKEY_RESIZE,   // generated interally in response to SIGWINCH
  NCKEY_UP,
  NCKEY_RIGHT,
  NCKEY_DOWN,
  NCKEY_LEFT,
  NCKEY_DC,       // delete
  // FIXME...
} ncspecial_key;

API int notcurses_getc(const struct notcurses* n, cell* c,
                       ncspecial_key* special);
API int notcurses_getc_blocking(const struct notcurses* n, cell* c,
                                ncspecial_key* special);

// Refresh our idea of the terminal's dimensions, reshaping the standard plane
// if necessary. Without a call to this function following a terminal resize
// (as signaled via SIGWINCH), notcurses_render() might not function properly.
// References to ncplanes remain valid following a resize operation, but the
// cursor might have changed position.
API int notcurses_resize(struct notcurses* n, int* y, int* x);

// Get a reference to the standard plane (one matching our current idea of the
// terminal size) for this terminal.
API struct ncplane* notcurses_stdplane(struct notcurses* nc);
API const struct ncplane* notcurses_stdplane_const(const struct notcurses* nc);

// Create a new ncplane at the specified offset (relative to the standard plane)
// and the specified size. The number of rows and columns must both be positive.
// This plane is initially at the top of the z-buffer, as if ncplane_move_top()
// had been called on it. The void* 'opaque' can be retrieved (and reset) later.
API struct ncplane* notcurses_newplane(struct notcurses* nc, int rows, int cols,
                                       int yoff, int xoff, void* opaque);

// Returns a 16-bit bitmask in the LSBs of supported curses-style attributes
// (CELL_STYLE_UNDERLINE, CELL_STYLE_BOLD, etc.) The attribute is only
// indicated as supported if the terminal can support it together with color.
API unsigned notcurses_supported_styles(const struct notcurses* nc);

// Returns the number of colors supported by the palette, or 1 if there is no
// color support.
API int notcurses_palette_size(const struct notcurses* nc);

typedef struct ncstats {
  uint64_t renders;          // number of notcurses_render() runs
  uint64_t render_bytes;     // bytes emitted to ttyfp
  uint64_t render_max_bytes; // max bytes emitted for a frame
  uint64_t render_min_bytes; // min bytes emitted for a frame
  uint64_t render_ns;        // nanoseconds spent in notcurses_render()
  int64_t render_max_ns;     // max ns spent in notcurses_render()
  int64_t render_min_ns;     // min ns spent in successful notcurses_render()
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

// Set the ncplane's background cell to this cell. It will be rendered anywhere
// that the ncplane's gcluster is 0. The default background is all zeroes.
// Erasing the ncplane does not eliminate the background.
API int ncplane_set_background(struct ncplane* ncp, const cell* c);

// Extract the ncplane's background cell into 'c'.
API int ncplane_background(struct ncplane* ncp, cell* c);

// Move this plane relative to the standard plane. It is an error to attempt to
// move the standard plane.
API void ncplane_move_yx(struct ncplane* n, int y, int x);

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

// Replace the cell underneath the cursor with the provided 7-bit char 'c',
// using the specified 'attr' and 'channels' for styling. Advance the cursor by
// 1. On success, returns 1. On failure, returns -1.
API int ncplane_putsimple(struct ncplane* n, char c, uint32_t attr, uint64_t channels);

// Replace the cell underneath the cursor with the provided EGC, using the
// specified 'attr' and 'channels' for styling, and advance the cursor by the
// width of the cluster (but not past the end of the plane). On success, returns
// the number of columns the cursor was advanced. On failure, -1 is returned.
// The number of bytes converted from gclust is written to 'sbytes' if non-NULL.
API int ncplane_putegc(struct ncplane* n, const char* gclust, uint32_t attr,
                       uint64_t channels, int* sbytes);

// Write a series of cells to the current location, using the current style.
// They will be interpreted as a series of columns (according to the definition
// of ncplane_putc()). Advances the cursor by some positive number of cells
// (though not beyond the end of the plane); this number is returned on success.
// On error, a non-positive number is returned, indicating the number of cells
// which were written before the error.
API int ncplane_putstr(struct ncplane* n, const char* gclustarr);

// The ncplane equivalents of printf(3) and vprintf(3).
API int ncplane_printf(struct ncplane* n, const char* format, ...);
API int ncplane_vprintf(struct ncplane* n, const char* format, va_list ap);

// Draw horizontal or vertical lines using the specified cell, starting at the
// current cursor position. The cursor will end at the cell following the last
// cell output (even, perhaps counter-intuitively, when drawing vertical
// lines), just as if ncplane_putc() was called at that spot. Return the
// number of cells drawn on success. On error, return the negative number of
// cells drawn.
API int ncplane_hline(struct ncplane* n, const cell* c, int len);
API int ncplane_vline(struct ncplane* n, const cell* c, int len);

// Draw a box with its upper-left corner at the current cursor position, and its
// lower-right corner at 'ystop'x'xstop'. The 6 cells provided are used to draw the
// upper-left, ur, ll, and lr corners, then the horizontal and vertical lines.
API int ncplane_box(struct ncplane* n, const cell* ul, const cell* ur,
                    const cell* ll, const cell* lr, const cell* hline,
                    const cell* vline, int ystop, int xstop);

// Draw a box with its upper-left corner at the current cursor position, having
// dimensions 'ylen'x'xlen'. See ncplane_box() for more information. The
// minimum box size is 2x2, and it cannot be drawn off-screen.
static inline int
ncplane_box_sized(struct ncplane* n, const cell* ul, const cell* ur,
                  const cell* ll, const cell* lr, const cell* hline,
                  const cell* vline, int ylen, int xlen){
  int y, x;
  ncplane_cursor_yx(n, &y, &x);
  return ncplane_box(n, ul, ur, ll, lr, hline, vline, y + ylen - 1, x + xlen - 1);
}

// Erase every cell in the ncplane, resetting all attributes to normal, all
// colors to the default color, and all cells to undrawn. All cells associated
// with this ncplane is invalidated, and must not be used after the call.
API void ncplane_erase(struct ncplane* n);

// Set the current fore/background color using RGB specifications. If the
// terminal does not support directly-specified 3x8b cells (24-bit "Direct
// Color", indicated by the "RGB" terminfo capability), the provided values
// will be interpreted in some lossy fashion. None of r, g, or b may exceed 255.
// "HP-like" terminals require setting foreground and background at the same
// time using "color pairs"; notcurses will manage color pairs transparently.
API int ncplane_fg_rgb8(struct ncplane* n, int r, int g, int b);
API int ncplane_bg_rgb8(struct ncplane* n, int r, int g, int b);

// use the default color for the foreground/background
API void ncplane_fg_default(struct ncplane* n);
API void ncplane_bg_default(struct ncplane* n);

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

#define CELL_STYLE_SHIFT     16u
#define CELL_STYLE_MASK      0xffff0000ul
#define CELL_ALPHA_MASK      0x0000fffful
// these are used for the style bitfield *after* it is shifted
#define CELL_STYLE_STANDOUT  0x0001u
#define CELL_STYLE_UNDERLINE 0x0002u
#define CELL_STYLE_REVERSE   0x0004u
#define CELL_STYLE_BLINK     0x0008u
#define CELL_STYLE_DIM       0x0010u
#define CELL_STYLE_BOLD      0x0020u
#define CELL_STYLE_INVIS     0x0040u
#define CELL_STYLE_PROTECT   0x0080u
#define CELL_STYLE_ITALIC    0x0100u

// Set the specified style bits for the cell 'c', whether they're actively
// supported or not.
static inline void
cell_styles_set(cell* c, unsigned stylebits){
  c->attrword = (c->attrword & ~CELL_STYLE_MASK) |
                ((stylebits & 0xffff) << 16u);
}

// Get the style bits, shifted over into the LSBs.
static inline unsigned
cell_styles(const cell* c){
  return (c->attrword & CELL_STYLE_MASK) >> 16u;
}

// Add the specified styles (in the LSBs) to the cell's existing spec, whether
// they're actively supported or not.
static inline void
cell_styles_on(cell* c, unsigned stylebits){
  c->attrword |= ((stylebits & 0xffff) << 16u);
}

// Remove the specified styles (in the LSBs) from the cell's existing spec.
static inline void
cell_styles_off(cell* c, unsigned stylebits){
  c->attrword &= ~((stylebits & 0xffff) << 16u);
}

static inline uint32_t
cell_fg_rgb(uint64_t channel){
  return (channel & 0x00ffffff00000000ull) >> 32u;
}

static inline uint32_t
cell_bg_rgb(uint64_t channel){
  return (channel & 0x0000000000ffffffull);
}

static inline unsigned
cell_rgb_red(uint32_t rgb){
  return (rgb & 0xff0000ull) >> 16u;
}

static inline unsigned
cell_rgb_green(uint32_t rgb){
  return (rgb & 0xff00ull) >> 8u;
}

static inline unsigned
cell_rgb_blue(uint32_t rgb){
  return (rgb & 0xffull);
}

#define CELL_INHERITSTYLE_MASK 0x8000000000000000ull
#define CELL_FGDEFAULT_MASK    0x4000000000000000ull
#define CELL_WIDEASIAN_MASK    0x2000000000000000ull
#define CELL_FG_MASK           0x00ffffff00000000ull
#define CELL_BGDEFAULT_MASK    0x0000000040000000ull
#define CELL_BG_MASK           0x0000000000ffffffull

static inline void
cell_rgb_get_fg(uint64_t channels, unsigned* r, unsigned* g, unsigned* b){
  uint32_t fg = ((channels & CELL_FG_MASK) >> 32u);
  *r = cell_rgb_red(fg);
  *g = cell_rgb_green(fg);
  *b = cell_rgb_blue(fg);
}

static inline void
cell_rgb_get_bg(uint64_t channels, unsigned* r, unsigned* g, unsigned* b){
  uint32_t bg = ((channels & CELL_BG_MASK));
  *r = cell_rgb_red(bg);
  *g = cell_rgb_green(bg);
  *b = cell_rgb_blue(bg);
}

// set the r, g, and b channels for either the foreground or background
// component of this 64-bit 'channels' variable. 'shift' is the base number
// of bits to shift r/g/b by; it ought either be 0 (bg) or 32 (fg). each of
// r, g, and b must be in [0, 256), or -1 is returned. 'mask' is the
// appropriate r/g/b mask, and 'nodefbit' is the appropriate nodefault bit.
static inline int
notcurses_channel_prep(uint64_t* channels, uint64_t mask, unsigned shift,
                       int r, int g, int b, uint64_t nodefbit){
  if(r >= 256 || g >= 256 || b >= 256){
    return -1;
  }
  if(r < 0 || g < 0 || b < 0){
    return -1;
  }
  uint64_t rgb = (r & 0xffull) << (shift + 16);
  rgb |= (g & 0xffull) << (shift + 8);
  rgb |= (b & 0xffull) << shift;
  rgb |= nodefbit;
  *channels = (*channels & ~(mask | nodefbit)) | rgb;
  return 0;
}

static inline int
notcurses_fg_prep(uint64_t* channels, int r, int g, int b){
  return notcurses_channel_prep(channels, CELL_FG_MASK, 32, r, g, b, CELL_FGDEFAULT_MASK);
}

static inline int
notcurses_bg_prep(uint64_t* channels, int r, int g, int b){
  return notcurses_channel_prep(channels, CELL_BG_MASK, 0, r, g, b, CELL_BGDEFAULT_MASK);
}

static inline void
cell_set_fg(cell* c, unsigned r, unsigned g, unsigned b){
  notcurses_fg_prep(&c->channels, r, g, b);
}

static inline void
cell_set_bg(cell* c, unsigned r, unsigned g, unsigned b){
  notcurses_bg_prep(&c->channels, r, g, b);
}

static inline void
cell_get_fg(const cell* c, unsigned* r, unsigned* g, unsigned* b){
  *r = cell_rgb_red(cell_fg_rgb(c->channels));
  *g = cell_rgb_green(cell_fg_rgb(c->channels));
  *b = cell_rgb_blue(cell_fg_rgb(c->channels));
}

static inline void
cell_get_bg(const cell* c, unsigned* r, unsigned* g, unsigned* b){
  *r = cell_rgb_red(cell_bg_rgb(c->channels));
  *g = cell_rgb_green(cell_bg_rgb(c->channels));
  *b = cell_rgb_blue(cell_bg_rgb(c->channels));
}

// does the cell passively retain the styling of the previously-rendered cell?
static inline bool
cell_inherits_style(const cell* c){
  return (c->channels & CELL_INHERITSTYLE_MASK);
}

// use the default color for the foreground
static inline void
cell_fg_default(cell* c){
  c->channels &= ~CELL_FGDEFAULT_MASK;
}

// is the cell using the terminal's default foreground color for its foreground?
static inline bool
cell_fg_default_p(const cell* c){
  return !(c->channels & CELL_FGDEFAULT_MASK);
}

// use the default color for the background
static inline void
cell_bg_default(cell* c){
  c->channels &= ~CELL_BGDEFAULT_MASK;
}

// is the cell using the terminal's default background color for its background?
static inline bool
cell_bg_default_p(const cell* c){
  return !(c->channels & CELL_BGDEFAULT_MASK);
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
  if((ulen = cell_load(n, ul, gclusters)) > 0){
    if((ulen = cell_load(n, ur, gclusters += ulen)) > 0){
      if((ulen = cell_load(n, ll, gclusters += ulen)) > 0){
        if((ulen = cell_load(n, lr, gclusters += ulen)) > 0){
          if((ulen = cell_load(n, hl, gclusters += ulen)) > 0){
            if((ulen = cell_load(n, vl, gclusters += ulen)) > 0){
              ul->attrword = attrs; ul->channels = channels;
              ur->attrword = attrs; ur->channels = channels;
              ll->attrword = attrs; ll->channels = channels;
              lr->attrword = attrs; lr->channels = channels;
              hl->attrword = attrs; hl->channels = channels;
              vl->attrword = attrs; vl->channels = channels;
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
                    int ystop, int xstop){
  int ret = 0;
  cell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  cell ll = CELL_TRIVIAL_INITIALIZER, lr = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if((ret = cells_rounded_box(n, attr, channels, &ul, &ur, &ll, &lr, &hl, &vl)) == 0){
    ret = ncplane_box(n, &ul, &ur, &ll, &lr, &hl, &vl, ystop, xstop);
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
                          int ylen, int xlen){
  int y, x;
  ncplane_cursor_yx(n, &y, &x);
  return ncplane_rounded_box(n, attr, channels, y + ylen - 1, x + xlen - 1);
}

static inline int
cells_double_box(struct ncplane* n, uint32_t attr, uint64_t channels,
                 cell* ul, cell* ur, cell* ll, cell* lr, cell* hl, cell* vl){
  return cells_load_box(n, attr, channels, ul, ur, ll, lr, hl, vl, "╔╗╚╝═║");
}

static inline int
ncplane_double_box(struct ncplane* n, uint32_t attr, uint64_t channels,
                   int ystop, int xstop){
  int ret = 0;
  cell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  cell ll = CELL_TRIVIAL_INITIALIZER, lr = CELL_TRIVIAL_INITIALIZER;
  cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if((ret = cells_double_box(n, attr, channels, &ul, &ur, &ll, &lr, &hl, &vl)) == 0){
    ret = ncplane_box(n, &ul, &ur, &ll, &lr, &hl, &vl, ystop, xstop);
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
                         int ylen, int xlen){
  int y, x;
  ncplane_cursor_yx(n, &y, &x);
  return ncplane_double_box(n, attr, channels, y + ylen - 1, x + xlen - 1);
}

// multimedia functionality
struct AVFrame;

// open a visual (image or video), associating it with the specified ncplane.
// returns NULL on any error, writing the AVError to 'averr'.
API struct ncvisual* ncplane_visual_open(struct ncplane* nc, const char* file,
                                         int* averr);

// destroy an ncvisual. rendered elements will not be disrupted, but the visual
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

// stream the entirety of the media, according to its own timing.
// blocking, obviously. pretty raw; beware.
API int ncvisual_stream(struct notcurses* nc, struct ncvisual* ncv, int* averr);

// A panelreel is an notcurses region devoted to displaying zero or more
// line-oriented, contained panels between which the user may navigate. If at
// least one panel exists, there is an active panel. As much of the active
// panel as is possible is always displayed. If there is space left over, other
// panels are included in the display. Panels can come and go at any time, and
// can grow or shrink at any time.
//
// This structure is amenable to line- and page-based navigation via keystrokes,
// scrolling gestures, trackballs, scrollwheels, touchpads, and verbal commands.

enum bordermaskbits {
  BORDERMASK_TOP    = 0x1,
  BORDERMASK_RIGHT  = 0x2,
  BORDERMASK_BOTTOM = 0x4,
  BORDERMASK_LEFT   = 0x8,
};

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
  cell tabletattr;     // attributes used for tablet borders
  cell focusedattr;    // attributes used for focused tablet borders, no color!
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

#undef API

#ifdef __cplusplus
} // extern "C"
#endif

#endif
