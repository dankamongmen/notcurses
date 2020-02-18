% notcurses_ncplane(3)
% nick black <nickblack@linux.com>
% v1.2.0

# NAME

notcurses_ncplane - operations on notcurses planes

# SYNOPSIS

**#include <notcurses.h>**

**struct ncplane* ncplane_new(struct notcurses* nc, int rows, int cols, int yoff, int xoff, void* opaque);**

**struct ncplane* ncplane_aligned(struct ncplane* n, int rows, int cols, int yoff, ncalign_e align, void* opaque);**

**struct ncplane* ncplane_dup(struct ncplane* n, void* opaque);**

**int ncplane_resize(struct ncplane* n, int keepy, int keepx, int keepleny, int keeplenx, int yoff, int xoff, int ylen, int xlen);**

**int ncplane_move_yx(struct ncplane* n, int y, int x);**

**void ncplane_yx(const struct ncplane* n, int* restrict y, int* restrict x);**

**int ncplane_set_base_cell(struct ncplane* ncp, const cell* c);**

**int ncplane_set_base(struct ncplane* ncp, uint64_t channels, uint32_t attrword, const char* egc);**

**int ncplane_base(struct ncplane* ncp, cell* c);**

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

**void ncplane_dim_yx(struct ncplane* n, int* restrict rows, int* restrict cols);**

**static inline int ncplane_dim_y(struct ncplane* n);**

**static inline int ncplane_dim_x(struct ncplane* n);**

**int ncplane_cursor_move_yx(struct ncplane* n, int y, int x);**

**void ncplane_cursor_yx(struct ncplane* n, int* restrict y, int* restrict x);**

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

**unsigned ncplane_styles(struct ncplane* n);**

**void ncplane_greyscale(struct ncplane* n);**

**int ncblit_bgrx(struct ncplane* nc, int placey, int placex, int linesize, const unsigned char* data, int begy, int begx, int leny, int lenx);**

**int ncblit_rgba(struct ncplane* nc, int placey, int placex, int linesize, const unsigned char* data, int begy, int begx, int leny, int lenx);**

**void notcurses_drop_planes(struct notcurses* nc);**

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

**notcurses_drop_planes** destroys all ncplanes other than the stdplane. Any
references to such planes are, of course, invalidated.

# RETURN VALUES

**ncplane_new(3)**, **ncplane_aligned(3)**, and **ncplane_dup(3)** all return a
new **struct ncplane** on success, or **NULL** on failure.

**ncplane_userptr(3)** returns the configured user pointer for the ncplane, and
cannot fail.

**ncplane_below(3)** returns the plane below the specified ncplane. If the provided
plane is the bottommost plane, NULL is returned. It cannot fail.

Functions returning **int** return 0 on success, and non-zero on error.

All other functions cannot fail (and return **void**).

# NOTES

It would be reasonable to expect many of these functions to accept `const struct notcurses`
parameters. Alas, almost all must manipulate the mutex contained within the object.

# SEE ALSO

**notcurses(3)**, **notcurses_cell(3)**, **notcurses_output(3)**,
**notcurses_stdplane(3)**
