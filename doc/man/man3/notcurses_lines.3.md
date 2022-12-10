% notcurses_lines(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses_lines - operations on lines and boxes

# SYNOPSIS

**#include <notcurses/notcurses.h>**

**int ncplane_hline_interp(struct ncplane* ***n***, const nccell* ***c***, unsigned ***len***, uint64_t ***c1***, uint64_t ***c2***);**

**static inline int ncplane_hline(struct ncplane* ***n***, const nccell* ***c***, unsigned ***len***);**

**int ncplane_vline_interp(struct ncplane* ***n***, const nccell* ***c***, unsigned ***len***, uint64_t ***c1***, uint64_t ***c2***);**

**static inline int ncplane_vline(struct ncplane* ***n***, const nccell* ***c***, unsigned ***len***);**

```c
#define NCBOXMASK_TOP    0x0001
#define NCBOXMASK_RIGHT  0x0002
#define NCBOXMASK_BOTTOM 0x0004
#define NCBOXMASK_LEFT   0x0008
#define NCBOXGRAD_TOP    0x0010
#define NCBOXGRAD_RIGHT  0x0020
#define NCBOXGRAD_BOTTOM 0x0040
#define NCBOXGRAD_LEFT   0x0080
#define NCBOXCORNER_MASK 0x0300
#define NCBOXCORNER_SHIFT 8u
```

**int ncplane_box(struct ncplane* ***n***, const nccell* ***ul***, const nccell* ***ur***, const nccell* ***ll***, const nccell* ***lr***, const nccell* ***hline***, const nccell* ***vline***, int ***ystop***, int ***xstop***, unsigned ***ctlword***);**

**static inline int ncplane_box_sized(struct ncplane* ***n***, const nccell* ***ul***, const nccell* ***ur***, const nccell* ***ll***, const nccell* ***lr***, const nccell* ***hline***, const nccell* ***vline***, int ***ylen***, int ***xlen***, unsigned ***ctlword***);**

**static inline int ncplane_perimeter(struct ncplane* ***n***, const nccell* ***ul***, const nccell* ***ur***, const nccell* ***ll***, const nccell* ***lr***, const nccell* ***hline***, const nccell* ***vline***, unsigned ***ctlword***)**

**static inline int nccells_load_box(struct ncplane* ***n***, uint16_t ***styles***, uint64_t ***channels***, nccell* ***ul***, nccell* ***ur***, nccell* ***ll***, nccell* ***lr***, nccell* ***hl***, nccell* ***vl***, const char* ***gclusters***);**

**static inline int nccells_rounded_box(struct ncplane* ***n***, uint16_t ***styles***, uint64_t ***channels***, nccell* ***ul***, nccell* ***ur***, nccell* ***ll***, nccell* ***lr***, nccell* ***hl***, nccell* ***vl***);**

**static inline int ncplane_rounded_box(struct ncplane* ***n***, uint16_t ***styles***, uint64_t ***channels***, int ***ystop***, int ***xstop***, unsigned ***ctlword***);**

**static inline int ncplane_rounded_box_sized(struct ncplane* ***n***, uint16_t ***styles***, uint64_t ***channels***, int ***ylen***, int ***xlen***, unsigned ***ctlword***);**

**static inline int nccells_double_box(struct ncplane* ***n***, uint16_t ***styles***, uint64_t ***channels***, nccell* ***ul***, nccell* ***ur***, nccell* ***ll***, nccell* ***lr***, nccell* ***hl***, nccell* ***vl***);**

**static inline int ncplane_double_box(struct ncplane* ***n***, uint16_t ***styles***, uint64_t ***channels***, int ***ystop***, int ***xstop***, unsigned ***ctlword***);**

**static inline int ncplane_double_box_sized(struct ncplane* ***n***, uint16_t ***styles***, uint64_t ***channels***, int ***ylen***, int ***xlen***, unsigned ***ctlword***);**

**int ncplane_polyfill_yx(struct ncplane* ***n***, unsigned ***y***, unsigned ***x***, const nccell* ***c***);**

**int ncplane_gradient(struct ncplane* ***n***, int ***y***, int ***x***, unsigned ***ylen***, unsigned ***xlen***, const char* ***egc***, uint16_t ***stylemask***, uint64_t ***ul***, uint64_t ***ur***, uint64_t ***ll***, uint64_t ***lr***);**

**int ncplane_highgradient2x1(struct ncplane* ***n***, int ***y***, int ***x***, unsigned ***ylen***, unsigned ***xlen***, uint32_t ***ul***, uint32_t ***ur***, uint32_t ***ll***, uint32_t ***lr***);**

**int ncplane_format(struct ncplane* ***n***, int ***y***, int ***x***, unsigned ***ylen***, unsigned ***xlen***, uint16_t ***stylemask***);**

**int ncplane_stain(struct ncplane* ***n***, int ***y***, int ***x***, unsigned ***ylen***, unsigned ***xlen***, uint64_t ***ul***, uint64_t ***ur***, uint64_t ***ll***, uint64_t ***lr***);**

# DESCRIPTION

**ncplane_polyfill_yx** starts at the specified ***y*** and ***x*** (provide
-1 to use the cursor's position in the relevant dimension). The cell at
these coordinates is replaced with ***c***. All connected cells having the
same content as this original cell are replaced with ***c***, recursively.
Two cells are connected if they are vertical or horizontal neighbors of one
another.

**ncplane_gradient** replaces all glyphs in the specified area with
***egc***, and colors them with the specified gradient. All content
within the specified area is destroyed.

**ncplane_gradient2x1** draws a high-definition gradient in the specified
area. It will return an error if UTF8 is not being used. The gradient is
drawn using the UPPER HALF BLOCK glyph, with two vertical steps and one
horizontal step in each cell. Since cells are often about twice as tall as
they are wide, this tends to result in very even color differences. All
content within the specified area is destroyed.

**ncplane_format** sets the attributes of every cell in the region having its
upper-left corner at ***y*** and ***x*** (provide -1 to use the cursor's
position in the relevant dimension), and its area defined by ***ylen*** and
***xlen*** (provide 0 to use all remaining area to the right and below,
respectively). Channels and glyphs will be unaffected.

**ncplane_stain** works the same way, but sets channels. Standard linear
interpolation is applied between the provided corner channels. Glyphs
and their attributes will be unaffected.

Box- and line-drawing is unaffected by a plane's scrolling status.

# RETURN VALUES

**ncplane_format**, **ncplane_stain**, **ncplane_gradient**,
**ncplane_gradient2x1**, and **ncplane_polyfill_yx** return -1 if
any coordinates are outside the plane, and otherwise the number of cells
affected.

**ncplane_hline_interp**, **ncplane_hline**, **ncplane_vline_interp**, and
**ncplane_vline** all return the number of glyphs drawn on success, or -1
on failure. Passing a length of 0 is an error.

# SEE ALSO

**notcurses(3)**,
**notcurses_cell(3)**,
**notcurses_plane(3)**
