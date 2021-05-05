% notcurses_lines(3)
% nick black <nickblack@linux.com>
% v2.2.10

# NAME

notcurses_lines - operations on lines and boxes

# SYNOPSIS

**#include <notcurses/notcurses.h>**

**int ncplane_hline_interp(struct ncplane* ***n***, const nccell* ***c***, int ***len***, uint64_t ***c1***, uint64_t ***c2***);**

**static inline int ncplane_hline(struct ncplane* ***n***, const nccell* ***c***, int ***len***);**

**int ncplane_vline_interp(struct ncplane* ***n***, const nccell* ***c***, int ***len***, uint64_t ***c1***, uint64_t ***c2***);**

**static inline int ncplane_vline(struct ncplane* ***n***, const nccell* ***c***, int ***len***);**

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

**static inline int nccells_load_box(struct ncplane* ***n***, uint32_t ***styles***, uint64_t ***channels***, nccell* ***ul***, nccell* ***ur***, nccell* ***ll***, nccell* ***lr***, nccell* ***hl***, nccell* ***vl***, const char* ***gclusters***);**

**static inline int nccells_rounded_box(struct ncplane* ***n***, uint32_t ***styles***, uint64_t ***channels***, nccell* ***ul***, nccell* ***ur***, nccell* ***ll***, nccell* ***lr***, nccell* ***hl***, nccell* ***vl***);**

**static inline int ncplane_rounded_box(struct ncplane* ***n***, uint32_t ***styles***, uint64_t ***channels***, int ***ystop***, int ***xstop***, unsigned ***ctlword***);**

**static inline int ncplane_rounded_box_sized(struct ncplane* ***n***, uint32_t ***styles***, uint64_t ***channels***, int ***ylen***, int ***xlen***, unsigned ***ctlword***);**

**static inline int nccells_double_box(struct ncplane* ***n***, uint32_t ***styles***, uint64_t ***channels***, nccell* ***ul***, nccell* ***ur***, nccell* ***ll***, nccell* ***lr***, nccell* ***hl***, nccell* ***vl***);**

**static inline int ncplane_double_box(struct ncplane* ***n***, uint32_t ***styles***, uint64_t ***channels***, int ***ystop***, int ***xstop***, unsigned ***ctlword***);**

**static inline int ncplane_double_box_sized(struct ncplane* ***n***, uint32_t ***styles***, uint64_t ***channels***, int ***ylen***, int ***xlen***, unsigned ***ctlword***);**

**int ncplane_polyfill_yx(struct ncplane* ***n***, int ***y***, int ***x***, const nccell* ***c***);**

**int ncplane_gradient(struct ncplane* ***n***, const char* ***egc***, uint32_t ***stylemask***, uint64_t ***ul***, uint64_t ***ur***, uint64_t ***ll***, uint64_t ***lr***, int ***ystop***, int ***xstop***);**

**static inline int ncplane_gradient_sized(struct ncplane* ***n***, const char* ***egc***, uint32_t ***stylemask***, uint64_t ***ul***, uint64_t ***ur***, uint64_t ***ll***, uint64_t ***lr***, int ***ylen***, int ***xlen***);**

**int ncplane_highgradient(struct ncplane* ***n***, uint32_t ***ul***, uint32_t ***ur***, uint32_t ***ll***, uint32_t ***lr***, int ***ystop***, int ***xstop***);**

**int ncplane_highgradient_sized(struct ncplane* ***n***, uint32_t ***ul***, uint32_t ***ur***, uint32_t ***ll***, uint32_t ***lr***, int ***ylen***, int ***xlen***);**

**int ncplane_format(struct ncplane* ***n***, int ***ystop***, int ***xstop***, uint32_t ***stylemask***);**

**int ncplane_stain(struct ncplane* ***n***, int ***ystop***, int ***xstop***, uint64_t ***ul***, uint64_t ***ur***, uint64_t ***ll***, uint64_t ***lr***);**

# DESCRIPTION

**ncplane_format** sets the attributes of every cell in the region having its
upper-left corner at the cursor's current position, and its lower-right corner
at **ystop**, **xstop**.

Box- and line-drawing is unaffected by a plane's scrolling status.

# RETURN VALUES

**ncplane_format** returns -1 if either **ystop** or **xstop** is less than the
current equivalent position, otherwise 0.

# SEE ALSO

**notcurses(3)**,
**notcurses_cell(3)**,
**notcurses_plane(3)**
