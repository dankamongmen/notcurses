% notcurses_plot(3)
% nick black <nickblack@linux.com>
% v1.2.5

# NAME

notcurses_plot - high level widget for plotting

# SYNOPSIS

**#include <notcurses.h>**

```c
typedef enum {
  NCPLOT_1x1, // full block               █
  NCPLOT_2x1, // full/lower blocks       █▄
  NCPLOT_1x1x4, // shaded full blocks  █▓▒░
  NCPLOT_4x1, // four vert levels      █▆▄▂
  NCPLOT_8x1, // eight vert levels █▇▆▅▄▃▂▁
} ncgridgeom_e;

typedef struct ncplot_options {
  // channels for the maximum and minimum levels.
  // lerp across the domain between these two.
  uint64_t maxchannel;
  uint64_t minchannel;
  // number of "pixels" per row x column
  ncgridgeom_e gridtype;
  // independent variable is a contiguous range
  uint64_t rangex;
  // dependent min and max. set both equal to 0 to
  // use domain autodiscovery.
  int64_t miny, maxy;
  bool labelaxisd;     // label dependent axis
  bool exponentialy;   // is dependent exponential?
  bool vertical_indep; // vertical independent variable
} ncplot_options;
```

**struct ncplot* ncplot_create(struct ncplane* n, const ncplot_options* opts);**

**struct ncplane* ncplot_plane(struct ncplot* n);**

**int ncplot_add_sample(struct ncplot* n, uint64_t x, int64_t y);**
**int ncplot_set_sample(struct ncplot* n, uint64_t x, int64_t y);**

**void ncplot_destroy(struct ncplot* n);**

# DESCRIPTION

# NOTES

Neither **exponentialy** not **vertical_indep** is yet implemented.

# RETURN VALUES

**ncplot_create** will return an error if **miny** equals **maxy**, but they
are non-zero. It will also return an error if **maxy** < **miny**. An invalid
**gridtype** will result in an error.

**ncplot_plane** returns the **ncplane** on which the plot is drawn. It cannot
fail.

# SEE ALSO

**notcurses(3)**,
**notcurses_ncplane(3)**
