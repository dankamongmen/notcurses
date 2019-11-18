# notcurses
cleanroom TUI library for modern terminal emulators. definitely not curses.

[![Build Status](https://drone.dsscaw.com:4443/api/badges/dankamongmen/notcurses/status.svg)](https://drone.dsscaw.com:4443/dankamongmen/notcurses)

* **What it is**: a library facilitating complex TUIs on modern terminal
    emulators, supporting vivid colors and wide characters to the
    maximum degree possible. Many tasks delegated to Curses can be
    achieved using notcurses (and vice versa).

* **What it is not**: a source-compatible X/Open Curses implementation, nor a
    replacement for NCURSES on existing systems, nor a widely-ported and -tested
    bedrock of Open Source, nor a battle-proven, veteran library.

notcurses abandons the X/Open Curses API bundled as part of the Single UNIX
Specification. The latter shows its age, seems not capable of making use of
terminal functionality such as unindexed 24-bit color ("DirectColor", not to be
confused with 8-bit indexed 24-bit color, aka "TrueColor"). For some necessary
background, consult Thomas E. Dickey's superb and authoritative [NCURSES
FAQ](https://invisible-island.net/ncurses/ncurses.faq.html#xterm_16MegaColors).
As such, it is not a drop-in Curses replacement. notcurses furthermore makes
use of the Terminfo library shipped with NCURSES. It is almost certainly less
portable, and definitely tested on less hardware.

notcurses opens up advanced functionality for the interactive user on
workstations, phones, laptops, and tablets, at the expense of e.g.
industrial and retail terminals (or even the Linux virtual console,
which offers only eight colors and limited glyphs).

Why use this non-standard library?

* A svelter design than that codified in X/Open. All exported identifiers
    are prefixed with `notcurses_` to avoid namespace pollution. Fewer
    identifiers overall. All APIs natively suport wide characters and
    24-bit RGB color.

* Visual features not directly available via NCURSES, including images,
    fonts, and video.

* Thread safety, and use in parallel programs, has been a design consideration
    from the beginning.

On the other hand, if you're targeting industrial or critical applications,
or wish to benefit from the time-tested reliability and portability of Curses,
you should by all means use that fine library.
