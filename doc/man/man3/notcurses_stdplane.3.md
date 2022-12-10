% notcurses_stdplane(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses_stdplane - acquire the standard ncplane

## SYNOPSIS

**#include <notcurses/notcurses.h>**

**struct ncplane* notcurses_stdplane(struct notcurses* ***nc***);**

**const struct ncplane* notcurses_stdplane_const(const struct notcurses* ***nc***);**

**static inline struct ncplane* notcurses_stddim_yx(struct notcurses* ***nc***, unsigned* restrict ***y***, unsigned* restrict ***x***);**

**static inline const struct ncplane* notcurses_stddim_yx_const(const struct notcurses* ***nc***, unsigned* restrict ***y***, unsigned* restrict ***x***);**

**int notcurses_enter_alternate_screen(struct notcurses* ***nc***);**

**int notcurses_leave_alternate_screen(struct notcurses* ***nc***);**

# DESCRIPTION

**notcurses_stdplane** returns a handle to the standard ncplane for the context
**nc**. The standard plane always exists, and is always the same size as the
screen. It is an error to call **ncplane_destroy(3)**, **ncplane_resize(3)**,
or **ncplane_move(3)** on the standard plane, but it can be freely moved
along the z-axis.

The standard plane's virtual cursor is initialized to its uppermost, leftmost
cell unless **NCOPTION_PRESERVE_CURSOR** is provided (see
**notcurses_init(3)**), in which case it is placed wherever the terminal's
real cursor was at startup.

**notcurses_stddim_yx** provides the same function, but also writes the
dimensions of the standard plane (and thus the real drawable area) into any
non-**NULL** parameters among **y** and **x**.

**notcurses_stdplane_const** allows a **const notcurses** to be safely used.

A resize event does not invalidate these references. They can be used until
**notcurses_stop(3)** is called on the associated **nc**.

**notcurses_enter_alternate_screen** and **notcurses_leave_alternate_screen**
only have meaning if the terminal implements the "alternate screen" via the
**smcup** and **rmcup** **terminfo(5)** capabilities (see the discussion of
**NCOPTION_NO_ALTERNATE_SCREEN** in **notcurses_init(3)**). If not currently
using the alternate screen, and assuming it is supported,
**notcurses_enter_alternate_screen** will switch to the alternate screen. This
redraws the contents, repositions the cursor, and usually makes scrollback
unavailable. The standard plane will have scrolling disabled upon a move to
the alternate plane.

# RETURN VALUES

**notcurses_enter_alternate_screen** will return -1 if the alternate screen
is unavailable. Both it and **notcurses_leave_alternate_screen** will return
-1 on an I/O failure.

Other functions cannot fail when provided a valid **struct notcurses**. They
will always return a valid pointer to the standard plane.

# SEE ALSO

**notcurses(3)**,
**notcurses_init(3)**,
**notcurses_plane(3)**,
**notcurses_stop(3)**,
**terminfo(5)**
