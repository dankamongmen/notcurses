% notcurses_reels(3)
% nick black <nickblack@linux.com>
% v1.1.6

# NAME

notcurses_reels - high-level widget for hierarchical data

# SYNOPSIS

**#include <notcurses.h>**

```c
typedef struct ncreel_options {
  // require this many rows and columns (including borders).
  // otherwise, a message will be displayed stating that a
  // larger terminal is necessary, and input will be queued.
  // if 0, no minimum will be enforced. may not be negative.
  // note that ncreel_create() does not return error if
  // given a WINDOW smaller than these minima; it instead
  // patiently waits for the screen to get bigger.
  int min_supported_cols;
  int min_supported_rows;

  // use no more than this many rows and columns (including
  // borders). may not be less than the corresponding minimum.
  // 0 means no maximum.
  int max_supported_cols;
  int max_supported_rows;

  // desired offsets within the surrounding WINDOW (top right
  // bottom left) upon creation / resize. a ncreel_move()
  // operation updates these.
  int toff, roff, boff, loff;
  // is scrolling infinite (can one move down or up forever, or is
  // an end reached?). if true, 'circular' specifies how to handle
  // the special case of an incompletely-filled reel.
  bool infinitescroll;
  // is navigation circular (does moving down from the last panel
  // move to the first, and vice versa)? only meaningful when
  // infinitescroll is true. if infinitescroll is false, this must
  // be false.
  bool circular;
  // notcurses can draw a border around the ncreel, and also
  // around the component tablets. inhibit borders by setting all
  // valid bits in the masks. partially inhibit borders by setting
  // individual bits in the masks. the appropriate attr and pair
  // values will be used to style the borders. focused and
  // non-focused tablets can have different styles. you can instead
  // draw your own borders, or forgo borders entirely.
  unsigned bordermask; // bitfield; 1s will not be drawn
  uint64_t borderchan; // attributes used for ncreel border
  unsigned tabletmask; // bitfield for tablet borders
  uint64_t tabletchan; // tablet border styling channel
  uint64_t focusedchan;// focused tablet border styling channel
  uint64_t bgchannel;  // background colors
} ncreel_options;
```

**struct ncreel* ncreel_create(struct ncplane* nc, const ncreel_options* popts, int efd);**

**struct ncplane* ncreel_plane(struct ncreel* nr);**

**typedef int (*tabletcb)(struct nctablet* t, int begx, int begy, int maxx, int maxy, bool cliptop);**

**struct nctablet* ncreel_add(struct ncreel* nr, struct nctablet* after, struct nctablet* before, tabletcb cb, void* opaque);**

**int ncreel_tabletcount(const struct ncreel* nr);**

**int ncreel_touch(struct ncreel* nr, struct nctablet* t);**

**int ncreel_del(struct ncreel* nr, struct nctablet* t);**

**int ncreel_del_focused(struct ncreel* nr);**

**int ncreel_move(struct ncreel* nr, int x, int y);**

**int ncreel_redraw(struct ncreel* nr);**

**struct nctablet* ncreel_focused(struct ncreel* nr);**

**struct nctablet* ncreel_next(struct ncreel* nr);**

**struct nctablet* ncreel_prev(struct ncreel* nr);**

**int ncreel_destroy(struct ncreel* nr);**

**void* nctablet_userptr(struct nctablet* t);**

**const void* nctablet_userptr_const(const struct nctablet* t);**

**struct ncplane* nctablet_ncplane(struct nctablet* t);**

**const struct ncplane* nctablet_ncplane_const(const struct nctablet* t);**

# DESCRIPTION

# RETURN VALUES

# SEE ALSO

**notcurses(3)**, **notcurses_ncplane(3)**
