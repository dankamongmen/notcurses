% notcurses_plot(3)
% nick black <nickblack@linux.com>
% v1.3.4

# NAME

notcurses_plot - high level widget for plotting

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
typedef enum {
  NCPLOT_1x1,   // full block                █
  NCPLOT_2x1,   // full/(upper|left) blocks  ▄█
  NCPLOT_1x1x4, // shaded full blocks        ▓▒░█
  NCPLOT_2x2,   // quadrants                 ▗▐ ▖▄▟▌▙█
  NCPLOT_4x1,   // four vert/horz levels     █▆▄▂ / ▎▌▊█
  NCPLOT_4x2,   // 4x2-way braille      ⡀⡄⡆⡇⢀⣀⣄⣆⣇⢠⣠⣤⣦⣧⢰⣰⣴⣶⣷⢸⣸⣼⣾⣿
  NCPLOT_8x1,   // eight vert/horz levels    █▇▆▅▄▃▂▁ / ▏▎▍▌▋▊▉█
} ncgridgeom_e;

#define NCPLOT_OPTIONS_LABELTICKSD  0x0001
#define NCPLOT_OPTIONS_EXPONENTIALD 0x0002
#define NCPLOT_OPTIONS_VERTICALI    0x0004

typedef struct ncplot_options {
  // channels for the maximum and minimum levels.
  // lerp across the domain between these two.
  uint64_t maxchannel;
  uint64_t minchannel;
  // number of "pixels" per row x column
  ncgridgeom_e gridtype;
  // independent variable is a contiguous range
  int rangex;
  bool labelaxisd;     // label dependent axis
  bool exponentially;   // is dependent exponential?
  bool vertical_indep; // vertical independent variable
} ncplot_options;
```

**struct ncuplot* ncuplot_create(struct ncplane* n, const ncplot_options* opts, uint64_t miny, uint64_t maxy);**

**struct ncdplot* ncdplot_create(struct ncplane* n, const ncplot_options* opts, double miny, double maxy);**

**struct ncplane* ncuplot_plane(struct ncuplot* n);**

**struct ncplane* ncdplot_plane(struct ncdplot* n);**

**int ncuplot_add_sample(struct ncuplot* n, uint64_t x, uint64_t y);**

**int ncdplot_add_sample(struct ncdplot* n, uint64_t x, double y);**

**int ncuplot_set_sample(struct ncuplot* n, uint64_t x, uint64_t y);**

**int ncdplot_set_sample(struct ncdplot* n, uint64_t x, double y);**

**void ncuplot_destroy(struct ncuplot* n);**

**void ncdplot_destroy(struct ncdplot* n);**

# DESCRIPTION

These functions support histograms. The independent variable is always an
**uint64_t**. The samples are either **uint64_t**s (**ncuplot**) or **double**s
(**ncdplot**). Only a window over the samples is retained at any given time,
and this window can only move towards larger values of the independent
variable. The window is moved forward whenever an **x** larger than the current
window's maximum is supplied to **add_sample** or **set_sample**.

**add_sample** increments the current value corresponding to this **x** by
**y**. **set_sample** replaces the current value corresponding to this **x**.

If **rangex** is 0, or larger than the bound plane will support, it is capped
to the available space. The domain can either be specified as **miny** and
**maxy**, or domain autodetection can be invoked via setting both to 0. If the
domain is specified, samples outside the domain are an error, and do not
contribute to the plot. Supplying an **x** below the current window is an
error, and has no effect.

The different **ncgridgeom_e** values select from among available glyph sets:

* **NCPLOT_1x1**: Full block (█) or empty glyph
* **NCPLOT_2x1**: Adds the lower half block (▄) to **NCPLOT_1x1**.
* **NCPLOT_1x1x4**: Adds three shaded full blocks (▓▒░) to **NCPLOT_1x1**.
* **NCPLOT_2x2**: Adds left and right half blocks (▌▐) and quadrants (▖▗▟▙) to **NCPLOT_2x1**.
* **NCPLOT_4x1**: Adds ¼ and ¾ blocks (▂▆) to **NCPLOT_2x1**.
* **NCPLOT_4x2**: 4 rows and 2 columns of braille (⡀⡄⡆⡇⢀⣀⣄⣆⣇⢠⣠⣤⣦⣧⢰⣰⣴⣶⣷⢸⣸⣼⣾⣿).
* **NCPLOT_8x1**: Adds ⅛, ⅜, ⅝, and ⅞ blocks (▇▅▃▁) to **NCPLOT_4x1**.

More granular block glyphs means more resolution in your plots, but they can
be difficult to differentiate at small text sizes. Quadrants and braille allow 
for more resolution on the independent variable. It can be difficult to predict
how the braille glyphs will look in a given font.

The same **ncplot_options** struct can be used with all ncplot types. The
**flags** field is a bitmask composed of:

* **NCPLOT_OPTIONS_LABELTICKSD**: Label dependent axis ticks.
* **NCPLOT_OPTIONS_EXPONENTIALD**: Use an exponential dependent axis.
* **NCPLOT_OPTIONS_VERTICALI**: Vertical independent axis.

# NOTES

Neither **exponentially** not **vertical_indep** is yet implemented.

# RETURN VALUES

**create** will return an error if **miny** equals **maxy**, but they are
non-zero. It will also return an error if **maxy** < **miny**. An invalid
**gridtype** will result in an error.

**plane** returns the **ncplane** on which the plot is drawn. It cannot fail.

# SEE ALSO

**notcurses(3)**,
**notcurses_plane(3)**
