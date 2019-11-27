#ifndef NOTCURSES_NOTCURSES
#define NOTCURSES_NOTCURSES

#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Get a human-readable string describing the running notcurses version.
const char* notcurses_version(void);

struct cell;      // a coordinate on an ncplane: an EGC plus styling
struct ncplane;   // a drawable notcurses surface, composed of cells
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
  // The classic NCURSES WA_* attributes (16 bits), plus 16 bits of alpha.
  uint32_t attrword;          // + 4b -> 8b
  // (channels & 0x8000000000000000ull): inherit styling from prior cell
  // (channels & 0x4000000000000000ull): foreground is *not* "default color"
  // (channels & 0x2000000000000000ull): wide character (left or right side)
  // (channels & 0x1f00000000000000ull): reserved, must be 0
  // (channels & 0x00ffffff00000000ull): foreground in 3x8 RGB (rrggbb)
  // (channels & 0x0000000080000000ull): in the middle of a multicolumn glyph
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
  // A file descriptor for this terminal on which we will generate output.
  // Must be a valid file descriptor attached to a terminal, or notcurses will
  // refuse to start. You'll usually want STDOUT_FILENO.
  int outfd;
  // If smcup/rmcup capabilities are indicated, notcurses defaults to making
  // use of the "alternate screen". This flag inhibits use of smcup/rmcup.
  bool inhibit_alternate_screen;
  // By default, we hide the cursor if possible. This flag inhibits use of
  // the civis capability, retaining the cursor.
  bool retain_cursor;
} notcurses_options;

// Initialize a notcurses context, corresponding to a connected terminal.
// Returns NULL on error, including any failure to initialize terminfo.
struct notcurses* notcurses_init(const notcurses_options* opts);

// Destroy a notcurses context.
int notcurses_stop(struct notcurses* nc);

// Make the physical screen match the virtual screen. Changes made to the
// virtual screen (i.e. most other calls) will not be visible until after a
// successful call to notcurses_render().
int notcurses_render(struct notcurses* nc);

// Refresh our idea of the terminal's dimensions, reshaping the standard plane
// if necessary. Without a call to this function following a terminal resize
// (as signaled via SIGWINCH), notcurses_render() might not function properly.
// References to ncplanes remain valid following a resize operation, but the
// cursor might have changed position.
int notcurses_resize(struct notcurses* n);

// Get a reference to the standard plane (one matching our current idea of the
// terminal size) for this terminal.
struct ncplane* notcurses_stdplane(struct notcurses* nc);
const struct ncplane* notcurses_stdplane_const(const struct notcurses* nc);

// Create a new plane at the specified offset (relative to the standard plane)
// and the specified size. The number of rows and columns must both be positive.
// This plane is initially at the top of the z-buffer, as if ncplane_move_top()
// had been called on it. The void* 'opaque' can be retrieved (and reset) later.
struct ncplane* notcurses_newplane(struct notcurses* nc, int rows, int cols,
                                   int yoff, int xoff, void* opaque);

// Destroy the specified ncplane. None of its contents will be visible after
// the next call to notcurses_render(). It is an error to attempt to destroy
// the standard plane.
int ncplane_destroy(struct ncplane* n);

// Move this plane relative to the standard plane.
int ncplane_move_yx(struct ncplane* n, int y, int x);

// Get the origin of this plane relative to the standard plane.
void ncplane_yx(const struct ncplane* n, int* y, int* x);

// Splice ncplane 'n' out of the z-buffer, and reinsert it above 'above'.
void ncplane_move_above(struct ncplane* n, struct ncplane* above);
// Splice ncplane 'n' out of the z-buffer, and reinsert it below 'below'.
void ncplane_move_below(struct ncplane* n, struct ncplane* below);
// Splice ncplane 'n' out of the z-buffer, and reinsert it at the top or bottom.
void ncplane_move_top(struct ncplane* n);
void ncplane_move_bottom(struct ncplane* n);

// Retrieve the topmost cell at this location on the screen, returning it in
// 'c'. If there is more than a byte of gcluster, it will be returned as a heap
// allocation in '*gclust', and '*c' will be 0x80.
void notcurses_getc(const struct notcurses* n, cell* c, char** gclust);

// Manipulate the opaque user pointer associated with this plane.
void ncplane_set_userptr(struct ncplane* n, void* opaque);
void* ncplane_userptr(struct ncplane* n);
const void* ncplane_userptr_const(const struct ncplane* n);

// Returns the dimensions of this ncplane.
void ncplane_dimyx(const struct ncplane* n, int* rows, int* cols);

// Return our current idea of the terminal dimensions in rows and cols.
static inline void
notcurses_term_dimyx(const struct notcurses* n, int* rows, int* cols){
  ncplane_dimyx(notcurses_stdplane_const(n), rows, cols);
}

// Move the cursor to the specified position (the cursor needn't be visible).
// Returns -1 on error, including negative parameters, or ones exceeding the
// plane's dimensions.
int ncplane_cursor_move_yx(struct ncplane* n, int y, int x);

// Get the current position of the cursor within n. y and/or x may be NULL.
void ncplane_cursor_yx(const struct ncplane* n, int* y, int* x);

// Replace the cell underneath the cursor with the provided cell 'c', and
// advance the cursor by the width of the cell (but not past the end of the
// plane). On success, returns the number of columns the cursor was advanced.
// On failure, -1 is returned.
int ncplane_putc(struct ncplane* n, const cell* c);

// Retrieve the cell under this plane's cursor, returning it in 'c'. If there
// is more than a byte of gcluster, it will be returned as a heap allocation in
// '*gclust', and '*c' will be 0x80. Returns -1 on error, 0 on success.
int ncplane_getc(const struct ncplane* n, cell* c, char** gclust);

// Write a series of cells to the current location, using the current style.
// They will be interpreted as a series of columns (according to the definition
// of ncplane_putc()). Advances the cursor by some positive number of cells
// (though not beyond the end of the plane); this number is returned on success.
// On error, a non-positive number is returned, indicating the number of cells
// which were written before the error.
int ncplane_putstr(struct ncplane* n, const char* gclustarr);

// The ncplane equivalents of printf(3) and vprintf(3).
int ncplane_printf(struct ncplane* n, const char* format, ...);
int ncplane_vprintf(struct ncplane* n, const char* format, va_list ap);

// Draw horizontal or vertical lines using the specified cell, starting at the
// current cursor position. The cursor will end at the cell following the last
// cell output (even, perhaps counter-intuitively, when drawing vertical
// lines), just as if ncplane_putc() was called at that spot. Return the
// number of cells drawn on success. On error, return the negative number of
// cells drawn.
int ncplane_hline(struct ncplane* n, const cell* c, int len);
int ncplane_vline(struct ncplane* n, const cell* c, int len);

// Draw a box with its upper-left corner at the current cursor position, having
// dimensions |ylen| x |xlen|. The 6 cells provided are used to draw the
// upper-left, ur, ll, and lr corners, then the horizontal and vertical lines.
int ncplane_box(struct ncplane* n, const cell* ul, const cell* ur,
                const cell* ll, const cell* lr, const cell* hline,
                const cell* vline, int ylen, int xlen);

// Erase all content in the ncplane, resetting all attributes to normal, all
// colors to -1, and all cells to undrawn.
void ncplane_erase(struct ncplane* n);

// Set the current fore/background color using RGB specifications. If the
// terminal does not support directly-specified 3x8b cells (24-bit "Direct
// Color", indicated by the "RGB" terminfo capability), the provided values
// will be interpreted in some lossy fashion. None of r, g, or b may exceed 255.
// "HP-like" terminals require setting foreground and background at the same
// time using "color pairs"; notcurses will manage color pairs transparently.
int ncplane_fg_rgb8(struct ncplane* n, int r, int g, int b);
int ncplane_bg_rgb8(struct ncplane* n, int r, int g, int b);

// Set the specified style bits for the ncplane 'n', whether they're actively
// supported or not.
void ncplane_set_style(struct ncplane* n, unsigned stylebits);

// Add the specified styles to the ncplane's existing spec.
void ncplane_enable_styles(struct ncplane* n, unsigned stylebits);

// Remove the specified styles from the ncplane's existing spec.
void ncplane_disable_styles(struct ncplane* n, unsigned stylebits);

// Fine details about terminal

// Returns a 16-bit bitmask in the LSBs of supported NCURSES-style attributes
// (WA_UNDERLINE, WA_BOLD, etc.) The attribute is only indicated as supported
// if the terminal can support it together with color.
unsigned notcurses_supported_styles(const struct notcurses* nc);

// Returns the number of colors supported by the palette, or 1 if there is no
// color support.
int notcurses_palette_size(const struct notcurses* nc);

// Working with cells

#define CELL_TRIVIAL_INITIALIZER { .gcluster = '\0', .attrword = 0, .channels = 0, }

static inline void
cell_init(cell* c){
  memset(c, 0, sizeof(*c));
}

// Breaks the UTF-8 string in 'gcluster' down, setting up the cell 'c'.
int cell_load(struct ncplane* n, cell* c, const char* gcluster);

// Duplicate 'c' into 'targ'. Not intended for external use; exposed for the
// benefit of unit tests.
int cell_duplicate(struct ncplane* n, cell* targ, const cell* c);

// Release resources held by the cell 'c'.
void cell_release(struct ncplane* n, cell* c);

#define CELL_STYLE_MASK 0xffff0000ul
#define CELL_ALPHA_MASK 0x0000fffful

// Set the specified style bits for the cell 'c', whether they're actively
// supported or not.
static inline void
cell_set_style(cell* c, unsigned stylebits){
  c->attrword = (c->attrword & ~CELL_STYLE_MASK) |
                ((stylebits & 0xffff) << 16u);
}

// Add the specified styles to the cell's existing spec.
static inline void
cell_enable_styles(cell* c, unsigned stylebits){
  c->attrword |= ((stylebits & 0xffff) << 16u);
}

// Remove the specified styles from the cell's existing spec.
static inline void
cell_disable_styles(cell* c, unsigned stylebits){
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

#define CELL_FGDEFAULT_MASK 0x4000000000000000ull
#define CELL_WIDEASIAN_MASK 0x2000000000000000ull
#define CELL_BGDEFAULT_MASK 0x0000000040000000ull

static inline void
cell_rgb_set_fg(uint64_t* channels, unsigned r, unsigned g, unsigned b){
  uint64_t rgb = (r & 0xffull) << 48u;
  rgb |= (g & 0xffull) << 40u;
  rgb |= (b & 0xffull) << 32u;
  rgb |= CELL_FGDEFAULT_MASK;
  *channels = (*channels & ~0x40ffffff00000000ull) | rgb;
}

static inline void
cell_rgb_set_bg(uint64_t* channels, unsigned r, unsigned g, unsigned b){
  uint64_t rgb = (r & 0xffull) << 16u;
  rgb |= (g & 0xffull) << 8u;
  rgb |= (b & 0xffull);
  rgb |= CELL_BGDEFAULT_MASK;
  *channels = (*channels & ~0x0000000040ffffffull) | rgb;
}

static inline void
cell_set_fg(cell* c, unsigned r, unsigned g, unsigned b){
  cell_rgb_set_fg(&c->channels, r, g, b);
}

static inline void
cell_set_bg(cell* c, unsigned r, unsigned g, unsigned b){
  cell_rgb_set_bg(&c->channels, r, g, b);
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

static inline bool
cell_fg_default_p(const cell* c){
  return !(c->channels & CELL_FGDEFAULT_MASK);
}

static inline bool
cell_bg_default_p(const cell* c){
  return !(c->channels & CELL_BGDEFAULT_MASK);
}

static inline bool
cell_wide_p(const cell* c){
  return (c->channels & CELL_WIDEASIAN_MASK);
}

// multimedia functionality
int notcurses_visual_open(struct notcurses* nc, const char* filename);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
