% notcurses(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses - TUI library for modern terminal emulators

# SYNOPSIS

**#include <notcurses/notcurses.h>** or
**#include <notcurses/notcurses-core.h>**

**-lnotcurses-core -lnotcurses** or **-lnotcurses-core**

# DESCRIPTION

Notcurses builds atop the **terminfo(5)** abstraction layer to provide
reasonably portable vivid character displays. It is an intellectual descendant
of **ncurses(3NCURSES)**, but goes beyond that library (and the X/Open Curses
API it implements).

A program wishing to use Notcurses will need to link it, ideally using the
output of **pkg-config --libs notcurses** (see **pkg-config(1)**). It is
advised to compile with the output of **pkg-config --cflags notcurses**. If
using CMake, a support file is provided, and can be accessed as **Notcurses**
(see **cmake(1)**). If multimedia capabilities are not needed, it is possible
to link against a minimal Notcurses using **pkg-config --libs notcurses-core**.

**notcurses_init(3)** can then be used to initialize a Notcurses instance for a
given **FILE*** (usually **stdout**, usually attached to a terminal).

## The alternate screen

Many terminals provide an "alternate screen" with its own contents, no
scrollback buffer, and no scrolling. Entering the alternate screen replaces
the current visible contents wholesale, as does returning to the regular
screen. Notcurses refers to the alternate screen's semantics as "TUI mode",
and the regular screen's semantics as "CLI mode". It is possible to swap
between the two modes at runtime using **notcurses_leave_alternate_screen(3)**
and **notcurses_enter_alternate_screen(3)**. Notcurses will enter TUI mode
by default on startup; to prevent this, use **NCOPTION_NO_ALTERNATE_SCREEN**
as described in **notcurses_init(3)**. On program exit, Notcurses will always
return to the regular screen, independent of the screen being used on
program start.

## Construction

Before calling into Notcurses—and usually as one of the first calls of the
program—be sure to call **setlocale** with an appropriate UTF-8 **LC_ALL**
locale. It is usually appropriate to use **setlocale(LC_ALL, "")**, relying on
the user to properly set the **LANG** environment variable. Notcurses will
refuse to start if **nl_langinfo(3)** doesn't indicate UTF-8 or ANSI_X3.4-1968
(aka US-ASCII). Be aware that capabilities are substantially reduced in ASCII.

**notcurses_init(3)** accepts a **struct notcurses_options** allowing fine-grained
control of Notcurses behavior, including signal handlers, alternative screens,
and overriding the TERM environment variable. A **terminfo** entry appropriate
for the actual terminal must be available.

**ncdirect_init(3)** makes available a restricted subset of Notcurses
functionality. This subset is intended to be interleaved with user-generated
output, and is limited to coloring and styling. Direct mode is documented in
**notcurses_direct(3)**.

Only one context can be active in a process at a time, whether direct mode
(**struct ncdirect**) or rendered mode (**struct notcurses**).

## Output

All output is performed on **struct ncplane**s (see [Ncplanes][] below). Output
is not visible until explicitly rendered via **notcurses_render(3)**.
Information on drawing functions is available at **notcurses_output(3)**.

## Input

Notcurses supports input from keyboards (via **stdin**) and pointing devices (via
a broker such as GPM, X, or Wayland). Input is delivered as 32-bit Unicode
code points. Synthesized events such as mouse button presses and arrow keys
are mapped into Unicode's
[Supplementary Private Use Area-B](https://unicode.org/charts/PDF/U100000.pdf).
Information on input is available at **notcurses_input(3)**. The included tool
**notcurses-input(1)** can be used to test input decoding.

## Ncpiles

A given **notcurses** context is made up of one or more piles. Piles provide
distinct rendering contexts: a thread can be rendering or mutating one pile,
while another thread concurrently renders or mutates another pile. A pile is
made up of planes, totally ordered on a z-axis. In addition to the z-ordering,
the planes of a pile are bound in a forest (a set of directed, acyclic graphs).
Those planes which are not bound to some other plane constitute the root planes
of a pile. A pile is destroyed when all its planes are destroyed, or moved to
other piles. Since the standard plane (see below) always exists, and cannot be
moved to another pile, one pile always exists, known as the standard pile.

Note that rasterizing a pile will replace all content within its margins.

For more information, see **notcurses_pile(3)**.

## Ncplanes

Following initialization, a single ncplane exists, the "standard plane" (see
**notcurses_stdplane(3)**). This plane cannot be destroyed nor manually resized,
and is always exactly as large as the screen (if run without a TTY, the "screen"
is assumed to be 80x24 cells). Further ncplanes can be created with
**ncplane_create(3)**. A total z-ordering always exists on the set of ncplanes,
and new ncplanes are placed at the top of the z-buffer. Ncplanes can be larger,
smaller, or the same size as the physical screen, and can be placed anywhere
relative to it (including entirely off-screen). Ncplanes are made up of
**nccell**s (see [NcCells][] below). Information on ncplanes is available at
**notcurses_plane(3)**.

## NcCells

**nccell**s make up the framebuffers backing each ncplane, one cell per
coordinate, one extended grapheme cluster (see **unicode(7)**) per cell. An
**nccell** consists of a gcluster (either a directly-encoded 7-bit ASCII
character (see **ascii(7)**), or a 25-bit index into the ncplane's egcpool), a
set of attributes, and two channels (one for the foreground, and one for the
background—see **notcurses_channels(3)**). Information on cells is available at
**notcurses_cell(3)**.

It is not usually necessary for users to interact directly with **nccell**s. They
are typically encountered when retrieving data from ncplanes or the rendered
scene (see e.g. **ncplane_at_yx(3)**), or to achieve peak performance when a
particular EGC is heavily reused within a plane.

## Visuals

Bitmaps can be loaded from disk or memory, or even synthesized from the
content of existing planes. These are stored in **ncvisual** objects, described
in **notcurses_visual(3)**. Visuals can be rendered to arbitrarily many
planes using a variety of blitters, varying in their aspect ratios and
resolution. If the terminal supports a pixel protocol such as Sixel or
Kitty, it is possible to render bitmaps at the pixel level (as opposed to
the cell level, using geometric glyphs). Otherwise, various Unicode-based
blitters are available to render bitmaps in the text paradigm.

## Widgets

A few high-level widgets are included, all built atop ncplanes:

* **notcurses_fds(3)** for dumping file descriptors/subprocesses to a plane
* **notcurses_menu(3)** for menu bars at the top or bottom of the screen
* **notcurses_multiselector(3)** for selecting one or more items from a set
* **notcurses_plot(3)** for drawing histograms and lineplots
* **notcurses_progbar(3)** for drawing progress bars
* **notcurses_reader(3)** for free-form input data
* **notcurses_reel(3)** for hierarchal display of block-based data
* **notcurses_tabbed(3)** for tabbed interfaces
* **notcurses_selector(3)** for selecting one item from a set
* **notcurses_tree(3)** for hierarchal display of line-based data

## Threads

Notcurses explicitly supports use in multithreaded environments, but it does
not itself perform any locking.

* Only one pile's rendered frame can be rasterized at a time, and it is **not**
  safe to concurrently render that pile. It is safe to rasterize a frame while
  rendering some other pile.
* It is otherwise always safe to operate concurrently on distinct piles.
* It is not safe to render a pile while concurrently modifying that pile.
* It is safe to output to multiple distinct ncplanes at the same time, even
  within the same pile.
* It is safe to output to ncplanes while adding or deleting some other ncplane.
* It is **not** safe for multiple threads to output to the same ncplane.
* It is **not** safe to add, delete, or reorder ncplanes within a single pile
  from multiple threads.

Only one thread may call **notcurses_get(3)** or any other input-related thread
at a time, but it **is** safe to call for input while another thread renders.

Since multiple threads can concurrently manipulate distinct ncplanes, peak
performance might require dividing the screen into several planes, and
manipulating them from multiple threads.

## Destruction

Before exiting, **notcurses_stop(3)** should be called. In addition to freeing up
resources, this is necessary to restore the terminal to a state useful for the
shell. By default, **notcurses_init(3)** installs signal handlers to catch all
signals which would normally terminate the process. The new handlers will try
to call **notcurses_stop(3)**, and then propagate the received signal to the
previous action.

# ENVIRONMENT VARIABLES

The **TERM** environment variable ought be correctly defined. It will be used
to index into the **terminfo(5)** database by way of **setupterm(3NCURSES)**.
Notcurses will additionally use **TERM_PROGRAM** to distinguish certain
terminals.

If the **COLORTERM** environment variable is defined as "**24bit**" or
"**truecolor**", Notcurses will assume the terminal capable of 24-bit RGB
color, even in the absence of "**RGB**" or "**Tc**" capabilities in
terminfo.

If the **NOTCURSES_LOGLEVEL** environment variable is defined as a number
between -1 and 8, inclusive, that will override any logging level specified
in the **struct notcurses_options** provided to **notcurses_init(3)**.

The **LOGNAME** environment variable, if defined, will be used for
**notcurses_accountname(3)**.

# NOTES

When using the C++ wrappers, **NCPP_EXCEPTIONS_PLEASE** can be defined in
order to turn most error returns into exceptions.

# SEE ALSO

**ncurses(3NCURSES)**,
**notcurses-demo(1)**,
**notcurses-input(1)**,
**notcurses_capabilities(3)**,
**notcurses_cell(3)**,
**notcurses_channels(3)**,
**notcurses_direct(3)**,
**notcurses_fade(3)**,
**notcurses_fds(3)**,
**notcurses_init(3)**,
**notcurses_input(3)**,
**notcurses_lines(3)**,
**notcurses_menu(3)**,
**notcurses_multiselector(3)**,
**notcurses_output(3)**,
**notcurses_palette(3)**,
**notcurses_pile(3)**,
**notcurses_plane(3)**,
**notcurses_plot(3)**,
**notcurses_progbar(3)**,
**notcurses_reader(3)**,
**notcurses_reel(3)**,
**notcurses_refresh(3)**,
**notcurses_render(3)**,
**notcurses_selector(3)**,
**notcurses_stats(3)**,
**notcurses_stdplane(3)**,
**notcurses_stop(3)**,
**notcurses_tabbed(3)**,
**notcurses_tree(3)**,
**notcurses_visual(3)**,
**terminfo(5)**, **ascii(7)**, **utf-8(7)**,
**unicode(7)**
