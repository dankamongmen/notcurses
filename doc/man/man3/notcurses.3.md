% notcurses(3)
% nick black <nickblack@linux.com>
% v1.1.5

# NAME

notcurses - TUI library for modern terminal emulators

# SYNOPSIS

**#include <notcurses.h>**

**-lnotcurses**

# DESCRIPTION

notcurses builds atop the **terminfo(5)** abstraction layer to provide
reasonably portable vivid character displays. It is an intellectual descendant
of **ncurses(3NCURSES)**, but goes beyond that library (and the X/Open Curses
API it implements). notcurses is capable of subregion fades, 24bpp DirectColor,
transparency, multimedia, and safe multithreaded use.

A program wishing to use notcurses will need to link it, ideally using the
output of **pkg-config --libs notcurses** (see **pkg-config(1)**). It is
advised to compile with the output of **pkg-config --cflags notcurses**. If
using CMake, a support file is provided, and can be accessed as **notcurses**
(see **cmake(1)**).

**notcurses_init(3)** can then be used to initialize a notcurses instance for a
given **FILE** (usually **stdout**), after calling **setlocale(3)** to prepare a
UTF-8 locale (see [Construction][]).

## Construction

Before calling into notcurses—and usually as one of the first calls of the
program—be sure to call **setlocale** with an appropriate UTF-8 **LC_ALL**
locale. It is usually appropriate to use **setlocale(LC_ALL, "")**, relying on
the user to properly set the **LANG** environment variable. notcurses will
refuse to start if **nl_langinfo(3)** doesn't indicate UTF-8.

**notcurses_init(3)** accepts a **struct notcurses_options** allowing fine-grained
control of notcurses behavior, including signal handlers, alternative screens,
and overriding the TERM environment variable. A **terminfo** entry appropriate
for the actual terminal must be available.

**notcurses_directmode(3)** makes available a very restricted subset of
notcurses functionality. This subset is intended to be interleaved with user-
generated output, and is limited to coloring and styling. Direct mode is
documented in **notcurses_directmode(3)**

## Output

All output is performed on **struct ncplane**s (see [Ncplanes][] below). Output
is not visible until explicitly rendered via **notcurses_render(3)**. It is safe to
output from multiple threads. Information on drawing functions is available at
**notcurses_output(3)**.

## Input

notcurses supports input from keyboards (via **stdin**) and pointing devices (via
a broker such as GPM, X, or Wayland). Input is delivered as 32-bit Unicode
code points. Synthesized events such as mouse button presses and arrow keys
are mapped into Unicode's
[Supplementary Private Use Area-B](https://unicode.org/charts/PDF/U1.0.10.pdf).
Information on input is available at **notcurses_input(3)**. The included tool
**notcurses-input(1)** can be used to test input decoding.

## Ncplanes

Following initialization, a single ncplane exists, the "standard plane" (see
**notcurses_stdplane(3)**). This plane cannot be destoyed nor manually resized,
and is always exactly as large as the screen. Further ncplanes can be created
with **ncplane_new(3)**. A total z-ordering always exists on the set of
ncplanes, and new ncplanes are placed at the top of the z-buffer. Ncplanes can
be larger, smaller, or the same size as the physical screen, and can be placed
anywhere relative to it (including entirely off-screen). Ncplanes are made up
of cells (see [Cells][] below). Information on ncplanes is available at
**notcurses_ncplane(3)**.

## Cells

Cells make up the framebuffers backing each ncplane, one cell per coordinate,
one extended grapheme cluster (see **unicode(7)**) per cell. A cell consists of
a gcluster (either a directly-encoded 7-bit ASCII character (see **ascii(7)**), or
a 25-bit index into the ncplane's egcpool), a set of attributes, and two
channels (one for the foreground, and one for the background—see
**notcurses_channels(3)**). Information on cells is available at
**notcurses_cell(3)**.

It is not usually necessary for users to interact directly with cells. They
are typically encountered when retrieving data from ncplanes or the rendered
scene (see e.g. **ncplane_at_yx(3)**), or to achieve peak performance when a
particular EGC is heavily reused within a plane.

## Widgets

A few high-level widgets are included, all built atop ncplanes:

* **notcurses_panelreel(3)** for hierarchal display of data
* **notcurses_selector(3)** for selecting one item from a set

## Threads

Notcurses explicitly supports use in multithreaded environments. Most functions
are safe to call concurrently, with exceptions including those which destroy
resources (**ncplane_destroy(3)**, **ncvisual_destroy(3)**, **notcurses_stop(3)**,
etc.). Multiple threads interacting with the same ncplane will block one another,
but threads operating on distinct ncplanes can run concurrently.
**notcurses_render(3)** blocks the majority of functions. Input functions only
block other input functions, not ncplane manipulation.

Since multiple threads can concurrently manipulate distinct ncplanes, peak
performance sometimes requires dividing the screen into several planes, and
manipulating them from multiple threads.

## Destruction

Before exiting, **notcurses_stop(3)** should be called. In addition to freeing up
resources, this is necessary to restore the terminal to a state useful for the
shell. By default, **notcurses_init(3)** installs signal handlers to catch all
signals which would normally terminate the process. The new handlers will try
to call **notcurses_stop(3)**, and then propagate the received signal to the
previous action.

# SEE ALSO

**ncurses(3NCURSES)**,
**notcurses-demo(1)**,
**notcurses-input(1)**,
**notcurses_cell(3)**, **notcurses_channels(3)**,
**notcurses_fade(3)**, **notcurses_init(3)**, **notcurses_input(3)**,
**notcurses_directmode(3)**,
**notcurses_lines(3)**,
**notcurses_ncplane(3)**,
**notcurses_ncvisual(3)**,
**notcurses_output(3)**,
**notcurses_palette(3)**,
**notcurses_panelreel(3)**,
**notcurses_render(3)**,
**notcurses_resize(3)**,
**notcurses_selector(3)**,
**notcurses_stats(3)**,
**notcurses_stdplane(3)**, **notcurses_stop(3)**,
**terminfo(5)**, **ascii(7)**, **utf-8(7)**,
**unicode(7)**
