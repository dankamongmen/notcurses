% notcurses_ncplane(3)
% nick black <nickblack@linux.com>
% v1.3.0

# NAME

notcurses_ncplane - operations on notcurses planes

# SYNOPSIS

**#include <notcurses.h>**

**struct ncplane* ncplane_new(struct notcurses* nc, int rows, int cols, int yoff, int xoff, void* opaque);**

**struct ncplane* ncplane_bound(struct ncplane* n, int rows, int cols, int yoff, int xoff, void* opaque);**

**struct ncplane* ncplane_reparent(struct ncplane* n, struct ncplane* newparent);**

**struct ncplane* ncplane_aligned(struct ncplane* n, int rows, int cols, int yoff, ncalign_e align, void* opaque);**

**struct ncplane* ncplane_dup(struct ncplane* n, void* opaque);**

**int ncplane_resize(struct ncplane* n, int keepy, int keepx, int keepleny, int keeplenx, int yoff, int xoff, int ylen, int xlen);**

**int ncplane_move_yx(struct ncplane* n, int y, int x);**

**void ncplane_yx(const struct ncplane* n, int* restrict y, int* restrict x);**

**int ncplane_set_base_cell(struct ncplane* ncp, const cell* c);**

**int ncplane_set_base(struct ncplane* ncp, uint64_t channels, uint32_t attrword, const char* egc);**

**int ncplane_base(struct ncplane* ncp, cell* c);**

**int ncplane_move_top(struct ncplane* n);**

**int ncplane_move_bottom(struct ncplane* n);**

**int ncplane_move_above(struct ncplane* n, struct ncplane* above);**

**int ncplane_move_below(struct ncplane* n, struct ncplane* below);**

**struct ncplane* ncplane_below(struct ncplane* n);**

**int ncplane_at_cursor(struct ncplane* n, cell* c);**

**int ncplane_at_yx(struct ncplane* n, int y, int x, cell* c);**

**void* ncplane_set_userptr(struct ncplane* n, void* opaque);**

**void* ncplane_userptr(struct ncplane* n);**

**void ncplane_dim_yx(const struct ncplane* n, int* restrict rows, int* restrict cols);**

**static inline int ncplane_dim_y(const struct ncplane* n);**

**static inline int ncplane_dim_x(const struct ncplane* n);**

**void ncplane_cursor_yx(const struct ncplane* n, int* restrict y, int* restrict x);**

**void ncplane_translate(const struct ncplane* src, const struct ncplane* dst, int* restrict y, int* restrict x);**

**bool ncplane_translate_abs(const struct ncplane* n, int* restrict y, int* restrict x);**

**uint64_t ncplane_channels(struct ncplane* n);**

**uint32_t ncplane_attr(struct ncplane* n);**

**static inline unsigned ncplane_bchannel(struct ncplane* nc);**

**static inline unsigned ncplane_fchannel(struct ncplane* nc);**

**static inline unsigned ncplane_fg(struct ncplane* nc);**

**static inline unsigned ncplane_bg(struct ncplane* nc);**

**static inline unsigned ncplane_fg_alpha(struct ncplane* nc);**

**static inline unsigned ncplane_bg_alpha(struct ncplane* nc);**

**static inline unsigned ncplane_fg_rgb(struct ncplane* n, unsigned* r, unsigned* g, unsigned* b);**

**static inline unsigned ncplane_bg_rgb(struct ncplane* n, unsigned* r, unsigned* g, unsigned* b);**

**int ncplane_set_fg_rgb(struct ncplane* n, int r, int g, int b);**

**int ncplane_set_bg_rgb(struct ncplane* n, int r, int g, int b);**

**void ncplane_set_fg_rgb_clipped(struct ncplane* n, int r, int g, int b);**

**void ncplane_set_bg_rgb_clipped(struct ncplane* n, int r, int g, int b);**

**int ncplane_set_fg(struct ncplane* n, unsigned channel);**

**int ncplane_set_bg(struct ncplane* n, unsigned channel);**

**void ncplane_set_fg_default(struct ncplane* n);**

**void ncplane_set_bg_default(struct ncplane* n);**

**int ncplane_set_fg_alpha(struct ncplane* n, int alpha);**

**int ncplane_set_bg_alpha(struct ncplane* n, int alpha);**

**int ncplane_set_fg_palindex(struct ncplane* n, int idx);**

**int ncplane_set_bg_palindex(struct ncplane* n, int idx);**

**void ncplane_styles_set(struct ncplane* n, unsigned stylebits);**

**void ncplane_styles_on(struct ncplane* n, unsigned stylebits);**

**void ncplane_styles_off(struct ncplane* n, unsigned stylebits);**

**unsigned ncplane_styles(const struct ncplane* n);**

**void ncplane_greyscale(struct ncplane* n);**

**int ncblit_bgrx(struct ncplane* nc, int placey, int placex, int linesize, const unsigned char* data, int begy, int begx, int leny, int lenx);**

**int ncblit_rgba(struct ncplane* nc, int placey, int placex, int linesize, const unsigned char* data, int begy, int begx, int leny, int lenx);**

**int ncplane_rotate_cw(struct ncplane* n);**

**int ncplane_rotate_ccw(struct ncplane* n);**

**int ncplane_destroy(struct ncplane* ncp);**

**void notcurses_drop_planes(struct notcurses* nc);**

**int ncplane_mergedown(struct ncplane* restrict src, struct ncplane* restrict dst);**

**void ncplane_erase(struct ncplane* n);**

**bool ncplane_set_scrolling(struct ncplane* n, bool scrollp);**

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

New planes can be created with **ncplane_new**, **ncplane_bound**, and
**ncplane_aligned**. If a plane is bound to another, x and y coordinates are
relative to the plane to which it is bound, and if that plane moves, all its
bound planes move along with it. When a plane is destroyed, all planes bound to
it (directly or transitively) are destroyed. **ncplane_reparent** detaches the
plane **n** from any plane to which it is bound, and binds it to **newparent**
if **newparent** is not **NULL**. All planes bound to **n** move along with it
during a reparenting operation.

**ncplane_destroy** destroys a particular ncplane, after which it must not be
used again. **notcurses_drop_planes** destroys all ncplanes other than the
stdplane. Any references to such planes are, of course, invalidated. It is
undefined to destroy a plane concurrently with any other operation involving
that plane, or any operation involving the z-axis.

It is an error for two threads to concurrently mutate a single ncplane. So long
as rendering is not taking place, however, multiple threads may safely output
to multiple ncplanes. So long as all threads are readers, multiple threads may
work with a single ncplane. A reading function is any which accepts a **const
struct ncplane**.

**ncplane_translate** translates coordinates expressed relative to the plane
**src**, and writes the coordinates of that cell relative to **dst**. The cell
need not intersect with **dst**, though this will yield coordinates which are
invalid for writing or reading on **dst**. If **dst** is **NULL**, it is taken
to refer to the standard plane. **ncplane_translate_abs** takes coordinates
expressed relative to the standard plane, and returns coordinates relative to
**dst**, returning **false** if the coordinates are invalid for **dst**.

**ncplane_mergedown** writes to **dst** the frame that would be rendered if only
**src** and **dst** existed on the z-axis, ad **dst** represented the entirety
of the rendering region. Only those cells where **src** intersects with **dst**
might see changes. It is an error to merge a plane onto itself.

## Scrolling

All planes, including the standard plane, are created with scrolling disabled.
Control scrolling on a per-plane basis with **ncplane_set_scrolling**.
Attempting to print past the end of a line will stop at the plane boundary, and
indicate an error. On a plane 10 columns wide and two rows high, printing
"0123456789" at the origin should succeed, but printing "01234567890" will by
default fail at the eleventh character. In either case, the cursor will be left
at location 0x10; it must be moved before further printing can take place. If
scrolling is enabled, the first row will be filled with 01234546789, the second
row will have 0 written to its first column, and the cursor will end up at 1x1.
Note that it is still an error to manually attempt to move the cursor
off-plane, or to specify off-plane output. Boxes do not scroll; attempting to
draw a 2x11 box on our 2x10 plane will result in an error and no output. When
scrolling is enabled, and output takes place while the cursor is past the end
of the last row, the first row is discarded, all other rows are moved up, the
last row is cleared, and output begins at the beginning of the last row. This
does not take place until output is generated (i.e. it is possible to fill a
plane when scrolling is enabled).

# RETURN VALUES

**ncplane_new**, **ncplane_bound**, **ncplane_aligned**, and **ncplane_dup**
all return a new **struct ncplane** on success, or **NULL** on failure.

**ncplane_userptr** returns the configured user pointer for the ncplane, and
cannot fail.

**ncplane_below** returns the plane below the specified ncplane. If the provided
plane is the bottommost plane, NULL is returned. It cannot fail.

**ncplane_set_scrolling** returns **true** if scrolling was previously enabled,
and **false** otherwise.

Functions returning **int** return 0 on success, and non-zero on error.

All other functions cannot fail (and return **void**).

# NOTES

# SEE ALSO

**notcurses(3)**, **notcurses_cell(3)**, **notcurses_output(3)**,
**notcurses_stdplane(3)**
