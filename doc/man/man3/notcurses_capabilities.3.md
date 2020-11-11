% notcurses_capabilities(3)
% nick black <nickblack@linux.com>
% v2.0.4

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

**bool notcurses_cansixel(const struct notcurses* ***nc***);**

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

**notcurses_cansixel** returns **true** if the terminal advertises
support for Sixel.

# BUGS

Notcurses does not yet have Sixel support, and thus **notcurses_cansixel**
will always return **false**.

# NOTES

Some terminals advertise support for TrueColor, but then downsample or
otherwise degrade the provided RGB. In this case **notcurses_cantruecolor**
will return **true**, but the full spectrum will not be available.

# RETURN VALUES

# SEE ALSO

**notcurses(3)**,
utf8(7)
