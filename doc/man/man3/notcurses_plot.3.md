% notcurses_plot(3)
% nick black <nickblack@linux.com>
% v1.2.4

# NAME

notcurses_plot - high level widget for selecting from a set

# SYNOPSIS

**#include <notcurses.h>**

```c
typedef struct ncplot_options {
  // styling of the maximum and minimum levels.
  // linear interpolation will be applied across
  // the domain between these two.
  uint64_t maxchannel;
  uint64_t minchannel;
} ncplot_options;
```

**struct ncplot* ncplot_create(struct ncplane* n, const ncplot_options* opts);**

**struct ncplane* ncplot_plane(struct ncplot* n);**

**void ncplot_destroy(struct ncplot* n, char\*\* item);**

# DESCRIPTION

# NOTES

# RETURN VALUES

**ncplot_plane** returns the **ncplane** on which the plot is drawn. It cannot
fail.

# SEE ALSO

**notcurses(3)**,
**notcurses_ncplane(3)**
