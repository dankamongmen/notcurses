% notcurses-info(1)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses-info - Display information about the terminal environment

# SYNOPSIS

**notcurses-info** [**-v**]

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
  * af: Foreground color can be set
  * ab: Background color can be set
  * sum: Synchronized Update Mode is supported
  * vpa: Cursor can be moved to an absolute vertical coordinate
  * hpa: Cursor can be moved to an absolute horizontal coordinate
  * sgr0: Styling can be reset via a single escape
  * op: Colors can be reset via a single escape
  * fgop: Foreground can be reset via a single escape
  * bgop: Background can be reset via a single escape
  * bce: The back-color-erase property is in play
  * rect: Rectangular editing is available

* The second line is more of the same:
  * bold: Boldface is available
  * ital: Italics are available
  * struck: Strikethrough is available
  * ucurl: Curled underlines are available
  * uline: Straight underlines are available
  * u7: Cursor position reporting
  * ccc: Palette can be reprogrammed
  * rgb: Colors can be specified as RGB wit eight bits/channel
  * el: Clearing can be performed through the end of the line

* The third line also covers UTF8 and decoding capabilities:
  * utf8: This is a UTF8 environment
  * 2x1: Upper- and lower-half blocks are available
  * 2x2: Quadrant blocks are available
  * 3x2: Sextant blocks are available
  * 4x2: Braille characters are available
  * img: Images can be decoded
  * vid: Video can be decoded
  * indn: Multiple-line scrolling is available
  * gpm: Connection is established to the GPM server
  * kbd: The Kitty keyboard protocol is in use

* The fourth line indicates the default background color, and whether that
  color is treated as transparent by the terminal (only **kitty** is known
  to do this), and the default foreground color. **pmouse** indicates
  whether pixel-precise mouse events are supported.

* The fifth line describes the available bitmap graphics. If Sixels are
  available, the maximum number of color registers and maximum Sixel
  geometry are reported. If Linux framebuffer graphics are available, that is
  reported. If the Kitty graphics protocol is detected, that will be reported
  with "rgba graphics are available"; if Kitty's animation support is also
  present, that will be reported with "rgba pixel animation support".

To the right of this material is the Notcurses homepage's URI, and the
Notcurses logo (the latter only if bitmap graphics are available).

The final eleven lines, only printed when in a UTF8 locale, show various
Unicode glyphs. The first four lines include the quadrant, sextant, and
box-drawing characters. The next four lines include the entire Braille set.
The following two lines include many of the Symbols for Legacy Computing
introduced in Unicode 13. The final line includes many emoji.

# OPTIONS

**-v**: Be verbose.

# NOTES

The behavior of **notcurses-info** (and indeed all of Notcurses) depends on
the **TERM** and **LANG** environment variables, the installed POSIX locales,
and the installed **terminfo(5)** databases.

# SEE ALSO

**tack(1)**,
**notcurses(3)**,
**terminfo(5)**
