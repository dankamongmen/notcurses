% notcurses_tree(3)
% nick black <nickblack@linux.com>
% v2.2.1

# NAME

notcurses_tree - high-level hierarchical line-based data

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
typedef struct nctree_item {
  void* curry;
  struct nctree_item* subs;
  unsigned subcount;
} nctree_item;

typedef struct nctree_options {
  const nctree_item* items; // top-level nctree_item array
  unsigned count;           // size of |items|
  uint64_t bchannels;       // base channels
  int (*nctreecb)(struct ncplane*, void*); // item callback function
  uint64_t flags;           // bitfield of NCTREE_OPTION_*
} nctree_options;
```

**struct nctree* nctree_create(struct ncplane* ***n***, const nctree_options* ***opts***);**

**struct ncplane* nctree_plane(struct nctree* ***n***);**

**int nctree_redraw(struct nctree* ***n***);**

**bool nctree_offer_input(struct nctree* ***n***, const ncinput* ***ni***);**

**void nctree_destroy(struct nctree* ***n***);**

# DESCRIPTION

**nctree**s organize static hierarchical items, and allow them to be browsed.
Each item can have arbitrary subitems. Items can be collapsed and expanded.
The display supports scrolling and searching. Items cannot be added or removed,
however; they must be provided in their entirety at creation time.

# RETURN VALUES

# NOTES

**nctree** shares many properties with **notcurses_reel**. Unlike the latter,
**nctree**s support arbitrary hierarchical levels, but they do not allow
elements to come and go across the lifetime of the widget.

# SEE ALSO

**notcurses(3)**,
**notcurses_input(3)**,
**notcurses_plane(3)**,
**notcurses_reel(3)**
