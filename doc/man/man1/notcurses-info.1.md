% notcurses-info(1)
% nick black <nickblack@linux.com>
% v2.3.7

# NAME

notcurses-info - Display information about the terminal environment

# SYNOPSIS

**notcurses-info**

# DESCRIPTION

**notcurses-info** prints all the information it knows about the current
terminal environment, including material loaded from **terminfo(5)** (based
on the **TERM** environment variable), replies from the terminal in
response to our queries, and built-in heuristics.

The Unicode half block, quadrant, sextant, and Braille glyphs are all included
in the output. If their appearance is irregular, it might behoove you to choose
another font.

The first five lines (the Notcurses initialization banner; see **notcurses_init(3)**)
provide:

* The Notcurses version and the derived terminal name, possibly including the
  terminal version. If Notcurses was able to unambiguously query the connected
  terminal, the internal name for the terminal will be shown. Otherwise, the
  terminal described by the **TERM** environment variable will be displayed.
  The terminal version is only acquired via query.
* The current cell geometry, cell-pixel geometry, and the derived window pixel
  geometry, the size of the **crender** structure, the number of colors in the
  palette, and whether RGB TrueColor is supported.
* The compiler name and version used to build Notcurses, the size of the
  **nccell** structure, and the endianness with which Notcurses was compiled.
  This buildtime endianness must match the runtime endianness.
* The version of libterminfo against which Notcurses was compiled.
* The version and name of the multimedia backend.

The next five lines describe properties of the terminal environment:

* The first line indicates that a given capability is present with a plus sign
  ('+'), or not present/detected with a minus sign ('-'):
  * rgb: Colors can be specified as RGB wit eight bits/channel
  * ccc: Palette can be reprogrammed
  * af: Foreground color can be set
  * ab: Background color can be set
  * sum: Synchronized Update Mode is supported
  * u7: Cursor position reporting
  * cup: Cursor can be arbitrarily placed
  * vpa: Cursor can be moved to an absolute vertical coordinate
  * hpa: Cursor can be moved to an absolute horizontal coordinate
  * sgr0: Styling can be reset via a single escape
  * op: Colors can be reset via a single escape
  * fgop: Foreground can be reset via a single escape
  * bgop: Background can be reset via a single escape

# OPTIONS

# NOTES

The behavior of **notcurses-info** (and indeed all of Notcurses) depends on
the **TERM** and **LANG** environment variables, the installed POSIX locales,
and the installed **terminfo(5)** databases.

# SEE ALSO

**tack(1)**,
**notcurses(3)**,
**terminfo(5)**
