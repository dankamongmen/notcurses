% notcurses_stdplane(3)
% nick black <nickblack@linux.com>
% v1.1.2

# NAME

notcurses_stdplane - acquire the standard ncplane

## SYNOPSIS

**#include <notcurses.h>**

**struct ncplane* notcurses_stdplane(struct notcurses* nc);**

**const struct ncplane* notcurses_const_stdplane(const struct notcurses* nc);**

# DESCRIPTION

**notcurses_stdplane** returns a handle to the standard ncplane for the context
**nc**. The standard plane always exists, and is always the same size as the
screen. It is an error to call **ncplane_destroy(3)**, **ncplane_resize(3)**,
or **ncplane_move(3)** on the standard plane, though it can be freely moved
along the z-axis.

A resize event does not invalidate this reference; it can be used until
**notcurses_stop(3)** is called on **nc**.

# RETURN VALUES

These functions cannot fail when provided a valid **struct notcurses**. They
will always return a valid pointer to the standard plane.

# SEE ALSO

**notcurses(3)**, **notcurses_ncplane(3)**, **notcurses_stop(3)**
