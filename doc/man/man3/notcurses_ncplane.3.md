% notcurses_ncplane(3)
% nick black <nickblack@linux.com>
% v1.0.0

# NAME

notcurses_ncplane-operations on notcurses planes

# SYNOPSIS

**#include <notcurses.h>**

**struct ncplane* ncplane_new(struct notcurses* nc, int rows, int cols, int yoff, int xoff, void* opaque);**

**struct ncplane* ncplane_aligned(struct ncplane* n, int rows, int cols, int yoff, ncalign_e align, void* opaque);**

**int ncplane_resize(struct ncplane* n, int keepy, int keepx, int keepleny, int keeplenx, int yoff, int xoff, int ylen, int xlen);**

**int ncplane_move_yx(struct ncplane* n, int y, int x);**

**void ncplane_yx(const struct ncplane* n, int* restrict y, int* restrict x);**

**int ncplane_set_default(struct ncplane* ncp, const cell* c);**

**int ncplane_default(struct ncplane* ncp, cell* c);**

**int ncplane_destroy(struct ncplane* ncp);**

**int ncplane_move_top(struct ncplane* n);**

**int ncplane_move_bottom(struct ncplane* n);**

**int ncplane_move_above(struct ncplane* n, struct ncplane* above);**

**int ncplane_move_below(struct ncplane* n, struct ncplane* below);**

**struct ncplane* ncplane_below(struct ncplane* n);**

**int ncplane_at_cursor(struct ncplane* n, cell* c);**

**int ncplane_at_yx(struct ncplane* n, int y, int x, cell* c);**

**void* ncplane_set_userptr(struct ncplane* n, void* opaque);**

**void* ncplane_userptr(struct ncplane* n);**

**const void* ncplane_userptr_const(const struct ncplane* n);**

**void ncplane_dim_yx(const struct ncplane* n, int* restrict rows, int* restrict cols);**

**int ncplane_cursor_move_yx(struct ncplane* n, int y, int x);**

**void ncplane_cursor_yx(const struct ncplane* n, int* restrict y, int* restrict x);**

## DESCRIPTION

Ncplanes are the fundamental drawing object of notcurses. All output functions
take a **struct ncplane** as an argument. They can be any size, and placed
anywhere. In addition to its framebuffer--a rectilinear matrix of cells
(see **notcurses_cell(3)**)--an ncplane is defined by:

* a base cell, used for any cell on the plane without a glyph,
* the egcpool backing its cells,
* a current cursor location,
* a current style, foreground channel, and background channel,
* its geometry,
* a configured user pointer,
* its position relative to the visible plane, and
* its z-index.

# RETURN VALUES

**ncplane_new(3)** and **ncplane_aligned(3)** both return a new **struct ncplane**, or
**NULL** on failure.

**ncplane_userptr(3)** and **ncplane_userptr_const(3)** both return the configured user
pointer for the ncplane. They cannot fail.

**ncplane_below(3)** returns the plane below the specified ncplane. If the provided
plane is the bottommost plane, NULL is returned. It cannot fail.

Functions returning **int** return 0 on success, and non-zero on error.

All other functions either cannot fail (and return **void**).

# SEE ALSO

**notcurses(3)**, **notcurses_cell(3)**, **notcurses_output(3)**,
**notcurses_stdplane(3)**
