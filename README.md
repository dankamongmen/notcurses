# notcurses
blingful TUI library for modern terminal emulators. definitely not curses.

* birthed screaming into this world by [nick black](https://nick-black.com/dankwiki/index.php/Hack_on) (<nickblack@linux.com>)
* C++ wrappers by [marek habersack](http://twistedcode.net/blog/) (<grendel@twistedcode.net>)

for more information, see [dankwiki](https://nick-black.com/dankwiki/index.php/Notcurses)
and the [man pages](https://nick-black.com/notcurses). I am working on a
coherent [guidebook](https://nick-black.com/htp-notcurses.pdf), though it has
not yet reached even first draft status. In addition, there is
[Doxygen](https://nick-black.com/notcurses/html/) output. There is a
[mailing list](https://groups.google.com/forum/#!forum/notcurses) which
can be reached via notcurses@googlegroups.com.

notcurses is available in the Arch [AUR](https://aur.archlinux.org/packages/notcurses/).
Packages for Debian Unstable and Ubuntu Focal are available from [DSSCAW](https://www.dsscaw.com/apt.html).

[![Build Status](https://drone.dsscaw.com:4443/api/badges/dankamongmen/notcurses/status.svg)](https://drone.dsscaw.com:4443/dankamongmen/notcurses)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

* [Introduction](#introduction)
* [Requirements](#requirements)
  * [Building](#building)
* [Use](#use)
  * [Direct Mode](#direct-mode)
  * [Alignment](#alignment)
  * [Input](#input)
  * [Planes](#planes) ([Plane Channels API](#plane-channels-api))
  * [Cells](#cells) ([Cell Channels API](#cell-channels-api))
  * [Multimedia](#multimedia)
  * [Reels](#reels)
  * [Selectors](#selectors)
  * [Menus](#menus)
  * [Channels](#channels)
* [Included tools](#included-tools)
* [Differences from NCURSES](#differences-from-ncurses)
  * [Features missing relative to NCURSES](#features-missing-relative-to-ncurses)
  * [Adapting NCURSES programs](#adapting-ncurses-programs)
* [Environment notes](#environment-notes)
  * [DirectColor detection](#DirectColor-detection)
  * [Fonts](#fonts)
  * [FAQs](#faqs)
* [Supplemental material](#supplemental-material)
  * [Useful links](#useful-links)
  * [Other TUI libraries](#other-tui-libraries-of-note)
  * [History](#history)
  * [Thanks](#thanks)

## Introduction

* **What it is**: a library facilitating complex TUIs on modern terminal
    emulators, supporting vivid colors and Unicode to the maximum degree
    possible. Many tasks delegated to Curses can be achieved using notcurses
    (and vice versa).

* **What it is not**: a source-compatible X/Open Curses implementation, nor a
    replacement for NCURSES on existing systems, nor a widely-ported and -tested
    bedrock of free software, nor a battle-proven, veteran library.

notcurses abandons the X/Open Curses API bundled as part of the Single UNIX
Specification. The latter shows its age, and seems not capable of making use of
terminal functionality such as unindexed 24-bit color ("DirectColor", not to be
confused with 8-bit indexed 24-bit color, aka "TrueColor" or (by NCURSES) as
"extended color"). For some necessary background, consult Thomas E. Dickey's
superb and authoritative [NCURSES FAQ](https://invisible-island.net/ncurses/ncurses.faq.html#xterm_16MegaColors).
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

* Thread safety, and efficient use in parallel programs, has been a design
  consideration from the beginning.

* A svelter design than that codified by X/Open:
  * Exported identifiers are prefixed to avoid common namespace collisions.
  * The library object exports a minimal set of symbols. Where reasonable,
    `static inline` header-only code is used. This facilitates compiler
    optimizations, and reduces loader time.

* All APIs natively (and exclusively) support UTF-8. The `cell` API is based
  around Unicode's [Extended Grapheme Cluster](https://unicode.org/reports/tr29/) concept.

* Visual features including images, fonts, video, high-contrast text, sprites,
  and transparent regions. All APIs natively support 24-bit color, quantized
  down as necessary for the terminal.

* It's Apache2-licensed in its entirety, as opposed to the
  [drama in several acts](https://invisible-island.net/ncurses/ncurses-license.html)
  that is the NCURSES license (the latter is [summarized](https://invisible-island.net/ncurses/ncurses-license.html#issues_freer)
  as "a restatement of MIT-X11").

Much of the above can be had with NCURSES, but they're not what NCURSES was
*designed* for. The most fundamental advantage in my mind, though, is
that notcurses is of the multithreaded era. On the other hand, if you're
targeting industrial or critical applications, or wish to benefit from the
time-tested reliability and portability of Curses, you should by all means use
that fine library.

## Requirements

* (build) A C11 and a C++17 compiler
* (build) CMake 3.14.0+
* (build+runtime) From NCURSES: terminfo 6.1+
* (OPTIONAL) (build+runtime) From FFMpeg: libswscale 5.0+, libavformat 57.0+, libavutil 56.0+
* (OPTIONAL) (testing) [Doctest](https://github.com/onqtam/doctest) 2.3.5+
* (OPTIONAL) (documentation) [pandoc](https://pandoc.org/index.html) 1.19.2+
* (OPTIONAL) (python bindings): Python 3.7+, CFFI 1.13.2+
* (OPTIONAL) (rust bindings, colloquy): rust 1.40.0+, cargo 0.40.0+, cmake-rs 0.1.42+

### Building

* Create a subdirectory, traditionally `build`. Enter the directory.
* `cmake ..`. You might want to set e.g. `CMAKE_BUILD_TYPE`.
* `make`
* `make test`

If you have unit test failures, *please* file a bug including the output of
`./notcurses-tester > log 2>&1` (`make test` also runs `notcurses-tester`, but
hides important output).

To watch the bitchin' demo, run `./notcurses-demo -p ../data`. More details can
be found on the `notcurses-demo(1)` man page.

## Use

A full API reference [is available](https://nick-black.com/notcurses/). Manual
pages ought have been installed along with notcurses.

A program wishing to use notcurses will need to link it, ideally using the
output of `pkg-config --libs notcurses`. It is advised to compile with the
output of `pkg-config --cflags notcurses`. If using CMake, a support file is
provided, and can be accessed as `notcurses`.

Before calling into notcurses—and usually as one of the first calls of the
program—be sure to call `setlocale(3)` with an appropriate UTF-8 `LC_ALL`
locale. It is usually appropriate to use `setlocale(LC_ALL, "")`, relying on
the user to properly set the `LANG` environment variable. notcurses will
refuse to start if `nl_langinfo(3)` doesn't indicate UTF-8. In addition, it is
wise to mask most signals early in the program, before any threads are spawned.
(this is particularly critical for `SIGWINCH`).

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
  // We typically install a signal handler for SIGINT and SIGQUIT that restores
  // the screen, and then calls the old signal handler. Set this to inhibit
  // registration of any signal handlers.
  bool no_quit_sighandlers;
  // We typically install a signal handler for SIGWINCH that generates a resize
  // event in the notcurses_getc() queue. Set this to inhibit the handler.
  bool no_winch_sighandler;
  // Notcurses typically prints version info in notcurses_init() and
  // performance info in notcurses_stop(). This inhibits that output.
  bool suppress_banner;
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
struct notcurses* notcurses_init(const notcurses_options* opts, FILE* fp);

// Destroy a notcurses context.
int notcurses_stop(struct notcurses* nc);
```

`notcurses_stop` should be called before exiting your program to restore the
terminal settings and free resources.

notcurses does not typically generate diagnostics (aside from the intro banner
and outro performance summary). When `stderr` is connected to the same terminal
to which graphics are being printed, printing to stderr will corrupt the output.
Setting `loglevel` to a value higher than `NCLOGLEVEL_SILENT` will cause
diagnostics to be printed to `stderr`: you could ensure `stderr` is redirected
if you make use of this functionality.

It's probably wise to export `inhibit_alternate_screen` to the user (e.g. via
command line option or environment variable). Developers and motivated users
might appreciate the ability to manipulate `loglevel` and `renderfp`. The
remaining options are typically of use only to application authors.

The notcurses API draws almost entirely into the virtual buffers of `ncplane`s.
Only upon a call to `notcurses_render` will the visible terminal display be
updated to reflect the changes:

```c
// Make the physical screen match the virtual screen. Changes made to the
// virtual screen (i.e. most other calls) will not be visible until after a
// successful call to notcurses_render().
int notcurses_render(struct notcurses* nc);

// Retrieve the contents of the specified cell as last rendered. The EGC is
// returned, or NULL on error. This EGC must be free()d by the caller. The cell
// 'c' is not bound to a plane, and thus its gcluster value must not be used--
// use the return value only.
char* notcurses_at_yx(struct notcurses* nc, int yoff, int xoff, cell* c);
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
notcurses_stddim_yx(struct notcurses* nc, int* restrict y, int* restrict x){
  struct ncplane* s = notcurses_stdplane(nc); // can't fail
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
// Return the topmost ncplane, of which there is always at least one.
struct ncplane* notcurses_top(struct notcurses* n);

// Destroy any ncplanes other than the stdplane.
void notcurses_drop_planes(struct notcurses* nc);

// Refresh our idea of the terminal's dimensions, reshaping the standard plane
// if necessary. Without a call to this function following a terminal resize
// (as signaled via SIGWINCH), notcurses_render() might not function properly.
// References to ncplanes (and the egcpools underlying cells) remain valid
// following a resize operation, but the cursor might have changed position.
int notcurses_resize(struct notcurses* n, int* restrict y, int* restrict x);

// Return our current idea of the terminal dimensions in rows and cols.
static inline void
notcurses_term_dim_yx(const struct notcurses* n, int* restrict rows,
                      int* restrict cols){
  ncplane_dim_yx(notcurses_stdplane(n), rows, cols);
}

// Refresh the physical screen to match what was last rendered (i.e., without
// reflecting any changes since the last call to notcurses_render()). This is
// primarily useful if the screen is externally corrupted.
int notcurses_refresh(struct notcurses* n);

// Returns a 16-bit bitmask in the LSBs of supported curses-style attributes
// (NCSTYLE_UNDERLINE, NCSTYLE_BOLD, etc.) The attribute is only
// indicated as supported if the terminal can support it together with color.
// For more information, see the "ncv" capability in terminfo(5).
unsigned notcurses_supported_styles(const struct notcurses* nc);

// Returns the number of simultaneous colors claimed to be supported, or 1 if
// there is no color support. Note that several terminal emulators advertise
// more colors than they actually support, downsampling internally.
int notcurses_palette_size(const struct notcurses* nc);

// Can we fade? Fading requires either the "rgb" or "ccc" terminfo capability.
bool notcurses_canfade(const struct notcurses* nc);

// Can we load images/videos? This requires being built against FFmpeg.
bool notcurses_canopen(const struct notcurses* nc);

// Can we change colors in the hardware palette? Requires "ccc" and "initc".
bool notcurses_canchangecolors(const struct notcurses* nc);
```

### Direct mode

"Direct mode" makes a limited subset of notcurses is available for manipulating
typical scrolling or file-backed output. These functions output directly and
immediately to the provided `FILE*`, and `notcurses_render()` is neither
supported nor necessary for such an instance. Use `notcurses_directmode()` to
create a direct mode context:

```c
struct ncdirect; // minimal state for a terminal

// Initialize a direct-mode notcurses context on the connected terminal at 'fp'.
// 'fp' must be a tty. You'll usually want stdout. Direct mode supportes a
// limited subset of notcurses routines which directly affect 'fp', and neither
// supports nor requires notcurses_render(). This can be used to add color and
// styling to text in the standard output paradigm. Returns NULL on error,
// including any failure initializing terminfo.
struct ncdirect* notcurses_directmode(const char* termtype, FILE* fp);

// Release 'nc' and any associated resources. 0 on success, non-0 on failure.
int ncdirect_stop(struct ncdirect* nc);
```

This context must be destroyed using `ncdirect_stop()`. The following functions
are available for direct mode:

```c
int ncdirect_fg(struct ncdirect* nc, unsigned rgb);
int ncdirect_bg(struct ncdirect* nc, unsigned rgb);

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

// Get the current number of columns/rows.
int ncdirect_dim_x(const struct ncdirect* nc);
int ncdirect_dim_y(const struct ncdirect* nc);

int ncdirect_fg_default(struct ncdirect* nc);
int ncdirect_bg_default(struct ncdirect* nc);
int ncdirect_styles_set(struct ncdirect* n, unsigned stylebits);
int ncdirect_styles_on(struct ncdirect* n, unsigned stylebits);
int ncdirect_styles_off(struct ncdirect* n, unsigned stylebits);
int ncdirect_clear(struct ncdirect* nc); // clear the screen

// Move the cursor in direct mode. -1 to retain current location on that axis.
int ncdirect_cursor_move_yx(struct ncdirect* n, int y, int x);
```

### Alignment

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

// Return the column at which 'c' cols ought start in order to be aligned
// according to 'align' within ncplane 'n'. Returns INT_MAX on invalid 'align'.
// Undefined behavior on negative 'c'.
static inline int
ncplane_align(const struct ncplane* n, ncalign_e align, int c){
  if(align == NCALIGN_LEFT){
    return 0;
  }
  int cols;
  ncplane_dim_yx(n, NULL, &cols);
  if(align == NCALIGN_CENTER){
    return (cols - c) / 2;
  }else if(align == NCALIGN_RIGHT){
    return cols - c;
  }
  return INT_MAX;
}
```

### Input

Input can currently be taken only from `stdin`, but on the plus side, stdin
needn't be a terminal device (unlike the ttyfp `FILE*` passed to `notcurses_init()`).
Generalized input ought happen soon. There is only one input queue per `struct
notcurses`.

Like NCURSES, notcurses will watch for escape sequences, check them against the
terminfo database, and return them as special keys (we hijack the Private Use
Area for special keys, specifically Supplementary Private Use Area B (u100000
through u10ffffd). Unlike NCURSES, the fundamental unit of input is the
UTF8-encoded Unicode codepoint. Note, however, that only one codepoint is
returned at a time (as opposed to an entire EGC).

It is generally possible for a false positive to occur, wherein keypresses
intended to be distinct are combined into an escape sequence. False negatives
where an intended escape sequence are read as an ESC key followed by distinct
keystrokes are also possible. NCURSES provides the `ESCDELAY` variable to
control timing. notcurses brooks no delay; all characters of an escape sequence
must be readable without delay for it to be interpreted as such.

```c
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

// Special composed key definitions. These values are added to 0x100000.
#define NCKEY_INVALID suppuabize(0)
#define NCKEY_RESIZE  suppuabize(1) // generated internally in response to SIGWINCH
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
// ... up to 100 function keys, egads
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
// ... up to 11 mouse buttons
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
  uint64_t seqnum; // input event number
} ncinput;

// See ppoll(2) for more detail. Provide a NULL 'ts' to block at length, a 'ts'
// of 0 for non-blocking operation, and otherwise a timespec to bound blocking.
// Signals in sigmask (less several we handle internally) will be atomically
// masked and unmasked per ppoll(2). It should generally contain all signals.
// Returns a single Unicode code point, or (char32_t)-1 on error. 'sigmask' may
// be NULL. Returns 0 on a timeout. If an event is processed, the return value
// is the 'id' field from that event. 'ni' may be NULL.
char32_t notcurses_getc(struct notcurses* n, const struct timespec* ts,
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
```

### Mice

notcurses supports mice, though only through brokers such as X or
[GPM](https://www.nico.schottelius.org/software/gpm/). It does not speak
directly to hardware. Mouse events must be explicitly enabled with a
successful call to `notcurses_mouse_enable()`, and can later be disabled.

```c
// Enable the mouse in "button-event tracking" mode with focus detection and
// UTF8-style extended coordinates. On failure, -1 is returned. On success, 0
// is returned, and mouse events will be published to notcurses_getc().
int notcurses_mouse_enable(struct notcurses* n);

// Disable mouse events. Any events in the input queue can still be delivered.
int notcurses_mouse_disable(struct notcurses* n);

// Was the provided mouse event 'ni' within the bounds of the ncplane 'n'? Note
// that this doesn't necessarily mean the event affected 'n'; there could be a
// plane above it, this plane could be transparent, etc.
bool ncplane_mouseevent_p(const struct ncplane* n, const struct ncinput *ni);
```

"Button-event tracking mode" implies the ability to detect mouse button
presses, and also mouse movement while holding down a mouse button (i.e. to
effect drag-and-drop). Mouse events are returned via the `NCKEY_MOUSE*` values,
with coordinate information in the `ncinput` struct.

### Planes

Fundamental to notcurses is a z-buffer of rectilinear virtual screens, known
as `ncplane`s. An `ncplane` can be larger than the physical screen, or smaller,
or the same size; it can be entirely contained within the physical screen, or
overlap in part, or lie wholly beyond the boundaries, never to be rendered.
In addition to its framebuffer--a rectilinear matrix of cells
(see [Cells](#cells))--an `ncplane` is defined by:

* a base cell, used for any cell on the plane without a glyph,
* the egcpool backing its cells,
* a current cursor location,
* a current style, foreground channel, and background channel,
* its geometry,
* a configured user curry (a `void*`),
* its position relative to the visible plane, and
* its z-index.

If opaque, a `cell` on a higher `ncplane` completely obstructs a corresponding
`cell` from a lower `ncplane` from being seen. An `ncplane` corresponds loosely
to an [NCURSES Panel](https://invisible-island.net/ncurses/ncurses-intro.html#panels),
but is the primary drawing surface of notcurses—there is no object
corresponding to a bare NCURSES `WINDOW`.

In addition to `ncplane_new()`, an `ncplane` can be created aligned relative
to an existing `ncplane` (including the standard plane) using `ncplane_aligned()`.
When an `ncplane` is no longer needed, free it with `ncplane_destroy()`. To
quickly reset the `ncplane`, use `ncplane_erase()`.

```c
// Create a new ncplane at the specified offset (relative to the standard plane)
// and the specified size. The number of rows and columns must both be positive.
// This plane is initially at the top of the z-buffer, as if ncplane_move_top()
// had been called on it. The void* 'opaque' can be retrieved (and reset) later.
struct ncplane* ncplane_new(struct notcurses* nc, int rows, int cols,
                            int yoff, int xoff, void* opaque);

// Create a new ncplane aligned relative to 'n'.
struct ncplane* ncplane_aligned(struct ncplane* n, int rows, int cols,
                                int yoff, ncalign_e align, void* opaque);

// Duplicate an existing ncplane. The new plane will have the same geometry,
// will duplicate all content, and will start with the same rendering state.
// The new plane will be immediately above the old one on the z axis.
struct ncplane* ncplane_dup(struct ncplane* n, void* opaque);

// Destroy the specified ncplane. None of its contents will be visible after
// the next call to notcurses_render(). It is an error to attempt to destroy
// the standard plane.
int ncplane_destroy(struct ncplane* ncp);

// Erase every cell in the ncplane, resetting all attributes to normal, all
// colors to the default color, and all cells to undrawn. All cells associated
// with this ncplane are invalidated, and must not be used after the call,
// excluding the base cell.
void ncplane_erase(struct ncplane* n);
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

// Move this plane relative to the standard plane. It is an error to attempt to
// move the standard plane.
int ncplane_move_yx(struct ncplane* n, int y, int x);

// Get the origin of this ncplane relative to the standard plane.
void ncplane_yx(struct ncplane* n, int* restrict y, int* restrict x);

// Return the dimensions of this ncplane.
void ncplane_dim_yx(struct ncplane* n, int* restrict rows, int* restrict cols);

static inline int
ncplane_dim_y(const struct ncplane* n){
  int dimy;
  ncplane_dim_yx(n, &dimy, NULL);
  return dimy;
}

static inline int
ncplane_dim_x(const struct ncplane* n){
  int dimx;
  ncplane_dim_yx(n, NULL, &dimx);
  return dimx;
}

// provided a coordinate relative to the origin of 'src', map it to the same
// absolute coordinate relative to thte origin of 'dst'. either or both of 'y'
// and 'x' may be NULL.
void ncplane_translate(const struct ncplane* src, const struct ncplane* dst,
                       int* restrict y, int* restrict x);
```

If a given cell's glyph is zero, or its foreground channel is fully transparent,
it is considered to have no foreground. A _default_ cell can be chosen for the
`ncplane`, to be consulted in this case. If the base cell's glyph is likewise
zero (or its foreground channel fully transparent), the plane's foreground is
not rendered. Note that the base cell, like every other cell, has its own
foreground and background channels.

```c
// Set the specified style bits for the ncplane 'n', whether they're actively
// supported or not.
void ncplane_styles_set(struct ncplane* n, unsigned stylebits);

// Add the specified styles to the ncplane's existing spec.
void ncplane_styles_on(struct ncplane* n, unsigned stylebits);

// Remove the specified styles from the ncplane's existing spec.
void ncplane_styles_off(struct ncplane* n, unsigned stylebits);

// Return the current styling for this ncplane.
unsigned ncplane_styles(const struct ncplane* n);

// Set the ncplane's base cell to this cell. It will be used for purposes of
// rendering anywhere that the ncplane's gcluster is 0. Erasing the ncplane
// does not reset the base cell; this function must be called with a zero 'c'.
int ncplane_set_base_cell(struct ncplane* ncp, const cell* c);

// Set the ncplane's base cell to this cell. It will be used for purposes of
// rendering anywhere that the ncplane's gcluster is 0. Erasing the ncplane
// does not reset the base cell; this function must be called with an empty
// 'egc'. 'egc' must be a single extended grapheme cluster.
int ncplane_set_base(struct ncplane* ncp, uint64_t channels,
                     uint32_t attrword, const char* egc);

// Extract the ncplane's base cell into 'c'. The reference is invalidated if
// 'ncp' is destroyed.
int ncplane_base(struct ncplane* ncp, cell* c);
```

`ncplane`s are completely ordered along an imaginary z-axis. Newly-created
`ncplane`s are on the top of the stack. They can be freely reordered.

```c
// Splice ncplane 'n' out of the z-buffer, and reinsert it at the top or bottom.
int ncplane_move_top(struct ncplane* n);
int ncplane_move_bottom(struct ncplane* n);

// Splice ncplane 'n' out of the z-buffer, and reinsert it below 'below'.
int ncplane_move_below(struct ncplane* restrict n, struct ncplane* restrict below);

// Splice ncplane 'n' out of the z-buffer, and reinsert it above 'above'.
int ncplane_move_above(struct ncplane* restrict n, struct ncplane* restrict above);

// Return the ncplane below this one, or NULL if this is at the stack's bottom.
struct ncplane* ncplane_below(struct ncplane* n);
```

Each plane holds a user pointer which can be retrieved and set (or ignored). In
addition, the plane's virtual framebuffer can be accessed (note that this does
not necessarily reflect anything on the actual screen).

```c
// Retrieve the cell at the cursor location on the specified plane, returning
// it in 'c'. This copy is safe to use until the ncplane is destroyed/erased.
int ncplane_at_cursor(struct ncplane* n, cell* c);

// Retrieve the cell at the specified location on the specified plane, returning
// it in 'c'. This copy is safe to use until the ncplane is destroyed/erased.
int ncplane_at_yx(struct ncplane* n, int y, int x, cell* c);

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
// Returns -1 on error, including negative parameters, or ones exceeding the
// plane's dimensions.
int ncplane_cursor_move_yx(struct ncplane* n, int y, int x);

// Get the current position of the cursor within n. y and/or x may be NULL.
void ncplane_cursor_yx(struct ncplane* n, int* restrict y, int* restrict x);

// Replace the cell at the specified coordinates with the provided cell 'c',
// and advance the cursor by the width of the cell (but not past the end of the
// plane). On success, returns the number of columns the cursor was advanced.
// On failure, -1 is returned.
int ncplane_putc_yx(struct ncplane* n, int y, int x, const cell* c);

// Call ncplane_putc_yx() for the current cursor location.
static inline int
ncplane_putc(struct ncplane* n, const cell* c){
  return ncplane_putc_yx(n, -1, -1, c);
}

// Replace the cell at the specified coordinates with the provided 7-bit char
// 'c'. Advance the cursor by 1. On success, returns 1. On failure, returns -1.
// This works whether the underlying char is signed or unsigned.
int ncplane_putsimple_yx(struct ncplane* n, int y, int x, char c);

// Call ncplane_putsimple_yx() at the current cursor location.
static inline int
ncplane_putsimple(struct ncplane* n, char c){
  return ncplane_putsimple_yx(n, -1, -1, c);
}

// Replace the EGC underneath us, but retain the styling. The current styling
// of the plane will not be changed.
int ncplane_putsimple_stainable(struct ncplane* n, char c);

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
int ncplane_putegc_stainable(struct ncplane* n, const char* gclust, int* sbytes);

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
int ncplane_putwegc_stainable(struct ncplane* n, const wchar_t* gclust, int* sbytes);

// Write a series of EGCs to the current location, using the current style.
// They will be interpreted as a series of columns (according to the definition
// of ncplane_putc()). Advances the cursor by some positive number of cells
// (though not beyond the end of the plane); this number is returned on success.
// On error, a non-positive number is returned, indicating the number of cells
// which were written before the error.
int ncplane_putstr_yx(struct ncplane* n, int y, int x, const char* gclustarr);

static inline int
ncplane_putstr(struct ncplane* n, const char* gclustarr){
  return ncplane_putstr_yx(n, -1, -1, gclustarr);
}

int ncplane_putstr_aligned(struct ncplane* n, int y, ncalign_e align, const char* s);

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

Lines and boxes can be drawn, interpolating their colors between their two
endpoints. For a line of a single color, be sure to specify the same channels
on both sides. Boxes allow fairly detailed specification of how they're drawn.

```c
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

int ncplane_box(struct ncplane* n, const cell* ul, const cell* ur,
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
```

Similarly, areas can be filled with a cell.

```c
// The specified coordinate must not currently have a glyph, or it is an error.
// Otherwise, that coordinate, and all cardinally-connected glyphless cells,
// will have 'c' written to them.
int ncplane_polyfill_yx(struct ncplane* n, int y, int x, const cell* c);

// Draw a gradient with its upper-left corner at the current cursor position,
// stopping at 'ystop'x'xstop'. The glyph composed of 'egc' and 'attrword' is
// used for all cells. The channels specified by 'ul', 'ur', 'll', and 'lr'
// are composed into foreground and background gradients. To do a vertical
// gradient, 'ul' ought equal 'ur' and 'll' ought equal 'lr'. To do a
// horizontal gradient, 'ul' ought equal 'll' and 'ur' ought equal 'ul'. To
// color everything the same, all four channels should be equivalent.
int ncplane_gradient(struct ncplane* n, const char* egc, uint32_t attrword,
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

// Set the given style throughout the specified region, keepying content and
// channels otherwise unchanged.
int ncplane_format(struct ncplane* n, int ystop, int xstop, uint32_t attrword);

// Set the given channels throughout the specified region, keepying content and
// attributes otherwise unchanged.
int ncplane_stain(struct ncplane* n, int ystop, int xstop,
                  uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr);
```

My 14 year-old self would never forgive me if we didn't have sweet palette fades.

```c
// Called for each delta performed in a fade on ncp. If anything but 0 is returned,
// the fading operation ceases immediately, and that value is propagated out. If provided
// and not NULL, the faders will not themselves call notcurses_render().
typedef int (*fadecb)(struct notcurses* nc, struct ncplane* ncp, void* curry);

// Fade the ncplane out over the provided time, calling the specified function
// when done. Requires a terminal which supports direct color, or at least
// palette modification (if the terminal uses a palette, our ability to fade
// planes is limited, and affected by the complexity of the rest of the screen).
// It is not safe to resize or destroy the plane during the fadeout FIXME.
int ncplane_fadeout(struct ncplane* n, const struct timespec* ts, fadecb fader, void* curry);

// Fade the ncplane in over the specified time. Load the ncplane with the
// target cells without rendering, then call this function. When it's done, the
// ncplane will have reached the target levels, starting from zeroes.
int ncplane_fadein(struct ncplane* n, const struct timespec* ts, fadecb fader, void* curry);

// Pulse the plane in and out until the callback returns non-zero, relying on
// the callback 'fader' to initiate rendering. 'ts' defines the half-period
// (i.e. the transition from black to full brightness, or back again). Proper
// use involves preparing (but not rendering) an ncplane, then calling
// ncplane_pulse(), which will fade in from black to the specified colors.
int ncplane_pulse(struct ncplane* n, const struct timespec* ts, fadecb fader, void* curry);
```

Finally, a raw stream of RGBA or BGRx data can be blitted directly to an ncplane:

```c
// Blit a flat array 'data' of BGRx 32-bit values to the ncplane 'nc', offset
// from the upper left by 'placey' and 'placex'. Each row ought occupy
// 'linesize' bytes (this might be greater than lenx * 4 due to padding). A
// subregion of the input can be specified with 'begy'x'begx' and 'leny'x'lenx'.
int ncblit_bgrx(struct ncplane* nc, int placey, int placex, int linesize,
                const unsigned char* data, int begy, int begx,
                int leny, int lenx);

// Blit a flat array 'data' of RGBA 32-bit values to the ncplane 'nc', offset
// from the upper left by 'placey' and 'placex'. Each row ought occupy
// 'linesize' bytes (this might be greater than lenx * 4 due to padding). A
// subregion of the input can be specified with 'begy'x'begx' and 'leny'x'lenx'.
int ncblit_rgba(struct ncplane* nc, int placey, int placex, int linesize,
                const unsigned char* data, int begy, int begx,
                int leny, int lenx);
```

#### Plane channels API

Helpers are provided to manipulate an `ncplane`'s `channels` member. They are
all implemented in terms of the lower-level [Channels API](#channels).

```c
// Get the current channels or attribute word for ncplane 'n'.
uint64_t ncplane_channels(const struct ncplane* n);
uint32_t ncplane_attr(const struct ncplane* n);

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

// Set the alpha parameters for ncplane 'n'.
int ncplane_set_fg_alpha(struct ncplane* n, int alpha);
int ncplane_set_bg_alpha(struct ncplane* n, int alpha);

// Extract 24 bits of foreground RGB from 'n', split into subcomponents.
static inline unsigned
ncplane_fg_rgb(const struct ncplane* n, unsigned* r, unsigned* g, unsigned*
  return channels_fg_rgb(ncplane_channels(n), r, g, b);
}

// Extract 24 bits of background RGB from 'n', split into subcomponents.
static inline unsigned
ncplane_bg_rgb(const struct ncplane* n, unsigned* r, unsigned* g, unsigned*
  return channels_bg_rgb(ncplane_channels(n), r, g, b);
}

// Set the current fore/background color using RGB specifications. If the
// terminal does not support directly-specified 3x8b cells (24-bit "Direct
// Color", indicated by the "RGB" terminfo capability), the provided values
// will be interpreted in some lossy fashion. None of r, g, or b may exceed 255.
// "HP-like" terminals require setting foreground and background at the same
// time using "color pairs"; notcurses will manage color pairs transparently.
int ncplane_set_fg_rgb(struct ncplane* n, int r, int g, int b);
int ncplane_set_bg_rgb(struct ncplane* n, int r, int g, int b);

// Same, but clipped to [0..255].
void ncplane_set_bg_rgb_clipped(struct ncplane* n, int r, int g, int b);
void ncplane_set_fg_rgb_clipped(struct ncplane* n, int r, int g, int b);

// Same, but with rgb assembled into a channel (i.e. lower 24 bits).
int ncplane_set_fg(struct ncplane* n, unsigned channel);
int ncplane_set_bg(struct ncplane* n, unsigned channel);

// Use the default color for the foreground/background.
void ncplane_set_fg_default(struct ncplane* n);
void ncplane_set_bg_default(struct ncplane* n);

int ncplane_set_fg_palindex(struct ncplane* n, int idx);
int ncplane_set_bg_palindex(struct ncplane* n, int idx);
```

### Cells

Unlike the `notcurses` or `ncplane` objects, the definition of `cell` is
available to the user. It is somewhat ironic, then, that the user typically
needn't (and shouldn't) use `cell`s directly. Use a `cell` when the EGC being
output is used several times. In this case, time otherwise spent running
`cell_load()` (which tokenizes and verifies EGCs) can be saved. It can also be
useful to use a `cell` when the same styling is used in a discontinuous manner.

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
```

`cell`s must be initialized with an initialization macro or `cell_init()`
before any other use. `cell_init()` and `CELL_TRIVIAL_INITIALIZER` both
simply zero out the `cell`.

```c
#define CELL_TRIVIAL_INITIALIZER { .gcluster = '\0', .attrword = 0, .channels = 0, }
#define CELL_SIMPLE_INITIALIZER(c) { .gcluster = (c), .attrword = 0, .channels = 0, }
#define CELL_INITIALIZER(c, a, chan) { .gcluster = (c), .attrword = (a), .channels = (chan), }

static inline void
cell_init(cell* c){
  memset(c, 0, sizeof(*c));
}
```

A `cell` has three fundamental elements:

* The EGC displayed at this coordinate, encoded in UTF-8. If the EGC is a
  single ASCII character (value less than 0x80), it is stored inline in
  the `cell`'s `gcluster` field. Otherwise, `gcluster`'s top 24 bits
  are a 128-biased offset into the associated `ncplane`'s egcpool. This
  implies that `cell`s are associated with `ncplane`s once prepared.
* The Curses-style attributes of the text.
* The 52 bits of foreground and background RGBA (2x8/8/8/2), plus a few flags.

The EGC should be loaded using `cell_load()`. Either a single NUL-terminated
EGC can be provided, or a string composed of multiple EGCs. In the latter case,
the first EGC from the string is loaded. Remember, backing storage for the EGC
is provided by the `ncplane` passed to `cell_load()`; if this `ncplane` is
destroyed (or even erased), the `cell` cannot safely be used. If you're done
using the `cell` before being done with the `ncplane`, call `cell_release()`
to free up the EGC resources.

```c
// Breaks the UTF-8 string in 'gcluster' down, setting up the cell 'c'. Returns
// the number of bytes copied out of 'gcluster', or -1 on failure. The styling
// of the cell is left untouched, but any resources are released.
int cell_load(struct ncplane* n, cell* c, const char* gcluster);

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
int cell_duplicate(struct ncplane* n, cell* targ, const cell* c);

// Release resources held by the cell 'c'.
void cell_release(struct ncplane* n, cell* c);

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
  c->attrword |= (stylebits & NCSTYLE_MASK;
}

// Remove the specified styles (in the LSBs) from the cell's existing spec.
static inline void
cell_styles_off(cell* c, unsigned stylebits){
  c->attrword &= ~(stylebits & NCSTYLE_MASK);
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

// return a pointer to the NUL-terminated EGC referenced by 'c'. this pointer
// is invalidated by any further operation on the plane 'n', so...watch out!
const char* cell_extended_gcluster(const struct ncplane* n, const cell* c);

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
cells_double_box(struct ncplane* n, uint32_t attr, uint64_t channels,
                 cell* ul, cell* ur, cell* ll, cell* lr, cell* hl, cell* vl){
  return cells_load_box(n, attr, channels, ul, ur, ll, lr, hl, vl, "╔╗╚╝═║");
}
```

#### Cell channels API

Helpers are provided to manipulate a `cell`'s `channels` member. They are all
implemented in terms of the lower-level [Channels API](#channels).

```c
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

static inline int
cell_set_fg_alpha(cell* c, int alpha){
  return channels_set_fg_alpha(&c->channels, alpha);
}

static inline int
cell_set_bg_alpha(cell* c, int alpha){
  return channels_set_bg_alpha(&c->channels, alpha);
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

```

### Multimedia

Media decoding and scaling is handled by libAV from FFmpeg, resulting in a
`notcurses_visual` object. This object generates frames, each one corresponding
to a renderable scene on the associated `ncplane`. If notcurses is built without
FFMpeg support, these functions will all return error.

```c
// Open a visual (image or video), associating it with the specified ncplane.
// Returns NULL on any error, writing the AVError to 'averr'.
struct ncvisual* ncplane_visual_open(struct ncplane* nc, const char* file,
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
// as possible (given the visible screen), either maintaining aspect ratio
// (NCSCALE_SCALE) or abandoning it (NCSCALE_STRETCH).
struct ncvisual* ncvisual_open_plane(struct notcurses* nc, const char* file,
                                     int* averr, int y, int x, ncscale_e style);

// Destroy an ncvisual. Rendered elements will not be disrupted, but the visual
// can be neither decoded nor rendered any further.
void ncvisual_destroy(struct ncvisual* ncv);

// Render the decoded frame to the associated ncplane. The frame will be scaled
// to the size of the ncplane per the ncscale_e style. A subregion of the
// frame can be specified using 'begx', 'begy', 'lenx', and 'leny'. To render
// the rectangle formed by begy x begx and the lower-right corner, zero can be
// supplied to 'leny' and 'lenx'. Zero for all four values will thus render the
// entire visual. Negative values for any of the four parameters are an error.
// It is an error to specify any region beyond the boundaries of the frame.
int ncvisual_render(const struct ncvisual* ncv, int begy, int begx, int leny, int lenx);

// Return the plane to which this ncvisual is bound.
struct ncplane* ncvisual_plane(struct ncvisual* ncv);

// If a subtitle ought be displayed at this time, return a heap-allocated copy
// of the UTF8 text.
char* ncvisual_subtitle(const struct ncvisual* ncv);

// Called for each frame rendered from 'ncv'. If anything but 0 is returned,
// the streaming operation ceases immediately, and that value is propagated out.
typedef int (*streamcb)(struct notcurses* nc, struct ncvisual* ncv, void*);

// Shut up and display my frames! Provide as an argument to ncvisual_stream().
// If you'd like subtitles to be decoded, provide an ncplane as the curry. If the
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
int ncvisual_stream(struct notcurses* nc, struct ncvisual* ncv, int* averr,
                    float timescale, streamcb streamer, void* curry);
```

### Reels

ncreels are a complex UI abstraction offered by notcurses, derived from my
similar work in [outcurses](https://github.com/dankamongmen/ncreels#ncreels).

The ncreel is a UI abstraction supported by notcurses in which
dynamically-created and -destroyed toplevel entities (referred to as tablets)
are arranged in a torus (circular loop), allowing for infinite scrolling
(infinite scrolling can be disabled, resulting in a line segment rather than a
torus). This works naturally with keyboard navigation, mouse scrolling wheels,
and touchpads (including the capacitive touchscreens of modern cell phones).
The "panel" comes from the underlying ncurses objects (each entity corresponds
to a single panel) and the "reel" from slot machines. An ncreel initially has
no tablets; at any given time thereafter, it has zero or more tablets, and if
there is at least one tablet, one tablet is focused (and on-screen). If the
last tablet is removed, no tablet is focused. A tablet can support navigation
within the tablet, in which case there is an in-tablet focus for the focused
tablet, which can also move among elements within the tablet.

The ncreel object tracks the size of the screen, the size, number,
information depth, and order of tablets, and the focuses. It also draws the
optional borders around tablets and the optional border of the reel itself. It
knows nothing about the actual content of a tablet, save the number of lines it
occupies at each information depth. The typical control flow is that an
application receives events (from the UI or other event sources), and calls
into notcurses saying e.g. "Tablet 2 now has 40 valid lines of information".
notcurses might then call back into the application, asking it to draw some
line(s) from some tablet(s) at some particular coordinate of that tablet's
panel. Finally, control returns to the application, and the cycle starts anew.

Each tablet might be wholly, partially, or not on-screen. notcurses always
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
* Create or destroy new panels atop the ncreel
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
// An ncreel is a notcurses region devoted to displaying zero or more
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
struct ncreel* ncreel_create(struct ncplane* nc, const ncreel_options* popts, int efd);

// Returns the ncplane on which this ncreel lives.
struct ncplane* ncreel_plane(struct ncreel* pr);

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

// Add a new tablet to the provided ncreel, having the callback object
// opaque. Neither, either, or both of after and before may be specified. If
// neither is specified, the new tablet can be added anywhere on the reel. If
// one or the other is specified, the tablet will be added before or after the
// specified tablet. If both are specified, the tablet will be added to the
// resulting location, assuming it is valid (after->next == before->prev); if
// it is not valid, or there is any other error, NULL will be returned.
struct nctablet* ncreel_add(struct ncreel* pr, struct nctablet* after,
                            struct nctablet* before, tabletcb cb, void* opaque);

// Return the number of tablets.
int ncreel_tabletcount(const struct ncreel* pr);

// Indicate that the specified tablet has been updated in a way that would
// change its display. This will trigger some non-negative number of callbacks
// (though not in the caller's context).
int ncreel_touch(struct ncreel* pr, struct nctablet* t);

// Delete the tablet specified by t from the ncreel specified by pr. Returns
// -1 if the tablet cannot be found.
int ncreel_del(struct ncreel* pr, struct nctablet* t);

// Delete the active tablet. Returns -1 if there are no tablets.
int ncreel_del_focused(struct ncreel* pr);

// Move to the specified location within the containing WINDOW.
int ncreel_move(struct ncreel* pr, int x, int y);

// Redraw the ncreel in its entirety, for instance after
// clearing the screen due to external corruption, or a SIGWINCH.
int ncreel_redraw(struct ncreel* pr);

// Return the focused tablet, if any tablets are present. This is not a copy;
// be careful to use it only for the duration of a critical section.
struct nctablet* ncreel_focused(struct ncreel* pr);

// Change focus to the next tablet, if one exists
struct nctablet* ncreel_next(struct ncreel* pr);

// Change focus to the previous tablet, if one exists
struct nctablet* ncreel_prev(struct ncreel* pr);

// Destroy an ncreel allocated with ncreel_create(). Does not destroy the
// underlying WINDOW. Returns non-zero on failure.
int ncreel_destroy(struct ncreel* pr);

// Returns a pointer to a user pointer associated with this nctablet.
void* nctablet_userptr(struct nctablet* t);

// Access the ncplane associated with this tablet, if one exists.
struct ncplane* nctablet_ncplane(struct nctablet* t);
```

#### ncreel examples

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

### Selectors

The selector widget provides an ncplane with a title riser and a body section.
The body section is populated with options and descriptions, and supports
infinite scrolling up and down. The widgets looks like:

```
                              ╭──────────────────────────╮
                              │This is the primary header│
╭──────────────────────this is the secondary header──────╮
│                                                        │
│ option1   Long text #1                                 │
│ option2   Long text #2                                 │
│ option3   Long text #3                                 │
│ option4   Long text #4                                 │
│ option5   Long text #5                                 │
│ option6   Long text #6                                 │
│                                                        │
╰────────────────────────────────────here's the footer───╯
```

At all times, exactly one item is selected (unless there are no items). A
selector is created with `ncselector_create` and destroyed with
`ncselector_destroy`.

```c
struct selector_item {
  char* option;
  char* desc;
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

struct ncselector* ncselector_create(struct ncplane* n, int y, int x,
                                     const selector_options* opts);

int ncselector_additem(struct ncselector* n, const struct selector_item* item);
int ncselector_delitem(struct ncselector* n, const char* item);

// Return a reference to the selected option, or NULL if there are no items.
const char* ncselector_selected(const struct ncselector* n);

// Return a reference to the ncselector's underlying ncplane.
struct ncplane* ncselector_plane(struct ncselector* n);

// Move up or down in the list. A reference to the newly-selected item is
// returned, or NULL if there are no items in the list.
const char* ncselector_previtem(struct ncselector* n);
const char* ncselector_nextitem(struct ncselector* n);

// Offer the input to the ncselector. If it's relevant, this function returns
// true, and the input ought not be processed further. If it's irrelevant to
// the selector, false is returned. Relevant inputs include:
//  * a mouse click on an item
//  * a mouse scrollwheel event
//  * a mouse click on the scrolling arrows
//  * a mouse click outside of an unrolled menu (the menu is rolled up)
//  * up, down, pgup, or pgdown on an unrolled menu (navigates among items)
bool ncselector_offer_input(struct ncselector* n, const struct ncinput* nc);

// Destroy the ncselector. If 'item' is not NULL, the last selected option will
// be strdup()ed and assigned to '*item' (and must be free()d by the caller).
void ncselector_destroy(struct ncselector* n, char** item);
```

### Menus

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

Horizontal menu bars are supported, on the top or bottom rows of the screen. If
the menu bar is longer than the screen, it will be only partially visible, but
any unrolled section will be visible. Menus may be either visible or invisible
by default; set the `hiding` option to get an invisible menu. In the event of a
screen resize, menus will be automatically moved/resized.

```c
typedef struct menu_options {
  bool bottom;              // on the bottom row, as opposed to top row
  bool hiding;              // hide the menu when not being used
  struct {
    char* name;             // utf-8 c string
    struct {
      char* desc;           // utf-8 menu item, NULL for horizontal separator
      ncinput shortcut;     // shortcut, all should be distinct
    }* items;
    int itemcount;
  }* sections;              // array of menu sections
  int sectioncount;         // must be positive
  uint64_t headerchannels;  // styling for header
  uint64_t sectionchannels; // styling for sections
} menu_options;

struct ncmenu;

// Create a menu with the specified options. Menus are currently bound to an
// overall notcurses object (as opposed to a particular plane), and are
// implemented as ncplanes kept atop other ncplanes.
struct ncmenu* ncmenu_create(struct notcurses* nc, const menu_options* opts);

// Unroll the specified menu section, making the menu visible if it was
// invisible, and rolling up any menu section that is already unrolled.
int ncmenu_unroll(struct ncmenu* n, int sectionidx);

// Roll up any unrolled menu section, and hide the menu if using hiding.
int ncmenu_rollup(struct ncmenu* n);

// Return the selected item description, or NULL if no section is unrolled. If
// 'ni' is not NULL, and the selected item has a shortcut, 'ni' will be filled
// in with that shortcut--this can allow faster matching.
const char* ncmenu_selected(const struct ncmenu* n, struct ncinput* ni);

// Return the ncplane backing this ncmenu.
struct ncplane* ncmenu_plane(struct ncmenu* n);

// Offer the input to the ncmenu. If it's relevant, this function returns true,
// and the input ought not be processed further. If it's irrelevant to the
// menu, false is returned. Relevant inputs include:
//  * mouse movement over a hidden menu
//  * a mouse click on a menu section (the section is unrolled)
//  * a mouse click outside of an unrolled menu (the menu is rolled up)
//  * left or right on an unrolled menu (navigates among sections)
//  * up or down on an unrolled menu (navigates among items)
//  * escape on an unrolled menu (the menu is rolled up)
bool ncmenu_offer_input(struct ncmenu* n, const struct ncinput* nc);

// Destroy a menu created with ncmenu_create().
int ncmenu_destroy(struct ncmenu* n);
```

### Channels

A channel encodes 24 bits of RGB color, using 8 bits for each component. It
additionally provides 2 bits of alpha channel, a bit for selecting terminal
default colors, and a bit to indicate whether it describes a Wide East Asian
character. The remaining four bits are reserved. Typically two channels are
bound together in a 64-bit unsigned integer (`uint64_t`), with eight bits
currently going unused. There is such a double-channel in every `cell` and
`ncplane` object.

Usually, the higher-level `ncplane` and `cell` functionality ought be used. It
will sometimes be necessary, however, to muck with channels at their lowest
level. The channel API facilitates such muckery. All channel-related `ncplane`
and `cell` functionality is implemented in terms of this API.

```c
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
channel_rgb(unsigned channel, unsigned* r, unsigned* g, unsigned* b){
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
channel_alpha(unsigned channel){
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
channels_bchannel(uint64_t channels){
  return channels & 0xfffffffflu;
}

// Extract the 32-bit foreground channel from a channel pair.
static inline unsigned
channels_fchannel(uint64_t channels){
  return channels_bchannel(channels >> 32u);
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

// Set the r, g, and b channels for the background component of this 64-bit
// 'channels' variable, and mark it as not using the default color.
static inline int
channels_set_bg_rgb(uint64_t* channels, int r, int g, int b){
  unsigned channel = channels_bchannel(*channels);
  if(channel_set_rgb(&channel, r, g, b) < 0){
    return -1;
  }
  *channels = (*channels & 0xffffffff00000000llu) | channel;
  return 0;
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
  *channels = (*channels & 0xffffffff00000000llu) | channel;
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
  *channels = (*channels & 0xffffffff00000000llu) | channel;
  return 0;
}

// Is the foreground using the "default foreground color"?
static inline bool
channels_fg_default_p(uint64_t channels){
  return channel_default_p(channels_fchannel(channels));
}

// Is the background using the "default background color"? The "default
// background color" must generally be used to take advantage of
// terminal-effected transparency.
static inline bool
channels_bg_default_p(uint64_t channels){
  return channel_default_p(channels_bchannel(channels));
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
  *channels = (*channels & 0xffffffff00000000llu) | channel;
  return *channels;
}
```

## Included tools

Five binaries are built as part of notcurses:
* `notcurses-demo`: some demonstration code
* `notcurses-view`: renders visual media (images/videos)
* `notcurses-input`: decode and print keypresses
* `notcurses-planereels`: play around with ncreels
* `notcurses-tester`: unit testing

To run `notcurses-demo` from a checkout, provide the `tests/` directory via
the `-p` argument. Demos requiring data files will otherwise abort. The base
delay used in `notcurses-demo` can be changed with `-d`, accepting a
floating-point multiplier. Values less than 1 will speed up the demo, while
values greater than 1 will slow it down.

`notcurses-tester` expects `../tests/` to exist, and be populated with the
necessary data files. It can be run by itself, or via `make test`.

## Differences from NCURSES

The biggest difference, of course, is that notcurses is not an implementation
of X/Open (aka XSI) Curses, nor part of SUS4-2018.

The detailed differences between notcurses and NCURSES probably can't be fully
enumerated, and if they could, no one would want to read them. With that said,
some design decisions might surprise NCURSES programmers:

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
  mutate different `ncplane`s in different threads.
* NCURSES has thread-ignorant and thread-semi-safe versions, trace-enabled and
  traceless versions, and versions with and without support for wide characters.
  notcurses is one library: no tracing, UTF-8, thread safety.
* There is no `ESCDELAY` concept; notcurses expects that all bytes of a
  keyboard escape sequence arrive at the same time. This improves latency
  and simplifies the API.
* It is an error in NCURSES to print to the bottommost, rightmost coordinate of
  the screen when scrolling is disabled (because the cursor cannot be advanced).
  Failure to advance the cursor does not result in an error in notcurses (but
  attempting to print at the cursor when it has been advanced off the plane
  *does*).

### Features missing relative to NCURSES

This isn't "features currently missing", but rather "features I do not intend
to implement".

* There is no support for soft labels (`slk_init()`, etc.).
* There is no concept of subwindows which share memory with their parents.
* There is no tracing functionality ala `trace(3NCURSES)`. Superior external
  tracing solutions exist, such as `bpftrace`.

### Adapting NCURSES programs

Do you really want to do such a thing? NCURSES and the Curses API it implements
are far more portable and better-tested than notcurses is ever likely to be.
Will your program really benefit from notcurses's advanced features? If not,
it's probably best left as it is.

Otherwise, most NCURSES concepts have clear partners in notcurses. Any functions
making implicit use of `stdscr` ought be replaced with their explicit
equivalents. `stdscr` ought then be replaced with the result of
`notcurses_stdplane()` (the standard plane). `PANEL`s become `ncplane`s; the
Panels API is otherwise pretty close. Anything writing a bare character will
become a simple `cell`; multibyte or wide characters become complex `cell`s.
Color no longer uses "color pairs". You can easily enough hack together a
simple table mapping your colors to RGB values, and color pairs to foreground
and background indices into said table. That'll work for the duration of a
porting effort, certainly.

I have adapted two large (~5k lines of C UI code each) programs from NCURSES to
notcurses, and found it a fairly painless process. It was helpful to introduce
a shim layer, e.g. `compat_mvwprintw` for NCURSES's `mvwprintw`:

```c
static int
compat_mvwprintw(struct ncplane* nc, int y, int x, const char* fmt, ...){
  va_list va;
  va_start(va, fmt);
  if(ncplane_vprintf_yx(nc, y, x, fmt, va) < 0){
    va_end(va);
    return ERR;
  }
  va_end(va);
  return OK;
}
```

These are pretty obvious, implementation-wise.

## Environment notes

* If your terminal has an option about default interpretation of "ambiguous-width
  characters" (this is actually a technical term from Unicode), ensure it is
  set to **Wide**, not narrow. If that doesn't work, ensure it is set to
  **Narrow**, heh.

* If you can disable BiDi in your terminal, do so while running notcurses
  applications, until I have that handled better. notcurses doesn't recognize
  the BiDi state machine transitions, and thus merrily continues writing
  left-to-right. Likewise, ultra-wide glyphs will have interesting effects.
  ﷽!

* The unit tests assume dimensions of at least 80x24. They might work in a
  smaller terminal. They might not. Don't file bugs on it.

### DirectColor detection

notcurses aims to use only information found in the terminal's terminfo entry to detect capabilities, DirectColor
being one of them. Support for this is indicated by terminfo having a flag, added in NCURSES 6.1, named `RGB` set
to `true`. However, as of today there are few and far between terminfo entries which have the capability in their
database entry and so DirectColor won't be used in most cases. Terminal emulators have had for years a kludge to
work around this limitation of terminfo in the form of the `COLORTERM` environment variable which, if set to either
`truecolor` or `24bit` does the job of indicating the capability of sending the escapes 48 and 38 together with a
tripartite RGB (0 ≤ c ≤ 255 for all three components) to specify fore- and background colors.
Checking for `COLORTERM` admittedly goes against the goal stated at the top of this section but, for all practical
purposes, makes the detection work quite well **today**.

### Fonts

Fonts end up being a whole thing, little of which is pleasant. I'll write this
up someday **FIXME**.

### FAQs

* *Q:* Why didn't you just use Sixel?
* *A:* Many terminal emulators don't support Sixel. Sixel doesn't work well
       with mouse selection. With that said, I do intend to support Sixel soon,
       as a backend, when available, for certain types of drawing.

* *Q:* I'm not seeing `NCKEY_RESIZE` until I press some other key.
* *A:* You've almost certainly failed to mask `SIGWINCH` in some thread, and
       that thread is receiving the signal instead of the thread which called
       `notcurses_getc_blocking()`. As a result, the `poll()` is not
       interrupted. Call `pthread_sigmask()` before spawning any threads.

* *Q:* One of the demos claimed to spend more than 100% of its runtime rendering. Do you know how to count?
* *A:* Runtime is wall clock time. A multithreaded demo can spend more than the wall-clock time rendering if the threads contend.

* *Q:* Using the C++ wrapper, how can I ensure that the `NotCurses` destructor is run when I return from `main()`?
* *A:* As noted in the [C++ FAQ](https://isocpp.org/wiki/faq/dtors#artificial-block-to-control-lifetimes), wrap it in an artificial scope (this assumes your `NotCurses` is scoped to `main()`).

* *Q:* How do I hide a plane I want to make visible later?
* *A:* Either move it above and to the left of the screen (preventing resizes from making it visible), or place it underneath another (opaque) plane.

* *Q:* Why isn't there an `ncplane_box_yx()`? Do you hate orthogonality, you dullard?
* *A:* `ncplane_box()` and friends already have far too many arguments, you monster.

## Supplemental material

### Useful links

* [BiDi in Terminal Emulators](https://terminal-wg.pages.freedesktop.org/bidi/)
* [The Xterm FAQ](https://invisible-island.net/xterm/xterm.faq.html)
  * [XTerm Control Sequences](https://invisible-island.net/xterm/ctlseqs/ctlseqs.pdf)
* [The NCURSES FAQ](https://invisible-island.net/ncurses/ncurses.faq.html)
* [ECMA-35 Character Code Structure and Extension Techniques](https://www.ecma-international.org/publications/standards/Ecma-035.htm) (ISO/IEC 2022)
* [ECMA-43 8-bit Coded Character Set Structure and Rules](https://www.ecma-international.org/publications/standards/Ecma-043.htm)
* [ECMA-48 Control Functions for Coded Character Sets](https://www.ecma-international.org/publications/standards/Ecma-048.htm) (ISO/IEC 6429)
* [Unicode 12.1 Full Emoji List](https://unicode.org/emoji/charts/full-emoji-list.html)
* [Unicode Standard Annex #29 Text Segmentation](http://www.unicode.org/reports/tr29)
* [Unicode Standard Annex #15 Normalization Forms](https://unicode.org/reports/tr15/)
* [The TTY demystified](http://www.linusakesson.net/programming/tty/)
* [Dark Corners of Unicode](https://eev.ee/blog/2015/09/12/dark-corners-of-unicode/)
* [UTF-8 Decoder Capability and Stress Test](https://www.cl.cam.ac.uk/~mgk25/ucs/examples/UTF-8-test.txt)

#### Useful man pages
* Linux: [console_codes(4)](http://man7.org/linux/man-pages/man4/console_codes.4.html)
* Linux: [termios(3)](http://man7.org/linux/man-pages/man3/termios.3.html)
* Linux: [ioctl_tty(2)](http://man7.org/linux/man-pages/man2/ioctl_tty.2.html)
* Linux: [ioctl_console(2)](http://man7.org/linux/man-pages/man2/ioctl_console.2.html)
* Portable: [terminfo(5)](http://man7.org/linux/man-pages/man5/terminfo.5.html)
* Portable: [user_caps(5)](http://man7.org/linux/man-pages/man5/user_caps.5.html)

### Other TUI libraries of note

* [tui-rs](https://github.com/fdehau/tui-rs) (Rust)
* [blessed-contrib](https://github.com/yaronn/blessed-contrib) (Javascript)
* [FINAL CUT](https://github.com/gansm/finalcut) (C++)

### History

* 2020-02-17: notcurses [1.2.0 "check the résumé, my record's impeccable"](https://github.com/dankamongmen/notcurses/releases/tag/v1.2.0).
* 2019-01-19: notcurses [1.1.0 "all the hustlas they love it just to see one of us make it"](https://github.com/dankamongmen/notcurses/releases/tag/v1.1.0).
    Much better video support, pulsing planes, palette256.
* 2019-01-04: notcurses [1.0.0 "track team, crack fiend, dying to geek"](https://github.com/dankamongmen/notcurses/releases/tag/v1.0.0)
    is released, six days ahead of schedule. 147 issues closed. 702 commits.
* 2019-12-18: notcurses [0.9.0 "You dig in! You dig out! You get out!"](https://github.com/dankamongmen/notcurses/releases/tag/v0.9.0),
    and also the first contributor besides myself (@grendello). Last major
    pre-GA release.
* 2019-12-05: notcurses [0.4.0 "TRAP MUSIC ALL NIGHT LONG"](https://github.com/dankamongmen/notcurses/releases/tag/v0.4.0),
    the first generally usable notcurses. I prepare a [demo](https://www.youtube.com/watch?v=eEv2YRyiEVM),
    and release it on YouTube.
* November 2019: I begin work on [Outcurses](https://github.com/dankamongmen/ncreels).
    Outcurses is a collection of routines atop NCURSES, including ncreels.
    I study the history of NCURSES, primarily using Thomas E. Dickey's FAQ and
    the mailing list archives.
    * 2019-11-14: I file [Outcurses issue #56](https://github.com/dankamongmen/ncreels/issues/56)
      regarding use of DirectColor in outcurses. This is partially inspired by
      Lexi Summer Hale's essay [everything you ever wanted to know about terminals](http://xn--rpa.cc/irl/term.html).
      I get into contact with Thomas E. Dickey and confirm that what I'm hoping
      to do doesn't really fit in with the codified Curses API.
    * 2019-11-16: I make the [first commit](https://github.com/dankamongmen/notcurses/commit/635d7039d79e4f94ba645e8cb601e3a6d82a6b30)
      to notcurses.
* September 2019: I extracted fade routines from Growlight and Omphalos, and
    offered them to NCURSES as extensions. They are not accepted, which is
    understandable. I mention that I intend to extract ncreels, and offer to
    include them in the CDK (Curses Development Kit). [Growlight issue #43](https://github.com/dankamongmen/growlight/issues/43)
    is created regarding this extraction. A few minor patches go into NCURSES.
* 2011, 2013: I develop [Growlight](https://github.com/dankamongmen/growlight)
    and [Omphalos](https://github.com/dankamongmen/omphalos), complicated TUIs
    making extensive use of NCURSES.

### Thanks

* Notcurses could never be what it is without decades of tireless, likely
    thankless work by Thomas E. Dickey on NCURSES. His FAQ is a model of
    engineering history. He exemplifies documentation excellence and
    conservative, thoughtful stewardship. The free software community owes
    Mr. Dickey a great debt.
* Robert Edmonds provided tremendous assistance Debianizing the package.
* Justine Tunney, one of my first friends at Google NYC, was always present
    with support, and pointed out the useful memstream functionality of
    POSIX, eliminating the need for me to cons up something similar.
* I one night read the entirety of Lexi Summer Hale's [essays](http://xn--rpa.cc/irl/index.html),
    and woke up intending to write notcurses.
* NES art was lifted from [The Spriters Resource](https://www.spriters-resource.com/nes/)
    and [NES Sprite](http://nes-sprite.resampled.ru/), the kind of sites that
    make the Internet great. It probably violates any number of copyrights. C'est la vie.
* Mark Ferrari, master of the pixel, for no good reason allowed me to reproduce
    his incredible and groundbreaking color-cycling artwork. Thanks Mark!
* Finally, the [demoscene](https://en.wikipedia.org/wiki/Demoscene) and general
    l33t scene of the 90s and early twenty-first century endlessly inspired a
    young hax0r. There is great joy in computing; no one will drive us from
    this paradise Turing has created!

> “Our fine arts were developed, their types and uses were established, in times
very different from the present, by men whose power of action upon things was
insignificant in comparison with ours. But the amazing growth of our
techniques, the adaptability and precision they have attained, the ideas and
habits they are creating, make it a certainty that _profound changes are
impending in the ancient craft of the Beautiful_.” —Paul Valéry
