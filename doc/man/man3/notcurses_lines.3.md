% notcurses_lines(3)
% nick black <nickblack@linux.com>
% v1.2.1

# NAME

notcurses_lines - operations on lines and boxes

# SYNOPSIS

**#include <notcurses.h>**

**int ncplane_hline_interp(struct ncplane* n, const cell* c, int len, uint64_t c1, uint64_t c2);**

**static inline int ncplane_hline(struct ncplane* n, const cell* c, int len);**

**int ncplane_vline_interp(struct ncplane* n, const cell* c, int len, uint64_t c1, uint64_t c2);**

**static inline int ncplane_vline(struct ncplane* n, const cell* c, int len);**

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

**int ncplane_box(struct ncplane* n, const cell* ul, const cell* ur,
                    const cell* ll, const cell* lr, const cell* hline,
                    const cell* vline, int ystop, int xstop,
                    unsigned ctlword);**

**static inline int
ncplane_box_sized(struct ncplane* n, const cell* ul, const cell* ur,
                  const cell* ll, const cell* lr, const cell* hline,
                  const cell* vline, int ylen, int xlen, unsigned ctlword);**

**static inline int ncplane_perimeter(struct ncplane* n, const cell* ul, const cell* ur, const cell* ll, const cell* lr, const cell* hline, const cell* vline, unsigned ctlword)**

**static inline int cells_load_box(struct ncplane* n, uint32_t attrs, uint64_t channels, cell* ul, cell* ur, cell* ll, cell* lr, cell* hl, cell* vl, const char* gclusters);**

**static inline int cells_rounded_box(struct ncplane* n, uint32_t attr, uint64_t channels, cell* ul, cell* ur, cell* ll, cell* lr, cell* hl, cell* vl);**

**static inline int ncplane_rounded_box(struct ncplane* n, uint32_t attr, uint64_t channels, int ystop, int xstop, unsigned ctlword);**

**static inline int ncplane_rounded_box_sized(struct ncplane* n, uint32_t attr, uint64_t channels, int ylen, int xlen, unsigned ctlword);**

**static inline int cells_double_box(struct ncplane* n, uint32_t attr, uint64_t channels, cell* ul, cell* ur, cell* ll, cell* lr, cell* hl, cell* vl);**

**static inline int ncplane_double_box(struct ncplane* n, uint32_t attr, uint64_t channels, int ystop, int xstop, unsigned ctlword);**

**static inline int ncplane_double_box_sized(struct ncplane* n, uint32_t attr, uint64_t channels, int ylen, int xlen, unsigned ctlword);**

**int ncplane_polyfill_yx(struct ncplane* n, int y, int x, const cell* c);**

**int ncplane_gradient(struct ncplane* n, const char* egc, uint32_t attrword, uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr, int ystop, int xstop);**

**static inline int ncplane_gradient_sized(struct ncplane* n, const char* egc, uint32_t attrword, uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr, int ylen, int xlen);**

# DESCRIPTION


# RETURN VALUES


# SEE ALSO

**notcurses(3)**,
**notcurses_cell(3)**,
**notcurses_ncplane(3)**
