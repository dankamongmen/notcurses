# Usage

As of version 2.0.0, the Notcurses API is stable, and the project is committed
to backwards compatibility.

* [Direct Mode](#direct-mode)
* [Alignment](#alignment)
* [Input](#input)
* [Planes](#planes) ([Plane Channels API](#plane-channels-api))
* [Cells](#cells) ([Cell Channels API](#cell-channels-api))
* [Reels](#reels) ([ncreel Examples](#ncreel-examples))
* [Widgets](#widgets) ([Plots](#plots)) ([Readers](#readers)) ([Progbars](#progbars)) ([Tabs](#tabs))
* [Channels](#channels)
* [Visuals](#visuals) ([QR codes](#qrcodes)) ([Multimedia](#multimedia)) ([Pixels](#pixels))
* [Stats](#stats)
* [C++](#c++)

A full API reference [is available](https://nick-black.com/notcurses/) in the
form of manual pages; these ought have been installed along with Notcurses. This document is a
secondary reference, and should not be considered authoritative. For a more
unified commentary, consider the [paperback](https://www.amazon.com/dp/B086PNVNC9)
(also available as a [free PDF](https://nick-black.com/dankwiki/index.php?title=Hacking_The_Planet!_with_Notcurses)).

A program wishing to use Notcurses will need to link it, ideally using the
output of `pkg-config --libs notcurses`. It is advised to compile with the
output of `pkg-config --cflags notcurses`. If using CMake, a support file is
provided, and can be accessed as `Notcurses`.

If your program makes no use of multimedia, you might want to link with only
the core Notcurses, and thus incur far fewer dependencies. To use the minimal
core Notcurses, use `pkg-config --libs notcurses-core` etc. In place of
`notcurses_init()` and/or `ncdirect_init()` (see below), you must also use
`notcurses_core_init()` and/or `ncdirect_core_init()`, or linking will fail.

Before calling into Notcurses—and usually as one of the first calls of the
program—be sure to call `setlocale(3)` with an appropriate UTF-8 locale. It is
usually appropriate to use `setlocale(LC_ALL, "")`, relying on the user to
properly set the `LANG` environment variable. Notcurses will refuse to start if
`nl_langinfo(3)` doesn't indicate `ANSI_X3.4-1968` or `UTF-8`. In addition, it
is wise to mask most signals early in the program, before any threads are
spawned (this is particularly critical for `SIGWINCH`).

Notcurses requires an available `terminfo(5)` definition appropriate for the
terminal. It is usually appropriate to pass `NULL` in the `termtype` field of a
`notcurses_options` struct, relying on the user to properly set the `TERM`
environment variable. This variable is usually set by the terminal itself. It
might be necessary to manually select a higher-quality definition for your
terminal, i.e. `xterm-direct` as opposed to `xterm` or `xterm-256color`.

Each terminal can be prepared via a call to `notcurses_init()`, which is
supplied a struct of type `notcurses_options`:

```c
// Get a human-readable string describing the running Notcurses version.
const char* notcurses_version(void);

// Cannot be inline, as we want to get the versions of the actual Notcurses
// library we loaded, not what we compile against.
void notcurses_version_components(int* major, int* minor, int* patch, int* tweak);

struct nccell;    // a coordinate on an ncplane: an EGC plus styling
struct ncplane;   // a drawable Notcurses surface, composed of cells
struct notcurses; // Notcurses state for a given terminal, composed of ncplanes

// These log levels consciously map cleanly to those of libav; Notcurses itself
// does not use this full granularity. The log level does not affect the opening
// and closing banners, which can be disabled via the Notcurses_option struct's
// 'suppress_banner'. Note that if stderr is connected to the same terminal on
// which we're rendering, any kind of logging will disrupt the output.
typedef enum {
  NCLOGLEVEL_SILENT,  // default. print nothing once fullscreen service begins
  NCLOGLEVEL_PANIC,   // print diagnostics immediately related to crashing
  NCLOGLEVEL_FATAL,   // we're hanging around, but we've had a horrible fault
  NCLOGLEVEL_ERROR,   // we can't keep doing this, but we can do other things
  NCLOGLEVEL_WARNING, // you probably don't want what's happening to happen
  NCLOGLEVEL_INFO,    // "standard information"
  NCLOGLEVEL_VERBOSE, // "detailed information"
  NCLOGLEVEL_DEBUG,   // this is honestly a bit much
  NCLOGLEVEL_TRACE,   // there's probably a better way to do what you want
} ncloglevel_e;

// Bits for notcurses_options->flags

// notcurses_init() will call setlocale() to inspect the current locale. If
// that locale is "C" or "POSIX", it will call setlocale(LC_ALL, "") to set
// the locale according to the LANG environment variable. Ideally, this will
// result in UTF8 being enabled, even if the client app didn't call
// setlocale() itself. Unless you're certain that you're invoking setlocale() 
// prior to notcurses_init(), you should not set this bit. Even if you are
// invoking setlocale(), this behavior shouldn't be an issue unless you're
// doing something weird (setting a locale not based on LANG).
#define NCOPTION_INHIBIT_SETLOCALE   0x0001

// We typically try to clear any preexisting bitmaps. If we ought *not* try
// to do this, pass NCOPTION_NO_CLEAR_BITMAPS. Note that they might still
// get cleared even if this is set, and they might not get cleared even if
// this is not set. It's a tough world out there.
#define NCOPTION_NO_CLEAR_BITMAPS    0x0002

// We typically install a signal handler for SIGWINCH that generates a resize
// event in the notcurses_get() queue. Set to inhibit this handler.
#define NCOPTION_NO_WINCH_SIGHANDLER 0x0004

// We typically install a signal handler for SIG{INT, SEGV, ABRT, QUIT} that
// restores the screen, and then calls the old signal handler. Set to inhibit
// registration of these signal handlers.
#define NCOPTION_NO_QUIT_SIGHANDLERS 0x0008

// Initialize the standard plane's virtual cursor to match the physical cursor
// at context creation time. Together with NCOPTION_NO_ALTERNATE_SCREEN and a
// scrolling standard plane, this facilitates easy scrolling-style programs in
// rendered mode.
#define NCOPTION_PRESERVE_CURSOR     0x0010

// Notcurses typically prints version info in notcurses_init() and performance
// info in notcurses_stop(). This inhibits that output.
#define NCOPTION_SUPPRESS_BANNERS    0x0020

// If smcup/rmcup capabilities are indicated, Notcurses defaults to making use
// of the "alternate screen". This flag inhibits use of smcup/rmcup.
#define NCOPTION_NO_ALTERNATE_SCREEN 0x0040

// Do not modify the font. Notcurses might attempt to change the font slightly,
// to support certain glyphs (especially on the Linux console). If this is set,
// no such modifications will be made. Note that font changes will not affect
// anything but the virtual console/terminal in which Notcurses is running.
#define NCOPTION_NO_FONT_CHANGES     0x0080

// Input may be freely dropped. This ought be provided when the program does not
// intend to handle input. Otherwise, input can accumulate in internal buffers,
// eventually preventing Notcurses from processing terminal messages.
#define NCOPTION_DRAIN_INPUT         0x0100

// Prepare the standard plane in scrolling mode, useful for CLIs. This is
// equivalent to calling ncplane_set_scrolling(notcurses_stdplane(nc), true).
#define NCOPTION_SCROLLING           0x0200ull

// "CLI mode" is just setting these four options.
#define NCOPTION_CLI_MODE (NCOPTION_NO_ALTERNATE_SCREEN \
                           |NCOPTION_NO_CLEAR_BITMAPS \
                           |NCOPTION_PRESERVE_CURSOR \
                           |NCOPTION_SCROLLING)

// Configuration for notcurses_init().
typedef struct notcurses_options {
  // The name of the terminfo database entry describing this terminal. If NULL,
  // the environment variable TERM is used. Failure to open the terminal
  // definition will result in failure to initialize Notcurses.
  const char* termtype;
  // Progressively higher log levels result in more logging to stderr. By
  // default, nothing is printed to stderr once fullscreen service begins.
  ncloglevel_e loglevel;
  // Desirable margins. If all are 0 (default), we will render to the entirety
  // of the screen. If the screen is too small, we do what we can--this is
  // strictly best-effort. Absolute coordinates are relative to the rendering
  // area ((0, 0) is always the origin of the rendering area).
  unsigned margin_t, margin_r, margin_b, margin_l;
  // General flags; see NCOPTION_*. This is expressed as a bitfield so that
  // future options can be added without reshaping the struct. Undefined bits
  // must be set to 0.
  uint64_t flags;
} notcurses_options;

// Lex a margin argument according to the standard Notcurses definition. There
// can be either a single number, which will define all margins equally, or
// there can be four numbers separated by commas.
int notcurses_lex_margins(const char* op, notcurses_options* opts);

// Initialize a Notcurses context on the connected terminal at 'fp'. 'fp' must
// be a tty. You'll usually want stdout. NULL can be supplied for 'fp', in
// which case /dev/tty will be opened. Returns NULL on error, including any
// failure initializing terminfo.
struct notcurses* notcurses_init(const notcurses_options* opts, FILE* fp);

// The same as notcurses_init(), but without any multimedia functionality,
// allowing for a svelter binary. Link with notcurses-core if this is used.
struct notcurses* notcurses_core_init(const notcurses_options* opts, FILE* fp);

// Destroy a Notcurses context.
int notcurses_stop(struct notcurses* nc);
```

`notcurses_stop` should be called before exiting your program to restore the
terminal settings and free resources.

An application can freely enter and exit the alternate screen:

```c
// Shift to the alternate screen, if available. If already using the alternate
// screen, this returns 0 immediately. If the alternate screen is not
// available, this returns -1 immediately. Entering the alternate screen turns
// off scrolling for the standard plane.
int notcurses_enter_alternate_screen(struct notcurses* nc);

// Exit the alternate screen. Immediately returns 0 if not currently using the
// alternate screen.
int notcurses_leave_alternate_screen(struct notcurses* nc);
```

Notcurses does not typically generate diagnostics (aside from the intro banner
and outro performance summary). When `stderr` is connected to the same terminal
to which graphics are being printed, printing to stderr will corrupt the output.
Setting `loglevel` to a value higher than `NCLOGLEVEL_SILENT` will cause
diagnostics to be printed to `stderr`: you could ensure `stderr` is redirected
if you make use of this functionality.

It's probably wise to export `NCOPTION_NO_ALTERNATE_SCREEN` to the user (e.g.
via command line option or environment variable). Motivated users might
appreciate the ability to manipulate `loglevel`. The remaining options are
typically of use only to application authors.

The Notcurses API draws almost entirely into the virtual buffers of `ncplane`s.
Only upon a call to `notcurses_render` will the visible terminal display be
updated to reflect the changes:

```c
// Renders the pile of which 'n' is a part. Rendering this pile again will blow
// away the render. To actually write out the render, call ncpile_rasterize().
int ncpile_render(struct ncplane* n);

// Make the physical screen match the last rendered frame from the pile of
// which 'n' is a part. This is a blocking call. Don't call this before the
// pile has been rendered (doing so will likely result in a blank screen).
int ncpile_rasterize(struct ncplane* n);

// Make the physical screen match the virtual screen. Changes made to the
// virtual screen (i.e. most other calls) will not be visible until after a
// successful call to notcurses_render().
int notcurses_render(struct notcurses* nc);

// Perform the rendering and rasterization portion of notcurses_render(), but
// do not write the resulting buffer out to the terminal. The returned buffer
// must be freed by the caller.
int ncpile_render_to_buffer(struct ncplane* p, char** buf, size_t* buflen);

// Write the last rendered frame, in its entirety, to 'fp'. If a frame has
// not yet been rendered, nothing will be written.
int ncpile_render_to_file(struct ncplane* p, FILE* fp);

int ncpile_render_to_file(struct ncplane* p, FILE* fp);

// Retrieve the contents of the specified cell as last rendered. The EGC is
// returned, or NULL on error. This EGC must be free()d by the caller. The
// styles and channels are written to 'attrword' and 'channels', respectively.
char* notcurses_at_yx(struct notcurses* nc, unsigned yoff, unsigned xoff,
                      uint16_t* styles, uint64_t* channels);
```

One `ncplane` is guaranteed to exist: the "standard plane". The user cannot
move, resize, reparent, or destroy the standard plane (it *can* be erased).
Its dimensions always match notcurses's conception of the visible terminal. A
handle on the standard plane can be acquired with two top-level functions:

```c
// Get a reference to the standard plane (one matching our current idea of the
// terminal size) for this terminal. The standard plane always exists, and its
// origin is always at the uppermost, leftmost cell of the screen.
struct ncplane* notcurses_stdplane(struct notcurses* nc);
const struct ncplane* notcurses_stdplane_const(const struct notcurses* nc);

// notcurses_stdplane(), plus free bonus dimensions written to non-NULL y/x!
static inline struct ncplane*
notcurses_stddim_yx(struct notcurses* nc, unsigned* restrict y, unsigned* restrict x){
  struct ncplane* s = notcurses_stdplane(nc); // can't fail
  ncplane_dim_yx(s, y, x); // accepts NULL
  return s;
}

static inline const struct ncplane*
notcurses_stddim_yx_const(const struct notcurses* nc, unsigned* restrict y, unsigned* restrict x){
  const struct ncplane* s = notcurses_stdplane_const(nc); // can't fail
  ncplane_dim_yx(s, y, x); // accepts NULL
  return s;
}
```

A reference to the standard plane *is* persistent across a screen resize, as are
any indexes into its egcpool, but its framebuffer *is not* necessarily
persistent across a screen resize. Thankfully, you shouldn't have a reference
to its framebuffer, and thus only the change to its dimensions can really catch
you off guard.

Utility functions operating on the toplevel `notcurses` object include:

```c
// Return the topmost ncplane of the standard pile.
struct ncplane* notcurses_top(struct notcurses* n);

// Return the bottommost ncplane of the standard pile.
struct ncplane* notcurses_bottom(struct notcurses* n);

// Return our current idea of the terminal dimensions in rows and cols.
static inline void
notcurses_term_dim_yx(const struct notcurses* n, unsigned* restrict rows,
                      unsigned* restrict cols){
  ncplane_dim_yx(notcurses_stdplane_const(n), rows, cols);
}

// Refresh the physical screen to match what was last rendered (i.e., without
// reflecting any changes since the last call to notcurses_render()). This is
// primarily useful if the screen is externally corrupted, or if an
// NCKEY_RESIZE event has been read and you're not yet ready to render. The
// current screen geometry is returned in 'y' and 'x', if they are not NULL.
int notcurses_refresh(struct notcurses* n, unsigned* restrict y, unsigned* restrict x);

// Get the default foreground color, if it is known. Returns -1 on error
// (unknown foreground). On success, returns 0, writing the RGB value to
// 'fg' (if non-NULL)
int notcurses_default_foreground(const struct notcurses* nc, uint32_t* fg);

// Get the default background color, if it is known. Returns -1 on error
// (unknown background). On success, returns 0, writing the RGB value to
// 'bg' (if non-NULL) and setting 'bgtrans' high iff the background color
// is treated as transparent.
int notcurses_default_background(const struct notcurses* nc,
                                 uint32_t* bg, unsigned* bgtrans);

// Enable or disable the terminal's cursor, if supported, placing it at
// 'y', 'x'. Immediate effect (no need for a call to notcurses_render()).
// It is an error if 'y', 'x' lies outside the standard plane. Can be
// called while already visible to move the cursor.
int notcurses_cursor_enable(struct notcurses* nc, int y, int x);

// Disable the hardware cursor. It is an error to call this while the
// cursor is already disabled.
int notcurses_cursor_disable(struct notcurses* nc);

// Get the current location of the terminal's cursor, whether visible or not.
int notcurses_cursor_yx(const struct notcurses* nc, int* y, int* x);

// Returns a 16-bit bitmask in the LSBs of supported curses-style attributes
// (NCSTYLE_UNDERLINE, NCSTYLE_BOLD, etc.) The attribute is only
// indicated as supported if the terminal can support it together with color.
// For more information, see the "ncv" capability in terminfo(5).
unsigned notcurses_supported_styles(const struct notcurses* nc);

// Returns the number of simultaneous colors claimed to be supported, or 1 if
// there is no color support. Note that several terminal emulators advertise
// more colors than they actually support, downsampling internally.
unsigned notcurses_palette_size(const struct notcurses* nc);

// Can we fade? Fading requires either the "rgb" or "ccc" terminfo capability.
bool notcurses_canfade(const struct notcurses* nc);

// Can we directly specify RGB values per cell, or only use palettes?
bool notcurses_cantruecolor(const struct notcurses* nc);

// Can we load images? This requires being built against FFmpeg/OIIO.
bool notcurses_canopen_images(const struct notcurses* nc);

// Can we load videos? This requires being built against FFmpeg.
bool notcurses_canopen_videos(const struct notcurses* nc);

// Can we change colors in the hardware palette? Requires "ccc" and "initc".
bool notcurses_canchangecolor(const struct notcurses* nc);

// Is our encoding UTF-8? Requires LANG being set to a UTF-8 locale.
bool notcurses_canutf8(const struct notcurses* nc);

// Can we reliably use Unicode halfblocks?
bool notcurses_canhalfblock(const struct notcurses* nc);

// Can we reliably use Unicode quadrants?
bool notcurses_canquadrant(const struct notcurses* nc);

// Can we draw sextants? This requires Unicode 13.
bool notcurses_cansextants(const struct notcurses* nc);

// Can we draw Braille? The Linux console cannot.
bool notcurses_canbraille(const struct notcurses* nc);

// Can we draw bitmaps?
bool notcurses_canpixel(const struct notcurses* nc);

// pixel blitting implementations. informative only; don't special-case
// based off any of this information!
typedef enum {
  NCPIXEL_NONE = 0,
  NCPIXEL_SIXEL,           // sixel
  NCPIXEL_LINUXFB,         // linux framebuffer
  NCPIXEL_ITERM2,          // iTerm2
  // C=1 (disabling scrolling) was only introduced in 0.20.0, at the same
  // time as animation. prior to this, graphics had to be entirely redrawn
  // on any change, and it wasn't possible to use the bottom line.
  NCPIXEL_KITTY_STATIC,
  // until 0.22.0's introduction of 'a=c' for self-referential composition, we
  // had to keep a complete copy of the RGBA data, in case a wiped cell needed
  // to be rebuilt. we'd otherwise have to unpack the glyph and store it into
  // the auxvec on the fly.
  NCPIXEL_KITTY_ANIMATED,
  // with 0.22.0, we only ever write transparent cells after writing the
  // original image (which we now deflate, since we needn't unpack it later).
  // the only data we need keep is the auxvecs.
  NCPIXEL_KITTY_SELFREF,
} ncpixelimpl_e;

// Returns a non-zero constant corresponding to some pixel-blitting
// mechanism if bitmap support (via any mechanism) has been detected,
// or else 0 (NCPIXEL_NONE).
ncpixelimpl_e notcurses_check_pixel_support(struct notcurses* nc);

// Returns a heap-allocated copy of the user name under which we are running.
char* notcurses_accountname(void);

// Returns a heap-allocated copy of the local host name.
char* notcurses_hostname(void);

// Returns a heap-allocated copy of human-readable OS name and version.
char* notcurses_osversion(void);
```

## Direct mode

"Direct mode" makes a limited subset of Notcurses is available for manipulating
typical scrolling or file-backed output. Its functions are exported via
`<notcurses/direct.h>`, and output directly and immediately to the provided
`FILE*`. `notcurses_render()` is neither supported nor necessary for such an
instance. Use `ncdirect_init()` to create a direct mode context:

```c
struct ncdirect; // minimal state for a terminal

// Initialize a direct-mode Notcurses context on the connected terminal at 'fp'.
// 'fp' must be a tty. You'll usually want stdout. Direct mode supports a
// limited subset of Notcurses routines which directly affect 'fp', and neither
// supports nor requires notcurses_render(). This can be used to add color and
// styling to text in the standard output paradigm. 'flags' is a bitmask over
// NCDIRECT_OPTION_*.
// Returns NULL on error, including any failure initializing terminfo.
struct ncdirect* ncdirect_init(const char* termtype, FILE* fp, uint64_t flags);

// The same as ncdirect_init(), but without any multimedia functionality,
// allowing for a svelter binary. Link with notcurses-core if this is used.
struct ncdirect* ncdirect_core_init(const char* termtype, FILE* fp, uint64_t flags);

// ncdirect_init() will call setlocale() to inspect the current locale. If
// that locale is "C" or "POSIX", it will call setlocale(LC_ALL, "") to set
// the locale according to the LANG environment variable. Ideally, this will
// result in UTF8 being enabled, even if the client app didn't call
// setlocale() itself. Unless you're certain that you're invoking setlocale()
// prior to notcurses_init(), you should not set this bit. Even if you are
// invoking setlocale(), this behavior shouldn't be an issue unless you're
// doing something weird (setting a locale not based on LANG).
#define NCDIRECT_OPTION_INHIBIT_SETLOCALE   0x0001ull

// *Don't* place the terminal into cbreak mode (see tcgetattr(3)). By default,
// echo and line buffering are turned off.
#define NCDIRECT_OPTION_INHIBIT_CBREAK      0x0002ull

// Input may be freely dropped. This ought be provided when the program does not
// intend to handle input. Otherwise, input can accumulate in internal buffers,
// eventually preventing Notcurses from processing terminal messages.
#define NCDIRECT_OPTION_DRAIN_INPUT         0x0004ull

// We typically install a signal handler for SIG{INT, SEGV, ABRT, QUIT} that
// restores the screen, and then calls the old signal handler. Set to inhibit
// registration of these signal handlers. Chosen to match fullscreen mode.
#define NCDIRECT_OPTION_NO_QUIT_SIGHANDLERS 0x0008ull

// Enable logging (to stderr) at the NCLOGLEVEL_WARNING level.
#define NCDIRECT_OPTION_VERBOSE             0x0010ull

// Enable logging (to stderr) at the NCLOGLEVEL_TRACE level. This will enable
// all diagnostics, a superset of NCDIRECT_OPTION_VERBOSE (which this implies).
#define NCDIRECT_OPTION_VERY_VERBOSE        0x0020ull

// Release 'nc' and any associated resources. 0 on success, non-0 on failure.
int ncdirect_stop(struct ncdirect* nc);
```

This context must be destroyed using `ncdirect_stop()`. The following functions
are available for direct mode:

```c
// Read a (heap-allocated) newline-delimited chunk of text. Returns NULL on
// failure. The NCDIRECT_OPTION_INHIBIT_CBREAK flag ought not be used together
// with this function, or the line-editing keybindings cannot be honored.
char* ncdirect_readline(struct ncdirect* nc, const char* prompt);

int ncdirect_fg_rgb(struct ncdirect* nc, unsigned rgb);
int ncdirect_bg_rgb(struct ncdirect* nc, unsigned rgb);

static inline int
ncdirect_bg_rgb8(struct ncdirect* nc, unsigned r, unsigned g, unsigned b){
  if(r > 255 || g > 255 || b > 255){
    return -1;
  }
  return ncdirect_bg_rgb(nc, (r << 16u) + (g << 8u) + b);
}

static inline int
ncdirect_fg_rgb8(struct ncdirect* nc, unsigned r, unsigned g, unsigned b){
  if(r > 255 || g > 255 || b > 255){
    return -1;
  }
  return ncdirect_fg_rgb(nc, (r << 16u) + (g << 8u) + b);
}

// Get the current number of columns/rows.
int ncdirect_dim_x(const struct ncdirect* nc);
int ncdirect_dim_y(const struct ncdirect* nc);

int ncdirect_fg_default(struct ncdirect* nc);
int ncdirect_bg_default(struct ncdirect* nc);
int ncdirect_styles_set(struct ncdirect* n, unsigned stylebits);
int ncdirect_styles_on(struct ncdirect* n, unsigned stylebits);
int ncdirect_styles_off(struct ncdirect* n, unsigned stylebits);
unsigned ncdirect_styles(struct ncdirect* n);
int ncdirect_clear(struct ncdirect* nc); // clear the screen

// Move the cursor in direct mode. -1 to retain current location on that axis.
int ncdirect_cursor_move_yx(struct ncdirect* n, int y, int x);
int ncdirect_cursor_enable(struct ncdirect* nc);
int ncdirect_cursor_disable(struct ncdirect* nc);

// Relative moves. num < 0 is an error.
int ncdirect_cursor_up(struct ncdirect* nc, int num);
int ncdirect_cursor_left(struct ncdirect* nc, int num);
int ncdirect_cursor_right(struct ncdirect* nc, int num);
int ncdirect_cursor_down(struct ncdirect* nc, int num);

// Get the cursor position, when supported. This requires writing to the
// terminal, and then reading from it. If the terminal doesn't reply, or
// doesn't reply in a way we understand, the results might be deleterious.
int ncdirect_cursor_yx(struct ncdirect* n, unsigned* y, unsigned* x);

// Push or pop the cursor location to the terminal's stack. The depth of this
// stack, and indeed its existence, is terminal-dependent.
int ncdirect_cursor_push(struct ncdirect* n);
int ncdirect_cursor_pop(struct ncdirect* n);

// Formatted printing (plus alignment relative to the terminal).
int ncdirect_printf_aligned(struct ncdirect* n, int y, ncalign_e align,
                            const char* fmt, ...);

// Output the string |utf8| according to the channels |channels|. Note that
// ncdirect_putstr() does not explicitly flush output buffers, so it will not
// necessarily be immediately visible.
int ncdirect_putstr(struct ncdirect* nc, uint64_t channels, const char* utf8);

// Output a single EGC (this might be several characters) from |utf8|,
// according to the channels |channels|. On success, the number of columns
// thought to have been used is returned, and if |sbytes| is not NULL,
// the number of bytes consumed will be written there.
int ncdirect_putegc(struct ncdirect* nc, uint64_t channels,
                    const char* utf8, int* sbytes);

// Draw horizontal/vertical lines using the specified channels, interpolating
// between them as we go. The EGC may not use more than one column. For a
// horizontal line, |len| cannot exceed the screen width minus the cursor's
// offset. For a vertical line, it may be as long as you'd like; the screen
// will scroll as necessary. All lines start at the current cursor position.
// A length of 0 is an error, resulting in a return of -1.
int ncdirect_hline_interp(struct ncdirect* n, const char* egc,
                          unsigned len, uint64_t h1, uint64_t h2);
int ncdirect_vline_interp(struct ncdirect* n, const char* egc,
                          unsigned len, uint64_t h1, uint64_t h2);

// Draw a box with its upper-left corner at the current cursor position, having
// dimensions |ylen|x|xlen|. See ncplane_box() for more information. The
// minimum box size is 2x2, and it cannot be drawn off-screen. |wchars| is an
// array of 6 wide characters: UL, UR, LL, LR, HL, VL.
int ncdirect_box(struct ncdirect* n, uint64_t ul, uint64_t ur,
                 uint64_t ll, uint64_t lr, const wchar_t* wchars,
                 int ylen, int xlen, unsigned ctlword);

// ncdirect_box() with the rounded box-drawing characters
int ncdirect_rounded_box(struct ncdirect* n, uint64_t ul, uint64_t ur,
                         uint64_t ll, uint64_t lr,
                         int ylen, int xlen, unsigned ctlword);

// ncdirect_box() with the double box-drawing characters
int ncdirect_double_box(struct ncdirect* n, uint64_t ul, uint64_t ur,
                        uint64_t ll, uint64_t lr,
                        int ylen, int xlen, unsigned ctlword);

// Display an image using the specified blitter and scaling. The image may
// be arbitrarily many rows -- the output will scroll -- but will only occupy
// the column of the cursor, and those to the right.
int ncdirect_render_image(struct ncdirect* nc, const char* filename,
                          ncblitter_e blitter, ncscale_e scale);

// Render an image using the specified blitter and scaling, but do not write
// the result. The image may be arbitrarily many rows -- the output will scroll
// -- but will only occupy the column of the cursor, and those to the right.
// To actually write (and free) this, invoke ncdirect_raster_frame(). 'maxx'
// and 'maxy', if greater than 0, are used for scaling; the terminal's geometry
// is otherwise used. It is an error to pass a negative 'maxy' or 'maxx'.
ncdirectv* ncdirect_render_frame(struct ncdirect* n, const char* filename,
                                 ncblitter_e blitter, ncscale_e scale,
                                 int maxy, int maxx);

// Takes the result of ncdirect_render_frame() and writes it to the output.
int ncdirect_raster_frame(struct ncdirect* n, ncdirectv* ncdv, ncalign_e align);
```

Several of the Notcurses capability predicates have `ncdirect` analogues:

```c
bool ncdirect_canopen_images(const struct ncdirect* n);
bool ncdirect_canutf8(const struct ncdirect* n);
```

## Alignment

Most functions that generate output can be aligned relative to an ncplane.
Alignment currently comes in three forms: `NCALIGN_LEFT`, `NCALIGN_CENTER`, and
`NCALIGN_RIGHT`.

```c
// Alignment within the ncplane. Left/right-justified, or centered.
typedef enum {
  NCALIGN_LEFT,
  NCALIGN_CENTER,
  NCALIGN_RIGHT,
} ncalign_e;

#define NCALIGN_TOP NCALIGN_LEFT
#define NCALIGN_BOTTOM NCALIGN_RIGHT

// Return the offset into 'availu' at which 'u' ought be output given the
// requirements of 'align'. Return -INT_MAX on invalid 'align'. Undefined
// behavior on negative 'availu' or 'u'.
static inline int
notcurses_align(int availu, ncalign_e align, int u){
  if(align == NCALIGN_LEFT || align == NCALIGN_TOP){
    return 0;
  }
  if(align == NCALIGN_CENTER){
    return (availu - u) / 2;
  }
  if(align == NCALIGN_RIGHT || align == NCALIGN_BOTTOM){
    return availu - u;
  }
  return -INT_MAX; // invalid |align|
}

// Return the column at which 'c' cols ought start in order to be aligned
// according to 'align' within ncplane 'n'. Return -INT_MAX on invalid
// 'align'. Undefined behavior on negative 'c'.
static inline int
ncplane_align(const struct ncplane* n, ncalign_e align, int c){
  return notcurses_align(ncplane_dim_x(n), align, c);
}

```

## Input

Input can currently be taken only from `stdin`, but on the plus side, stdin
needn't be a terminal device (unlike the ttyfp `FILE*` passed to `notcurses_init()`).
Generalized input ought happen soon. There is only one input queue per `struct
notcurses`.

Like NCURSES, Notcurses will watch for escape sequences, check them against the
terminfo database, and return them as special keys (we hijack the Private Use
Area for special keys, specifically Supplementary Private Use Area B (u100000
through u10ffffd). Unlike NCURSES, the fundamental unit of input is the
UTF8-encoded Unicode codepoint. Note, however, that only one codepoint is
returned at a time (as opposed to an entire EGC).

It is generally possible for a false positive to occur, wherein keypresses
intended to be distinct are combined into an escape sequence. False negatives
where an intended escape sequence are read as an ESC key followed by distinct
keystrokes are also possible. NCURSES provides the `ESCDELAY` variable to
control timing. Notcurses brooks no delay; all characters of an escape sequence
must be readable without delay for it to be interpreted as such.

```c
// All input is taken from stdin. We attempt to read a single UTF8-encoded
// Unicode codepoint, *not* an entire Extended Grapheme Cluster. It is also
// possible that we will produce a synthesized event, i.e. anything that
// doesn't correspond to a Unicode codepoint (e.g. arrow keys, function keys,
// screen resize events, etc.). The full list of synthesized events can be
// found in <notcurses/nckeys.h>.
//
// notcurses_get_nblock() is nonblocking. notcurses_get_blocking() blocks
// until a codepoint or special key is read, or until interrupted by a signal.
// notcurses_get() allows an optional timeout to be controlled.
//
// In the case of a valid read, the provided ncinput is filled in, and the
// id field is returned. 0 is returned to indicate that no input was available.
// On any error, (uint32_t)-1 is returned.

// An input event. Cell coordinates are currently defined only for mouse events.
typedef struct ncinput {
  uint32_t id;     // identifier (unicode codepoint or synthesized NCKEY event)
  int y, x;        // y/x cell coordinate of event, -1 for undefined
  bool alt;        // was alt held?
  bool shift;      // was shift held?
  bool ctrl;       // was ctrl held?
  enum {
    NCTYPE_UNKNOWN,
    NCTYPE_PRESS,
    NCTYPE_REPEAT,
    NCTYPE_RELEASE,
  } evtype;
  int ypx, xpx;      // pixel offsets within cell, -1 for undefined
} ncinput;

// Read a UTF-32-encoded Unicode codepoint from input. This might only be part
// of a larger EGC. Provide a NULL 'ts' to block at length, and otherwise a
// timespec specifying an absolute deadline calculated using CLOCK_MONOTONIC.
// Returns a single Unicode code point, or a synthesized special key constant,
// or (uint32_t)-1 on error. Returns 0 on a timeout. If an event is processed,
// the return value is the 'id' field from that event. 'ni' may be NULL.
uint32_t notcurses_get(struct notcurses* n, const struct timespec* ts,
                       ncinput* ni);

// Acquire up to 'vcount' ncinputs at the vector 'ni'. The number read will be
// returned, or -1 on error without any reads, 0 on timeout.
int notcurses_getvec(struct notcurses* n, const struct timespec* ts,
                     ncinput* ni, int vcount);

// 'ni' may be NULL if the caller is uninterested in event details. If no
// event is ready, returns 0.
static inline uint32_t
notcurses_get_nblock(struct notcurses* n, ncinput* ni){
  struct timespec ts = { .tv_sec = 0, .tv_nsec = 0 };
  return notcurses_get(n, &ts, ni);
}

// 'ni' may be NULL if the caller is uninterested in event details. Blocks
// until an event is processed or a signal is received.
static inline uint32_t
notcurses_get_blocking(struct notcurses* n, ncinput* ni){
  return notcurses_get(n, NULL, ni);
}

static inline bool
ncinput_nomod_p(const ncinput* ni){
  return !(ni->modifiers);
}
```

By default, certain keys are mapped to signals by the terminal's line
discipline. This can be disabled with `notcurses_linesigs_disable()`, and
reenabled with `notcurses_linesigs_enable()`.

```c
// Disable signals originating from the terminal's line discipline, i.e.
// SIGINT (^C), SIGQUIT (^\), and SIGTSTP (^Z). They are enabled by default.
int notcurses_linesigs_disable(struct notcurses* n);

// Restore signals originating from the terminal's line discipline, i.e.
// SIGINT (^C), SIGQUIT (^\), and SIGTSTP (^Z), if disabled.
int notcurses_linesigs_enable(struct notcurses* n);
```

## Mice

notcurses supports mice, though only through brokers such as X or
[GPM](https://www.nico.schottelius.org/software/gpm/). It does not speak
directly to hardware. Mouse events must be explicitly enabled with a
successful call to `notcurses_mouse_enable()`, and can later be disabled.

```c
// Enable the mouse in "button-event tracking" mode with focus detection and
// UTF8-style extended coordinates. On failure, -1 is returned. On success, 0
// is returned, and mouse events will be published to notcurses_get().
int notcurses_mouse_enable(struct notcurses* n);

// Disable mouse events. Any events in the input queue can still be delivered.
int notcurses_mouse_disable(struct notcurses* n);

// Is the event a synthesized mouse event?
static inline bool
nckey_mouse_p(uint32_t r){
  return r >= NCKEY_BUTTON1 && r <= NCKEY_BUTTON11;
}
```

"Button-event tracking mode" implies the ability to detect mouse button
presses, and also mouse movement while holding down a mouse button (i.e. to
effect drag-and-drop). Mouse events are returned via the `NCKEY_MOUSE*` values,
with coordinate information in the `ncinput` struct.

## Planes

Fundamental to Notcurses is a z-buffer of rectilinear virtual screens, known
as `ncplane`s. An `ncplane` can be larger than the physical screen, or smaller,
or the same size; it can be entirely contained within the physical screen, or
overlap in part, or lie wholly beyond the boundaries, never to be rendered.
In addition to its framebuffer--a rectilinear matrix of `nccell`s
(see [Cells](#cells))--an `ncplane` is defined by:

* a base `nccell`, used for any cell on the plane without a glyph,
* the egcpool backing its `nccell`s,
* a current cursor location,
* a current style, foreground channel, and background channel,
* its geometry,
* a configured user curry (a `void*`),
* its position relative to the visible plane,
* its z-index, and
* an optional resize callback,
* a name (used only for debugging).

If opaque, a `nccell` on a higher `ncplane` completely obstructs a corresponding
`nccell` from a lower `ncplane` from being seen. An `ncplane` corresponds loosely
to an [NCURSES Panel](https://invisible-island.net/ncurses/ncurses-intro.html#panels),
but is the primary drawing surface of notcurses—there is no object
corresponding to a bare NCURSES `WINDOW`.

An `ncplane` can be created aligned relative to an existing `ncplane`
(including the standard plane) using `NCPLANE_OPTION_HORALIGNED`.

When an `ncplane` is no longer needed, free it with
`ncplane_destroy()`. To quickly reset the `ncplane`, use `ncplane_erase()`.

```c
// Horizontal alignment relative to the parent plane. Use ncalign_e for 'x'.
#define NCPLANE_OPTION_HORALIGNED   0x0001ull
// Vertical alignment relative to the parent plane. Use ncalign_e for 'y'.
#define NCPLANE_OPTION_VERALIGNED   0x0002ull
// Maximize relative to the parent plane, modulo the provided margins. The
// margins are best-effort; the plane will always be at least 1 column by
// 1 row. If the margins can be effected, the plane will be sized to all
// remaining space. 'y' and 'x' are overloaded as the top and left margins
// when this flag is used. 'rows' and 'cols' must be 0 when this flag is
// used. This flag is exclusive with both of the alignment flags.
#define NCPLANE_OPTION_MARGINALIZED 0x0004ull
// If this plane is bound to a scrolling plane, it ought *not* scroll along
// with the parent (it will still move with the parent, maintaining its
// relative position, if the parent is moved to a new location).
#define NCPLANE_OPTION_FIXED        0x0008ull
// Enable automatic growth of the plane to accommodate output. Creating a
// plane with this flag is equivalent to immediately calling
// ncplane_set_autogrow(p, true) following plane creation.
#define NCPLANE_OPTION_AUTOGROW     0x0010ull
// Enable vertical scrolling of the plane to accommodate output. Creating a
// plane with this flag is equivalent to immediately calling
// ncplane_set_scrolling(p, true) following plane creation.
#define NCPLANE_OPTION_VSCROLL      0x0020ull

typedef struct ncplane_options {
  int y;            // vertical placement relative to parent plane
  int x;            // horizontal placement relative to parent plane
  int rows;         // rows, must be positive (unless NCPLANE_OPTION_MARGINALIZED)
  int cols;         // columns, must be positive (unless NCPLANE_OPTION_MARGINALIZED)
  void* userptr;    // user curry, may be NULL
  const char* name; // name (used only for debugging), may be NULL
  int (*resizecb)(struct ncplane*); // callback when parent is resized
  uint64_t flags;   // closure over NCPLANE_OPTION_*
  unsigned margin_b, margin_r; // margins (require NCPLANE_OPTION_MARGINALIZED)
} ncplane_options;

// Create a new ncplane bound to plane 'n', at the offset 'y'x'x' (relative to
// the origin of 'n') and the specified size. The number of 'rows' and 'cols'
// must both be positive. This plane is initially at the top of the z-buffer,
// as if ncplane_move_top() had been called on it. The void* 'userptr' can be
// retrieved (and reset) later. A 'name' can be set, used in debugging.
struct ncplane* ncplane_create(struct ncplane* n, const ncplane_options* nopts);

// Plane 'n' will be unbound from its parent plane, and will be made a bound
// child of 'newparent'. It is an error if 'n' or 'newparent' are NULL. If
// 'newparent' is equal to 'n', 'n' becomes the root of a new pile, unless 'n'
// is already the root of a pile, in which case this is a no-op. Returns 'n'.
// The standard plane cannot be reparented. Any planes bound to 'n' are
// reparented to the previous parent of 'n'.
struct ncplane* ncplane_reparent(struct ncplane* n, struct ncplane* newparent);

// The same as ncplane_reparent(), except any planes bound to 'n' come along
// with it to its new destination. Their z-order is maintained.
struct ncplane* ncplane_reparent_family(struct ncplane* n, struct ncplane* newparent);

// Destroy ncplane 'n'. None of its contents will be visible after the next
// call to notcurses_render(). It is an error to attempt to destroy the
// standard plane. It is a noop if 'n' is NULL.
int ncplane_destroy(struct ncplane* n);

// Destroy ncplane 'n' and all its bound descendants. It is a noop if 'n'
// is NULL. It is an error to attempt to destroy the standard plane.
int ncplane_family_destroy(struct ncplane *n);

// Replace the ncplane's existing resizecb with 'resizecb' (which may be NULL).
void ncplane_set_resizecb(struct ncplane* n, int(*resizecb)(struct ncplane*));

// Returns the ncplane's current resize callback.
int (*ncplane_resizecb(const struct ncplane* n))(struct ncplane*);

// Suitable for use as a 'resizecb', this will resize the plane to the visual
// region's size. It is used for the standard plane.
int ncplane_resize_maximize(struct ncplane* n);

// Suitable for use as a 'resizecb' with planes created with
// NCPLANE_OPTION_MARGINALIZED. This will resize the plane 'n' against its
// parent, attempting to enforce the supplied margins.
int ncplane_resize_marginalized(struct ncplane* n);

// Suitable for use as a 'resizecb'. This will realign the plane 'n' against
// its parent, using the alignment specified at ncplane_create()-time.
int ncplane_resize_realign(struct ncplane* n);

// Get the plane to which the plane 'n' is bound, if any.
struct ncplane* ncplane_parent(struct ncplane* n);
const struct ncplane* ncplane_parent_const(const struct ncplane* n);

// Set the plane's name, replacing any current name.
int ncplane_set_name(struct ncplane* n, const char* name);

// Return a heap-allocated copy of the plane's name, or NULL if it has none.
char* ncplane_name(const struct ncplane* n);

// Duplicate an existing ncplane. The new plane will have the same geometry,
// will duplicate all content, and will start with the same rendering state.
struct ncplane* ncplane_dup(struct ncplane* n, void* opaque);

// Merge the ncplane 'src' down onto the ncplane 'dst'. This is most rigorously
// defined as "write to 'dst' the frame that would be rendered were the entire
// stack made up only of the specified subregion of 'src' and, below it, the
// subregion of 'dst' having the specified origin. Supply -1 to indicate the
// current cursor position in the relevant dimension. Merging is independent of
// the position of 'src' viz 'dst' on the z-axis. It is an error to define a
// subregion that is not entirely contained within 'src'. It is an error to
// define a target origin such that the projected subregion is not entirely
// contained within 'dst'.  Behavior is undefined if 'src' and 'dst' are
// equivalent. 'dst' is modified, but 'src' remains unchanged. Neither 'src'
// nor 'dst' may have sprixels. Lengths of 0 mean "everything left".
int ncplane_mergedown(struct ncplane* RESTRICT src, struct ncplane* RESTRICT dst,
                      int begsrcy, int begsrcx, unsigned leny, unsigned lenx,
                      int dsty, int dstx);

// Merge the entirety of 'src' down onto the ncplane 'dst'. If 'src' does not
// intersect with 'dst', 'dst' will not be changed, but it is not an error.
int ncplane_mergedown_simple(struct ncplane* restrict src,
                             struct ncplane* restrict dst);

// Erase every cell in the ncplane, resetting all attributes to normal, all
// colors to the default color, and all cells to undrawn. All cells associated
// with this ncplane are invalidated, and must not be used after the call,
// excluding the base cell. The cursor is homed.
void ncplane_erase(struct ncplane* n);

// Erase every cell in the region starting at {ystart, xstart} and having size
// {|ylen|x|xlen|} for non-zero lengths. If ystart and/or xstart are -1, the current
// cursor position along that axis is used; other negative values are an error. A
// negative ylen means to move up from the origin, and a negative xlen means to move
// left from the origin. A positive ylen moves down, and a positive xlen moves right.
// A value of 0 for the length erases everything along that dimension. It is an error
// if the starting coordinate is not in the plane, but the ending coordinate may be
// outside the plane.
//
// For example, on a plane of 20 rows and 10 columns, with the cursor at row 10 and
// column 5, the following would hold:
//
//  (-1, -1, 0, 1): clears the column to the right of the cursor (column 6)
//  (-1, -1, 0, -1): clears the column to the left of the cursor (column 4)
//  (-1, -1, INT_MAX, 0): clears all rows with or below the cursor (rows 10--19)
//  (-1, -1, -INT_MAX, 0): clears all rows with or above the cursor (rows 0--10)
//  (-1, 4, 3, 3): clears from row 5, column 4 through row 7, column 6
//  (-1, 4, -3, -3): clears from row 5, column 4 through row 3, column 2
//  (4, -1, 0, 3): clears columns 5, 6, and 7
//  (-1, -1, 0, 0): clears the plane *if the cursor is in a legal position*
//  (0, 0, 0, 0): clears the plane in all cases
int ncplane_erase_region(struct ncplane* n, int ystart, int xstart,
                         int ylen, int xlen);
```

All planes, including the standard plane, are created with scrolling disabled.
Attempting to print past the end of a line will stop at the plane boundary,
and indicate an error. On a plane 10 columns wide and two rows high, printing
"0123456789" at the origin should succeed, but printing "01234567890" will by
default fail at the eleventh character. In either case, the cursor will be left
at location 0x10; it must be moved before further printing can take place. If
scrolling is enabled, the first row will be filled with 01234546789, the second
row will have 0 written to its first column, and the cursor will end up at 1x1.
Note that it is still an error to manually attempt to move the cursor off-plane,
or to specify off-plane output. Boxes do not scroll; attempting to draw a 2x11
box on our 2x10 plane will result in an error and no output. When scrolling is
enabled, and output takes place while the cursor is past the end of the last
row, the first row is discarded, all other rows are moved up, the last row is
cleared, and output begins at the beginning of the last row. This does not take
place until output is generated (i.e. it is possible to fill a plane when
scrolling is enabled).

```c
// All planes are created with scrolling disabled. Scrolling can be dynamically
// controlled with ncplane_set_scrolling(). Returns true if scrolling was
// previously enabled, or false if it was disabled.
bool ncplane_set_scrolling(struct ncplane* n, unsigned scrollp);

// Returns true iff the plane is scrolling.
bool ncplane_scrolling_p(const struct ncplane* n);

// Effect |r| scroll events on the plane |n|. Returns an error if |n| is not
// a scrolling plane, and otherwise returns the number of lines scrolled.
int ncplane_scrollup(struct ncplane* n, int r);

// Scroll |n| up until |child| is no longer hidden beneath it. Returns an
// error if |child| is not a child of |n|, or |n| is not scrolling, or |child|
// is fixed. Returns the number of scrolling events otherwise (might be 0).
int ncplane_scrollup_child(struct ncplane* n, const struct ncplane* child);
```

Planes can be freely resized, though they must retain a positive size in
both dimensions. The powerful `ncplane_resize()` allows resizing an `ncplane`,
retaining all or a portion of the plane's existing content, and translating
the plane in one step. The helper function `ncplane_resize_simple()` allows
resizing an `ncplane` without movement, retaining all possible data. To move
the plane without resizing it or changing its content, use `ncplane_move_yx()`.
It is an error to invoke these functions on the standard plane.

```c
// Resize the specified ncplane. The four parameters 'keepy', 'keepx',
// 'keepleny', and 'keeplenx' define a subset of the ncplane to keep,
// unchanged. This may be a region of size 0, though none of these four
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
int ncplane_resize(struct ncplane* n, int keepy, int keepx, int keepleny,
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

// Move this plane relative to the standard plane, or the plane to which it is
// bound (if it is bound to a plane). It is an error to attempt to move the
// standard plane.
int ncplane_move_yx(struct ncplane* n, int y, int x);

// Move this plane relative to its current location. Negative values move up
// and left, respectively. Pass 0 to hold an axis constant.
__attribute__ ((nonnull (1))) static inline int
ncplane_move_rel(struct ncplane* n, int y, int x){
  int oy, ox;
  ncplane_yx(n, &oy, &ox);
  return ncplane_move_yx(n, oy + y, ox + x);
}

// Get the origin of plane 'n' relative to its bound plane, or its pile (if
// 'n' is a root plane).
void ncplane_yx(const struct ncplane* n, int* restrict y, int* restrict x);
int ncplane_y(const struct ncplane* n);
int ncplane_x(const struct ncplane* n);

// Get the origin of plane 'n' relative to its pile. Either or both of 'x' and
// 'y' may be NULL.
void ncplane_abs_yx(const struct ncplane* n, int* y, int* x);
int ncplane_abs_y(const struct ncplane* n);
int ncplane_abs_x(const struct ncplane* n);

// Return the dimensions of this ncplane.
void ncplane_dim_yx(struct ncplane* n, unsigned* restrict rows, unsigned* restrict cols);

static inline unsigned
ncplane_dim_y(const struct ncplane* n){
  int dimy;
  ncplane_dim_yx(n, &dimy, NULL);
  return dimy;
}

static inline unsigned
ncplane_dim_x(const struct ncplane* n){
  int dimx;
  ncplane_dim_yx(n, NULL, &dimx);
  return dimx;
}

// Retrieve pixel geometry for the display region ('pxy', 'pxx'), each cell
// ('celldimy', 'celldimx'), and the maximum displayable bitmap ('maxbmapy',
// 'maxbmapx'). If bitmaps are not supported, 'maxbmapy' and 'maxbmapx' will
// be 0. Any of the geometry arguments may be NULL.
void ncplane_pixel_geom(struct ncplane* n,
                        unsigned* restrict pxy, unsigned* restrict pxx,
                        unsigned* restrict celldimy, unsigned* restrict celldimx,
                        unsigned* restrict maxbmapy, unsigned* restrict maxbmapx);

// provided a coordinate relative to the origin of 'src', map it to the same
// absolute coordinate relative to the origin of 'dst'. either or both of 'y'
// and 'x' may be NULL. if 'dst' is NULL, it is taken to be the standard plane.
void ncplane_translate(const struct ncplane* src, const struct ncplane* dst,
                       int* restrict y, int* restrict x);

// Fed absolute 'y'/'x' coordinates, determine whether that coordinate is
// within the ncplane 'n'. If not, return false. If so, return true. Either
// way, translate the absolute coordinates relative to 'n'. If the point is not
// within 'n', these coordinates will not be within the dimensions of the plane.
bool ncplane_translate_abs(const struct ncplane* n, int* restrict y, int* restrict x);
```

Normally, when text reaches the end of a plane (the horizontal end unless
scrolling is enabled, and the vertical end otherwise), more cannot be written.
If a plane is autogrowing, it will be enlarged to accommodate the extra text.
If it is scrolling, it will be enlarged down; it will otherwise be enlarged
right. Note that attempting to explicitly move the cursor outside the plane is
still an error. The growth occurs when text is written, *not* when the cursor
is moved.

```c
// By default, planes are created with autogrow disabled. Autogrow can be
// dynamically controlled with ncplane_set_autogrow(). Returns true if
// autogrow was previously enabled, or false if it was disabled.
API bool ncplane_set_autogrow(struct ncplane* n, unsigned growp);

API bool ncplane_autogrow_p(const struct ncplane* n);
```

If a given cell's glyph is zero, or its foreground channel is fully transparent,
it is considered to have no foreground. A _default_ cell can be chosen for the
`ncplane`, to be consulted in this case. If the base cell's glyph is likewise
zero (or its foreground channel fully transparent), the plane's foreground is
not rendered. Note that the base cell, like every other cell, has its own
foreground and background channels.

```c
uint16_t ncplane_styles(const struct ncplane* n);**

// Set the specified style bits for the ncplane 'n', whether they're actively
// supported or not.
void ncplane_set_styles(struct ncplane* n, unsigned stylebits);

// Add the specified styles to the ncplane's existing spec.
void ncplane_on_styles(struct ncplane* n, unsigned stylebits);

// Remove the specified styles from the ncplane's existing spec.
void ncplane_off_styles(struct ncplane* n, unsigned stylebits);

// Set the ncplane's base nccell to 'c'. The base cell is used for purposes of
// rendering anywhere that the ncplane's gcluster is 0. Note that the base cell
// is not affected by ncplane_erase(). 'c' must not be a secondary cell from a
// multicolumn EGC.
int ncplane_set_base_cell(struct ncplane* n, const nccell* c);

// Set the ncplane's base nccell. It will be used for purposes of rendering
// anywhere that the ncplane's gcluster is 0. Note that the base cell is not
// affected by ncplane_erase(). 'egc' must be an extended grapheme cluster.
int ncplane_set_base(struct ncplane* n, const char* egc,
                     uint16_t stylemask, uint64_t channels);

// Extract the ncplane's base cell into 'c'. The reference is invalidated if
// 'ncp' is destroyed.
int ncplane_base(struct ncplane* ncp, nccell* c);
```

`ncplane`s are completely ordered along an imaginary z-axis. Newly-created
`ncplane`s are on the top of the stack. They can be freely reordered.

```c
// Splice ncplane 'n' out of the z-buffer, and reinsert it at the top or bottom.
__attribute__ ((nonnull (1)))
static inline void
ncplane_move_top(struct ncplane* n){
  ncplane_move_below(n, NULL);
}

__attribute__ ((nonnull (1)))
static inline void
ncplane_move_bottom(struct ncplane* n){
  ncplane_move_above(n, NULL);
}

// Splice ncplane 'n' and its bound planes out of the z-buffer, and reinsert
// them at the top or bottom. Relative order will be maintained between the
// reinserted planes. For a plane E bound to C, with z-ordering A B C D E,
// moving the C family to the top results in C E A B D, while moving it to
// the bottom results in A B D C E.
void ncplane_move_family_top(struct ncplane* n);
void ncplane_move_family_bottom(struct ncplane* n);

// Splice ncplane 'n' out of the z-buffer, and reinsert it above 'above'.
// Returns non-zero if 'n' is already in the desired location. 'n' and
// 'above' must not be the same plane. If 'above' is NULL, 'n' is moved
// to the bottom of its pile.
int ncplane_move_above(struct ncplane* RESTRICT n,
                       struct ncplane* RESTRICT above);

// Splice ncplane 'n' out of the z-buffer, and reinsert it below 'below'.
// Returns non-zero if 'n' is already in the desired location. 'n' and
// 'below' must not be the same plane. If 'below' is NULL, 'n' is moved to
// the top of its pile.
int ncplane_move_below(struct ncplane* RESTRICT n,
                       struct ncplane* RESTRICT below);

int ncplane_move_family_above(struct ncplane* RESTRICT n,
                              struct ncplane* RESTRICT above);

int ncplane_move_family_below(struct ncplane* RESTRICT n,
                              struct ncplane* RESTRICT below);

// Return the ncplane below this one, or NULL if this is at the stack's bottom.
struct ncplane* ncplane_below(struct ncplane* n);

// Return the ncplane above this one, or NULL if this is at the stack's top.
struct ncplane* ncplane_above(struct ncplane* n);

// Return the topmost plane of the pile containing 'n'.
struct ncplane* ncpile_top(struct ncplane* n);

// Return the bottommost plane of the pile containing 'n'.
struct ncplane* ncpile_bottom(struct ncplane* n);
```

Each plane holds a user pointer which can be retrieved and set (or ignored). In
addition, the plane's virtual framebuffer can be accessed (note that this does
not necessarily reflect anything on the actual screen).

```c
// Retrieve the current contents of the cell under the cursor. The EGC is
// returned, or NULL on error. This EGC must be free()d by the caller. The
// styles and channels are written to 'styles' and 'channels', respectively.
char* ncplane_at_cursor(struct ncplane* n, uint16_t* styles, uint64_t* channels);

// Retrieve the current contents of the cell under the cursor into 'c'. This
// cell is invalidated if the associated plane is destroyed.
int ncplane_at_cursor_cell(struct ncplane* n, nccell* c);

// Retrieve the current contents of the specified cell. The EGC is returned, or
// NULL on error. This EGC must be free()d by the caller. The stylemask and
// channels are written to 'stylemask' and 'channels', respectively. The return
// represents how the cell will be used during rendering, and thus integrates
// any base cell where appropriate. If called upon the secondary columns of a
// wide glyph, the EGC will be returned (i.e. this function does not distinguish
// between the primary and secondary columns of a wide glyph).
char* ncplane_at_yx(const struct ncplane* n, int y, int x,
                    uint16_t* stylemask, uint64_t* channels);

// Retrieve the current contents of the specified cell into 'c'. This cell is
// invalidated if the associated plane is destroyed. Returns the number of
// bytes in the EGC, or -1 on error. Unlike ncplane_at_yx(), when called upon
// the secondary columns of a wide glyph, the return can be distinguished from
// the primary column (nccell_wide_right_p(c) will return true).
int ncplane_at_yx_cell(struct ncplane* n, int y, int x, nccell* c);

// Create an RGBA flat array from the selected region of the ncplane 'nc'.
// Start at the plane's 'begy'x'begx' coordinate (which must lie on the
// plane), continuing for 'leny'x'lenx' cells. Either or both of 'leny' and
// 'lenx' can be specified as 0 to go through the boundary of the plane.
// Only glyphs from the specified blitset may be present. If 'pxdimy' and/or
// 'pxdimx' are non-NULL, they will be filled in with the pixel geometry.
uint32_t* ncplane_as_rgba(const struct ncplane* n, ncblitter_e blit,
                          unsigned begy, unsigned begx, unsigned leny,
                          unsigned lenx, unsigned* pxdimy, unsigned* pxdimx);

// Create a flat string from the EGCs of the selected region of the ncplane
// 'n'. Start at the plane's 'begy'x'begx' coordinate (which must lie on the
// plane), continuing for 'leny'x'lenx' cells. Either or both of 'leny' and
// 'lenx' can be specified as 0 to go through the boundary of the plane.
// -1 can be specified for 'begx'/'begy' to use the current cursor location.
char* ncplane_contents(const struct ncplane* nc, int begy, int begx,
                       unsigned leny, unsigned lenx);

// Manipulate the opaque user pointer associated with this plane.
// ncplane_set_userptr() returns the previous userptr after replacing
// it with 'opaque'. the others simply return the userptr.
void* ncplane_set_userptr(struct ncplane* n, void* opaque);
void* ncplane_userptr(struct ncplane* n);
```

All output is to `ncplane`s. There is no cost in moving the cursor around the
virtual framebuffer. Output that's never rendered still has some memory transfer
cost as the virtual framebuffer is prepared, but new data overwrites it in
memory.

```c
// Move the cursor to the specified position (the cursor needn't be visible).
// Pass -1 as either coordinate to hold that axis constant. Returns -1 if the
// move would place the cursor outside the plane.
int ncplane_cursor_move_yx(struct ncplane* n, int y, int x);

// Move the cursor relative to the current cursor position (the cursor needn't
// be visible). Returns -1 on error, including target position exceeding the
// plane's dimensions.
int ncplane_cursor_move_rel(struct ncplane* n, int y, int x);

// Get the current position of the cursor within n. y and/or x may be NULL.
void ncplane_cursor_yx(const struct ncplane* n, int* restrict y, int* restrict x);

static inline unsigned
ncplane_cursor_y(const struct ncplane* n){
  unsigned y;
  ncplane_cursor_yx(n, &y, NULL);
  return y;
}

static inline unsigned
ncplane_cursor_x(const struct ncplane* n){
  unsigned x;
  ncplane_cursor_yx(n, NULL, &x);
  return x;
}

// Replace the cell at the specified coordinates with the provided cell 'c',
// and advance the cursor by the width of the cell (but not past the end of the
// plane). On success, returns the number of columns the cursor was advanced.
// On failure, -1 is returned.
int ncplane_putc_yx(struct ncplane* n, int y, int x, const nccell* c);

// Call ncplane_putc_yx() for the current cursor location.
static inline int
ncplane_putc(struct ncplane* n, const nccell* c){
  return ncplane_putc_yx(n, -1, -1, c);
}

// Replace the nccell at the specified coordinates with the provided 7-bit char
// 'c'. Advance the cursor by 1. On success, returns 1. On failure, returns -1.
// This works whether the underlying char is signed or unsigned.
static inline int
ncplane_putchar_yx(struct ncplane* n, int y, int x, char c){
  nccell ce = CELL_INITIALIZER(c, ncplane_styles(n), ncplane_channels(n));
  return ncplane_putc_yx(n, y, x, &ce);
}

// Call ncplane_putchar_yx() at the current cursor location.
static inline int
ncplane_putchar(struct ncplane* n, char c){
  return ncplane_putchar_yx(n, -1, -1, c);
}

// Replace the EGC underneath us, but retain the styling. The current styling
// of the plane will not be changed.
int ncplane_putchar_stained(struct ncplane* n, char c);

// Replace the nccell at the specified coordinates with the provided wide char
// 'w'. Advance the cursor by the character's width as reported by wcwidth().
// On success, returns the number of columns written. On failure, returns -1.
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

// Replace the cell at the specified coordinates with the provided EGC, and
// advance the cursor by the width of the cluster (but not past the end of the
// plane). On success, returns the number of columns the cursor was advanced.
// On failure, -1 is returned. The number of bytes converted from gclust is
// written to 'sbytes' if non-NULL.
int ncplane_putegc_yx(struct ncplane* n, int y, int x, const char* gclust, int* sbytes);

// Call ncplane_putegc() at the current cursor location.
static inline int
ncplane_putegc(struct ncplane* n, const char* gclust, int* sbytes){
  return ncplane_putegc_yx(n, -1, -1, gclust, sbytes);
}

// Replace the EGC underneath us, but retain the styling. The current styling
// of the plane will not be changed.
int ncplane_putegc_stained(struct ncplane* n, const char* gclust, int* sbytes);

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

// Replace the EGC underneath us, but retain the styling. The current styling
// of the plane will not be changed.
int ncplane_putwegc_stained(struct ncplane* n, const wchar_t* gclust, int* sbytes);

// Write a series of EGCs to the current location, using the current style.
// They will be interpreted as a series of columns (according to the definition
// of ncplane_putc()). Advances the cursor by some positive number of columns
// (though not beyond the end of the plane); this number is returned on success.
// On error, a non-positive number is returned, indicating the number of columns
// which were written before the error.
int ncplane_putstr_yx(struct ncplane* n, int y, int x, const char* gclusters);

static inline int
ncplane_putstr(struct ncplane* n, const char* gclustarr){
  return ncplane_putstr_yx(n, -1, -1, gclustarr);
}

int ncplane_putstr_aligned(struct ncplane* n, int y, ncalign_e align, const char* s);

// Replace a string's worth of glyphs at the current cursor location, but
// retain the styling. The current styling of the plane will not be changed.
int ncplane_putstr_stained(struct ncplane* n, const char* s);

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

// Write a series of EGCs to the current location, using the current style.
// They will be interpreted as a series of columns (according to the definition
// of ncplane_putc()). Advances the cursor by some positive number of columns
// (though not beyond the end of the plane); this number is returned on success.
// On error, a non-positive number is returned, indicating the number of columns
// which were written before the error. No more than 's' bytes will be written.
int ncplane_putnstr_yx(struct ncplane* n, int y, int x, size_t s, const char* gclusters);

static inline int
ncplane_putnstr(struct ncplane* n, size_t s, const char* gclustarr){
  return ncplane_putnstr_yx(n, -1, -1, s, gclustarr);
}

int ncplane_putnstr_aligned(struct ncplane* n, int y, ncalign_e align,
                            size_t s, const char* s);

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

// The ncplane equivalents of printf(3) and vprintf(3).
int ncplane_vprintf_aligned(struct ncplane* n, int y, ncalign_e align,
                            const char* format, va_list ap);

int ncplane_vprintf_yx(struct ncplane* n, int y, int x,
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
ncplane_printf_aligned(struct ncplane* n, int y, ncalign_e align,
                       const char* format, ...)
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
ncplane_printf_yx(struct ncplane* n, int y, int x, const char* format, ...)
  __attribute__ ((format (printf, 4, 5)));

static inline int
ncplane_printf_aligned(struct ncplane* n, int y, ncalign_e align, const char* format, ...){
  va_list va;
  va_start(va, format);
  int ret = ncplane_vprintf_aligned(n, y, align, format, va);
  va_end(va);
  return ret;
}
```

Multiline chunks of human-readable text can be written with
`ncplane_puttext()` even if the plane does not have scrolling enabled. Such
text will be broken up across lines using the Unicode line-breaking algorithm
of [Unicode Annex #14](http://www.unicode.org/reports/tr14/tr14-34.html).

```c
// Write the specified text to the plane, breaking lines sensibly, beginning at
// the specified line. Returns the number of columns written. When breaking a
// line, the line will be cleared to the end of the plane (the last line will
// *not* be so cleared). The number of bytes written from the input is written
// to '*bytes' if it is not NULL. Cleared columns are included in the return
// value, but *not* included in the number of bytes written. Leaves the cursor
// at the end of output. A partial write will be accomplished as far as it can;
// determine whether the write completed by inspecting '*bytes'.
int ncplane_puttext(struct ncplane* n, int y, ncalign_e align,
                    const char* text, size_t* bytes);
```

Lines and boxes can be drawn, interpolating their colors between their two
endpoints. For a line of a single color, be sure to specify the same channels
on both sides. Boxes allow fairly detailed specification of how they're drawn.

```c
// Draw horizontal or vertical lines using the specified cell, starting at the
// current cursor position. The cursor will end at the cell following the last
// cell output (even, perhaps counter-intuitively, when drawing vertical
// lines), just as if ncplane_putc() was called at that spot. Return the
// number of cells drawn on success. A length of 0 is an error.
int ncplane_hline_interp(struct ncplane* n, const nccell* c,
                         unsigned len, uint64_t c1, uint64_t c2);

static inline int
ncplane_hline(struct ncplane* n, const nccell* c, unsigned len){
  return ncplane_hline_interp(n, c, len, c->channels, c->channels);
}

int ncplane_vline_interp(struct ncplane* n, const nccell* c,
                         unsigned len, uint64_t c1, uint64_t c2);

static inline int
ncplane_vline(struct ncplane* n, const nccell* c, unsigned len){
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

int ncplane_box(struct ncplane* n, const nccell* ul, const nccell* ur,
                const nccell* ll, const nccell* lr, const nccell* hline,
                const cell* vline, int ystop, int xstop, unsigned ctlword);

// Draw a box with its upper-left corner at the current cursor position, having
// dimensions 'ylen'x'xlen'. See ncplane_box() for more information. The
// minimum box size is 2x2, and it cannot be drawn off-screen.
static inline int
ncplane_box_sized(struct ncplane* n, const nccell* ul, const nccell* ur,
                  const nccell* ll, const nccell* lr, const nccell* hline,
                  const nccell* vline, int ylen, int xlen, unsigned ctlword){
  int y, x;
  ncplane_cursor_yx(n, &y, &x);
  return ncplane_box(n, ul, ur, ll, lr, hline, vline, y + ylen - 1,
                     x + xlen - 1, ctlword);
}

static inline int
ncplane_perimeter(struct ncplane* n, const nccell* ul, const nccell* ur,
                  const nccell* ll, const nccell* lr, const nccell* hline,
                  const nccell* vline, unsigned ctlword){
  if(ncplane_cursor_move_yx(n, 0, 0)){
    return -1;
  }
  int dimy, dimx;
  ncplane_dim_yx(n, &dimy, &dimx);
  return ncplane_box_sized(n, ul, ur, ll, lr, hline, vline, dimy, dimx, ctlword);
}

static inline int
ncplane_rounded_box(struct ncplane* n, uint16_t attr, uint64_t channels,
                    int ystop, int xstop, unsigned ctlword){
  int ret = 0;
  nccell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  nccell ll = CELL_TRIVIAL_INITIALIZER, lr = CELL_TRIVIAL_INITIALIZER;
  nccell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if((ret = nccells_rounded_box(n, attr, channels, &ul, &ur, &ll, &lr, &hl, &vl)) == 0){
    ret = ncplane_box(n, &ul, &ur, &ll, &lr, &hl, &vl, ystop, xstop, ctlword);
  }
  nccell_release(n, &ul); nccell_release(n, &ur);
  nccell_release(n, &ll); nccell_release(n, &lr);
  nccell_release(n, &hl); nccell_release(n, &vl);
  return ret;
}

static inline int
ncplane_rounded_box_sized(struct ncplane* n, uint16_t attr, uint64_t channels,
                          int ylen, int xlen, unsigned ctlword){
  int y, x;
  ncplane_cursor_yx(n, &y, &x);
  return ncplane_rounded_box(n, attr, channels, y + ylen - 1,
                             x + xlen - 1, ctlword);
}

static inline int
ncplane_double_box(struct ncplane* n, uint16_t attr, uint64_t channels,
                   int ystop, int xstop, unsigned ctlword){
  int ret = 0;
  nccell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
  nccell ll = CELL_TRIVIAL_INITIALIZER, lr = CELL_TRIVIAL_INITIALIZER;
  nccell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
  if((ret = nccells_double_box(n, attr, channels, &ul, &ur, &ll, &lr, &hl, &vl)) == 0){
    ret = ncplane_box(n, &ul, &ur, &ll, &lr, &hl, &vl, ystop, xstop, ctlword);
  }
  nccell_release(n, &ul); nccell_release(n, &ur);
  nccell_release(n, &ll); nccell_release(n, &lr);
  nccell_release(n, &hl); nccell_release(n, &vl);
  return ret;
}

static inline int
ncplane_double_box_sized(struct ncplane* n, uint16_t attr, uint64_t channels,
                         int ylen, int xlen, unsigned ctlword){
  int y, x;
  ncplane_cursor_yx(n, &y, &x);
  return ncplane_double_box(n, attr, channels, y + ylen - 1,
                            x + xlen - 1, ctlword);
}
```

Similarly, areas can be filled with a cell.

```c
// Starting at the specified coordinate, if its glyph is different from that of
// 'c', 'c' is copied into it, and the original glyph is considered the fill
// target. We do the same to all cardinally-connected cells having this same
// fill target. Returns the number of cells polyfilled. An invalid initial y, x
// is an error. Returns the number of cells filled, or -1 on error.
int ncplane_polyfill_yx(struct ncplane* n, unsigned y, unsigned x, const nccell* c);

// Draw a gradient with its upper-left corner at the position specified by 'y'/'x',
// where -1 means the current cursor position in that dimension. The area is
// specified by 'ylen'/'xlen', where 0 means "everything remaining below or
// to the right, respectively." The glyph composed of 'egc' and 'styles' is
// used for all cells. The channels specified by 'ul', 'ur', 'll', and 'lr'
// are composed into foreground and background gradients. To do a vertical
// gradient, 'ul' ought equal 'ur' and 'll' ought equal 'lr'. To do a
// horizontal gradient, 'ul' ought equal 'll' and 'ur' ought equal 'ul'. To
// color everything the same, all four channels should be equivalent. The
// resulting alpha values are equal to incoming alpha values. Returns the
// number of cells filled on success, or -1 on failure.
int ncplane_gradient(struct ncplane* n, int y, int x, unsigned ylen,
                     unsigned xlen, const char* egc, uint16_t styles,
                     uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr);

// Do a high-resolution gradient using upper blocks and synced backgrounds.
// This doubles the number of vertical gradations, but restricts you to
// half blocks (appearing to be full blocks). Returns the number of cells
// filled on success, or -1 on error.
int ncplane_gradient2x1(struct ncplane* n, int y, int x, unsigned ylen,
                        unsigned xlen, uint32_t ul, uint32_t ur,
                        uint32_t ll, uint32_t lr);

// Set the given style throughout the specified region, keeping content and
// channels unchanged. The upper left corner is at 'y', 'x', and -1 may be
// specified to indicate the cursor's position in that dimension. The area
// is specified by 'ylen', 'xlen', and 0 may be specified to indicate everything
// remaining to the right and below, respectively. It is an error for any
// coordinate to be outside the plane. Returns the number of cells set,
// or -1 on failure.
int ncplane_format(struct ncplane* n, int y, int x, unsigned ylen,
                   unsigned xlen, uint16_t stylemask);

// Set the given channels throughout the specified region, keeping content and
// channels unchanged. The upper left corner is at 'y', 'x', and -1 may be
// specified to indicate the cursor's position in that dimension. The area
// is specified by 'ylen', 'xlen', and 0 may be specified to indicate everything
// remaining to the right and below, respectively. It is an error for any
// coordinate to be outside the plane. Returns the number of cells set,
// or -1 on failure.
int ncplane_stain(struct ncplane* n, int y, int x, unsigned ylen,
                  unsigned xlen, uint64_t ul, uint64_t ur,
                  uint64_t ll, uint64_t lr);
```

My 14 year-old self would never forgive me if we didn't have sweet palette
fades. The simple fade API runs the operation over a time interval, adapting
to the actual runtime, invoking a callback at each iteration.

```c
// Called for each fade iteration on 'ncp'. If anything but 0 is returned,
// the fading operation ceases immediately, and that value is propagated out.
// The recommended absolute display time target is passed in 'tspec'.
typedef int (*fadecb)(struct notcurses* nc, struct ncplane* ncp,
                      const struct timespec*, void* curry);

// Fade the ncplane out over the provided time, calling 'fader' at each
// iteration. Requires a terminal which supports truecolor, or at least palette
// modification (if the terminal uses a palette, our ability to fade planes is
// limited, and affected by the complexity of the rest of the screen).
int ncplane_fadeout(struct ncplane* n, const struct timespec* ts,
                    fadecb fader, void* curry);

// Fade the ncplane in over the specified time. Load the ncplane with the
// target cells without rendering, then call this function. When it's done, the
// ncplane will have reached the target levels, starting from zeroes.
int ncplane_fadein(struct ncplane* n, const struct timespec* ts,
                   fadecb fader, void* curry);

// Rather than the simple ncplane_fade{in/out}(), ncfadectx_setup() can be
// Pulse the plane in and out until the callback returns non-zero, relying on
// the callback 'fader' to initiate rendering. 'ts' defines the half-period
// (i.e. the transition from black to full brightness, or back again). Proper
// use involves preparing (but not rendering) an ncplane, then calling
// ncplane_pulse(), which will fade in from black to the specified colors.
int ncplane_pulse(struct ncplane* n, const struct timespec* ts, fadecb fader, void* curry);

```

The more flexible fade API allows for fine control of the process.

```c
// paired with a loop over ncplane_fade{in/out}_iteration() + ncfadectx_free().
struct ncfadectx* ncfadectx_setup(struct ncplane* n);

// Return the number of iterations through which 'nctx' will fade.
int ncfadectx_iterations(const struct ncfadectx* nctx);

// Fade out through 'iter' iterations, where
// 'iter' < 'ncfadectx_iterations(nctx)'.
int ncplane_fadeout_iteration(struct ncplane* n, struct ncfadectx* nctx,
                              int iter, fadecb fader, void* curry);

// Fade in through 'iter' iterations, where
// 'iter' < 'ncfadectx_iterations(nctx)'.
int ncplane_fadein_iteration(struct ncplane* n, struct ncfadectx* nctx,
                             int iter, fadecb fader, void* curry);

// Release the resources associated with 'nctx'.
void ncfadectx_free(struct ncfadectx* nctx);
```

Raw streams of RGBA or BGRx data can be blitted directly to an ncplane:

```c
// Blit a flat array 'data' of RGBA 32-bit values to the ncplane 'vopts->n',
// which mustn't be NULL. the blit begins at 'vopts->y' and 'vopts->x' relative
// to the specified plane. Each source row ought occupy 'linesize' bytes (this
// might be greater than 'vopts->lenx' * 4 due to padding or partial blits). A
// subregion of the input can be specified with the 'begy'x'begx' and
// 'leny'x'lenx' fields from 'vopts'. Returns the number of pixels blitted, or
// -1 on error.
int ncblit_rgba(const void* data, int linesize,
                const struct ncvisual_options* vopts);

// Same as ncblit_rgba(), but for RGBx, with 'alpha' supplied as an alpha value
// throughout, 0 <= 'alpha' <= 255. linesize ought be a multiple of 4.
int ncblit_rgb_loose(const void* data, int linesize,
                     const struct ncvisual_options* vopts, int alpha);

// Same as ncblit_rgba(), but for RGB, with 'alpha' supplied as an alpha value
// throughout, 0 <= 'alpha' <= 255.
int ncblit_rgb_packed(const void* data, int linesize,
                      const struct ncvisual_options* vopts, int alpha);

// Same as ncblit_rgba(), but for BGRx.
int ncblit_bgrx(const void* data, int linesize,
                const struct ncvisual_options* vopts);
```

### Plane channels API

Helpers are provided to manipulate an `ncplane`'s `channels` member. They are
all implemented in terms of the lower-level [Channels API](#channels).

```c
// Get the current colors and alpha values for ncplane 'n'.
uint64_t ncplane_channels(const struct ncplane* n);

// Get the current styling for the ncplane 'n'.
uint16_t ncplane_styles(const struct ncplane* n);

// Set the alpha and coloring bits of the plane's current channels from a
// 64-bit pair of channels.
API void ncplane_set_channels(struct ncplane* n, uint64_t channels)
  __attribute__ ((nonnull (1)));

// Extract the background alpha and coloring bits from a 64-bit channel
// pair as a single 32-bit value.
static inline uint32_t
ncplane_bchannel(const struct ncplane* n){
  return ncchannels_bchannel(ncplane_channels(n));
}

// Extract the foreground alpha and coloring bits from a 64-bit channel
// pair as a single 32-bit value.
static inline uint32_t
ncplane_fchannel(const struct ncplane* n){
  return ncchannels_fchannel(ncplane_channels(n));
}

// Set the background alpha and coloring bits of the plane's current
// channels from a single 32-bit value.
uint64_t ncplane_set_bchannel(struct ncplane* n, uint32_t channel);
  __attribute__ ((nonnull (1)));

// Set the foreground alpha and coloring bits of the plane's current
// channels from a single 32-bit value.
uint64_t ncplane_set_fchannel(struct ncplane* n, uint32_t channel);

// Extract 24 bits of working foreground RGB from an ncplane, shifted to LSBs.
static inline unsigned
ncplane_fg_rgb(const struct ncplane* nc){
  return ncchannels_fg_rgb(ncplane_channels(nc));
}

// Extract 24 bits of working background RGB from an ncplane, shifted to LSBs.
static inline unsigned
ncplane_bg_rgb(const struct ncplane* nc){
  return ncchannels_bg_rgb(ncplane_channels(nc));
}

// Extract 2 bits of foreground alpha from 'struct ncplane', shifted to LSBs.
static inline unsigned
ncplane_fg_alpha(const struct ncplane* nc){
  return ncchannels_fg_alpha(ncplane_channels(nc));
}

// Extract 2 bits of background alpha from 'struct ncplane', shifted to LSBs.
static inline unsigned
ncplane_bg_alpha(const struct ncplane* nc){
  return ncchannels_bg_alpha(ncplane_channels(nc));
}

// Set the alpha parameters for ncplane 'n'.
int ncplane_set_fg_alpha(struct ncplane* n, unsigned alpha);
int ncplane_set_bg_alpha(struct ncplane* n, unsigned alpha);

// Extract 24 bits of foreground RGB from 'n', split into subcomponents.
static inline unsigned
ncplane_fg_rgb8(const struct ncplane* n, unsigned* r, unsigned* g, unsigned*
  return ncchannels_fg_rgb8(ncplane_channels(n), r, g, b);
}

// Extract 24 bits of background RGB from 'n', split into subcomponents.
static inline unsigned
ncplane_bg_rgb8(const struct ncplane* n, unsigned* r, unsigned* g, unsigned*
  return ncchannels_bg_rgb8(ncplane_channels(n), r, g, b);
}

// Set the current fore/background color using RGB specifications. If the
// terminal does not support directly-specified 3x8b cells (24-bit "TrueColor",
// indicated by the "RGB" terminfo capability), the provided values will be
// interpreted in some lossy fashion. None of r, g, or b may exceed 255.
// "HP-like" terminals require setting foreground and background at the same
// time using "color pairs"; Notcurses will manage color pairs transparently.
int ncplane_set_fg_rgb8(struct ncplane* n, unsigned r, unsigned g, unsigned b);
int ncplane_set_bg_rgb8(struct ncplane* n, unsigned r, unsigned g, unsigned b);

// Same, but clipped to [0..255].
void ncplane_set_bg_rgb8_clipped(struct ncplane* n, int r, int g, int b);
void ncplane_set_fg_rgb8_clipped(struct ncplane* n, int r, int g, int b);

// Same, but with rgb assembled into a channel (i.e. lower 24 bits).
int ncplane_set_fg_rgb(struct ncplane* n, uint32_t channel);
int ncplane_set_bg_rgb(struct ncplane* n, uint32_t channel);

// Use the default color for the foreground/background.
void ncplane_set_fg_default(struct ncplane* n);
void ncplane_set_bg_default(struct ncplane* n);

// Provide a palette index on [0..255].
int ncplane_set_fg_palindex(struct ncplane* n, unsigned idx);
int ncplane_set_bg_palindex(struct ncplane* n, unsigned idx);
```

## Cells

Unlike the `notcurses` or `ncplane` objects, the definition of `nccell` is
available to the user. It is somewhat ironic, then, that the user typically
needn't (and shouldn't) use `nccell`s directly. Use an `nccell` when the EGC
being output is used several times. In this case, time otherwise spent running
`nccell_load()` (which tokenizes and verifies EGCs) can be saved. It can also
be useful to use an `nccell` when the same styling is used in a discontinuous
manner.

```c
// An nccell corresponds to a single character cell on some plane, which can be
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
// other things, like cuneiform. True display width is a *function of the
// font and terminal*. Among the longest Unicode codepoints is
//
//    U+FDFD ARABIC LIGATURE BISMILLAH AR-RAHMAN AR-RAHEEM ﷽
//
// wcwidth() rather optimistically claims this most exalted glyph to occupy
// a single column. BiDi text is too complicated for me to even get into here.
// Be assured there are no easy answers; ours is indeed a disturbing Universe.
//
// Each nccell occupies 16 static bytes (128 bits). The surface is thus ~1.6MB
// for a (pretty large) 500x200 terminal. At 80x43, it's less than 64KB.
// Dynamic requirements (the egcpool) can add up to 16MB to an ncplane, but
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
//
// This structure is exposed only so that most functions can be inlined. Do not
// directly modify or access the fields of this structure; use the API.
typedef struct nccell {
  // These 32 bits, together with the associated plane's associated egcpool,
  // completely define this cell's EGC. Unless the EGC requires more than four
  // bytes to encode as UTF-8, it will be inlined here. If more than four bytes
  // are required, it will be spilled into the egcpool. In either case, there's
  // a NUL-terminated string available without copying, because (1) the egcpool
  // is all NUL-terminated sequences and (2) the fifth byte of this struct (the
  // gcluster_backstop field, see below) is guaranteed to be zero, as are any
  // unused bytes in gcluster.
  //
  // The gcluster + gcluster_backstop thus form a valid C string of between 0
  // and 4 non-NUL bytes. Interpreting them in this fashion requires that
  // gcluster be stored as a little-endian number (strings have no byte order).
  // This gives rise to three simple rules:
  //
  //  * when storing to gcluster from a numeric, always use htole()
  //  * when loading from gcluster for numeric use, always use htole()
  //  * when referencing gcluster as a string, always use a pointer cast
  //
  // Uses of gcluster ought thus always have exactly one htole() or pointer
  // cast associated with them, and we otherwise always work as host-endian.
  //
  // A spilled EGC is indicated by the value 0x01XXXXXX. This cannot alias a
  // true supra-ASCII EGC, because UTF-8 only encodes bytes <= 0x80 when they
  // are single-byte ASCII-derived values. The XXXXXX is interpreted as a 24-bit
  // index into the egcpool. These pools may thus be up to 16MB.
  //
  // The cost of this scheme is that the character 0x01 (SOH) cannot be encoded
  // in a nccell, which is absolutely fine because what 70s horseshit is SOH?
  // It must not be allowed through the API, or havoc will result.
  uint32_t gcluster;          // 4B → 4B little endian EGC
  uint8_t gcluster_backstop;  // 1B → 5B (8 bits of zero)
  // we store the column width in this field. for a multicolumn EGC of N
  // columns, there will be N nccells, and each has a width of N...for now.
  // eventually, such an EGC will set more than one subsequent cell to
  // WIDE_RIGHT, and this won't be necessary. it can then be used as a
  // bytecount. see #1203. FIXME iff width >= 2, the cell is part of a
  // multicolumn glyph. whether a cell is the left or right side of the glyph
  // can be determined by checking whether ->gcluster is zero.
  uint8_t width;              // 1B → 6B (8 bits of EGC column width)
  uint16_t stylemask;         // 2B → 8B (16 bits of NCSTYLE_* attributes)
  // (channels & 0x8000000000000000ull): blitted to upper-left quadrant
  // (channels & 0x4000000000000000ull): foreground is *not* "default color"
  // (channels & 0x3000000000000000ull): foreground alpha (2 bits)
  // (channels & 0x0800000000000000ull): foreground uses palette index
  // (channels & 0x0400000000000000ull): blitted to upper-right quadrant
  // (channels & 0x0200000000000000ull): blitted to lower-left quadrant
  // (channels & 0x0100000000000000ull): blitted to lower-right quadrant
  // (channels & 0x00ffffff00000000ull): foreground in 3x8 RGB (rrggbb)
  // (channels & 0x0000000080000000ull): reserved, must be 0
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
} nccell;

#define NCALPHA_HIGHCONTRAST 0x30000000ull
#define NCALPHA_TRANSPARENT  0x20000000ull
#define NCALPHA_BLEND        0x10000000ull
#define NCALPHA_OPAQUE       0x00000000ull
```

`nccell`s must be initialized with an initialization macro or `nccell_init()`
before any other use. `nccell_init()` and `CELL_TRIVIAL_INITIALIZER` both
simply zero out the `nccell`.

```c
#define CELL_TRIVIAL_INITIALIZER { }
#define CELL_CHAR_INITIALIZER(c) { .gcluster = (c), .gcluster_backstop = 0, .reserved = 0, .stylemask = 0, .channels = 0, }
#define CELL_INITIALIZER(c, s, chan) { .gcluster = (c), .gcluster_backstop = 0, .reserved = 0, .stylemask = (s), .channels = (chan), }

static inline void
nccell_init(nccell* c){
  memset(c, 0, sizeof(*c));
}
```

An `nccell` has three fundamental elements:

* The EGC displayed at this coordinate, encoded in UTF-8. If the EGC is a
  single ASCII character (value less than 0x80), it is stored inline in
  the `nccell`'s `gcluster` field. Otherwise, `gcluster`'s top 24 bits
  are a 128-biased offset into the associated `ncplane`'s egcpool. This
  implies that `nccell`s are associated with `ncplane`s once prepared.
* The Curses-style attributes of the text.
* The 52 bits of foreground and background RGBA (2x8/8/8/2), plus a few flags.

The EGC should be loaded using `nccell_load()`. Either a single NUL-terminated
EGC can be provided, or a string composed of multiple EGCs. In the latter case,
the first EGC from the string is loaded. Remember, backing storage for the EGC
is provided by the `ncplane` passed to `nccell_load()`; if this `ncplane` is
destroyed (or even erased), the `nccell` cannot safely be used. If you're done
using the `nccell` before being done with the `ncplane`, call `nccell_release()`
to free up the EGC resources.

```c
// Breaks the UTF-8 string in 'gcluster' down, setting up the nccell 'c'.
// Returns the number of bytes copied out of 'gcluster', or -1 on failure. The
// styling of the cell is left untouched, but any resources are released.
int nccell_load(struct ncplane* n, nccell* c, const char* gcluster);

// nccell_load(), plus blast the styling with 'attr' and 'channels'.
static inline int
nccell_prime(struct ncplane* n, nccell* c, const char* gcluster,
             uint16_t stylemask, uint64_t channels){
  c->stylemask = stylemask;
  c->channels = channels;
  int ret = nccell_load(n, c, gcluster);
  return ret;
}

// Duplicate 'c' into 'targ'. Not intended for external use; exposed for the
// benefit of unit tests.
int nccell_duplicate(struct ncplane* n, nccell* targ, const cell* c);

// Release resources held by the cell 'c'.
void nccell_release(struct ncplane* n, nccell* c);

// return the number of columns occupied by 'c'. see ncstrwidth() for an
// equivalent for multiple EGCs.
static inline int
nccell_cols(const nccell* c){
  return c->width ? c->width : 1;
}

#define NCSTYLE_MASK      0xffffu
#define NCSTYLE_ITALIC    0x0010u
#define NCSTYLE_UNDERLINE 0x0008u
#define NCSTYLE_UNDERCURL 0x0004u
#define NCSTYLE_BOLD      0x0002u
#define NCSTYLE_STRUCK    0x0001u
#define NCSTYLE_NONE      0

// copy the UTF8-encoded EGC out of the cell, whether simple or complex. the
// result is not tied to the ncplane, and persists across erases / destruction.
static inline char*
nccell_strdup(const struct ncplane* n, const nccell* c){
  return strdup(nccell_extended_gcluster(n, c));
}

// Set the specified style bits for the cell 'c', whether they're actively
// supported or not. Only the lower 16 bits are meaningful.
static inline void
nccell_styles_set(nccell* c, unsigned stylebits){
  c->stylemask = stylebits & NCSTYLE_MASK;
}

// Extract the style bits from the cell.
static inline unsigned
nccell_styles(const nccell* c){
  return c->stylemask;
}

// Add the specified styles (in the LSBs) to the cell's existing spec, whether
// they're actively supported or not.
static inline void
nccell_on_styles(nccell* c, unsigned stylebits){
  c->stylemask |= (stylebits & NCSTYLE_MASK);
}

// Remove the specified styles (in the LSBs) from the cell's existing spec.
static inline void
nccell_off_styles(nccell* c, unsigned stylebits){
  c->stylemask &= ~(stylebits & NCSTYLE_MASK);
}

// Is the cell part of a multicolumn element?
static inline bool
nccell_double_wide_p(const nccell* c){
  return (c->width >= 2);
}

// Load a 7-bit char 'ch' into the nccell 'c'. Returns the number of bytes
// used, or -1 on error.
static inline int
nccell_load_char(struct ncplane* n, nccell* c, char ch){
  char gcluster[2];
  gcluster[0] = ch;
  gcluster[1] = '\0';
  return nccell_load(n, c, gcluster);
}

// Load a UTF-8 encoded EGC of up to 4 bytes into the nccell 'c'. Returns the
// number of bytes used, or -1 on error.
static inline int
nccell_load_egc32(struct ncplane* n, nccell* c, uint32_t egc){
  char gcluster[sizeof(egc) + 1];
  egc = htole(egc);
  memcpy(gcluster, &egc, sizeof(egc));
  gcluster[4] = '\0';
  return nccell_load(n, c, gcluster);
}

// Load a UCS-32 codepoint into the nccell 'c'. Returns the number of bytes
// used, or -1 on error.
static inline int
nccell_load_ucs32(struct ncplane* n, nccell* c, uint32_t u){
  unsigned char utf8[WCHAR_MAX_UTF8BYTES];
  if(notcurses_ucs32_to_utf8(&u, 1, utf8, sizeof(utf8)) < 0){
    return -1;
  }
  uint32_t utf8asegc;
  _Static_assert(WCHAR_MAX_UTF8BYTES == sizeof(utf8asegc));
  memcpy(&utf8asegc, utf8, sizeof(utf8));
  return nccell_load_egc32(n, c, utf8asegc);
}

// return a pointer to the NUL-terminated EGC referenced by 'c'. this pointer
// is invalidated by any further operation on the plane 'n', so...watch out!
const char* nccell_extended_gcluster(const struct ncplane* n, const nccell* c);

// Returns true if the two nccells are distinct EGCs, attributes, or channels.
// The actual egcpool index needn't be the same--indeed, the planes needn't even
// be the same. Only the expanded EGC must be equal. The EGC must be bit-equal;
// it would probably be better to test whether they're Unicode-equal FIXME.
static inline bool
nccellcmp(const struct ncplane* n1, const nccell* RESTRICT c1,
          const struct ncplane* n2, const nccell* RESTRICT c2){
  if(c1->stylemask != c2->stylemask){
    return true;
  }
  if(c1->channels != c2->channels){
    return true;
  }
  return strcmp(nccell_extended_gcluster(n1, c1), nccell_extended_gcluster(n2, c2));
}

// load up six cells with the EGCs necessary to draw a box. returns 0 on
// success, -1 on error. on error, any cells this function might
// have loaded before the error are nccell_release()d. There must be at least
// six EGCs in gcluster.
static inline int
nccells_load_box(struct ncplane* n, uint16_t style, uint64_t channels,
                 nccell* ul, nccell* ur, nccell* ll, nccell* lr,
                 nccell* hl, nccell* vl, const char* gclusters){
  int ulen;
  if((ulen = nccell_prime(n, ul, gclusters, style, channels)) > 0){
    if((ulen = nccell_prime(n, ur, gclusters += ulen, style, channels)) > 0){
      if((ulen = nccell_prime(n, ll, gclusters += ulen, style, channels)) > 0){
        if((ulen = nccell_prime(n, lr, gclusters += ulen, style, channels)) > 0){
          if((ulen = nccell_prime(n, hl, gclusters += ulen, style, channels)) > 0){
            if(nccell_prime(n, vl, gclusters + ulen, style, channels) > 0){
              return 0;
            }
            nccell_release(n, hl);
          }
          nccell_release(n, lr);
        }
        nccell_release(n, ll);
      }
      nccell_release(n, ur);
    }
    nccell_release(n, ul);
  }
  return -1;
}


static inline int
nccells_rounded_box(struct ncplane* n, uint16_t attr, uint64_t channels,
                    nccell* ul, nccell* ur, nccell* ll, nccell* lr, nccell* hl, nccell* vl){
  return nccells_load_box(n, attr, channels, ul, ur, ll, lr, hl, vl, "╭╮╰╯─│");
}

static inline int
nccells_double_box(struct ncplane* n, uint16_t attr, uint64_t channels,
                   nccell* ul, nccell* ur, nccell* ll, nccell* lr, nccell* hl, nccell* vl){
  return nccells_load_box(n, attr, channels, ul, ur, ll, lr, hl, vl, "╔╗╚╝═║");
}
```

It is sometimes useful to find the number of bytes and columns represented by
a UTF-8 string. `ncstrwidth_valid()` returns -1 if it encounters an invalid
character, and the number of columns otherwise. Even if there is an error, if
`validbytes` and/or `validwidth` are not `NULL`, the number of bytes and
columns (respectively) consumed before error are returned via these parameters.

```c
// Returns the number of columns occupied by a the valid prefix of a multibyte
// (UTF-8) string. If an invalid character is encountered, -1 will be returned,
// and the number of valid bytes and columns will be written into *|validbytes|
// and *|validwidth| (assuming them non-NULL). If the entire string is valid,
// *|validbytes| and *|validwidth| reflect the entire string.
int ncstrwidth_valid(const char* egcs, int* validbytes, int* validwidth);
```

### Cell channels API

Helpers are provided to manipulate an `nccell`'s `channels` member. They are
all implemented in terms of the lower-level [Channels API](#channels).

```c
// Extract the background alpha and coloring bits from a cell's channels
// as a single 32-bit value.
static inline uint32_t
nccell_bchannel(const nccell* cl){
  return ncchannels_bchannel(cl->channels);
}

// Extract the foreground alpha and coloring bits from a cell's channels
// as a single 32-bit value.
static inline uint32_t
nccell_fchannel(const nccell* cl){
  return ncchannels_fchannel(cl->channels);
}

// Extract 24 bits of foreground RGB from 'cl', shifted to LSBs.
static inline uint32_t
nccell_fg_rgb(const nccell* cl){
  return ncchannels_fg_rgb(cl->channels);
}

// Extract 24 bits of background RGB from 'cl', shifted to LSBs.
static inline uint32_t
nccell_bg_rgb(const nccell* cl){
  return ncchannels_bg_rgb(cl->channels);
}

// Extract 2 bits of foreground alpha from 'cl', shifted to LSBs.
static inline unsigned
nccell_fg_alpha(const nccell* cl){
  return ncchannels_fg_alpha(cl->channels);
}

// Extract 2 bits of background alpha from 'cl', shifted to LSBs.
static inline unsigned
nccell_bg_alpha(const nccell* cl){
  return ncchannels_bg_alpha(cl->channels);
}

// Extract 24 bits of foreground RGB from 'cl', split into subcell.
static inline uint32_t
nccell_fg_rgb8(const nccell* cl, unsigned* r, unsigned* g, unsigned* b){
  return ncchannels_fg_rgb8(cl->channels, r, g, b);
}

// Extract 24 bits of background RGB from 'cl', split into subcell.
static inline uint32_t
nccell_bg_rgb8(const nccell* cl, unsigned* r, unsigned* g, unsigned* b){
  return ncchannels_bg_rgb8(cl->channels, r, g, b);
}

// Set the r, g, and b cell for the foreground component of this 64-bit
// 'cell' variable, and mark it as not using the default color.
static inline int
nccell_set_fg_rgb8(nccell* cl, unsigned r, unsigned g, unsigned b){
  return ncchannels_set_fg_rgb8(&cl->channels, r, g, b);
}

// Same, but clipped to [0..255].
static inline void
nccell_set_fg_rgb8_clipped(nccell* cl, int r, int g, int b){
  ncchannels_set_fg_rgb8_clipped(&cl->channels, r, g, b);
}

// Same, but with an assembled 24-bit RGB value.
static inline int
nccell_set_fg_rgb(nccell* c, uint32_t channel){
  return ncchannels_set_fg_rgb(&c->channels, channel);
}

// Set the r, g, and b cell for the background component of this 64-bit
// 'cell' variable, and mark it as not using the default color.
static inline int
nccell_set_bg_rgb8(nccell* cl, unsigned r, unsigned g, unsigned b){
  return ncchannels_set_bg_rgb8(&cl->channels, r, g, b);
}

// Same, but clipped to [0..255].
static inline void
nccell_set_bg_rgb8_clipped(nccell* cl, int r, int g, int b){
  ncchannels_set_bg_rgb8_clipped(&cl->channels, r, g, b);
}

// Same, but with an assembled 24-bit RGB value.
static inline int
nccell_set_bg_rgb(nccell* c, uint32_t channel){
  return ncchannels_set_bg_rgb(&c->channels, channel);
}

static inline int
nccell_set_fg_alpha(nccell* c, unsigned alpha){
  return ncchannels_set_fg_alpha(&c->channels, alpha);
}

static inline int
nccell_set_bg_alpha(nccell* c, unsigned alpha){
  return ncchannels_set_bg_alpha(&c->channels, alpha);
}

// Is the foreground using the "default foreground color"?
static inline bool
nccell_fg_default_p(const nccell* cl){
  return ncchannels_fg_default_p(cl->channels);
}

// Is the background using the "default background color"? The "default
// background color" must generally be used to take advantage of
// terminal-effected transparency.
static inline bool
nccell_bg_default_p(const nccell* cl){
  return ncchannels_bg_default_p(cl->channels);
}

// Use the default color for the foreground.
static inline void
nccell_set_fg_default(nccell* c){
  ncchannels_set_fg_default(&c->channels);
}

// Use the default color for the background.
static inline void
nccell_set_bg_default(nccell* c){
  ncchannels_set_bg_default(&c->channels);
}

```

## Reels

ncreels are a complex UI abstraction offered by notcurses, derived from my
similar work in [outcurses](https://github.com/dankamongmen/ncreels#ncreels).

The ncreel is a UI abstraction supported by Notcurses in which
dynamically-created and -destroyed toplevel entities (referred to as tablets)
are arranged as if on a cylinder, allowing for infinite scrolling
(infinite scrolling can be disabled, resulting in a rectangle rather than a
cylinder). This works naturally with keyboard navigation, mouse scrolling wheels,
and touchpads (including the capacitive touchscreens of modern cell phones).
The term "reel" derives from slot machines. An ncreel initially has
no tablets; at any given time thereafter, it has zero or more tablets, and if
there is at least one tablet, one tablet is focused (and on-screen). If the
last tablet is removed, no tablet is focused. A tablet can support navigation
within the tablet, in which case there is an in-tablet focus for the focused
tablet, which can also move among elements within the tablet.

The ncreel object tracks the size of the screen, the size, number,
information depth, and order of tablets, and the foci. It also draws the
optional borders around tablets and the optional border of the reel itself. It
knows nothing about the actual content of a tablet, save the number of lines it
occupies at each information depth. The typical control flow is that an
application receives events (from the UI or other event sources), and calls
into Notcurses saying e.g. "Tablet 2 now has 40 valid lines of information".
notcurses might then call back into the application, asking it to draw some
line(s) from some tablet(s) at some particular coordinate of that tablet's
plane. Finally, control returns to the application, and the cycle starts anew.

Each tablet might be wholly, partially, or not on-screen. Notcurses always
places as much of the focused tablet as is possible on-screen (if the focused
tablet has more lines than the actual reel does, it cannot be wholly on-screen.
In this case, the focused subelements of the tablet are always on-screen). The
placement of the focused tablet depends on how it was reached (when moving to
the next tablet, offscreen tablets are brought onscreen at the bottom. When
moving to the previous tablet, offscreen tablets are brought onscreen at the
top. When moving to an arbitrary tablet which is neither the next nor previous
tablet, it will be placed in the center).

The controlling application can, at any time,

* Insert a new tablet somewhere in the reel (possibly off-screen)
* Delete a (possibly off-screen) tablet from the reel
* Change focus to the next or previous tablet, bringing it on-screen if it is off
* Change focus to some arbitrary other tablet, bringing it on-screen if it is off
* Expand or collapse the information depth of a tablet
* Change the content of a tablet, updating it if it is on-screen
  * Remove content from a tablet, possibly resizing it, and possibly changing focus within the tablet
  * Add content to the tablet, possibly resizing it, and possibly creating focus within the tablet
* Navigate within the focused tablet
* Create or destroy new planes atop the ncreel
* Indicate that the screen has been resized or needs be redrawn

A special case arises when moving among the tablets of a reel having multiple
tablets, all of which fit entirely on-screen, and infinite scrolling is in use.
Normally, upon moving to the next tablet from the bottommost tablet, the
(offscreen) next tablet is pulled up into the bottom of the reel (the reverse
is true when moving to the previous tablet from the topmost). When all tablets
are onscreen with infinite scrolling, there are two possibilities: either the
focus scrolls (moving from the bottom tablet to the top tablet, for instance),
or the reel scrolls (preserving order among the tablets, but changing their
order on-screen). In this latter case, moving to the next tablet from the
bottommost tablet results in the tablet which is gaining focus being brought to
the bottom of the screen from the top, and all other tablets moving up on the
screen. Moving to the previous tablet from the topmost tablet results in the
bottommost tablet moving to the top of the screen, and all other tablets moving
down. This behavior matches the typical behavior precisely, and avoids a rude
UI discontinuity when the tablets grow to fill the entire screen (or shrink to
not fill it). If it is not desired, however, scrolling of focus can be
configured instead.

```c
// An ncreel is a Notcurses region devoted to displaying zero or more
// line-oriented, contained tablets between which the user may navigate. If at
// least one tablets exists, there is a "focused tablet". As much of the focused
// tablet as is possible is always displayed. If there is space left over, other
// tablets are included in the display. Tablets can come and go at any time, and
// can grow or shrink at any time.
//
// This structure is amenable to line- and page-based navigation via keystrokes,
// scrolling gestures, trackballs, scrollwheels, touchpads, and verbal commands.

// is scrolling infinite (can one move down or up forever, or is an end
// reached?). if true, 'circular' specifies how to handle the special case of
// an incompletely-filled reel.
#define NCREEL_OPTION_INFINITESCROLL 0x0001ull
// is navigation circular (does moving down from the last tablet move to the
// first, and vice versa)? only meaningful when infinitescroll is true. if
// infinitescroll is false, this must be false.
#define NCREEL_OPTION_CIRCULAR       0x0002ull

typedef struct ncreel_options {
  // Notcurses can draw a border around the ncreel, and also around the
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
  uint64_t flags;      // bitfield over NCREEL_OPTION_*
} ncreel_options;

struct nctablet;
struct ncreel;

// Take over the ncplane 'nc' and use it to draw a reel according to 'popts'.
// The plane will be destroyed by ncreel_destroy(); this transfers ownership.
struct ncreel* ncreel_create(struct ncplane* n, const ncreel_options* popts)
  __attribute__ ((nonnull (1)));

// Returns the ncplane on which this ncreel lives.
struct ncplane* ncreel_plane(struct ncreel* pr);

// Tablet draw callback, provided a tablet (from which the ncplane and userptr
// may be extracted), and a bool indicating whether output ought be drawn from
// the top (true) or bottom (false). Returns non-negative count of output lines,
// which must be less than or equal to ncplane_dim_y(nctablet_plane(t)).
typedef int (*tabletcb)(struct nctablet* t, bool drawfromtop);

// Add a new nctablet to the provided ncreel 'nr', having the callback object
// 'opaque'. Neither, either, or both of 'after' and 'before' may be specified.
// If neither is specified, the new tablet can be added anywhere on the reel.
// If one or the other is specified, the tablet will be added before or after
// the specified tablet. If both are specified, the tablet will be added to the
// resulting location, assuming it is valid (after->next == before->prev); if
// it is not valid, or there is any other error, NULL will be returned.
struct nctablet* ncreel_add(struct ncreel* nr, struct nctablet* after,
                            struct nctablet* before, tabletcb cb, void* opaque);

// Return the number of nctablets in the ncreel 'nr'.
int ncreel_tabletcount(const struct ncreel* nr);

// Delete the tablet specified by t from the ncreel 'nr'. Returns -1 if the
// tablet cannot be found.
int ncreel_del(struct ncreel* nr, struct nctablet* t);

// Redraw the ncreel 'nr' in its entirety. The reel will be cleared, and
// tablets will be lain out, using the focused tablet as a fulcrum. Tablet
// drawing callbacks will be invoked for each visible tablet.
int ncreel_redraw(struct ncreel* nr);

// Offer input 'ni' to the ncreel 'nr'. If it's relevant, this function returns
// true, and the input ought not be processed further. If it's irrelevant to
// the reel, false is returned. Relevant inputs include:
//  * a mouse click on a tablet (focuses tablet)
//  * a mouse scrollwheel event (rolls reel)
//  * up, down, pgup, or pgdown (navigates among items)
bool ncreel_offer_input(struct ncreel* nr, const ncinput* ni);

// Return the focused tablet, if any tablets are present. This is not a copy;
// be careful to use it only for the duration of a critical section.
struct nctablet* ncreel_focused(struct ncreel* nr);

// Change focus to the next tablet, if one exists
struct nctablet* ncreel_next(struct ncreel* nr);

// Change focus to the previous tablet, if one exists
struct nctablet* ncreel_prev(struct ncreel* nr);

// Destroy an ncreel allocated with ncreel_create().
void ncreel_destroy(struct ncreel* nr);

// Returns a pointer to a user pointer associated with this nctablet.
void* nctablet_userptr(struct nctablet* t);

// Access the ncplane associated with nctablet 't', if one exists.
struct ncplane* nctablet_plane(struct nctablet* t);
```

### ncreel examples

Let's say we have a screen of 11 lines, and 3 tablets of one line each. Both
a screen border and tablet borders are in use. The tablets are A, B, and C.
No gap is in use between tablets. Xs indicate focus. If B currently has focus,
and the next tablet is selected, the result would be something like:

```
 -------------                         -------------
 | --------- |                         | --------- |
 | |   A   | |                         | |   A   | |
 | --------- |                         | --------- |
 | --------- | ---- "next tablet" ---> | --------- |
 | |XX B XX| |                         | |   B   | |
 | --------- |                         | --------- |
 | --------- |                         | --------- |
 | |   C   | |                         | |XX C XX| |
 | --------- |                         | --------- |
 -------------                         -------------
```

If instead the previous tablet had been selected, we would of course get:

```
 -------------                         -------------
 | --------- |                         | --------- |
 | |   A   | |                         | |XX A XX| |
 | --------- |                         | --------- |
 | --------- | ---- "prev tablet" ---> | --------- |
 | |XX B XX| |                         | |   B   | |
 | --------- |                         | --------- |
 | --------- |                         | --------- |
 | |   C   | |                         | |   C   | |
 | --------- |                         | --------- |
 -------------                         -------------
```

If A instead has the focus, choosing the "next tablet" is trivial: the tablets
do not change, and focus shifts to B. If we choose the "previous tablet", there
are three possibilities:

* Finite scrolling: No change. The tablets stay in place. A remains focused.

```
 -------------                         -------------
 | --------- |                         | --------- |
 | |XX A XX| |                         | |XX A XX| |
 | --------- |                         | --------- |
 | --------- | ---- "prev tablet" ---> | --------- |
 | |   B   | |     (finite scroll)     | |   B   | |
 | --------- |                         | --------- |
 | --------- |                         | --------- |
 | |   C   | |                         | |   C   | |
 | --------- |                         | --------- |
 -------------                         -------------
```

* Infinite scrolling with rotation: Focus shifts to C, which moves to the top:

```
 -------------                         -------------
 | --------- |                         | --------- |
 | |XX A XX| |                         | |XX C XX| |
 | --------- |                         | --------- |
 | --------- | ---- "prev tablet" ---> | --------- |
 | |   B   | |  (infinite scroll with  | |   A   | |
 | --------- |        rotation)        | --------- |
 | --------- |                         | --------- |
 | |   C   | |                         | |   B   | |
 | --------- |                         | --------- |
 -------------                         -------------
```

* Infinite scrolling with focus rotation: Focus shifts to C, and moves to the bottom:

```
 -------------                         -------------
 | --------- |                         | --------- |
 | |XX A XX| |                         | |   A   | |
 | --------- |                         | --------- |
 | --------- | ---- "prev tablet" ---> | --------- |
 | |   B   | |  (infinite scroll with  | |   B   | |
 | --------- |     focus rotation)     | --------- |
 | --------- |                         | --------- |
 | |   C   | |                         | |XX C XX| |
 | --------- |                         | --------- |
 -------------                         -------------
```

Now imagine us to have the same 3 tablets, but each is now 4 lines. It is
impossible to have two of these tablets wholly onscreen at once, let alone all
three. If we started with A focused and at the top, the result after all three
tablets have grown will be:

```
 -------------                         -------------
 | --------- |                         | --------- | A remains at the top, and
 | |XX A XX| |                         | |XXXXXXX| | is wholly on-screen. B is
 | --------- |                         | |XX A XX| | below it, but we can show
 | --------- | ---- "grow tablet" ---> | |XXXXXXX| | only the first two lines.
 | |   B   | |       A (focused)       | |XXXXXXX| | C has been pushed
 | --------- |                         | --------- | off-screen.
 | --------- |                         | --------- |
 | |   C   | |                         | |       | |
 | --------- |                         | |   B   | |
 -------------                         -------------
```

When a tablet is enlarged, it grows towards the nearest boundary, unless that
would result in the focused tablet being moved, in which case the growing
tablet instead grows in the other direction (if the tablet is in the middle
of the screen exactly, it grows down). There is one exception to this rule: if
the tablets are not making full use of the screen, growth is always down (the
screen is always filled from the top), even if it moves the focused tablet.

A 12-line screen has three tablets: A (2 lines), B (1 line), C (1 line), filling
the screen exactly. B is focused, and grows two lines:

```
 -------------                         -------------
 | --------- |                         | --------- | B grows down, since it is
 | |   A   | |                         | |   A   | | closer to the bottom (3
 | |       | |                         | |       | | lines) than the top (4
 | --------- | ---- "grow tablet" ---> | --------- | lines). C is pushed almost
 | --------- |       B (focused)       | --------- | entirely off-screen. A is
 | |XX B XX| |                         | |XXXXXXX| | untouched.
 | --------- |                         | |XX B XX| |
 | --------- |                         | |XXXXXXX| |
 | |   C   | |                         | --------- |
 | --------- |                         | --------- |
 -------------                         -------------
```

Starting with the same situation, A grows by 2 lines instead:

```
 -------------                         -------------
 | --------- |                         | |       | | A grows up. It would have
 | |   A   | |                         | |   A   | | grown down, but that would
 | |       | |                         | |       | | have moved B, which has
 | --------- | ---- "grow tablet" ---> | --------- | the focus. B and C remain
 | --------- |     A (not focused)     | --------- | where they are; A moves
 | |XX B XX| |                         | |XX B XX| | partially off-screen.
 | --------- |                         | --------- |
 | --------- |                         | --------- |
 | |   C   | |                         | |   C   | |
 | --------- |                         | --------- |
 -------------                         -------------
```

If we started with the same situation, and B grew by 7 lines, it would first
push C entirely off-screen (B would then have four lines of text), and then
push A off-screen. B would then have eight lines of text, the maximum on a
12-line screen with both types of borders.

## Widgets

Selectors:

```
                              ╭──────────────────────────╮
                              │This is the primary header│
╭──────────────────────this is the secondary header──────╮
│        ↑                                               │
│ option1 Long text #1                                   │
│ option2 Long text #2                                   │
│ option3 Long text #3                                   │
│ option4 Long text #4                                   │
│ option5 Long text #5                                   │
│ option6 Long text #6                                   │
│        ↓                                               │
╰────────────────────────────────────here's the footer───╯
```

Multiselectors:

```
                                                   ╭───────────────────╮
                                                   │ short round title │
╭now this secondary is also very, very, very outlandishly long, you see┤
│  ↑                                                                   │
│ ☐ Pa231 Protactinium-231 (162kg)                                     │
│ ☐ U233 Uranium-233 (15kg)                                            │
│ ☐ U235 Uranium-235 (50kg)                                            │
│ ☒ Np236 Neptunium-236 (7kg)                                          │
│ ☐ Np237 Neptunium-237 (60kg)                                         │
│ ☐ Pu238 Plutonium-238 (10kg)                                         │
│ ☐ Pu239 Plutonium-239 (10kg)                                         │
│ ☒ Pu240 Plutonium-240 (40kg)                                         │
│ ☐ Pu241 Plutonium-241 (13kg)                                         │
│ ☐ Am241 Americium-241 (100kg)                                        │
│  ↓                                                                   │
╰────────────────────────press q to exit (there is sartrev("no exit"))─╯
```

Menus:

```
  Schwarzgerät  File                                    Help
xxxxxxxxxxxxxxxx╭─────────────╮xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
xxxxxxxxxxxxxxxx│New    Ctrl+n│xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
xxxxxxxxxxxxxxxx│Open   Ctrl+o│xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
xxxxxxxxxxxxxxxx│Close  Ctrl+c│xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
xxxxxxxxxxxxxxxx├─────────────┤xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
xxxxxxxxxxxxxxxx│Quit   Ctrl+q│xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
xxxxxxxxxxxxxxxx╰─────────────╯xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
```

### Plots

**FIXME**

### Readers

ncreaders provide freeform input in a (possibly multiline) region, supporting
optional libreadline-style keybindings.

```c
typedef struct ncreader_options {
  uint64_t tchannels; // channels used for input
  uint32_t tattrword; // attributes used for input
  bool scroll; // allow more than the physical area's worth of input
} ncreader_options;

// takes ownership of 'n', destroying it on any error (ncreader_destroy()
// otherwise destroys the ncplane).
struct ncreader* ncreader_create(struct ncplane* n, const ncreader_options* opts);

// empty the ncreader of any user input, and home the cursor.
int ncreader_clear(struct ncreader* n);

struct ncplane* ncreader_plane(struct ncreader* n);

// Offer the input to the ncreader. If it's relevant, this function returns
// true, and the input ought not be processed further. Almost all inputs
// are relevant to an ncreader, save synthesized ones.
bool ncreader_offer_input(struct ncreader* n, const ncinput* ni);

// return a nul-terminated heap copy of the current (UTF-8) contents.
char* ncreader_contents(const struct ncreader* n);

// destroy the reader and its bound plane(s). if 'contents' is not NULL, the
// UTF-8 input will be heap-duplicated and written to 'contents'.
void ncreader_destroy(struct ncreader* n, char** contents);
```

### Progbars

Progress bars proceed linearly in any of four directions. The entirety of the
provided plane will be used -- any border should be provided by the caller on
another plane. The plane will not be erased; text preloaded into the plane
will be consumed by the progress indicator. The bar is redrawn for each
provided progress report (a double between 0 and 1), and can regress with
lower values. The procession will take place along the longer dimension (at
the time of each redraw), with the horizontal length scaled by 2 for
purposes of comparison. I.e. for a plane of 20 rows and 50 columns, the
progress will be to the right (50 > 40) or left with `OPTION_RETROGRADE`.

```c
// Takes ownership of the ncplane 'n', which will be destroyed by
// ncprogbar_destroy(). The progress bar is initially at 0%.
struct ncprogbar* ncprogbar_create(struct ncplane* n, const ncprogbar_options* opts);

// Return a reference to the ncprogbar's underlying ncplane.
#define NCPROGBAR_OPTION_RETROGRADE        0x0001u // proceed left/down

typedef struct ncprogbar_options {
  uint32_t ulchannel; // upper-left channel. in the context of a progress bar,
  uint32_t urchannel; // "up" is the direction we are progressing towards, and
  uint32_t blchannel; // "bottom" is the direction of origin. for monochromatic
  uint32_t brchannel; // bar, all four channels ought be the same.
  uint64_t flags;
} ncprogbar_options;

struct ncplane* ncprogbar_plane(struct ncprogbar* n);

// Set the progress bar's completion, a double 0 <= 'p' <= 1.
int ncprogbar_set_progress(struct ncprogbar* n, double p);

// Get the progress bar's completion, a double on [0, 1].
double ncprogbar_progress(const struct ncprogbar* n);

// Destroy the progress bar and its underlying ncplane.
void ncprogbar_destroy(struct ncprogbar* n);
```

### Tabs

Tabbed widgets. The tab list is displayed at the top or at the bottom of the
plane, and only one tab is visible at a time.

```c
// Display the tab list at the bottom instead of at the top of the plane
#define NCTABBED_OPTION_BOTTOM 0x0001ull

typedef struct nctabbed_options {
  uint64_t selchan; // channel for the selected tab header
  uint64_t hdrchan; // channel for unselected tab headers
  uint64_t sepchan; // channel for the tab separator
  char* separator;  // separator string
  uint64_t flags;   // bitmask of NCTABBED_OPTION_*
} nctabbed_options;

// Tab content drawing callback. Takes the tab it was associated to, the ncplane
// on which tab content is to be drawn, and the user pointer of the tab.
// It is called during nctabbed_redraw().
typedef void (*tabcb)(struct nctab* t, struct ncplane* ncp, void* curry);

// Creates a new nctabbed widget, associated with the given ncplane 'n', and with
// additional options given in 'opts'. When 'opts' is NULL, it acts as if it were
// called with an all-zero opts. The widget takes ownership of 'n', and destroys
// it when the widget is destroyed. Returns the newly created widget. Returns
// NULL on failure, also destroying 'n'.
struct nctabbed* nctabbed_create(struct ncplane* n, const nctabbed_options* opts);

// Destroy an nctabbed widget. All memory belonging to 'nt' is deallocated,
// including all tabs and their names. The plane associated with 'nt' is also
// destroyed. Calling this with NULL does nothing.
void nctabbed_destroy(struct nctabbed* nt);

// Redraw the widget. This calls the tab callback of the currently selected tab
// to draw tab contents, and draws tab headers. The tab content plane is not
// modified by this function, apart from resizing the plane is necessary.
void nctabbed_redraw(struct nctabbed* nt);

// Make sure the tab header of the currently selected tab is at least partially
// visible. (by rotating tabs until at least one column is displayed)
// Does nothing if there are no tabs.
void nctabbed_ensure_selected_header_visible(struct nctabbed* nt);

// Returns the currently selected tab, or NULL if there are no tabs.
struct nctab* nctabbed_selected(struct nctabbed* nt);

// Returns the leftmost tab, or NULL if there are no tabs.
struct nctab* nctabbed_leftmost(struct nctabbed* nt);

// Returns the number of tabs in the widget.
int nctabbed_tabcount(struct nctabbed* nt);

// Returns the plane associated to 'nt'.
struct ncplane* nctabbed_plane(struct nctabbed* nt);

// Returns the tab content plane.
struct ncplane* nctabbed_content_plane(struct nctabbed* nt);

// Returns the tab callback.
tabcb nctab_cb(struct nctab* t)
  __attribute__ ((nonnull (1)));

// Returns the tab name. This is not a copy and it should not be stored.
const char* nctab_name(struct nctab* t);

// Returns the width (in columns) of the tab's name.
int nctab_name_width(struct nctab* t);

// Returns the tab's user pointer.
void* nctab_userptr(struct nctab* t);

// Returns the tab to the right of 't'. This does not change which tab is selected.
struct nctab* nctab_next(struct nctab* t);

// Returns the tab to the left of 't'. This does not change which tab is selected.
struct nctab* nctab_prev(struct nctab* t);

// Add a new tab to 'nt' with the given tab callback, name, and user pointer.
// If both 'before' and 'after' are NULL, the tab is inserted after the selected
// tab. Otherwise, it gets put after 'after' (if not NULL) and before 'before'
// (if not NULL). If both 'after' and 'before' are given, they must be two
// neighboring tabs (the tab list is circular, so the last tab is immediately
// before the leftmost tab), otherwise the function returns NULL. If 'name' is
// NULL or a string containing illegal characters, the function returns NULL.
// On all other failures the function also returns NULL. If it returns NULL,
// none of the arguments are modified, and the widget state is not altered.
struct nctab* nctabbed_add(struct nctabbed* nt, struct nctab* after,
                           struct nctab* before, tabcb tcb,
                           const char* name, void* opaque);

// Remove a tab 't' from 'nt'. Its neighboring tabs become neighbors to each
// other. If 't' if the selected tab, the tab after 't' becomes selected.
// Likewise if 't' is the leftmost tab, the tab after 't' becomes leftmost.
// If 't' is the only tab, there will no more be a selected or leftmost tab,
// until a new tab is added. Returns -1 if 't' is NULL, and 0 otherwise.
int nctabbed_del(struct nctabbed* nt, struct nctab* t);

// Move 't' after 'after' (if not NULL) and before 'before' (if not NULL).
// If both 'after' and 'before' are NULL, the function returns -1, otherwise
// it returns 0.
int nctab_move(struct nctabbed* nt, struct nctab* t, struct nctab* after,
               struct nctab* before);

// Move 't' to the right by one tab, looping around to become leftmost if needed.
void nctab_move_right(struct nctabbed* nt, struct nctab* t);

// Move 't' to the right by one tab, looping around to become the last tab if needed.
void nctab_move_left(struct nctabbed* nt, struct nctab* t);

// Rotate the tabs of 'nt' right by 'amt' tabs, or '-amt' tabs left if 'amt' is
// negative. Tabs are rotated only by changing the leftmost tab; the selected tab
// stays the same. If there are no tabs, nothing happens.
void nctabbed_rotate(struct nctabbed* nt, int amt);

// Select the tab after the currently selected tab, and return the newly selected
// tab. Returns NULL if there are no tabs.
struct nctab* nctabbed_next(struct nctabbed* nt);

// Select the tab before the currently selected tab, and return the newly selected
// tab. Returns NULL if there are no tabs.
struct nctab* nctabbed_prev(struct nctabbed* nt);

// Change the selected tab to be 't'. Returns the previously selected tab.
struct nctab* nctabbed_select(struct nctabbed* nt, struct nctab* t);

// Write the channels for tab headers, the selected tab header, and the separator
// to '*hdrchan', '*selchan', and '*sepchan' respectively.
void nctabbed_channels(struct nctabbed* nt, uint64_t* RESTRICT hdrchan,
                       uint64_t* RESTRICT selchan, uint64_t* RESTRICT sepchan);

static inline uint64_t
nctabbed_hdrchan(struct nctabbed* nt){
  uint64_t ch;
  nctabbed_channels(nt, &ch, NULL, NULL);
  return ch;
}

static inline uint64_t
nctabbed_selchan(struct nctabbed* nt){
  uint64_t ch;
  nctabbed_channels(nt, NULL, &ch, NULL);
  return ch;
}

static inline uint64_t
nctabbed_sepchan(struct nctabbed* nt){
  uint64_t ch;
  nctabbed_channels(nt, NULL, NULL, &ch);
  return ch;
}

// Returns the tab separator. This is not a copy and it should not be stored.
// This can be NULL, if the separator was set to NULL in ncatbbed_create() or
// nctabbed_set_separator().
const char* nctabbed_separator(struct nctabbed* nt);

// Returns the tab separator width, or zero if there is no separator.
int nctabbed_separator_width(struct nctabbed* nt);

// Set the tab headers channel for 'nt'.
void nctabbed_set_hdrchan(struct nctabbed* nt, uint64_t chan);

// Set the selected tab header channel for 'nt'.
void nctabbed_set_selchan(struct nctabbed* nt, uint64_t chan);

// Set the tab separator channel for 'nt'.
void nctabbed_set_sepchan(struct nctabbed* nt, uint64_t chan);

// Set the tab callback function for 't'. Returns the previous tab callback.
tabcb nctab_set_cb(struct nctab* t, tabcb newcb);

// Change the name of 't'. Returns -1 if 'newname' is NULL, and 0 otherwise.
int nctab_set_name(struct nctab* t, const char* newname);

// Set the user pointer of 't'. Returns the previous user pointer.
void* nctab_set_userptr(struct nctab* t, void* newopaque);

// Change the tab separator for 'nt'. Returns -1 if 'separator' is not NULL and
// is not a valid string, and 0 otherwise.
int nctabbed_set_separator(struct nctabbed* nt, const char* separator);
```

## Channels

A channel encodes 24 bits of RGB color, using 8 bits for each component. It
additionally provides 2 bits of alpha channel, a bit for selecting terminal
default colors, and a bit to indicate whether it describes a Wide East Asian
character. The remaining four bits are reserved. Typically two channels are
bound together in a 64-bit unsigned integer (`uint64_t`), with eight bits
currently going unused. There is such a double-channel in every `nccell` and
`ncplane` object.

Usually, the higher-level `ncplane` and `nccell` functionality ought be used. It
will sometimes be necessary, however, to muck with channels at their lowest
level. The channel API facilitates such muckery. All channel-related `ncplane`
and `nccell` functionality is implemented in terms of this API.

```c
// Extract the 8-bit red component from a 32-bit channel.
static inline unsigned
channel_r(uint32_t channel){
  return (channel & 0xff0000u) >> 16u;
}

// Extract the 8-bit green component from a 32-bit channel.
static inline unsigned
channel_g(uint32_t channel){
  return (channel & 0x00ff00u) >> 8u;
}

// Extract the 8-bit blue component from a 32-bit channel.
static inline unsigned
channel_b(uint32_t channel){
  return (channel & 0x0000ffu);
}

// Extract the three 8-bit R/G/B components from a 32-bit channel.
static inline unsigned
channel_rgb8(uint32_t channel, unsigned* r, unsigned* g, unsigned* b){
  *r = channel_r(channel);
  *g = channel_g(channel);
  *b = channel_b(channel);
  return channel;
}

// Set the three 8-bit components of a 32-bit channel, and mark it as not using
// the default color. Retain the other bits unchanged.
static inline int
ncchannel_set_rgb8(uint32_t* channel, unsigned r, unsigned g, unsigned b){
  if(r >= 256 || g >= 256 || b >= 256){
    return -1;
  }
  uint32_t c = (r << 16u) | (g << 8u) | b;
  // clear the existing rgb bits, clear the palette index indicator, set
  // the not-default bit, and or in the new rgb.
  *channel = (*channel & ~(NC_BG_RGB_MASK | NC_BG_PALETTE)) | NC_BGDEFAULT_MASK | c;
  return 0;
}

// Set the three 8-bit components of a 32-bit channel, and mark it as not using
// the default color. Retain the other bits unchanged. r, g, and b will be
// clipped to the range [0..255].
static inline void
ncchannel_set_rgb8_clipped(uint32_t* channel, int r, int g, int b){
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
  uint32_t c = (r << 16u) | (g << 8u) | b;
  *channel = (*channel & ~(NC_BG_RGB_MASK | NC_BG_PALETTE)) | NC_BGDEFAULT_MASK | c;
}

// Same, but provide an assembled, packed 24 bits of rgb.
static inline int
ncchannel_set(uint32_t* channel, uint32_t rgb){
  if(rgb > 0xffffffu){
    return -1;
  }
  *channel = (*channel & ~(NC_BG_RGB_MASK | NC_BG_PALETTE)) | NC_BGDEFAULT_MASK | rgb;
  return 0;
}

// Extract the 2-bit alpha component from a 32-bit channel.
static inline unsigned
channel_alpha(unsigned channel){
  return channel & NC_BG_ALPHA_MASK;
}

// Set the 2-bit alpha component of the 32-bit channel.
static inline int
channel_set_alpha(unsigned* channel, unsigned alpha){
  if(alpha & ~NC_BG_ALPHA_MASK){
    return -1;
  }
  *channel = alpha | (*channel & ~CHANNEL_ALPHA_MASK);
  if(alpha != NCALPHA_OPAQUE){
    *channel |= NC_BGDEFAULT_MASK;
  }
  return 0;
}

// Is this channel using the "default color" rather than its RGB?
static inline bool
ncchannel_default_p(uint32_t channel){
  return !(channel & NC_BGDEFAULT_MASK);
}

// Mark the channel as using its default color.
static inline uint32_t
ncchannel_set_default(uint32_t* channel){
  return *channel &= ~NC_BGDEFAULT_MASK;
}

// Extract the background alpha and coloring bits from a 64-bit channel pair.
static inline uint64_t
ncchannels_channels(uint64_t channels){
  return ncchannels_bchannel(channels) |
         ((uint64_t)ncchannels_fchannel(channels) << 32u);
}

// Set the alpha and coloring bits of a channel pair from another channel pair.
static inline uint64_t
ncchannels_set_channels(uint64_t* dst, uint64_t channels){
  ncchannels_set_bchannel(dst, channels & 0xffffffffull);
  ncchannels_set_fchannel(dst, (channels >> 32u) & 0xffffffffull);
  return *dst;
}

// Extract the background alpha and coloring bits from a 64-bit channel
// pair as a single 32-bit value.
static inline unsigned
ncchannels_bchannel(uint64_t channels){
  return channels & (NC_BG_RGB_MASK | NC_BG_PALETTE |
                     NC_BGDEFAULT_MASK | NC_BG_ALPHA_MASK);
}

// Extract the foreground alpha and coloring bits from a 64-bit channel
// pair as a single 32-bit value.
static inline uint32_t
ncchannels_fchannel(uint64_t channels){
  return ncchannels_bchannel(channels >> 32u);
}

// Returns the channels with the color information swapped, but not
// alpha, nor other housekeeping bits.
static inline uint64_t
ncchannels_reverse(uint64_t channels){
  const uint64_t raw = ((uint64_t)ncchannels_bchannel(channels) << 32u) +
                       ncchannels_fchannel(channels);
  const uint64_t statemask = ((NC_NOBACKGROUND_MASK | NC_BG_ALPHA_MASK) << 32u) |
                             NC_NOBACKGROUND_MASK | NC_BG_ALPHA_MASK;
  uint64_t ret = raw & ~statemask;
  ret |= channels & statemask;
  if(ncchannels_bg_alpha(ret) != NCALPHA_OPAQUE){
    if(!ncchannels_bg_rgb_p(ret)){
      ncchannels_set_bg_alpha(&ret, NCALPHA_OPAQUE);
    }
  }
  if(ncchannels_fg_alpha(ret) != NCALPHA_OPAQUE){
    if(!ncchannels_fg_rgb_p(ret)){
      ncchannels_set_fg_alpha(&ret, NCALPHA_OPAQUE);
    }
  }
  return ret;
}

// Extract 24 bits of foreground RGB from 'channels', shifted to LSBs.
static inline unsigned
ncchannels_fg_rgb(uint64_t channels){
  return ncchannels_fchannel(channels) & NC_BG_MASK;
}

// Extract 24 bits of background RGB from 'channels', shifted to LSBs.
static inline unsigned
ncchannels_bg_rgb(uint64_t channels){
  return ncchannels_bchannel(channels) & NC_BG_MASK;
}

// Extract 2 bits of foreground alpha from 'channels', shifted to LSBs.
static inline unsigned
ncchannels_fg_alpha(uint64_t channels){
  return ncchannel_alpha(channels_fchannel(channels));
}

// Extract 2 bits of background alpha from 'channels', shifted to LSBs.
static inline unsigned
ncchannels_bg_alpha(uint64_t channels){
  return ncchannel_alpha(channels_bchannel(channels));
}

// Extract the 24-bit RGB value from a 32-bit channel.
// Only valid if ncchannel_rgb_p() would return true for the channel.
static inline uint32_t
ncchannel_rgb(uint32_t channel){
  return channel & NC_BG_RGB_MASK;
}

// Extract the three 8-bit R/G/B components from a 32-bit channel.
// Only valid if ncchannel_rgb_p() would return true for the channel.
static inline uint32_t
ncchannel_rgb8(uint32_t channel, unsigned* restrict r, unsigned* restrict g,
               unsigned* restrict b){
  *r = ncchannel_r(channel);
  *g = ncchannel_g(channel);
  *b = ncchannel_b(channel);
  return channel;
}

// Extract 24 bits of foreground RGB from 'channels', split into subchannels.
static inline uint32_t
ncchannels_fg_rgb8(uint64_t channels, unsigned* r, unsigned* g, unsigned* b){
  return ncchannel_rgb8(channels_fchannel(channels), r, g, b);
}

// Extract 24 bits of background RGB from 'channels', split into subchannels.
static inline uint32_t
ncchannels_bg_rgb8(uint64_t channels, unsigned* r, unsigned* g, unsigned* b){
  return ncchannel_rgb8(channels_bchannel(channels), r, g, b);
}

// Set the r, g, and b channels for the foreground component of this 64-bit
// 'channels' variable, and mark it as not using the default color.
static inline int
ncchannels_set_fg_rgb8(uint64_t* channels, unsigned r, unsigned g, unsigned b){
  unsigned channel = ncchannels_fchannel(*channels);
  if(channel_set_rgb8(&channel, r, g, b) < 0){
    return -1;
  }
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
  return 0;
}

// Set the r, g, and b channels for the background component of this 64-bit
// 'channels' variable, and mark it as not using the default color.
static inline int
ncchannels_set_bg_rgb8(uint64_t* channels, unsigned r, unsigned g, unsigned b){
  unsigned channel = ncchannels_bchannel(*channels);
  if(channel_set_rgb8(&channel, r, g, b) < 0){
    return -1;
  }
  *channels = (*channels & 0xffffffff00000000llu) | channel;
  return 0;
}

// Same, but set an assembled 32 bit channel at once.
static inline int
ncchannels_set_fg_rgb(uint64_t* channels, unsigned rgb){
  unsigned channel = ncchannels_fchannel(*channels);
  if(channel_set(&channel, rgb) < 0){
    return -1;
  }
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
  return 0;
}

static inline int
ncchannels_set_bg_rgb(uint64_t* channels, unsigned rgb){
  unsigned channel = ncchannels_bchannel(*channels);
  if(channel_set(&channel, rgb) < 0){
    return -1;
  }
  *channels = (*channels & 0xffffffff00000000llu) | channel;
  return 0;
}

// Set the 2-bit alpha component of the foreground channel.
static inline int
ncchannels_set_fg_alpha(uint64_t* channels, unsigned alpha){
  unsigned channel = ncchannels_fchannel(*channels);
  if(channel_set_alpha(&channel, alpha) < 0){
    return -1;
  }
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
  return 0;
}

// Set the 2-bit alpha component of the background channel.
static inline int
ncchannels_set_bg_alpha(uint64_t* channels, unsigned alpha){
  if(alpha == NCALPHA_HIGHCONTRAST){ // forbidden for background alpha
    return -1;
  }
  unsigned channel = ncchannels_bchannel(*channels);
  if(channel_set_alpha(&channel, alpha) < 0){
    return -1;
  }
  *channels = (*channels & 0xffffffff00000000llu) | channel;
  return 0;
}

// Is the foreground using the "default foreground color"?
static inline bool
ncchannels_fg_default_p(uint64_t channels){
  return channel_default_p(ncchannels_fchannel(channels));
}

// Is the background using the "default background color"? The "default
// background color" must generally be used to take advantage of
// terminal-effected transparency.
static inline bool
ncchannels_bg_default_p(uint64_t channels){
  return channel_default_p(ncchannels_bchannel(channels));
}

// Mark the foreground channel as using its default color.
static inline uint64_t
ncchannels_set_fg_default(uint64_t* channels){
  unsigned channel = ncchannels_fchannel(*channels);
  channel_set_default(&channel);
  *channels = ((uint64_t)channel << 32llu) | (*channels & 0xffffffffllu);
  return *channels;
}

// Mark the foreground channel as using its default color.
static inline uint64_t
ncchannels_set_bg_default(uint64_t* channels){
  unsigned channel = ncchannels_bchannel(*channels);
  channel_set_default(&channel);
  *channels = (*channels & 0xffffffff00000000llu) | channel;
  return *channels;
}
```

## Visuals

`ncvisual`s are virtual pixel framebuffers. They can be operated upon using
familiar pixel graphics routines, and then rendered to a (character-
graphics) plane using a variety of blitting methods:

* Space with background color -- the only cell blitter that works in ASCII
  mode. 1:1 pixels map losslessly to 2:1 cells.
* Unicode upper- and lower-half blocks (▀ and ▄, respectively). 2:1 pixels
  map losslessly to 2:1 cells. The default blitting mode.
* Unicode half blocks plus quadrants. 2x2 pixels map to 2:1 cells.
* Unicode sextants. 3x2 pixels map to 2:1 cells.
* Braille. 4:2 pixels map to 2:1 cells. Useful when only two colors are needed
  in a small area, due to high resolution.
* Sixel- and Kitty-based bitmaps.

It is most typicaly to prepare `ncvisual`s from files on disk (see 
[Multimedia](#multimedia) below); this requires Notcurses to be built against
a multimedia engine. Even without such an engine, `ncvisual`s can be
constructed directly from RGBA or BGRA 8bpc memory:

```c
// Prepare an ncvisual, and its underlying plane, based off RGBA content in
// memory at 'rgba'. 'rgba' is laid out as 'rows' lines, each of which is
// 'rowstride' bytes in length. Each line has 'cols' 32-bit 8bpc RGBA pixels
// followed by possible padding (there will be 'rowstride' - 'cols' * 4 bytes
// of padding). The total size of 'rgba' is thus (rows * rowstride) bytes, of
// which (rows * cols * 4) bytes are actual non-padding data.
struct ncvisual* ncvisual_from_rgba(const void* rgba, int rows,
                                    int rowstride, int cols);

// ncvisual_from_rgba(), but the pixels are 4-byte RGBx. A is filled in
// throughout using 'alpha'. rowstride must be a multiple of 4.
struct ncvisual* ncvisual_from_rgb_packed(const void* rgba, int rows,
                                          int rowstride, int cols, int alpha);

// ncvisual_from_rgba(), but the pixels are 3-byte RGB. A is filled in
// throughout using 'alpha'.
struct ncvisual* ncvisual_from_rgb_loose(const void* rgba, int rows,
                                         int rowstride, int cols, int alpha);

// ncvisual_from_rgba(), but for BGRA.
struct ncvisual* ncvisual_from_bgra(struct notcurses* nc, const void* bgra,
                                    int rows, int rowstride, int cols);

// ncvisual_from_rgba(), but 'data' is 'pstride'-byte palette-indexed pixels,
// arranged in 'rows' lines of 'rowstride' bytes each, composed of 'cols'
// pixels. 'palette' is an array of at least 'palsize' ncchannels.
struct ncvisual* ncvisual_from_palidx(const void* data, int rows,
                                      int rowstride, int cols, int palsize,
                                      int pstride, const uint32_t* palette);

// Construct an ncvisual from a nul-terminated Sixel control sequence.
struct ncvisual* ncvisual_from_sixel(const char* s, unsigned leny, unsigned lenx);
```

`ncvisual`s can also be loaded from the contents of a plane:

```c
// Promote an ncplane 'n' to an ncvisual. The plane may contain only spaces,
// half blocks, and full blocks. The latter will be checked, and any other
// glyph will result in a NULL being returned. This function exists so that
// planes can be subjected to ncvisual transformations. If possible, it's
// better to create the ncvisual from memory using ncvisual_from_rgba().
// Lengths of 0 are interpreted to mean "all available remaining area".
struct ncvisual* ncvisual_from_plane(const struct ncplane* n,
                                     ncblitter_e blit,
                                     unsigned begy, unsigned begx,
                                     unsigned leny, unsigned lenx);
```

Various transformations can be applied to an `ncvisual`, regardless of how
it was built up:

```c
// describes all geometries of an ncvisual: those which are inherent, and those
// dependent upon a given rendering regime. pixy and pixx are the true internal
// pixel geometry, taken directly from the load (and updated by
// ncvisual_resize()). cdimy/cdimx are the cell-pixel geometry *at the time
// of this call* (it can change with a font change, in which case all values
// other than pixy/pixx are invalidated). rpixy/rpixx are the pixel geometry as
// handed to the blitter, following any scaling. scaley/scalex are the number
// of input pixels drawn to a single cell; when using NCBLIT_PIXEL, they are
// equivalent to cdimy/cdimx. rcelly/rcellx are the cell geometry as written by
// the blitter, following any padding (there is padding whenever rpix{y, x} is
// not evenly divided by scale{y, x}, and also sometimes for Sixel).
// maxpixely/maxpixelx are defined only when NCBLIT_PIXEL is used, and specify
// the largest bitmap that the terminal is willing to accept. blitter is the
// blitter which will be used, a function of the requested blitter and the
// blitters actually supported by this environment. if no ncvisual was
// supplied, only cdimy/cdimx are filled in.
typedef struct ncvgeom {
  unsigned pixy, pixx;     // true pixel geometry of ncvisual data
  unsigned cdimy, cdimx;   // terminal cell geometry when this was calculated
  unsigned rpixy, rpixx;   // rendered pixel geometry (per visual_options)
  unsigned rcelly, rcellx; // rendered cell geometry (per visual_options)
  unsigned scaley, scalex; // pixels per filled cell (scale == c for bitmaps)
  unsigned begy, begx;     // upper-left corner of used region
  unsigned leny, lenx;     // geometry of used region
  unsigned maxpixely, maxpixelx; // only defined for NCBLIT_PIXEL
  ncblitter_e blitter;     // blitter that will be used
} ncvgeom;

// all-purpose ncvisual geometry solver. one or both of 'nc' and 'n' must be
// non-NULL. if 'nc' is NULL, only pixy/pixx will be filled in, with the true
// pixel geometry of 'n'. if 'n' is NULL, only cdimy/cdimx, blitter,
// scaley/scalex, and maxpixely/maxpixelx are filled in. cdimy/cdimx and
// maxpixely/maxpixelx are only ever filled in if we know them.
int ncvisual_geom(const struct notcurses* nc, const struct ncvisual* n,
                  const struct ncvisual_options* vopts, ncvgeom* geom);

// Scale the visual to 'rows' X 'columns' pixels, using the best scheme
// available. This is a lossy transformation, unless the size is unchanged.
int ncvisual_rotate(struct ncvisual* n, double rads);

// Resize the visual so that it is 'rows' X 'columns'. This is a lossy
// transformation, unless the size is unchanged.
int ncvisual_resize(struct ncvisual* n, int rows, int cols);

// Scale the visual to 'rows' X 'columns' pixels, using non-interpolative
// (naive) scaling. No new colors will be introduced as a result.
int ncvisual_resize_noninterpolative(struct ncvisual* n, int rows, int cols);

// Polyfill at the specified location within the ncvisual 'n', using 'rgba'.
int ncvisual_polyfill_yx(struct ncvisual* n, unsigned y, unsigned x, uint32_t rgba);

// Get the specified pixel from the specified ncvisual.
int ncvisual_at_yx(const struct ncvisual* n, unsigned y, unsigned x,
                   uint32_t* pixel);

// Set the specified pixel in the specified ncvisual.
int ncvisual_set_yx(const struct ncvisual* n, unsigned y, unsigned x,
                    uint32_t pixel);

// If a subtitle ought be displayed at this time, return a new plane (bound
// to 'parent' containing the subtitle, which might be text or graphics
// (depending on the input format).
struct ncplane* ncvisual_subtitle_plane(struct ncplane* parent,
                                        const struct ncvisual* ncv);
```

And finally, the `ncvisual` can be blitted to one or more `ncplane`s:

```c
// Render the decoded frame according to the provided options (which may be
// NULL). The plane used for rendering depends on vopts->n and vopts->flags.
// If NCVISUAL_OPTION_CHILDPLANE is set, vopts->n must not be NULL, and the
// plane will always be created as a child of vopts->n. If this flag is not
// set, and vopts->n is NULL, a new plane is created as root of a new pile.
// If the flag is not set and vopts->n is not NULL, we render to vopts->n.
// A subregion of the visual can be rendered using 'begx', 'begy', 'lenx', and
// 'leny'. Negative values for any of thse are an error. It is an error to
// specify any region beyond the boundaries of the frame. Returns the (possibly
// newly-created) plane to which we drew. Pixels may not be blitted to the
// standard plane.
struct ncplane* ncvisual_blit(struct notcurses* nc, struct ncvisual* ncv,
                              const struct ncvisual_options* vopts);

// Create a new plane as prescribed in opts, either as a child of 'vopts->n',
// or the root of a new pile if 'vopts->n' is NULL (or 'vopts' itself is NULL).
// Blit 'ncv' to the created plane according to 'vopts'. If 'vopts->n' is
// non-NULL, NCVISUAL_OPTION_CHILDPLANE must be supplied.
static inline struct ncplane*
ncvisualplane_create(struct notcurses* nc, const struct ncplane_options* opts,
                     struct ncvisual* ncv, struct ncvisual_options* vopts);

// decode the next frame ala ncvisual_decode(), but if we have reached the end,
// rewind to the first frame of the ncvisual. a subsequent `ncvisual_blit()`
// will render the first frame, as if the ncvisual had been closed and reopened.
// the return values remain the same as those of ncvisual_decode().
int ncvisual_decode_loop(struct ncvisual* nc);

// we never blit full blocks, but instead spaces (more efficient) with the
// background set to the desired foreground.
typedef enum {
  NCBLIT_DEFAULT, // let the ncvisual pick
  NCBLIT_1x1,     // space, compatible with ASCII
  NCBLIT_2x1,     // halves + 1x1 (space)     ▄▀
  NCBLIT_2x2,     // quadrants + 2x1          ▗▐ ▖▀▟▌▙
  NCBLIT_3x2,     // sextants + 2x1           🬀🬁🬂🬃🬄🬅🬆🬇🬈🬉🬊🬋🬌🬍🬎🬏🬐🬑🬒🬓🬔🬕🬖🬗🬘🬙🬚🬛🬜🬝🬞
  NCBLIT_4x2,     // octants + 2x2            there are a great many octants
  NCBLIT_BRAILLE, // 4 rows, 2 cols (braille) ⡀⡄⡆⡇⢀⣀⣄⣆⣇⢠⣠⣤⣦⣧⢰⣰⣴⣶⣷⢸⣸⣼⣾⣿
  NCBLIT_PIXEL,   // pixel graphics (also work in ASCII)
  NCBLIT_4x1,     // four vertical levels     █▆▄▂     (plots only)
  NCBLIT_8x1,     // eight vertical levels    █▇▆▅▄▃▂▁ (plots only)
} ncblitter_e;

// Lex a blitter.
int notcurses_lex_blitter(const char* op, ncblitter_e* blitter);

// Get the name of a blitter.
const char* notcurses_str_blitter(ncblitter_e blitter);

#define NCVISUAL_OPTION_NODEGRADE     0x0001ull // fail rather than degrade
#define NCVISUAL_OPTION_BLEND         0x0002ull // use NCALPHA_BLEND
#define NCVISUAL_OPTION_HORALIGNED    0x0004ull // x is an alignment, not abs
#define NCVISUAL_OPTION_VERALIGNED    0x0008ull // y is an alignment, not abs
#define NCVISUAL_OPTION_ADDALPHA      0x0010ull // transcolor is in effect
#define NCVISUAL_OPTION_CHILDPLANE    0x0020ull // interpret n as parent
#define NCVISUAL_OPTION_NOINTERPOLATE 0x0040ull // non-interpolative scaling

struct ncvisual_options {
  // if no ncplane is provided, one will be created using the exact size
  // necessary to render the source with perfect fidelity (this might be
  // smaller or larger than the rendering area).
  struct ncplane* n;
  // the scaling is ignored if no ncplane is provided (it ought be NCSCALE_NONE
  // in this case). otherwise, the source is stretched/scaled relative to the
  // provided ncplane.
  ncscale_e scaling;
  // if an ncplane is provided, y and x specify where the visual will be
  // rendered on that plane. otherwise, they specify where the created ncplane
  // will be placed relative to the standard plane's origin. x is an ncalign_e
  // value if NCVISUAL_OPTION_HORALIGNED is provided. y is an ncalign_e
  // value if NCVISUAL_OPTION_VERALIGNED is provided.
  int y, x;
  // the region of the visual that ought be rendered. for the entire visual,
  // pass an origin of 0, 0 and a size of 0, 0 (or the true height and width).
  // these numbers are all in terms of ncvisual pixels. negative values are
  // prohibited.
  int begy, begx; // origin of rendered region
  int leny, lenx; // size of rendered region
  // use NCBLIT_DEFAULT if you don't care, an appropriate blitter will be
  // chosen for your terminal, given your scaling. NCBLIT_PIXEL is never
  // chosen for NCBLIT_DEFAULT.
  ncblitter_e blitter; // glyph set to use (maps input to output cells)
  uint64_t flags; // bitmask over NCVISUAL_OPTION_*
  uint32_t transcolor; // used only if NCVISUAL_OPTION_ADDALPHA is set
  // pixel offsets within the cell. if NCBLIT_PIXEL is used, the bitmap will
  // be drawn offset from the upper-left cell's origin by these amounts. it is
  // an error if either number exceeds the cell-pixel geometry in its
  // dimension. if NCBLIT_PIXEL is not used, these fields are ignored.
  // this functionality can be used for smooth bitmap movement.
  // FIXME not yet implemented.
  unsigned pxoffy, pxoffx;
};

typedef enum {
  NCSCALE_NONE,
  NCSCALE_SCALE,
  NCSCALE_STRETCH,
  NCSCALE_SCALE_HIRES,
  NCSCALE_NONE_HIRES,
  NCSCALE_INFLATE,
} ncscale_e;

// Lex a scaling mode (one of "none", "stretch", "scale", "hires",
// "inflate", or "scalehi").
int notcurses_lex_scalemode(const char* op, ncscale_e* scalemode);

// Get the name of a scaling mode.
const char* notcurses_str_scalemode(ncscale_e scalemode);

// the streaming operation ceases immediately, and that value is propagated out.
// The recommended absolute display time target is passed in 'tspec'.
typedef int (*streamcb)(struct ncplane*, struct ncvisual*,
                        const struct timespec*, void*);

// Shut up and display my frames! Provide as an argument to ncvisual_stream().
// If you'd like subtitles to be decoded, provide an ncplane as the curry. If the
// curry is NULL, subtitles will not be displayed.
int ncvisual_simple_streamer(struct ncvisual* ncv, struct ncvisual_options* vopts,
                             const struct timespec* tspec, void* curry);

// Stream the entirety of the media, according to its own timing. Blocking,
// obviously. streamer may be NULL; it is otherwise called for each frame, and
// its return value handled as outlined for stream cb. If streamer() returns
// non-zero, the stream is aborted, and that value is returned. By convention,
// return a positive number to indicate intentional abort from within
// streamer(). 'timescale' allows the frame duration time to be scaled. For a
// visual naturally running at 30FPS, a 'timescale' of 0.1 will result in
// 300FPS, and a 'timescale' of 10 will result in 3FPS. It is an error to
// supply 'timescale' less than or equal to 0.
int ncvisual_stream(struct notcurses* nc, struct ncvisual* ncv, float timescale,
                    streamcb streamer, const struct ncvisual_options* vopts,
                    void* curry);
```

### QR codes

If built with libqrcodegen support, `ncplane_qrcode()` can be used to draw
a QR code for arbitrary data.

```c
// Draw a QR code at the current position on the plane. If there is insufficient
// room to draw the code here, or there is any other error, non-zero will be
// returned. Otherwise, the QR code "version" (size) is returned. The QR code
// is (version * 4 + 17) columns wide, and ⌈version * 4 + 17⌉ rows tall (the
// properly-scaled values are written back to '*ymax' and '*xmax').
int ncplane_qrcode(struct ncplane* n, unsigned* ymax, unsigned* xmax,
                   const void* data, size_t len);
```

### Multimedia

When compiled against a suitable engine (FFmpeg and OpenImageIO are both
currently supported), Notcurses can populate a visual with pixels decoded
from an image or video using `ncvisual_from_file()`. Once opened,
`ncvisual_decode()` should be used to extract each frame (an image will
have only one frame), until it returns `NCERR_EOF`:

```c
// Open a visual at 'file', extracting a codec and parameters.
struct ncvisual* ncvisual_from_file(const char* file);

// extract the next frame from an ncvisual. returns NCERR_EOF on end of file,
// and NCERR_SUCCESS on success, otherwise some other NCERR.
int ncvisual_decode(struct ncvisual* nc);
```

### Pixels

It is sometimes desirable to modify the pixels of an `ncvisual` directly.

```c
static inline int
ncpixel_set_r(uint32_t* pixel, int r){
  if(r > 255 || r < 0){
    return -1;
  }
  *pixel = (*pixel & 0xffffff00ul) | (r);
  return 0;
}

static inline int
ncpixel_set_g(uint32_t* pixel, int g){
  if(g > 255 || g < 0){
    return -1;
  }
  *pixel = (*pixel & 0xff00fffful) | (g << 16u);
  return 0;
}

static inline int
ncpixel_set_b(uint32_t* pixel, int b){
  if(b > 255 || b < 0){
    return -1;
  }
  *pixel = (*pixel & 0xffff00fful) | (b << 8u);
  return 0;
}

// set the RGB values of an RGB pixel
static inline int
ncpixel_set_rgb8(uint32_t* pixel, unsigned r, unsigned g, unsigned b){
  if(pixel_set_r(pixel, r) || pixel_set_g(pixel, g) || pixel_set_b(pixel, b)){
    return -1;
  }
  return 0;
}
```

## Stats

Notcurses supplies a number of stats related to performance and state.
Cumulative stats can be reset at any time.

```c
typedef struct ncstats {
  // purely increasing stats
  uint64_t renders;          // successful ncpile_render() runs
  uint64_t writeouts;        // successful ncpile_rasterize() runs
  uint64_t failed_renders;   // aborted renders, should be 0
  uint64_t failed_writeouts; // aborted writes
  uint64_t raster_bytes;     // bytes emitted to ttyfp
  int64_t raster_max_bytes;  // max bytes emitted for a frame
  int64_t raster_min_bytes;  // min bytes emitted for a frame
  uint64_t render_ns;        // nanoseconds spent rendering
  int64_t render_max_ns;     // max ns spent in render for a frame
  int64_t render_min_ns;     // min ns spent in render for a frame
  uint64_t raster_ns;        // nanoseconds spent rasterizing
  int64_t raster_max_ns;     // max ns spent in raster for a frame
  int64_t raster_min_ns;     // min ns spent in raster for a frame
  uint64_t writeout_ns;      // nanoseconds spent writing frames to terminal
  int64_t writeout_max_ns;   // max ns spent writing out a frame
  int64_t writeout_min_ns;   // min ns spent writing out a frame
  uint64_t cellelisions;     // cells we elided entirely thanks to damage maps
  uint64_t cellemissions;    // total number of cells emitted to terminal
  uint64_t fgelisions;       // RGB fg elision count
  uint64_t fgemissions;      // RGB fg emissions
  uint64_t bgelisions;       // RGB bg elision count
  uint64_t bgemissions;      // RGB bg emissions
  uint64_t defaultelisions;  // default color was emitted
  uint64_t defaultemissions; // default color was elided
  uint64_t refreshes;        // refresh requests (non-optimized redraw)
  uint64_t sprixelemissions; // sprixel draw count
  uint64_t sprixelelisions;  // sprixel elision count
  uint64_t sprixelbytes;     // sprixel bytes emitted
  uint64_t appsync_updates;  // how many application-synchronized updates?
  uint64_t input_errors;     // errors processing control sequences/utf8
  uint64_t input_events;     // characters returned to userspace
  uint64_t hpa_gratuitous;   // unnecessary hpas issued

  // current state -- these can decrease
  uint64_t fbbytes;          // total bytes devoted to all active framebuffers
  unsigned planes;           // number of planes currently in existence
} ncstats;

// Allocate an ncstats object. Use this rather than allocating your own, since
// future versions of Notcurses might enlarge this structure.
ncstats* notcurses_stats_alloc(const struct notcurses* nc);

// Acquire an atomic snapshot of the Notcurses object's stats.
void notcurses_stats(struct notcurses* nc, ncstats* stats);

// Reset all cumulative stats (immediate ones, such as fbbytes, are not reset),
// first copying them into |*stats| (if |stats| is not NULL).
void notcurses_stats_reset(struct notcurses* nc, ncstats* stats);
```

## C++

Marek Habersack has contributed (and maintains) C++ wrappers installed to
`include/ncpp/`, `libnotcurses++.so`, and `libnotcurses++.a`.

In their default mode, these wrappers throw exceptions only from the type 
constructors (RAII). If `NCPP_EXCEPTIONS_PLEASE` is defined prior to including 
any NCPP headers, they will throw exceptions.
