% notcurses_stdplane(3)
% nick black <nickblack@linux.com>
% v2.0.11

# NAME

notcurses_stdplane - acquire the standard ncplane

## SYNOPSIS

**#include <notcurses/notcurses.h>**

**struct ncplane* notcurses_stdplane(struct notcurses* ***nc***);**

**const struct ncplane* notcurses_stdplane_const(const struct notcurses* ***nc***);**

**static inline struct ncplane* notcurses_stddim_yx(struct notcurses* ***nc***, int* restrict ***y***, int* restrict ***x***);**

**static inline const struct ncplane* notcurses_stddim_yx_const(const struct notcurses* ***nc***, int* restrict ***y***, int* restrict ***x***);**

# DESCRIPTION

**notcurses_stdplane** returns a handle to the standard ncplane for the context
**nc**. The standard plane always exists, and is always the same size as the
screen. It is an error to call **ncplane_destroy(3)**, **ncplane_resize(3)**,
or **ncplane_move(3)** on the standard plane, but it can be freely moved
along the z-axis.

**notcurses_stddim_yx** provides the same function, but also writes the
dimensions of the standard plane (and thus the real drawable area) into any
non-**NULL** parameters among **y** and **x**.

**notcurses_stdplane_const** allows a **const notcurses** to be safely used.

A resize event does not invalidate these references. They can be used until
**notcurses_stop(3)** is called on the associated **nc**.

# RETURN VALUES

These functions cannot fail when provided a valid **struct notcurses**. They
will always return a valid pointer to the standard plane.

# SEE ALSO

**notcurses(3)**,
**notcurses_plane(3)**,
**notcurses_stop(3)**
