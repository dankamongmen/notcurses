% notcurses_render(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses_render - sync the physical display to a virtual pile

# SYNOPSIS

**#include <notcurses/notcurses.h>**

**int ncpile_render(struct ncplane* n);**

**int ncpile_rasterize(struct ncplane* n);**

**int notcurses_render(struct notcurses* ***nc***);**

**char* notcurses_at_yx(struct notcurses* ***nc***, unsigned ***yoff***, unsigned ***xoff***, uint16_t* ***styles***, uint64_t* ***channels***);**

**int ncpile_render_to_file(struct ncplane* ***p***, FILE* ***fp***);**

**int ncpile_render_to_buffer(struct ncplane* ***p***, char\*\* ***buf***, size_t* ***buflen***);**

# DESCRIPTION

Rendering reduces a pile of **ncplane**s to a single plane, proceeding from the
top to the bottom along a pile's z-axis. The result is a matrix of **nccell**s
(see **notcurses_cell**). Rasterizing takes this matrix, together with the
current state of the visual area, and produces a stream of optimized control
sequences and EGCs for the terminal. By writing this stream to the terminal,
the physical display is synced to some pile's planes.

**ncpile_render** performs the first of these tasks for the pile of which **n**
is a part. The output is maintained internally; calling **ncpile_render** again
on the same pile will replace this state with a fresh render. Multiple piles
can be concurrently rendered. **ncpile_rasterize** performs rasterization, and
writes the result to the terminal. It is a blocking call, and only one
rasterization operation may proceed at a time. **notcurses_render** calls
**ncpile_render** and **ncpile_rasterize** on the standard plane, for backwards
compatibility. It is an exclusive blocking call.

It is necessary to call **ncpile_rasterize** or **notcurses_render** to
generate any visible output; the various notcurses_output(3) calls only draw to
the virtual ncplanes. Most of the notcurses statistics are updated as a result
of a render (see **notcurses_stats(3)**), and screen geometry is refreshed
(similarly to **notcurses_refresh(3)**) *following* the render.

While **notcurses_render** is called, you **must not call any other functions
modifying the same pile**. Other piles may be freely accessed and modified.
The pile being rendered may be accessed, but not modified.

**ncpile_render_to_buffer** performs the render and raster processes of
**ncpile_render** and **ncpile_rasterize**, but does not write the resulting
buffer to the terminal. The user is responsible for writing the buffer to the
terminal in its entirety. If there is an error, subsequent frames will be out
of sync, and **notcurses_refresh(3)** must be called.

A render operation consists of two logical phases: generation of the rendered
scene, and blitting this scene to the terminal (these two phases might actually
be interleaved, streaming the output as it is rendered). Frame generation
requires determining an extended grapheme cluster, foreground color, background
color, and style for each cell of the physical terminal. Writing the scene
requires synthesizing a set of UTF-8-encoded characters and escape codes
appropriate for the terminal (relying on terminfo(5)), and writing this
sequence to the output **FILE**.

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
* If we have not yet locked in a foreground color, and **C** is not foreground-transparent, use the foreground color of **C** (see [BUGS][] below). If **C** is **NCALPHA_OPAQUE**, lock the color in.
* If we have not yet locked in a background color, and **C** is not background-transparent, use the background color of **C** (see [BUGS][] below). If **C** is **NCALPHA_OPAQUE**, lock the color in.

If the algorithm concludes without an EGC, the cell is rendered with no glyph
and a default background. If the algorithm concludes without a color locked in,
the color as computed thus far is used.

**notcurses_at_yx** retrieves a cell *as rendered*. The EGC in that cell is
copied and returned; it must be **free(3)**d by the caller. If the cell is a
secondary column of a wide glyph, the glyph is still returned.

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
**notcurses_input(3)**,
**notcurses_output(3)**,
**notcurses_plane(3)**,
**notcurses_refresh(3)**,
**notcurses_stats(3)**,
**notcurses_visual(3)**,
**console_codes(4)**,
**utf-8(7)**
