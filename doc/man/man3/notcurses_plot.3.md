% notcurses_plot(3)
% nick black <nickblack@linux.com>
% v1.6.17

# NAME

notcurses_plot - high level widget for plotting

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
#define NCPLOT_OPTION_LABELTICKSD   0x0001u
#define NCPLOT_OPTION_EXPONENTIALD  0x0002u
#define NCPLOT_OPTION_VERTICALI     0x0004u
#define NCPLOT_OPTION_NODEGRADE     0x0008u
#define NCPLOT_OPTION_DETECTMAXONLY 0x0010u

typedef struct ncplot_options {
  // channels for the maximum and minimum levels.
  // lerp across the domain between these two.
  uint64_t maxchannels;
  uint64_t minchannels;
  // styling used for labels (NCPLOT_OPTION_LABELTICKSD)
  uint16_t legendstyle;
  // number of "pixels" per row x column
  ncblitter_e gridtype;
  // independent variable is a contiguous range
  int rangex;
  bool labelaxisd;     // label dependent axis
  bool exponentially;   // is dependent exponential?
  bool vertical_indep; // vertical independent variable
  const char* title;   // optional title
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

**int ncuplot_sample(const struct ncuplot* n, uint64_t x, uint64_t* y);**

**int ncdplot_sample(const struct ncdplot* n, uint64_t x, double* y);**

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

More granular block glyphs means more resolution in your plots, but they can
be difficult to differentiate at small text sizes. Quadrants and braille allow 
for more resolution on the independent variable. It can be difficult to predict
how the braille glyphs will look in a given font.

The same **ncplot_options** struct can be used with all ncplot types. The
**flags** field is a bitmask composed of:

* **NCPLOT_OPTION_LABELTICKSD**: Label dependent axis ticks
* **NCPLOT_OPTION_EXPONENTIALD**: Use an exponential dependent axis
* **NCPLOT_OPTION_VERTICALI**: Vertical independent axis
* **NCPLOT_OPTION_NODEGRADE**: Fail rather than degrade blitter
* **NCPLOT_OPTION_DETECTMAXONLY**: Detect only max domain, not min

If **NCPLOT_OPTION_LABELTICKSD** is supplied, the **legendstyle** field will be
used to style the labels. It is otherwise ignored.

# NOTES

Neither **exponentially** not **vertical_indep** is yet implemented.

# RETURN VALUES

**create** will return an error if **miny** equals **maxy**, but they are
non-zero. It will also return an error if **maxy** < **miny**. An invalid
**gridtype** will result in an error.

**plane** returns the **ncplane** on which the plot is drawn. It cannot fail.

# SEE ALSO

**notcurses(3)**,
**notcurses_plane(3)**,
**notcurses_visual(3)**
