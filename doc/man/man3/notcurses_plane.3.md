% notcurses_plane(3)
% nick black <nickblack@linux.com>
% v2.0.7

# NAME

notcurses_plane - operations on ncplanes

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
#define NCPLANE_OPTION_HORALIGNED 0x0001ull

typedef struct ncplane_options {
  int y;            // vertical placement relative to parent plane
  int x;            // horizontal placement relative to parent plane
  int rows;         // number of rows, must be positive
  int cols;         // number of columns, must be positive
  void* userptr;    // user curry, may be NULL
  const char* name; // name (used only for debugging), may be NULL
  int (*resizecb)(struct ncplane*); // called on parent resize
  uint64_t flags;   // closure over NCPLANE_OPTION_*
} ncplane_options;
```

**struct ncplane* ncplane_create(struct ncplane* ***n***, const ncplane_options* ***nopts***);**

**struct ncplane* notcurses_top(struct notcurses* ***n***);**

**struct ncplane* notcurses_bottom(struct notcurses* ***n***);**

**struct ncplane* ncplane_reparent(struct ncplane* ***n***, struct ncplane* ***newparent***);**

**struct ncplane* ncplane_reparent_family(struct ncplane* ***n***, struct ncplane* ***newparent***);**

**int ncplane_resize_realign(struct ncplane* ***n***);**

**void ncplane_set_resizecb(struct ncplane* ***n***, int(*resizecb)(struct ncplane*));**

**int (*ncplane_resizecb(const struct ncplane* ***n***))(struct ncplane*);**

**struct ncplane* ncplane_dup(struct ncplane* ***n***, void* ***opaque***);**

**int ncplane_resize(struct ncplane* ***n***, int ***keepy***, int ***keepx***, int ***keepleny***, int ***keeplenx***, int ***yoff***, int ***xoff***, int ***ylen***, int ***xlen***);**

**int ncplane_move_yx(struct ncplane* ***n***, int ***y***, int ***x***);**

**void ncplane_yx(const struct ncplane* ***n***, int* restrict ***y***, int* restrict ***x***);**

**int ncplane_y(const struct ncplane* ***n***);**

**int ncplane_x(const struct ncplane* ***n***);**

**struct ncplane* ncplane_parent(struct ncplane* ***n***);**

**const struct ncplane* ncplane_parent_const(const struct ncplane* ***n***);**

**int ncplane_set_base_cell(struct ncplane* ***ncp***, const cell* ***c***);**

**int ncplane_set_base(struct ncplane* ***ncp***, const char* ***egc***, uint32_t ***stylemask***, uint64_t ***channels***);**

**int ncplane_base(struct ncplane* ***ncp***, cell* ***c***);**

**int ncplane_move_top(struct ncplane* ***n***);**

**int ncplane_move_bottom(struct ncplane* ***n***);**

**int ncplane_move_above(struct ncplane* restrict ***n***, struct ncplane* restrict ***above***);**

**int ncplane_move_below(struct ncplane* restrict ***n***, struct ncplane* restrict ***below***);**

**struct ncplane* ncplane_below(struct ncplane* ***n***);**

**struct ncplane* ncplane_above(struct ncplane* ***n***);**

**char* ncplane_at_cursor(struct ncplane* ***n***, uint16_t* ***stylemask***, uint64_t* ***channels***);**

**int ncplane_at_cursor_cell(struct ncplane* ***n***, cell* ***c***);**

**char* ncplane_at_yx(const struct ncplane* ***n***, int ***y***, int ***x***, uint16_t* ***stylemask***, uint64_t* ***channels***);**

**int ncplane_at_yx_cell(struct ncplane* ***n***, int ***y***, int ***x***, cell* ***c***);**

**uint32_t* ncplane_rgba(const struct ncplane* ***nc***, int ***begy***, int ***begx***, int ***leny***, int ***lenx***);**

**char* ncplane_contents(const struct ncplane* ***nc***, int ***begy***, int ***begx***, int ***leny***, int ***lenx***);**

**void* ncplane_set_userptr(struct ncplane* ***n***, void* ***opaque***);**

**void* ncplane_userptr(struct ncplane* ***n***);**

**void ncplane_dim_yx(const struct ncplane* ***n***, int* restrict ***rows***, int* restrict ***cols***);**

**static inline int ncplane_dim_y(const struct ncplane* ***n***);**

**static inline int ncplane_dim_x(const struct ncplane* ***n***);**

**void ncplane_cursor_yx(const struct ncplane* ***n***, int* restrict ***y***, int* restrict ***x***);**

**void ncplane_translate(const struct ncplane* ***src***, const struct ncplane* ***dst***, int* restrict ***y***, int* restrict ***x***);**

**bool ncplane_translate_abs(const struct ncplane* ***n***, int* restrict ***y***, int* restrict ***x***);**

**uint64_t ncplane_channels(const struct ncplane* ***n***);**

**void ncplane_set_channels(struct ncplane* ***nc***, uint64_t ***channels***);**

**static inline unsigned ncplane_bchannel(struct ncplane* ***nc***);**

**static inline unsigned ncplane_fchannel(struct ncplane* ***nc***);**

**static inline unsigned ncplane_fg_rgb8(struct ncplane* ***nc***);**

**static inline unsigned ncplane_bg_rgb8(struct ncplane* ***nc***);**

**static inline unsigned ncplane_fg_alpha(struct ncplane* ***nc***);**

**static inline unsigned ncplane_bg_alpha(struct ncplane* ***nc***);**

**static inline unsigned ncplane_fg_rgb8(struct ncplane* ***n***, unsigned* ***r***, unsigned* ***g***, unsigned* ***b***);**

**static inline unsigned ncplane_bg_rgb8(struct ncplane* ***n***, unsigned* ***r***, unsigned* ***g***, unsigned* ***b***);**

**int ncplane_set_fg_rgb8(struct ncplane* ***n***, int ***r***, int ***g***, int ***b***);**

**int ncplane_set_bg_rgb8(struct ncplane* ***n***, int ***r***, int ***g***, int ***b***);**

**void ncplane_set_fg_rgb8_clipped(struct ncplane* ***n***, int ***r***, int ***g***, int ***b***);**

**void ncplane_set_bg_rgb8_clipped(struct ncplane* ***n***, int ***r***, int ***g***, int ***b***);**

**int ncplane_set_fg_rgb8(struct ncplane* ***n***, uint32_t ***channel***);**

**int ncplane_set_bg_rgb8(struct ncplane* ***n***, uint32_t ***channel***);**

**void ncplane_set_fg_default(struct ncplane* ***n***);**

**void ncplane_set_bg_default(struct ncplane* ***n***);**

**int ncplane_set_fg_alpha(struct ncplane* ***n***, unsigned ***alpha***);**

**int ncplane_set_bg_alpha(struct ncplane* ***n***, unsigned ***alpha***);**

**int ncplane_set_fg_palindex(struct ncplane* ***n***, int ***idx***);**

**int ncplane_set_bg_palindex(struct ncplane* ***n***, int ***idx***);**

**uint16_t ncplane_styles(const struct ncplane* ***n***);**

**void ncplane_set_styles(struct ncplane* ***n***, unsigned ***stylebits***);**

**void ncplane_on_styles(struct ncplane* ***n***, unsigned ***stylebits***);**

**void ncplane_off_styles(struct ncplane* ***n***, unsigned ***stylebits***);**

**void ncplane_greyscale(struct ncplane* ***n***);**

**int ncplane_blit_bgrx(struct ncplane* ***nc***, int ***placey***, int ***placex***, int ***linesize***, ncblitter_e ***blitter***, const unsigned char* ***data***, int ***begy***, int ***begx***, int ***leny***, int ***lenx***);**

**int ncplane_blit_rgba(struct ncplane* ***nc***, int ***placey***, int ***placex***, int ***linesize***, ncblitter_e ***blitter***, const unsigned char* ***data***, int ***begy***, int ***begx***, int ***leny***, int ***lenx***);**

**int ncplane_destroy(struct ncplane* ***ncp***);**

**void notcurses_drop_planes(struct notcurses* ***nc***);**

**int ncplane_mergedown(const struct ncplane* ***src***, struct ncplane* ***dst***, int ***begsrcy***, int ***begsrcx***, int ***leny***, int ***lenx***, int ***dsty***, int ***dstx***);**

**int ncplane_mergedown_simple(const struct ncplane* restrict ***src***, struct ncplane* restrict ***dst***);**

**void ncplane_erase(struct ncplane* ***n***);**

**bool ncplane_set_scrolling(struct ncplane* ***n***, bool ***scrollp***);**

**int ncplane_rotate_cw(struct ncplane* ***n***);**

**int ncplane_rotate_ccw(struct ncplane* ***n***);**

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
* position relative to the standard plane,
* the plane, if any, to which it is bound,
* the next plane bound by the plane to which it is bound,
* the head of the list of its bound planes,
* its z-index, and
* a name (used only for debugging).

New planes can be created with **ncplane_create**. If a plane is bound to
another, x and y coordinates are relative to the plane to which it is bound,
and if this latter plane moves, all its bound planes move along with it. When a
plane is destroyed, all planes bound to it (directly or transitively) are
destroyed.

**ncplane_reparent** detaches the plane **n** from any plane to which it is
bound, and binds it to **newparent**. Its children are reparented to its
previous parent. The standard plane cannot be reparented. If **newparent** is
**NULL**, the plane becomes the root plane of a new, unrendered stack. When
**ncplane_reparent_family** is used, all planes bound to **n** move along with
it during a reparenting operation. See [Piles][] below.

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

**ncplane_erase** zeroes out every cell of the plane, dumps the egcpool, and
homes the cursor. The base cell is preserved.

When a plane is resized (whether by **ncplane_resize**, **SIGWINCH**, or any
other mechanism), a depth-first recursion is performed on its children.
Each child plane having a non-**NULL** **resizecb** will see that callback
invoked following resizing of its parent's plane. If it returns non-zero, the
resizing cascade terminates, returning non-zero. Otherwise, resizing proceeds
recursively.

## Piles

A single **notcurses** context is made up of one or more piles. A pile is a
set of one or more **ncplane**s, including the partial orderings made up of
their binding and z-axis pointers. A pile has a top and bottom **ncplane**
(this might be a single plane), and one or more root planes (planes which are
bound to themselves). Multiple threads can concurrently operate on distinct
piles, even changing one while rendering another.

Each plane is part of one and only one pile. By default, a plane is part of the
same pile containing that plane to which it is bound. If the **n** argument
provided to **ncplane_create** is **NULL**, the returned plane becomes the root
plane, top, and bottom of a new pile. As a root plane, it is bound to itself. A
new pile can also be created by reparenting a plane to itself, though if the
plane is already a root plane, this is a no-op.

When a plane is moved to a different pile (whether new or preexisting), any
planes which were bound to it are rebound to its previous parent. If the plane
was a root plane of some pile, any bound planes become root planes. The new
plane is placed immediately atop its new parent on its new pile's z-axis.
When `ncplane_reparent_family()` is used, all planes bound to the reparented
plane are moved along with it. Their relative z-order is maintained.

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

**ncplane_create** and **ncplane_dup** return a new **struct ncplane** on
success, or **NULL** on failure.

**ncplane_userptr** returns the configured user pointer for the ncplane, and
cannot fail.

**ncplane_below** returns the plane below the specified ncplane. If the provided
plane is the bottommost plane, NULL is returned. It cannot fail.

**ncplane_set_scrolling** returns **true** if scrolling was previously enabled,
and **false** otherwise.

**ncplane_at_yx** and **ncplane_at_cursor** return a heap-allocated copy of the
EGC at the relevant cell, or NULL if the cell is invalid. The caller should free
this result. **ncplane_at_yx_cell** and **ncplane_at_cursor_cell** instead load
these values into a **cell**, which is invalidated if the associated plane is
destroyed. The caller should release this cell with **cell_release**.

Functions returning **int** return 0 on success, and non-zero on error.

All other functions cannot fail (and return **void**).

# NOTES

**ncplane_new** is defined as a deprecated wrapper around **ncplane_create**.
It should not be used in new code.

# SEE ALSO

**notcurses(3)**, **notcurses_cell(3)**, **notcurses_output(3)**,
**notcurses_stdplane(3)**
