% notcurses_capabilities(3)
% nick black <nickblack@linux.com>
% v2.3.0

# NAME

notcurses_capabilities - runtime capability detection

# SYNOPSIS

**#include <notcurses/notcurses.h>**

**unsigned notcurses_supported_styles(const struct notcurses* ***nc***);**

**unsigned notcurses_palette_size(const struct notcurses* ***nc***);**

**bool notcurses_cantruecolor(const struct notcurses* ***nc***);**

**bool notcurses_canfade(const struct notcurses* ***nc***);**

**bool notcurses_canchangecolor(const struct notcurses* ***nc***);**

**bool notcurses_canopen_images(const struct notcurses* ***nc***);**

**bool notcurses_canopen_videos(const struct notcurses* ***nc***);**

**bool notcurses_canutf8(const struct notcurses* ***nc***);**

**bool notcurses_canhalfblock(const struct notcurses* ***nc***);**

**bool notcurses_canquadrant(const struct notcurses* ***nc***);**

**bool notcurses_cansextant(const struct notcurses* ***nc***);**

**bool notcurses_canbraille(const struct notcurses* ***nc***);**

**int notcurses_check_pixel_support(struct notcurses* ***nc***);**

# DESCRIPTION

**notcurses_supported_styles** returns a bitmask representing those styles
for which the terminal advertises support.

**notcurses_palette_size** returns the size of the terminal's palette, used
for palette-indexed color. It will always return at least 1. This is
independent of RGB TrueColor support. No terminal is know to support
more than 256-indexed color.

**notcurses_cantruecolor** returns **true** if the terminal advertises
support for RGB TrueColor. Note that the RGB APIs of Notcurses can be used
even in the absence of terminal RGB support (Notcurses will map the RGB
values to the palette).

**notcurses_canfade** returns **true** if Notcurses has a means by which
it can effect fades.

**notcurses_canchangecolor** returns **true** if the terminal advertises
support for changing its palette entries.

**notcurses_canopen_images** returns **true** if Notcurses was built with
multimedia support.

**notcurses_canopen_video** returns **true** if Notcurses was built with
multimedia support capable of decoding videos.

**notcurses_canutf8** returns **true** if the configured locale uses
UTF-8 encoding, and the locale was successfully loaded.

**notcurses_cansextant** returns **true** if the heuristics suggest
that the terminal can properly render Unicode 13 sextants. Likewise,
**notcurses_canquadrant** and **notcurses_canhalfblock** return **true**
if the heuristics suggest that the terminal can properly render Unicode
quadrants and halfblocks, respectively. **notcurses_canbraille** returns
**true** if Unicode Braille is expected to work on the terminal. None of
these functions return **true** unless UTF-8 encoding is in use.

**notcurses_check_pixel_support** returns 1 if bitmap support (via any
mechanism) is detected; **NCBLIT_PIXEL** can be used after such a return.
It returns 0 a lack of bitmap support was confirmed, and -1 on error.

# NOTES

Some terminals advertise support for TrueColor, but then downsample or
otherwise degrade the provided RGB. In this case **notcurses_cantruecolor**
will return **true**, but the full spectrum will not be available.

# RETURN VALUES

# SEE ALSO

**notcurses(3)**,
**notcurses_init(3)**,
**notcurses_visual(3)**,
utf8(7)
