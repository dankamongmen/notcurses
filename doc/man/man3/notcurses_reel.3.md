% notcurses_reel(3)
% nick black <nickblack@linux.com>
% v1.6.18

# NAME

notcurses_reel - high-level widget for hierarchical data

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
#define NCREEL_OPTION_INFINITESCROLL 0x0001
#define NCREEL_OPTION_CIRCULAR       0x0002

typedef struct ncreel_options {
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
  unsigned flags;      // bitfield over NCREEL_OPTION_*
} ncreel_options;
```

**struct ncreel* ncreel_create(struct ncplane* nc, const ncreel_options* popts);**

**struct ncplane* ncreel_plane(struct ncreel* nr);**

**typedef int (*tabletcb)(struct nctablet* t, bool cliptop);**

**struct nctablet* ncreel_add(struct ncreel* nr, struct nctablet* after, struct nctablet* before, tabletcb cb, void* opaque);**

**int ncreel_tabletcount(const struct ncreel* nr);**

**int ncreel_del(struct ncreel* nr, struct nctablet* t);**

**int ncreel_redraw(struct ncreel* nr);**

**struct nctablet* ncreel_focused(struct ncreel* nr);**

**struct nctablet* ncreel_next(struct ncreel* nr);**

**struct nctablet* ncreel_prev(struct ncreel* nr);**

**int ncreel_destroy(struct ncreel* nr);**

**void* nctablet_userptr(struct nctablet* t);**

**struct ncplane* nctablet_ncplane(struct nctablet* t);**

# DESCRIPTION

# RETURN VALUES

# SEE ALSO

**notcurses(3)**,
**notcurses_plane(3)**
