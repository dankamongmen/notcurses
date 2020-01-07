% notcurses_panelreels(3)
% nick black <nickblack@linux.com>
% v1.1.0

# NAME

notcurses_panelreels - high-level widget for hierarchal data

# SYNOPSIS

**#include <notcurses.h>**

```c
typedef struct panelreel_options {
  // require this many rows and columns (including borders).
  // otherwise, a message will be displayed stating that a
  // larger terminal is necessary, and input will be queued.
  // if 0, no minimum will be enforced. may not be negative.
  // note that panelreel_create() does not return error if
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
  // bottom left) upon creation / resize. a panelreel_move()
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
  // notcurses can draw a border around the panelreel, and also
  // around the component tablets. inhibit borders by setting all
  // valid bits in the masks. partially inhibit borders by setting
  // individual bits in the masks. the appropriate attr and pair
  // values will be used to style the borders. focused and
  // non-focused tablets can have different styles. you can instead
  // draw your own borders, or forgo borders entirely.
  unsigned bordermask; // bitfield; 1s will not be drawn
  uint64_t borderchan; // attributes used for panelreel border
  unsigned tabletmask; // bitfield for tablet borders
  uint64_t tabletchan; // tablet border styling channel
  uint64_t focusedchan;// focused tablet border styling channel
  uint64_t bgchannel;  // background colors
} panelreel_options;
```

**struct panelreel* panelreel_create(struct ncplane* nc,
                                       const panelreel_options* popts,
                                       int efd);**

**struct ncplane* panelreel_plane(struct panelreel* pr);**

**typedef int (*tabletcb)(struct tablet* t, int begx, int begy, int maxx,
                        int maxy, bool cliptop);**

**struct tablet* panelreel_add(struct panelreel* pr, struct tablet* after,
                                 struct tablet* before, tabletcb cb,
                                 void* opaque);**

**int panelreel_tabletcount(const struct panelreel* pr);**

**int panelreel_touch(struct panelreel* pr, struct tablet* t);**

**int panelreel_del(struct panelreel* pr, struct tablet* t);**

**int panelreel_del_focused(struct panelreel* pr);**

**int panelreel_move(struct panelreel* pr, int x, int y);**

**int panelreel_redraw(struct panelreel* pr);**

**struct tablet* panelreel_focused(struct panelreel* pr);**

**struct tablet* panelreel_next(struct panelreel* pr);**

**struct tablet* panelreel_prev(struct panelreel* pr);**

**int panelreel_destroy(struct panelreel* pr);**

**void* tablet_userptr(struct tablet* t);**

**const void* tablet_userptr_const(const struct tablet* t);**

**struct ncplane* tablet_ncplane(struct tablet* t);**

**const struct ncplane* tablet_ncplane_const(const struct tablet* t);**

# DESCRIPTION

# RETURN VALUES

# SEE ALSO

**notcurses(3)**, **notcurses_ncplane(3)**
