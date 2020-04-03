% notcurses_plot(3)
% nick black <nickblack@linux.com>
% v1.2.4

# NAME

notcurses_plot - high level widget for selecting from a set

# SYNOPSIS

**#include <notcurses.h>**

```c
typedef struct ncplot_options {
  // channels for the maximum and minimum levels.
  // lerp across the domain between these two.
  uint64_t maxchannel;
  uint64_t minchannel;
  // independent variable is vertical, not horizontal
  bool vertical_indep;
  // number of "pixels" per row x column
  ncgridgeom_e gridtype;
  // independent variable is a contiguous range
  uint64_t rangex;
  // y axis min and max. set both equal to 0 for
  // use with range autodiscovery.
  int64_t miny, maxy;
  bool exponentialy;  // is y-axis exponential?
  bool discoverrange;
} ncplot_options;
```

**struct ncplot* ncplot_create(struct ncplane* n, const ncplot_options* opts);**

**struct ncplane* ncplot_plane(struct ncplot* n);**

**int ncplot_add_sample(struct ncplot* n, uint64_t x, int64_t y);**
**int ncplot_set_sample(struct ncplot* n, uint64_t x, int64_t y);**

**void ncplot_destroy(struct ncplot* n);**

# DESCRIPTION

# NOTES

# RETURN VALUES

**ncplot_create** will return an error if **discoverrange** is set, and either
**miny** or **maxy** are non-zero. It will also return an error if
**maxy** < **miny**.

**ncplot_plane** returns the **ncplane** on which the plot is drawn. It cannot
fail.

# SEE ALSO

**notcurses(3)**,
**notcurses_ncplane(3)**
