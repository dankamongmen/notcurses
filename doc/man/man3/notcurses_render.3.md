% notcurses_render(3)
% nick black <nickblack@linux.com>
% v1.3.4

# NAME

notcurses_render - sync the physical display to the virtual ncplanes

# SYNOPSIS

**#include <notcurses/notcurses.h>**

**int notcurses_render(struct notcurses* nc);**

**char* notcurses_at_yx(struct notcurses* nc, int yoff, int xoff, uint32_t* attrword, uint64_t* channels);**

# DESCRIPTION

**notcurses_render** syncs the physical display to the context's prepared
ncplanes. It is necessary to call **notcurses_render** to generate any visible
output; the various notcurses_output(3) calls only draw to the virtual
ncplanes. Most of the notcurses statistics are updated as a result of a
render (see notcurses_stats(3)), and screen geometry is refreshed (similarly to
**notcurses_refresh**) *following* the render.

While **notcurses_render** is called, you **must not call any other functions
on the same notcurses context**, with the one exception of **notcurses_getc**
(and its input-related helpers).

A render operation consists of two logical phases: generation of the rendered
scene, and blitting this scene to the terminal (these two phases might actually
be interleaved, streaming the output as it is rendered). Frame generation
requires determining an extended grapheme cluster, foreground color, background
color, and style for each cell of the physical terminal. Writing the scene
requires synthesizing a set of UTF-8-encoded characters and escape codes
appropriate for the terminal (relying on terminfo(5)), and writing this
sequence to the output **FILE**. If the **renderfp** value was not NULL in the
original call to **notcurses_init**, the frame will be written to that **FILE**
as well. This write does not affect statistics.

Each cell can be rendered in isolation, though synthesis of the stream carries
dependencies between cells.

## Cell rendering algorithm

Recall that there is a total ordering on the N ncplanes, and that the standard
plane always exists, with geometry equal to the physical screen. Each cell of
the physical screen is thus intersected by some totally ordered subset of
planes **P0**, **P1**...**Pi**, where 0 < **i** ≤ **N**. At each cell, rendering starts at
the topmost intersecting plane **P0**. The algorithm descends until either:

* it has locked in an extended grapheme cluster, and fore/background colors, or
* all **i** planes have been examined

At each plane **P**, we consider a cell **C**. This cell is the intersecting cell,
unless that cell has no EGC. In that case, **C** is the plane's default cell.

* If we have not yet determined an EGC, and **C** has a non-zero EGC, use the EGC and style of **C**.
* If we have not yet locked in a foreground color, and **C** is not foreground-transparent, use the foreground color of **C** (see [BUGS][] below). If **C** is **CELL_ALPHA_OPAQUE**, lock the color in.
* If we have not yet locked in a background color, and **C** is not background-transparent, use the background color of **C** (see [BUGS][] below). If **C** is **CELL_ALPHA_OPAQUE**, lock the color in.

If the algorithm concludes without an EGC, the cell is rendered with no glyph
and a default background. If the algorithm concludes without a color locked in,
the color as computed thus far is used.

**notcurses_at_yx** retrieves a call *as rendered*. The EGC in that cell is
copied and returned; it must be **free(3)**d by the caller.

# RETURN VALUES

On success, 0 is returned. On failure, a non-zero value is returned. A success
will result in the **renders** stat being increased by 1. A failure will result
in the **failed_renders** stat being increased by 1.

**notcurses_at_yx** returns a heap-allocated copy of the cell's EGC on success,
and **NULL** on failure.

# BUGS

In addition to the RGB colors, it is possible to use the "default foreground color"
and "default background color" inherited from the terminal. Since
notcurses doesn't know what these colors are, they are not considered for
purposes of color blending.

# SEE ALSO

**notcurses(3)**,
**notcurses_cell(3)**,
**notcurses_plane(3)**,
**notcurses_output(3)**,
**notcurses_refresh(3)**,
**notcurses_stats(3)**,
**console_codes(4)**,
**utf-8(7)**
