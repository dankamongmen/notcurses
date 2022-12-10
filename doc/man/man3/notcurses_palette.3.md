% notcurses_palette(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses_palette - operations on notcurses palettes

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
typedef struct ncpalette {
  // We store the RGB values as a regular ol' channel
  uint32_t chans[NCPALETTESIZE];
} ncpalette;
```

**bool notcurses_cantruecolor(const struct notcurses* ***nc***);**

**ncpalette* ncpalette_new(struct notcurses* ***nc***);**

**int ncpalette_use(struct notcurses* ***nc***, const ncpalette* ***p***);**

**int ncpalette_set_rgb8(ncpalette* ***p***, int ***idx***, int ***r***, int ***g***, int ***b***);**

**int ncpalette_set(ncpalette* ***p***, int ***idx***, unsigned ***rgb***);**

**int ncpalette_get(const ncpalette* ***p***, int ***idx***, uint32_t* ***palent***);**

**int ncpalette_get_rgb8(const ncpalette* ***p***, int ***idx***, int* restrict ***r***, int* restrict ***g***, int* restrict ***b***);**

**void ncpalette_free(ncpalette* ***p***);**

**bool notcurses_canchangecolors(const struct notcurses* ***nc***);**

# DESCRIPTION

Some terminals only support 256 colors, but allow the full palette to be
specified with arbitrary RGB colors. In all cases, it's more performant to use
indexed colors, since it's much less data to write to the terminal. If you can
limit yourself to 256 colors, that's probably for the best.

In addition, palette-based color allows for very fast color cycling effects,
since a single command can affect many cells on the screen.

# RETURN VALUES

Functions returning **int** return -1 on failure, or 0 on success. Failure is
always due to invalid inputs. Functions returning **bool** are predicates, and
return the requested value. Functions returning **unsigned** forms return the
input, modified as requested.

# SEE ALSO

**notcurses(3)**,
**notcurses_cell(3)**,
**notcurses_channels(3)**,
**notcurses_output(3)**,
**notcurses_plane(3)**
