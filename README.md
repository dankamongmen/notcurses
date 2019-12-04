# notcurses
cleanroom TUI library for modern terminal emulators. definitely not curses.

* [Introduction](#introduction)
* [Requirements](#requirements)
* [Use](#use)
  * [Input](#input)
  * [Planes](#planes)
  * [Cells](#cells)
  * [Perf](#perf)
* [Included tools](#included-tools)
* [Differences from NCURSES](#differences-from-ncurses)
  * [Features missing relative to NCURSES](#features-missing-relative-to-ncurses)
* [Useful links](#cells)

[![Build Status](https://drone.dsscaw.com:4443/api/badges/dankamongmen/notcurses/status.svg)](https://drone.dsscaw.com:4443/dankamongmen/notcurses)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

## Introduction

* **What it is**: a library facilitating complex TUIs on modern terminal
    emulators, supporting vivid colors and Unicode to the maximum degree
    possible. Many tasks delegated to Curses can be achieved using notcurses
    (and vice versa).

* **What it is not**: a source-compatible X/Open Curses implementation, nor a
    replacement for NCURSES on existing systems, nor a widely-ported and -tested
    bedrock of Open Source, nor a battle-proven, veteran library.

notcurses abandons the X/Open Curses API bundled as part of the Single UNIX
Specification. The latter shows its age, and seems not capable of making use of
terminal functionality such as unindexed 24-bit color ("DirectColor", not to be
confused with 8-bit indexed 24-bit color, aka "TrueColor" or (by NCURSES) as
"extended color"). For some necessary
background, consult Thomas E. Dickey's superb and authoritative [NCURSES
FAQ](https://invisible-island.net/ncurses/ncurses.faq.html#xterm_16MegaColors).
As such, notcurses is not a drop-in Curses replacement. It is almost certainly
less portable, and definitely tested on less hardware. Sorry about that.
Ultimately, I hope to properly support all terminals *supporting the features
necessary for complex TUIs*. I would argue that teletypes etc. are
fundamentally unsuitable. Most operating systems seem reasonable targets, but I
only have Linux and FreeBSD available for testing.

Whenever possible, notcurses makes use of the Terminfo library shipped with
NCURSES, benefiting greatly from its portability and thoroughness.

notcurses opens up advanced functionality for the interactive user on
workstations, phones, laptops, and tablets, at the expense of e.g.
industrial and retail terminals (or even the Linux virtual console,
which offers only eight colors and limited glyphs).

Why use this non-standard library?

* A svelter design than that codified in X/Open. All exported identifiers
  are prefixed to avoid namespace collisions. Far fewer identifiers are
  exported overall. All APIs natively suport UTF-8, and the `cell` API is based
  around Unicode's [Extended Grapheme Cluster](https://unicode.org/reports/tr29/) concept.

* Visual features not directly available via NCURSES, including images,
  fonts, video, high-contrast text, and transparent regions. All APIs
  natively support 24-bit color, quantized down as necessary for the terminal.

* Thread safety, and use in parallel programs, has been a design consideration
  from the beginning.

* It's Apache2-licensed in its entirety, as opposed to the
  [drama in several acts](https://invisible-island.net/ncurses/ncurses-license.html)
  that is the NCURSES license (the latter is [summarized](https://invisible-island.net/ncurses/ncurses-license.html#issues_freer)
  as "a restatement of MIT-X11").

On the other hand, if you're targeting industrial or critical applications,
or wish to benefit from the time-tested reliability and portability of Curses,
you should by all means use that fine library.

## Requirements

* A C11 and a C++14 compiler
* CMake 3.13.0+
* NCurses 6.1+
* From FFMpeg: libswscale 5.0+, libavformat 57.0+, libavutil 56.0+

## Use

A program wishing to use notcurses will need to link it, ideally using the
output of `pkg-config --libs notcurses`. It is advised to compile with the
output of `pkg-config --cflags notcurses`. If using CMake, a support file is
provided, and can be accessed as `notcurses`.

Before calling into notcurses—and usually as one of the first calls of the
program—be sure to call `setlocale(3)` with an appropriate UTF-8 `LC_ALL`
locale. It is usually appropriate to pass `NULL` to `setlocale()`, relying on
the user to properly set the `LANG` environment variable.

notcurses requires an available `terminfo(5)` definition appropriate for the
terminal. It is usually appropriate to pass `NULL` in the `termtype` field of a
`notcurses_options` struct, relying on the user to properly set the `TERM`
environment variable. This variable is usually set by the terminal itself. It
might be necessary to manually select a higher-quality definition for your
terminal, i.e. `xterm-direct` as opposed to `xterm` or `xterm-256color`.

Each terminal can be prepared via a call to `notcurses_init()`, which is
supplied a struct of type `notcurses_options`:

```c
// Get a human-readable string describing the running notcurses version.
const char* notcurses_version(void);

struct cell;      // a coordinate on an ncplane: an EGC plus styling
struct ncplane;   // a drawable notcurses surface, composed of cells
struct notcurses; // notcurses state for a given terminal, composed of ncplanes

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
struct notcurses* notcurses_init(const notcurses_options* opts);

// Destroy a notcurses context.
int notcurses_stop(struct notcurses* nc);
```

`notcurses_stop` should be called before exiting your program to restore the
terminal settings and free resources.

The vast majority of the notcurses API draws into virtual buffers. Only upon
a call to `notcurses_render` will the visible terminal display be updated to
reflect the changes:

```c
// Make the physical screen match the virtual screen. Changes made to the
// virtual screen (i.e. most other calls) will not be visible until after a
// successful call to notcurses_render().
int notcurses_render(struct notcurses* nc);
```

### Input

Input can currently be taken only from `stdin`, but on the plus side, stdin
needn't be a terminal device (unlike the ttyfp `FILE*` passed in a
`notcurses_options`). Generalized input ought happen soon. There is only one
input queue per `struct notcurses`.

Like NCURSES, notcurses will watch for escape sequences, check them against the
terminfo database, and return them as special keys. Unlike NCURSES, the
fundamental unit of input is the UTF8-encoded Unicode codepoint.

```c
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
// 0 is returned only by notcurses_getc(), to indicate that no input was
// available. Otherwise (including on EOF) -1 is returned.
enum {
  NCKEY_RESIZE,
  NCKEY_UP,
  NCKEY_RIGHT,
  NCKEY_DOWN,
  NCKEY_LEFT,
  // FIXME...
} ncspecial_keys;

int notcurses_getc(const struct notcurses* n, cell* c, int* special);
int notcurses_getc_blocking(const struct notcurses* n, cell* c, int* special);
```

### Planes

Fundamental to notcurses is a z-buffer of rectilinear virtual screens, known
as `ncplane`s. An `ncplane` can be larger than the physical screen, or smaller,
or the same size; it can be entirely contained within the physical screen, or
overlap in part, or lie wholly beyond the boundaries, never to be rendered.
Each `ncplane` has a current writing state (cursor position, foreground and
background color, etc.), a backing array of UTF-8 EGCs, and a z-index. If
opaque, a cell on a higher `ncplane` completely obstructs a corresponding cell
from a lower `ncplane` from being seen. An `ncplane` corresponds loosely to an
[NCURSES Panel](https://invisible-island.net/ncurses/ncurses-intro.html#panels),
but is the primary drawing surface of notcurses—there is no object
corresponding to a bare NCURSES `WINDOW`.

```c
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
int ncplane_resize(struct ncplane* n, int keepy, int keepx, int keepleny,
                       int keeplenx, int yoff, int xoff, int ylen, int xlen);

// Destroy the specified ncplane. None of its contents will be visible after
// the next call to notcurses_render(). It is an error to attempt to destroy
// the standard plane.
int ncplane_destroy(struct ncplane* ncp);

// Set the ncplane's background cell to this cell. It will be rendered anywhere
// that the ncplane's gcluster is 0. The default background is all zeroes.
// Erasing the ncplane does not eliminate the background.
int ncplane_set_background(struct ncplane* ncp, const cell* c);

// Extract the ncplane's background cell into 'c'.
int ncplane_background(struct ncplane* ncp, cell* c);

// Move this plane relative to the standard plane. It is an error to attempt to
// move the standard plane.
void ncplane_move_yx(struct ncplane* n, int y, int x);

// Get the origin of this plane relative to the standard plane.
void ncplane_yx(const struct ncplane* n, int* RESTRICT y, int* RESTRICT x);

// Splice ncplane 'n' out of the z-buffer, and reinsert it at the top or bottom.
int ncplane_move_top(struct ncplane* n);
int ncplane_move_bottom(struct ncplane* n);

// Splice ncplane 'n' out of the z-buffer, and reinsert it below 'below'.
int ncplane_move_below(struct ncplane* RESTRICT n, struct ncplane* RESTRICT below);

// Splice ncplane 'n' out of the z-buffer, and reinsert it above 'above'.
int ncplane_move_above(struct ncplane* RESTRICT n, struct ncplane* RESTRICT above);

// Retrieve the cell at the cursor location on the specified plane, returning
// it in 'c'. This copy is safe to use until the ncplane is destroyed/erased.
int ncplane_at_cursor(struct ncplane* n, cell* c);

// Manipulate the opaque user pointer associated with this plane.
// ncplane_set_userptr() returns the previous userptr after replacing
// it with 'opaque'. the others simply return the userptr.
void* ncplane_set_userptr(struct ncplane* n, void* opaque);
void* ncplane_userptr(struct ncplane* n);
const void* ncplane_userptr_const(const struct ncplane* n);

// Returns the dimensions of this ncplane.
void ncplane_dim_yx(const struct ncplane* n, int* RESTRICT rows,
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
int ncplane_cursor_move_yx(struct ncplane* n, int y, int x);

// Get the current position of the cursor within n. y and/or x may be NULL.
void ncplane_cursor_yx(const struct ncplane* n, int* RESTRICT y,
                           int* RESTRICT x);

// Replace the cell underneath the cursor with the provided cell 'c', and
// advance the cursor by the width of the cell (but not past the end of the
// plane). On success, returns the number of columns the cursor was advanced.
// On failure, -1 is returned.
int ncplane_putc(struct ncplane* n, const cell* c);

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
int ncplane_hline_interp(struct ncplane* n, const cell* c, int len,
                         uint64_t c1, uint64_t c2);

static inline int
ncplane_hline(struct ncplane* n, const cell* c, int len){
  return ncplane_hline_interp(n, c, len, c->channels, c->channels);
}

int ncplane_vline_interp(struct ncplane* n, const cell* c, int len,
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
// if the gradient bit is not set, the styling from the hl/vl cells is used for
// the horizontal and vertical lines, respectively. if the gradient bit is set,
// the color is linearly interpolated between the two relevant corner cells. if
// the bordermask bit is set, that side of the box is not drawn. iff either edge
// connecting to a corner is drawn, the corner is drawn.

#define NCBOXMASK_TOP    0x01
#define NCBOXMASK_RIGHT  0x02
#define NCBOXMASK_BOTTOM 0x04
#define NCBOXMASK_LEFT   0x08
#define NCBOXGRAD_TOP    0x10
#define NCBOXGRAD_RIGHT  0x20
#define NCBOXGRAD_BOTTOM 0x40
#define NCBOXGRAD_LEFT   0x80

int ncplane_box(struct ncplane* n, const cell* ul, const cell* ur,
                const cell* ll, const cell* lr, const cell* hl,
                const cell* vl, int ystop, int xstop, unsigned ctlword);

// Draw a box with its upper-left corner at the current cursor position, having
// dimensions 'ylen'x'xlen'. See ncplane_box() for more information. The
// minimum box size is 2x2, and it cannot be drawn off-screen.
static inline int
ncplane_box_sized(struct ncplane* n, const cell* ul, const cell* ur,
                  const cell* ll, const cell* lr, const cell* hl,
                  const cell* vl, int ylen, int xlen, unsigned ctlword){
  int y, x;
  ncplane_cursor_yx(n, &y, &x);
  return ncplane_box(n, ul, ur, ll, lr, hl, vl,
                     y + ylen - 1, x + xlen - 1, ctlword);
}

// Erase every cell in the ncplane, resetting all attributes to normal, all
// colors to the default color, and all cells to undrawn. All cells associated
// with this ncplane is invalidated, and must not be used after the call.
void ncplane_erase(struct ncplane* n);

// Set the current fore/background color using RGB specifications. If the
// terminal does not support directly-specified 3x8b cells (24-bit "Direct
// Color", indicated by the "RGB" terminfo capability), the provided values
// will be interpreted in some lossy fashion. None of r, g, or b may exceed 255.
// "HP-like" terminals require setting foreground and background at the same
// time using "color pairs"; notcurses will manage color pairs transparently.
int ncplane_fg_rgb8(struct ncplane* n, int r, int g, int b);
int ncplane_bg_rgb8(struct ncplane* n, int r, int g, int b);

// use the default color for the foreground/background
void ncplane_fg_default(struct ncplane* n);
void ncplane_bg_default(struct ncplane* n);

// Set the specified style bits for the ncplane 'n', whether they're actively
// supported or not.
void ncplane_styles_set(struct ncplane* n, unsigned stylebits);

// Add the specified styles to the ncplane's existing spec.
void ncplane_styles_on(struct ncplane* n, unsigned stylebits);

// Remove the specified styles from the ncplane's existing spec.
void ncplane_styles_off(struct ncplane* n, unsigned stylebits);

// Return the current styling for this ncplane.
unsigned ncplane_styles(const struct ncplane* n);
```

### Cells

Unlike the `notcurses` or `ncplane` objects, the definition of `cell` is
available to the user:

```c
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
```

A `cell` ought be initialized with `CELL_TRIVIAL_INITIALIZER` or the
`cell_init()` function before it is further used. These just zero out the
`cell`. A `cell` has three fundamental elements:

* The EGC displayed at this coordinate, encoded in UTF-8. If the EGC is a
  single ASCII character (value less than 0x80), it is stored inline in
  the `cell`'s `gcluster` field. Otherwise, `gcluster`'s top 24 bits
  are a 128-biased offset into the associated `ncplane`'s egcpool. This
  implies that `cell`s are associated with `ncplane`s once prepared.
* The Curses-style attributes of the text, and a 16-bit alpha channel.
* The 48 bits of foreground and background RGB, plus a few flags.

The EGC should be loaded using `cell_load()`. Either a single NUL-terminated
EGC can be provided, or a string composed of multiple EGCs. In the latter case,
the first EGC from the string is loaded. Remember, backing storage for the EGC
is provided by the `ncplane` passed to `cell_load()`; if this `ncplane` is
destroyed (or even erased), the `cell` cannot safely be used. If you're done
using the `cell` before being done with the `ncplane`, call `cell_release()`
to free up the EGC resources.

```c
#define CELL_TRIVIAL_INITIALIZER { .gcluster = '\0', .attrword = 0, .channels = 0, }

static inline void
cell_init(cell* c){
  memset(c, 0, sizeof(*c));
}

// Breaks the UTF-8 string in 'gcluster' down, setting up the cell 'c'. Returns
// the number of bytes copied out of 'gcluster', or -1 on failure. The styling
// of the cell is left untouched, but any resources are released.
int cell_load(struct ncplane* n, cell* c, const char* gcluster);

// Duplicate 'c' into 'targ'. Not intended for external use; exposed for the
// benefit of unit tests.
int cell_duplicate(struct ncplane* n, cell* targ, const cell* c);

// Release resources held by the cell 'c'.
void cell_release(struct ncplane* n, cell* c);

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

static inline int
cell_rgb_set_fg(uint64_t* channels, int r, int g, int b){
  if(r >= 256 || g >= 256 || b >= 256){
    return -1;
  }
  if(r < 0 || g < 0 || b < 0){
    return -1;
  }
  uint64_t rgb = (r & 0xffull) << 48u;
  rgb |= (g & 0xffull) << 40u;
  rgb |= (b & 0xffull) << 32u;
  rgb |= CELL_FGDEFAULT_MASK;
  *channels = (*channels & ~(CELL_FGDEFAULT_MASK | CELL_FG_MASK)) | rgb;
  return 0;
}

static inline int
cell_rgb_set_bg(uint64_t* channels, int r, int g, int b){
  if(r >= 256 || g >= 256 || b >= 256){
    return -1;
  }
  if(r < 0 || g < 0 || b < 0){
    return -1;
  }
  uint64_t rgb = (r & 0xffull) << 16u;
  rgb |= (g & 0xffull) << 8u;
  rgb |= (b & 0xffull);
  rgb |= CELL_BGDEFAULT_MASK;
  *channels = (*channels & ~(CELL_BGDEFAULT_MASK | CELL_BG_MASK)) | rgb;
  return 0;
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

// is the cell simple (a lone ASCII character)?
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
const char* cell_extended_gcluster(const struct ncplane* n, const cell* c);
```

### Perf

notcurses tracks statistics across its operation, and a snapshot can be
acquired using the `notcurses_stats()` function. This function cannot fail.

```c
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
void notcurses_stats(const struct notcurses* nc, ncstats* stats);
```

Timings for renderings are across the breadth of `notcurses_render()`: they
include all per-render preprocessing, output generation, and dumping of the
output (including any sleeping while waiting on the terminal).

The notcurses rendering algorithm starts by moving the physical cursor to the
upper left corner of the visible screen (it does *not* clear the screen
beforehand). At each coordinate, it finds the topmost visible `ncplane`. There
will always be at least one `ncplane` visible at each coordinate, due to the
default plane. Once the plane is determined, the damage map is consulted to see
whether the cell need be redrawn. If so, it will be redrawn, and the virtual
cursor is updated based on the width of the output. Along the way, notcurses
attempts to minimize total amount of data written by eliding unnecessary color
and style specifications, and moving the cursor over large unchanged areas.

The worst case input frame (in terms of output size) is one whose colors change
from coordinate to coordinate, uses multiple combining characters within each
grapheme cluster, and the geometry is large. Peculiarities of the terminal
make it impossible to comment more meaningfully regarding delay.


## Included tools

Four binaries are built as part of notcurses:
* `notcurses-demo`: some demonstration code
* `notcurses-view`: renders visual media (images/videos)
* `notcurses-tester`: unit testing
* `notcurses-input`: decode and print keypresses

## Differences from NCURSES

The biggest difference, of course, is that notcurses is not an implementation
of X/Open (aka XSI) Curses, nor part of SUS4-2018.

The detailed differences between notcurses and NCURSES probably can't be fully
enumerated, and if they could, no one would want to read it. With that said,
some design decisions might surprise NCURSES programmers:

* The screen is not cleared on entry.
* There is no distinct `PANEL` type. The z-buffer is a fundamental property,
  and all drawable surfaces are ordered along the z axis. There is no
  equivalent to `update_panels()`.
* Scrolling is disabled by default, and cannot be globally enabled.
* The Curses `cchar_t` has a fixed-size array of `wchar_t`. The notcurses
  `cell` instead supports a UTF-8 encoded extended grapheme cluster of
  arbitrary length. The only supported charsets are `C` and `UTF-8`. notcurses
  does not generally make use of `wchar_t`.
* The hardware cursor is disabled by default, when supported (`civis` capability).
* Echoing of input is disabled by default, and `cbreak` mode is used by default.
* Colors are always specified as 24 bits in 3 components (RGB). If necessary,
  these will be quantized for the actual terminal. There are no "color pairs".
* There is no distinct "pad" concept (these are NCURSES `WINDOW`s created with
  the `newpad()` function). All drawable surfaces can exceed the display size.
* Multiple threads can freely call into notcurses, so long as they're not
  accessing the same data. In particular, it is always safe to concurrently
  mutate different ncplanes in different threads.
* NCURSES has thread-ignorant and thread-semi-safe versions, trace-enabled and
  traceless versions, and versions with and without support for wide characters.
  notcurses is one library: no tracing, UTF-8, thread safety.

### Features missing relative to NCURSES

This isn't "features currently missing", but rather "features I do not intend
to implement".

* There is no immediate-output mode (`immedok()`, `echochar()` etc.).
  `ncplane_putc()` followed by `notcurses_render()` ought be just as fast as
  `echochar()`.
* There is no support for soft labels (`slk_init()`, etc.).
* There is no concept of subwindows which share memory with their parents.
* There is no tracing functionality ala `trace(3NCURSES)`. Superior external
  tracing solutions exist, such as `bpftrace`.
* There is no timeout functionality for input (`timeout()`, `halfdelay()`, etc.).
  Roll your own with any of the four thousand ways to do it.

## Environment notes

* If your terminal has an option about default interpretation of "ambiguous-width
  characters" (this is actually a technical term from Unicode), ensure it is
  set to **Wide**, not narrow.

* If you can disable BiDi in your terminal, do so while running notcurses
  applications, until I have that handled better. notcurses doesn't recognize
  the BiDi state machine transitions, and thus merrily continues writing
  left-to-right. ﷽

## Useful links

* [BiDi in Terminal Emulators](https://terminal-wg.pages.freedesktop.org/bidi/)
* [The Xterm FAQ](https://invisible-island.net/xterm/xterm.faq.html)
* [The NCURSES FAQ](https://invisible-island.net/ncurses/ncurses.faq.html)
* [ECMA-35 Character Code Structure and Extension Techniques](https://www.ecma-international.org/publications/standards/Ecma-035.htm) (ISO/IEC 2022)
* [ECMA-43 8-bit Coded Character Set Structure and Rules](https://www.ecma-international.org/publications/standards/Ecma-043.htm)
* [ECMA-48 Control Functions for Coded Character Sets](https://www.ecma-international.org/publications/standards/Ecma-048.htm) (ISO/IEC 6429)
* [Unicode 12.1 Full Emoji List](https://unicode.org/emoji/charts/full-emoji-list.html)
* [Unicode Standard Annex #29 Text Segmentation](http://www.unicode.org/reports/tr29)
* [Unicode Standard Annex #15 Normalization Forms](https://unicode.org/reports/tr15/)
* [The TTY demystified](http://www.linusakesson.net/programming/tty/)
