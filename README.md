# notcurses
cleanroom TUI library for modern terminal emulators. definitely not curses.

[![Build Status](https://drone.dsscaw.com:4443/api/badges/dankamongmen/notcurses/status.svg)](https://drone.dsscaw.com:4443/dankamongmen/notcurses)

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

## Basic use

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
// Get a human-readable string describing the running ncurses version.
const char* notcurses_version(void);

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

struct notcurses; // notcurses state for a given terminal

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

## Planes

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
