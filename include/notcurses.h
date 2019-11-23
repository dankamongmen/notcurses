#ifndef NOTCURSES_NOTCURSES
#define NOTCURSES_NOTCURSES

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Get a human-readable string describing the running notcurses version.
const char* notcurses_version(void);

struct cell;      // a coordinate on an ncplane: wchar_t(s) and styling
struct ncplane;   // a drawable notcurses surface, composed of cells
struct notcurses; // notcurses state for a given terminal, composed of ncplanes

// A cell corresponds to a single character cell on some plane. At any cell, we
// can have a short array of wchar_t (L'\0'-terminated; we need support an
// array due to the possibility of combining characters), a foreground color,
// a background color, and an attribute set. The rules on the wchar_t array are
// the same as those for an ncurses 6.1 cchar_t:
//
// FIXME i don't care for this (large) static array one whit. we're not bound
// to X/Open, and owe cchar_t no fealty. i do like the attrs and colors being
// bound up with it, though. this definition is almost certain to change. we
// could overload some invalid UTF-8 construction (say a first byte greater
// than 0x7f) to escape out to some attached storage pool, using the
// difference as an index into the pool. we would have 25 bits, after all...
//
//  * At most one spacing character, which must be the first if present.
//  * Up to NCCHARW_MAX-1 nonspacing characters follow. Extra spacing
//    characters are ignored. A nonspacing character is one for which wcwidth()
//    returns zero, and is not the wide NUL (L'\0').
//  * A single control character can be present, with no other characters (save
//    an immediate wide NUL (L'\0').
//  * If there are fewer than NCCHARW_MAX wide characters, they must be
//    terminated with a wide NUL (L'\0').
//
// Multi-column characters can only have a single attribute/color.
// https://pubs.opengroup.org/onlinepubs/007908799/xcurses/intov.html
//
// Each cell occupies 16 bytes (128 bits). The surface is thus ~2MB for a
// (pretty large) 500x200 terminal. At 80x43, it's less than 100KB.
#define NCCHARW_MAX 1
typedef struct cell {
  wchar_t cchar[NCCHARW_MAX]; // 1 * 4b -> 4b
  // The classic NCURSES WA_* attributes (16 bits), plus 16 bits of alpha.
  uint32_t attrword;          // + 4b -> 8b
  // (channels & 0x8000000000000000ull): inherit styling from prior cell
  // (channels & 0x4000000000000000ull): foreground is *not* "default color"
  // (channels & 0x3f00000000000000ull): reserved, must be 0
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

// FIXME we'll need to expose this definition for ncplane_getwc()
struct cell;      // the contents of a single cell on a single plane

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
// Following a call to notcurses_resize(), any references to the standard plane
// ought be considered invalidated.
int notcurses_resize(struct notcurses* n);

// Get a reference to the standard plane (one matching our current idea of the
// terminal size) for this terminal. Invalidated following a call to
// notcurses_resize().
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

// Replace the cell underneath the cursor with the provided cell 'c', and
// advance the cursor by one cell *unless we are at the end of the plane*.
// On success, returns 1 if the cursor was advanced, and 0 otherwise. On
// failure, -1 is returned.
int ncplane_putwc(struct ncplane* n, const cell* c);

// Retrieve the cell under the cursor, returning it in 'c'.
void ncplane_getwc(const struct ncplane* n, cell* c);

// Write a series of wchar_ts to the current location. They will be interpreted
// as a series of columns (according to the definition of ncplane_putwc()).
// Advances the cursor by some positive number of cells; this number is returned
// on success. On error, a non-positive number is returned, indicating the
// number of cells which were written before the error.
int ncplane_putwstr(struct ncplane* n, const wchar_t* wstr);

// The ncplane equivalent of wprintf(3) and vwprintf(3), themselves the
// wide-character equivalents of printf(3) and vprintf(3).
int ncplane_wprintf(struct ncplane* n, const wchar_t* format, ...);

// Draw horizontal or vertical lines using the specified cell of wchar_t's,
// starting at the current cursor position. The cursor will end at the cell
// following the last cell output (even, perhaps counter-intuitively, when
// drawing vertical lines), just as if ncplane_putwc() was called at that spot.
// Returns the number of cells drawn on success. On error, returns the negative
// number of cells drawn.
int ncplane_hline(struct ncplane* n, const wchar_t* wcs, int len);
int ncplane_vline(struct ncplane* n, int yoff, const wchar_t* wcs, int len);

// Erase all content in the ncplane, resetting all attributes to normal, all
// colors to -1, and all cells to undrawn.
void ncplane_erase(struct ncplane* n);

// Retrieve the cell under the cursor, returning it in 'c'.
void ncplane_getwc(const struct ncplane* n, struct cell* c);

// Write a series of wchar_ts to the current location. They will be interpreted
// as a series of columns (according to the definition of ncplane_putwc()).
// Advances the cursor by some positive number of cells; this number is returned
// on success. On error, a non-positive number is returned, indicating the
// number of cells which were written before the error.
int ncplane_putwstr(struct ncplane* n, const wchar_t* wstr);

// The ncplane equivalent of wprintf(3) and vwprintf(3), themselves the
// wide-character equivalents of printf(3) and vprintf(3).
int ncplane_wprintf(struct ncplane* n, const wchar_t* format, ...);

// Set the current fore/background color using RGB specifications. If the
// terminal does not support directly-specified 3x8b cells (24-bit "Direct
// Color", indicated by the "RGB" terminfo capability), the provided values
// will be interpreted in some lossy fashion. None of r, g, or b may exceed 255.
// "HP-like" terminals require setting foreground and background at the same
// time using "color pairs"; notcurses will manage color pairs transparently.
int ncplane_fg_rgb8(struct ncplane* n, int r, int g, int b);
int ncplane_bg_rgb8(struct ncplane* n, int r, int g, int b);

// Fine details about terminal

// Returns a 16-bit bitmask in the LSBs of supported NCURSES-style attributes
// (WA_UNDERLINE, WA_BOLD, etc.) The attribute is only indicated as supported
// if the terminal can support it together with color.
unsigned notcurses_supported_styles(const struct notcurses* nc);

// Returns the number of colors supported by the palette, or 0 if there is no
// palette (DirectColor or no colors).
int notcurses_palette_size(const struct notcurses* nc);

// Working with cells

// Copies as many wchar_ts out of 'wstr' and into 'c' as it can, according to
// the rules of cell composition. If the leading part of wstr is not a valid
// cell, -1 is returned. Returns the number of wchar_ts copied, not including
// the terminating L'\0' (if 'wstr' is empty, zero is returned).
int load_cell(cell* c, const wchar_t* wstr);

static inline uint32_t
cell_fg_rgb(uint64_t channel){
  return (channel & 0x00ffffff00000000ull) >> 32u;
}

static inline uint32_t
cell_bg_rgb(uint64_t channel){
  return (channel & 0xffffffull);
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

static inline void
cell_rgb_set_fg(uint64_t* channels, unsigned r, unsigned g, unsigned b){
  uint64_t rgb = (r & 0xffull) << 48u;
  rgb |= (g & 0xffull) << 40u;
  rgb |= (b & 0xffull) << 32u;
  *channels = (*channels & ~0x00ffffff00000000ull) | rgb;
}

static inline void
cell_rgb_set_bg(uint64_t* channels, unsigned r, unsigned g, unsigned b){
  uint64_t rgb = (r & 0xffull) << 16u;
  rgb |= (g & 0xffull) << 8u;
  rgb |= (b & 0xffull);
  *channels = (*channels & ~0x0000000000ffffffull) | rgb;
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

#ifdef __cplusplus
} // extern "C"
#endif

#endif
