% notcurses_plane(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses_plane - operations on ncplanes

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
#define NCPLANE_OPTION_HORALIGNED   0x0001ull
#define NCPLANE_OPTION_VERALIGNED   0x0002ull
#define NCPLANE_OPTION_MARGINALIZED 0x0004ull
#define NCPLANE_OPTION_FIXED        0x0008ull
#define NCPLANE_OPTION_AUTOGROW     0x0010ull
#define NCPLANE_OPTION_VSCROLL      0x0020ull

typedef struct ncplane_options {
  int y;            // vertical placement relative to parent plane
  int x;            // horizontal placement relative to parent plane
  int rows;         // number of rows, must be positive
  int cols;         // number of columns, must be positive
  void* userptr;    // user curry, may be NULL
  const char* name; // name (used only for debugging), may be NULL
  int (*resizecb)(struct ncplane*); // called on parent resize
  uint64_t flags;   // closure over NCPLANE_OPTION_*
  unsigned margin_b, margin_r; // bottom and right margins
} ncplane_options;

#define NCSTYLE_MASK      0xffffu
#define NCSTYLE_ITALIC    0x0010u
#define NCSTYLE_UNDERLINE 0x0008u
#define NCSTYLE_UNDERCURL 0x0004u
#define NCSTYLE_BOLD      0x0002u
#define NCSTYLE_STRUCK    0x0001u
#define NCSTYLE_NONE      0
```

**struct ncplane* ncplane_create(struct ncplane* ***n***, const ncplane_options* ***nopts***);**

**struct ncplane* ncpile_create(struct notcurses* ***n***, const ncplane_options* ***nopts***);**

**struct ncplane* ncplane_reparent(struct ncplane* ***n***, struct ncplane* ***newparent***);**

**struct ncplane* ncplane_reparent_family(struct ncplane* ***n***, struct ncplane* ***newparent***);**

**int ncplane_descendant_p(const struct ncplane* ***n***, const struct ncplane* ***ancestor***);**

**int ncplane_resize_realign(struct ncplane* ***n***);**

**int ncplane_resize_maximize(struct ncplane* ***n***);**

**int ncplane_resize_marginalized(struct ncplane* ***n***);**

**int ncplane_resize_placewithin(struct ncplane* ***n***);**

**void ncplane_set_resizecb(struct ncplane* ***n***, int(*resizecb)(struct ncplane*));**

**int (*ncplane_resizecb(const struct ncplane* ***n***))(struct ncplane*);**

**struct ncplane* ncplane_dup(struct ncplane* ***n***, void* ***opaque***);**

**int ncplane_resize(struct ncplane* ***n***, int ***keepy***, int ***keepx***, int ***keepleny***, int ***keeplenx***, int ***yoff***, int ***xoff***, int ***ylen***, int ***xlen***);**

**int ncplane_move_yx(struct ncplane* ***n***, int ***y***, int ***x***);**

**int ncplane_move_rel(struct ncplane* ***n***, int ***y***, int ***x***);**

**void ncplane_yx(const struct ncplane* ***n***, int* restrict ***y***, int* restrict ***x***);**

**int ncplane_y(const struct ncplane* ***n***);**

**int ncplane_x(const struct ncplane* ***n***);**

**void ncplane_abs_yx(const struct ncplane* ***n***, int* ***y***, int* ***x***);**

**int ncplane_abs_y(const struct ncplane* ***n***);**

**int ncplane_abs_x(const struct ncplane* ***n***);**

**struct ncplane* ncplane_parent(struct ncplane* ***n***);**

**const struct ncplane* ncplane_parent_const(const struct ncplane* ***n***);**

**int ncplane_set_base_cell(struct ncplane* ***ncp***, const nccell* ***c***);**

**int ncplane_set_base(struct ncplane* ***ncp***, const char* ***egc***, uint16_t ***stylemask***, uint64_t ***channels***);**

**int ncplane_base(struct ncplane* ***ncp***, nccell* ***c***);**

**static inline void ncplane_move_top(struct ncplane* ***n***);**

**static inline void ncplane_move_bottom(struct ncplane* ***n***);**

**void ncplane_move_family_top(struct ncplane* ***n***);**

**void ncplane_move_family_bottom(struct ncplane* ***n***);**

**int ncplane_move_above(struct ncplane* restrict ***n***, struct ncplane* restrict ***targ***);**

**int ncplane_move_below(struct ncplane* restrict ***n***, struct ncplane* restrict ***targ***);**

**int ncplane_move_family_above(struct ncplane* restrict ***n***, struct ncplane* restrict ***targ***);**

**int ncplane_move_family_below(struct ncplane* restrict ***n***, struct ncplane* restrict ***targ***);**

**struct ncplane* ncplane_below(struct ncplane* ***n***);**

**struct ncplane* ncplane_above(struct ncplane* ***n***);**

**char* ncplane_at_cursor(struct ncplane* ***n***, uint16_t* ***stylemask***, uint64_t* ***channels***);**

**int ncplane_at_cursor_cell(struct ncplane* ***n***, nccell* ***c***);**

**char* ncplane_at_yx(const struct ncplane* ***n***, int ***y***, int ***x***, uint16_t* ***stylemask***, uint64_t* ***channels***);**

**int ncplane_at_yx_cell(struct ncplane* ***n***, int ***y***, int ***x***, nccell* ***c***);**

**uint32_t* ncplane_as_rgba(const struct ncplane* ***nc***, ncblitter_e ***blit***, unsigned ***begy***, unsigned ***begx***, unsigned ***leny***, unsigned ***lenx***, unsigned* ***pxdimy***, unsigned* ***pxdimx***);**

**char* ncplane_contents(const struct ncplane* ***nc***, int ***begy***, int ***begx***, unsigned ***leny***, unsigned ***lenx***);**

**void* ncplane_set_userptr(struct ncplane* ***n***, void* ***opaque***);**

**void* ncplane_userptr(struct ncplane* ***n***);**

**void ncplane_dim_yx(const struct ncplane* ***n***, unsigned* restrict ***rows***, unsigned* restrict ***cols***);**

**static inline unsigned ncplane_dim_y(const struct ncplane* ***n***);**

**static inline unsigned ncplane_dim_x(const struct ncplane* ***n***);**

**void ncplane_cursor_yx(const struct ncplane* ***n***, unsigned* restrict ***y***, unsigned* restrict ***x***);**

**unsigned ncplane_cursor_y(const struct ncplane* ***n***);**

**unsigned ncplane_cursor_x(const struct ncplane* ***n***);**

**void ncplane_translate(const struct ncplane* ***src***, const struct ncplane* ***dst***, int* restrict ***y***, int* restrict ***x***);**

**bool ncplane_translate_abs(const struct ncplane* ***n***, int* restrict ***y***, int* restrict ***x***);**

**uint64_t ncplane_channels(const struct ncplane* ***n***);**

**void ncplane_set_channels(struct ncplane* ***nc***, uint64_t ***channels***);**

**static inline unsigned ncplane_bchannel(struct ncplane* ***nc***);**

**static inline unsigned ncplane_fchannel(struct ncplane* ***nc***);**

**uint64_t ncplane_set_bchannel(struct ncplane* ***nc***, uint32_t ***channel***);**

**uint64_t ncplane_set_fchannel(struct ncplane* ***nc***, uint32_t ***channel***);**

**static inline unsigned ncplane_fg_rgb(struct ncplane* ***nc***);**

**static inline unsigned ncplane_bg_rgb(struct ncplane* ***nc***);**

**int ncplane_set_fg_rgb(struct ncplane* ***n***, uint32_t ***channel***);**

**int ncplane_set_bg_rgb(struct ncplane* ***n***, uint32_t ***channel***);**

**static inline unsigned ncplane_fg_alpha(struct ncplane* ***nc***);**

**static inline unsigned ncplane_bg_alpha(struct ncplane* ***nc***);**

**int ncplane_set_fg_alpha(struct ncplane* ***n***, unsigned ***alpha***);**

**int ncplane_set_bg_alpha(struct ncplane* ***n***, unsigned ***alpha***);**

**static inline unsigned ncplane_fg_rgb8(struct ncplane* ***n***, unsigned* ***r***, unsigned* ***g***, unsigned* ***b***);**

**static inline unsigned ncplane_bg_rgb8(struct ncplane* ***n***, unsigned* ***r***, unsigned* ***g***, unsigned* ***b***);**

**int ncplane_set_fg_rgb8(struct ncplane* ***n***, unsigned ***r***, unsigned ***g***, unsigned ***b***);**

**int ncplane_set_bg_rgb8(struct ncplane* ***n***, unsigned ***r***, unsigned ***g***, unsigned ***b***);**

**void ncplane_set_fg_rgb8_clipped(struct ncplane* ***n***, int ***r***, int ***g***, int ***b***);**

**void ncplane_set_bg_rgb8_clipped(struct ncplane* ***n***, int ***r***, int ***g***, int ***b***);**

**void ncplane_set_fg_default(struct ncplane* ***n***);**

**void ncplane_set_bg_default(struct ncplane* ***n***);**

**int ncplane_set_fg_palindex(struct ncplane* ***n***, unsigned ***idx***);**

**int ncplane_set_bg_palindex(struct ncplane* ***n***, unsigned ***idx***);**

**uint16_t ncplane_styles(const struct ncplane* ***n***);**

**void ncplane_set_styles(struct ncplane* ***n***, unsigned ***stylebits***);**

**void ncplane_on_styles(struct ncplane* ***n***, unsigned ***stylebits***);**

**void ncplane_off_styles(struct ncplane* ***n***, unsigned ***stylebits***);**

**void ncplane_greyscale(struct ncplane* ***n***);**

**int ncplane_blit_bgrx(struct ncplane* ***nc***, int ***placey***, int ***placex***, int ***linesize***, ncblitter_e ***blitter***, const unsigned char* ***data***, int ***begy***, int ***begx***, int ***leny***, int ***lenx***);**

**int ncplane_blit_rgba(struct ncplane* ***nc***, int ***placey***, int ***placex***, int ***linesize***, ncblitter_e ***blitter***, const unsigned char* ***data***, int ***begy***, int ***begx***, int ***leny***, int ***lenx***);**

**int ncplane_destroy(struct ncplane* ***ncp***);**

**void notcurses_drop_planes(struct notcurses* ***nc***);**

**int ncplane_mergedown(struct ncplane* ***src***, struct ncplane* ***dst***, int ***begsrcy***, int ***begsrcx***, unsigned ***leny***, unsigned ***lenx***, int ***dsty***, int ***dstx***);**

**int ncplane_mergedown_simple(struct ncplane* restrict ***src***, struct ncplane* restrict ***dst***);**

**void ncplane_erase(struct ncplane* ***n***);**

**int ncplane_erase_region(struct ncplane* ***n***, int ***ystart***, int ***xstart***, int ***ylen***, int ***xlen***);**

**bool ncplane_set_scrolling(struct ncplane* ***n***, unsigned ***scrollp***);**

**bool ncplane_scrolling_p(const struct ncplane* ***n***);**

**bool ncplane_set_autogrow(struct ncplane* ***n***, unsigned ***growp***);**

**bool ncplane_autogrow_p(const struct ncplane* ***n***);**

**int ncplane_scrollup(struct ncplane* ***n***, int ***r***);**

**int ncplane_scrollup_child(struct ncplane* ***n***, const struct ncplane* ***child***);**

**int ncplane_rotate_cw(struct ncplane* ***n***);**

**int ncplane_rotate_ccw(struct ncplane* ***n***);**

**void ncplane_pixel_geom(const struct notcurses* ***n***, unsigned* restrict ***pxy***, unsigned* restrict ***pxx***, unsigned* restrict ***celldimy***, unsigned* restrict ***celldimx***, unsigned* restrict ***maxbmapy***, unsigned* restrict ***maxbmapx***);**

**int ncplane_set_name(struct ncplane* ***n***, const char* ***name***);**

**char* ncplane_name(const struct ncplane* ***n***);**

# DESCRIPTION

Ncplanes are the fundamental drawing object of Notcurses. All output functions
take a **struct ncplane** as an argument. They can be any size, and placed
anywhere. In addition to its framebuffer--a rectilinear matrix of **nccell**s
(see **notcurses_cell(3)**)--an ncplane is defined by:

* a base **nccell**, used for any cell on the plane without a glyph,
* the egcpool backing its **nccell**s,
* a current cursor location,
* a current style, foreground channel, and background channel,
* its geometry,
* a configured user pointer,
* position relative to the standard plane,
* the plane, if any, to which it is bound,
* the next plane bound by the plane to which it is bound,
* the head of the list of its bound planes,
* its resize methodology,
* whether a sprixel (see **notcurses_visual(3)**) is associated,
* its z-index, and
* a name (used only for debugging).

New planes can be created with **ncplane_create**. If a plane is bound to
another, x and y coordinates are relative to the plane to which it is bound,
and if this latter plane moves, all its bound planes move along with it. When a
plane is destroyed, all planes bound to it (directly or transitively) are
destroyed.

If the **NCPLANE_OPTION_HORALIGNED** flag is provided, ***x*** is interpreted
as an **ncalign_e** rather than an absolute position. If the
**NCPLANE_OPTION_VERALIGNED** flag is provided, ***y*** is interpreted as an
**ncalign_e** rather than an absolute postiion. Either way, all positions
are relative to the parent plane. **ncplane_resize_realign** should usually be
used together with these flags, so that the plane is automatically realigned
upon a resize of its parent.

If the **NCPLANE_OPTION_MARGINALIZED** flag is provided, neither
**NCPLANE_OPTION_HORALIGNED** nor **NCPLANE_OPTION_VERALIGNED** may be
provided, and ***rows*** and ***cols*** must both be 0. ***y*** and ***x***
will be interpreted as top and left margins. ***margin_b*** and ***margin_r***
will be interpreted as bottom and right margins. The plane will take the maximum
space possible subject to its parent planes and these margins. The plane cannot
become smaller than 1x1 (the margins are best-effort).
**ncplane_resize_marginalized** should usually be used together with this flag,
so that the plane is automatically resized.

**ncplane_reparent** detaches the plane ***n*** from any plane to which it is
bound, and binds it to ***newparent***. Its children are reparented to its
previous parent. The standard plane cannot be reparented. If ***newparent*** is
**NULL**, the plane becomes the root plane of a new, unrendered stack. When
**ncplane_reparent_family** is used, all planes bound to ***n*** move along with
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

A plane can be moved relative to its parent plane's origin with
**ncplane_move_yx**. If the plane has no parent, the move is relative to
the rendering area. A plane can be moved off-screen entirely, in which case
it will not be visible following rasterization; it can also be partially
off-screen.

A plane has a virtual cursor; set its new position with **ncplane_cursor_move_yx**.
Specifying -1 as one or both coordinates will hold that axis constant. You may
move a cursor relatively to its current position with **ncplane_cursor_move_rel**.
Unless coordinates are specified for a call, action takes place at the plane's
virtual cursor, which automatically moves along with output. The current virtual
cursor location can be acquired with **ncplane_cursor_yx**.

**ncplane_yx** returns the coordinates of the specified plane's origin, relative
to the plane to which it is bound. Either or both of ***y*** and ***x*** may
be **NULL**. **ncplane_y** and **ncplane_x** allow a single component of this
location to be retrieved. **ncplane_abs_yx** returns the coordinates of the
specified plane's origin relative to its pile.

**ncplane_translate** translates coordinates expressed relative to the plane
***src***, and writes the coordinates of that cell relative to ***dst***. The cell
need not intersect with ***dst***, though this will yield coordinates which are
invalid for writing or reading on ***dst***. If ***dst*** is **NULL**, it is taken
to refer to the standard plane. **ncplane_translate_abs** takes coordinates
expressed relative to the standard plane, and returns coordinates relative to
***dst***, returning **false** if the coordinates are invalid for ***dst***.

**ncplane_mergedown** writes to ***dst*** the frame that would be rendered if only
***src*** and ***dst*** existed on the z-axis, ad ***dst*** represented the entirety
of the rendering region. Only those cells where ***src*** intersects with ***dst***
might see changes. It is an error to merge a plane onto itself.

**ncplane_erase** zeroes out every cell of the plane, dumps the egcpool, and
homes the cursor. The base cell is preserved, as are the active attributes.
**ncplane_erase_region** does the same for a subregion of the plane. For the
latter, supply 0 for ***ylen*** and/or ***xlen*** to erase through that
dimension, starting at the specified point. Supply -1 for ***ystart*** and/or
***xstart*** to use the cursor's current position along that axis for a starting
point. Negative ***ylen*** and ***xlen*** move up and to the left from the starting
coordinate; positive ***ylen*** and ***xlen*** move down and to the right from same.
See [BUGS][] below.

When a plane is resized (whether by **ncplane_resize**, **SIGWINCH**, or any
other mechanism), a depth-first recursion is performed on its children.
Each child plane having a non-**NULL** **resizecb** will see that callback
invoked following resizing of its parent's plane. If it returns non-zero, the
resizing cascade terminates, returning non-zero. Otherwise, resizing proceeds
recursively.

**ncplane_move_top** and **ncplane_move_bottom** extract their argument
***n*** from the z-axis, and reinsert it at the top or bottom, respectively,
of its pile. These functions are both O(1). **ncplane_move_family_top** and
**ncplane_move_family_bottom** do the same, and move any bound descendants
along with the plane. Ordering among the plane and its descendants will be
maintained. For example, assume a pile with A at the top of its z-axis,
followed by B, C, D, and E, where E is bound to C. Calling
**ncplane_move_family_top** on C will result in the order CEABD. Calling
**ncplane_move_family_bottom** on C will result in the order ABDCE. Calling
**ncplane_move_family_top** or **ncplane_move_top** on E will result in EABCD.
Calling **ncplane_move_family_bottom** on E is a no-op. These two functions
are O(N) on the number of planes in the pile.

**ncplane_move_above** and **ncplane_move_below** move the argument ***n***
above or below, respectively, the argument ***targ***. Both operate in O(1).

**ncplane_at_yx** and **ncplane_at_yx_cell** retrieve the contents of the plane
at the specified coordinate. The content is returned as it will be used during
rendering, and thus integrates any base cell as appropriate. If called on the
secondary columns of a wide glyph, **ncplane_at_yx** returns the EGC, and thus
cannot be used to distinguish between primary and secondary columns.
**ncplane_at_yx_cell**, however, preserves this information: retrieving a
secondary column of a wide glyph with **ncplane_at_yx_cell** will fill in
the **nccell** argument such that **nccell_extended_gcluster(3)** returns an
empty string, and **nccell_wide_right_p(3)** returns **true**.

If **ncplane_at_yx** is invoked upon a sprixel plane, the control sequence will
be returned for any valid coordinates (note that this may be quite large).
This does not apply to **ncplane_at_yx_cell**, which will return an error.

**ncplane_set_name** sets the plane's name, freeing any old name. ***name***
may be **NULL**. **ncplane_set_name** duplicates the provided name internally.

## Base cells

Each plane has a base cell, initialized to all zeroes. When rendering, the
cells of the plane are examined in turn. Each cell has three independent
rendering elements--its EGC, its foreground channel, and its background
channel. Any default channel is replaced with the corresponding channel from
that plane's base cell. **ncplane_erase** has no effect on the base cell.
Calling **ncplane_erase** on a plane whose base cell is a purple 'A' results
(for rendering purposes) in a plane made up entirely of purple 'A's.

**ncplane_set_base_cell** uses the **nccell** ***c*** (which must be bound to
the **ncplane** ***ncp***, and must be the first **nccell** of a multicolumn
sequence) to set the base cell. **ncplane_set_base** does the same with
***egc***, ***stylemask***, and ***channels***.

## Piles

A single Notcurses context is made up of one or more piles. A pile is a
set of one or more **ncplane**s, including the partial orderings made up of
their binding and z-axis pointers. A pile has a top and bottom **ncplane**
(this might be a single plane), and one or more root planes (planes which are
bound to themselves). Multiple threads can concurrently operate on distinct
piles, even changing one while rendering another.

Each plane is part of one and only one pile. By default, a plane is part of the
same pile containing that plane to which it is bound. If **ncpile_create** is
used in the place of **ncplane_create**, the returned plane becomes the root
plane, top, and bottom of a new pile. As a root plane, it is bound to itself. A
new pile can also be created by reparenting a plane to itself, though if the
plane is already a root plane, this is a no-op.

When a plane is moved to a different pile (whether new or preexisting), any
planes which were bound to it are rebound to its previous parent. If the plane
was a root plane of some pile, any bound planes become root planes. The new
plane is placed immediately atop its new parent on its new pile's z-axis.
When **ncplane_reparent_family** is used, all planes bound to the reparented
plane are moved along with it. Their relative z-order is maintained.

More information is available from **notcurses_pile(3)**.

## Binding

The planes of a pile make up a directed acyclic forest. Planes bound to
themselves make up the root planes of the pile. Every plane is either a
root plane, or bound to some other plane in its pile. A plane and its
descendants make up a family. When a plane is moved using **ncplane_move_yx**,
its family is moved along with it.

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
off-plane, or to specify off-plane output. Box-drawing does not result in
scrolling; attempting to draw a 2x11 box on our 2x10 plane will result in an
error and no output. When scrolling is enabled, and output takes place while
the cursor is past the end of the last row, the first row is discarded, all
other rows are moved up, the last row is cleared, and output begins at the
beginning of the last row. This does not take place until output is generated
(i.e. it is possible to fill a plane when scrolling is enabled).

Creating a plane with the **NCPLANE_OPTION_VSCROLL** flag is equivalent to
immediately calling **ncplane_set_scrolling** on that plane with an argument
of **true**.

By default, planes bound to a scrolling plane will scroll along with it, if
they intersect the plane. This can be disabled by creating them with the
**NCPLANE_OPTION_FIXED** flag.

## Autogrow

Normally, once output reaches the right boundary of a plane, it is impossible
to place more output unless the cursor is first moved. If scrolling is
enabled, the cursor will automatically move down and to the left in this case,
but upon reaching the bottom right corner of the plane, it is impossible to
place more output without a scrolling event. If autogrow is in play, the plane
will automatically be enlarged to accommodate output. If scrolling is disabled,
growth takes place to the right; it otherwise takes place at the bottom. The
plane only grows in one dimension. Autogrow cannot be enabled for the standard
plane.

Creating a plane with the **NCPLANE_OPTION_AUTOGROW** flag is equivalent to
immediately calling **ncplane_set_autogrow** on that plane with an argument
of **true**.

## Bitmaps

**ncplane_pixel_geom** retrieves pixel geometry details. **pxy** and **pxx**
return the size of the plane in pixels. **celldimy** and **celldimx** return
the size of a cell in pixels (these ought be the same across planes).
**maxbmapy** and **maxbmapx** describe the largest bitmap which can be
displayed in the plane. Any parameter (save **n**) may be **NULL**.

When a plane is blitted to using **ncvisual_blit** and **NCBLIT_PIXEL** (see
**notcurses_visual(3)**), it ceases to accept cell-based output. The sprixel
will remain associated until a new sprixel is blitted to the plane, the plane
is resized, the plane is erased, or the plane is destroyed. The base cell of a
sprixelated plane has no effect; if the sprixel is not even multiples of the
cell geometry, the "excess plane" is ignored during rendering.

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
EGC at the relevant cell, or **NULL** if the cell is invalid. The caller should
free this result. **ncplane_at_yx_cell** and **ncplane_at_cursor_cell** instead
load these values into an **nccell**, which is invalidated if the associated
plane is destroyed. The caller should release this **nccell** with
**nccell_release**.

**ncplane_as_rgba** returns a heap-allocated array of **uint32_t** values,
each representing a single RGBA pixel, or **NULL** on failure.

**ncplane_erase_region** returns -1 if **ystart** or **xstart** are less than -1,
or outside the plane.

**ncplane_cursor_move_yx** returns -1 if the coordinates are beyond the
dimensions of the specified plane (except for the special value -1).

**ncplane_cursor_move_rel** returns -1 if the coordinates are beyond the
dimensions of the specified plane.

**ncplane_name** returns a heap-allocated copy of the plane's name, or NULL if
it has no name (or on error).

Functions returning **int** return 0 on success, and non-zero on error.

All other functions cannot fail (and return **void**).

# NOTES

# BUGS

**ncplane_at_yx** doesn't yet account for bitmap-based graphics (see
**notcurses_visual(3)**). Whatever glyph-based contents existed on the plane when
the bitmap was blitted will continue to be returned.

When the alternate screen is not used (see **notcurses_init(3)**), the contents
of the terminal at startup remain present until obliterated on a cell-by-cell
basis. **ncplane_erase** and **ncplane_erase_region** **cannot** be used to
clear the terminal of startup content. If you want the screen cleared on
startup, but do not want to use (or rely on) the alternate screen, use something
like:

```c
ncplane_set_base(notcurses_stdplane(nc), " ", 0, 0);
notcurses_render(nc);
```

or simply:

```c
notcurses_refresh(nc);
```

# SEE ALSO

**notcurses(3)**,
**notcurses_capabilities(3)**,
**notcurses_cell(3)**,
**notcurses_output(3)**,
**notcurses_pile(3)**,
**notcurses_stdplane(3)**,
**notcurses_visual(3)**
