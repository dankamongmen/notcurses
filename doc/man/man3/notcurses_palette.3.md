% notcurses_palette(3)
% nick black <nickblack@linux.com>
% v2.2.6

# NAME

notcurses_palette - operations on notcurses palettes

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
typedef struct palette256 {
  // We store the RGB values as a regular ol' channel
  uint32_t chans[256];
} palette256;
```

**bool notcurses_cantruecolor(const struct notcurses* ***nc***);**

**palette256* palette256_new(struct notcurses* ***nc***);**

**int palette256_use(struct notcurses* ***nc***, const palette256* ***p***);**

**int palette256_set_rgb8(palette256* ***p***, int ***idx***, int ***r***, int ***g***, int ***b***);**

**int palette256_set(palette256* ***p***, int ***idx***, unsigned ***rgb***);**

**int palette256_get_rgb8(const palette256* ***p***, int ***idx***, int* restrict ***r***, int* restrict ***g***, int* restrict ***b***);**

**void palette256_free(palette256* ***p***);**

**bool notcurses_canchangecolors(const struct notcurses* ***nc***);**

# DESCRIPTION

Some terminals only support 256 colors, but allow the full palette to be
specified with arbitrary RGB colors. In all cases, it's more performant to use
indexed colors, since it's much less data to write to the terminal. If you can
limit yourself to 256 colors, that's probably for the best.

In addition, palette-based color allows for very fast color cycling effects,
since a single command can affect many cells on the screen.

# RETURN VALUES

Functions returning `int` return -1 on failure, or 0 on success. Failure is
always due to invalid inputs. Functions returning `bool` are predicates, and
return the requested value. Functions returning `unsigned` forms return the
input, modified as requested.

# SEE ALSO

**notcurses(3)**,
**notcurses_cell(3)**,
**notcurses_channels(3)**,
**notcurses_output(3)**,
**notcurses_plane(3)**
