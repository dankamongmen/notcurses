% notcurses_tree(3)
% nick black <nickblack@linux.com>
% v2.2.1

# NAME

notcurses_tree - high-level hierarchical line-based data

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
struct nctree;
struct ncplane;

typedef struct nctree_item {
  void* curry;
  struct nctree_item* subs;
  unsigned subcount;
} nctree_item;

typedef struct nctree_options {
  const nctree_item* items; // top-level nctree_item array
  unsigned count;           // size of |items|
  uint64_t bchannels;       // base channels
  int (*nctreecb)(struct ncplane*, void*, int); // item callback
  int indentcols;           // columns to indent per hierarchy
  uint64_t flags;           // bitfield of NCTREE_OPTION_*
} nctree_options;

```

**struct nctree* nctree_create(struct ncplane* ***n***, const nctree_options* ***opts***);**

**struct ncplane* nctree_plane(struct nctree* ***n***);**

**int nctree_redraw(struct nctree* ***n***);**

**bool nctree_offer_input(struct nctree* ***n***, const ncinput* ***ni***);**

**void* nctree_focused(struct nctree* ***n***);**

**void* nctree_next(struct nctree* ***n***);**

**void* nctree_prev(struct nctree* ***n***);**

**void* nctree_goto(struct nctree* ***n***, const int* ***spec***, size_t ***specdepth***, int* ***failspec***);**

**void nctree_destroy(struct nctree* ***n***);**

# DESCRIPTION

**nctree**s organize static hierarchical items, and allow them to be browsed.
Each item can have arbitrary subitems. Items can be collapsed and expanded.
The display supports scrolling and searching. Items cannot be added or removed,
however; they must be provided in their entirety at creation time.

An **nctree** cannot be empty. **count** must be non-zero, and **items** must
not be **NULL**. The callback function **nctreecb** must also be non-**NULL**.

The callback function **nctreecb** is called on tree items when the tree is
redrawn. It will be called on each visible item, and any item which has become
hidden. If the item is newly hidden, the **ncplane** argument will be **NULL**.
If the item is newly visible, the **ncplane** argument will be an empty plane.
If the item was already visible, the **ncplane** argument will be the same
plane passed before. If the item has not changed since the last time the
callback was invoked, there is no need to change the plane, and the callback
can return immediately. Otherwise, the plane ought be drawn by the callback.
Any unused rows ought be trimmed away using **ncplane_resize**. If the plane
is expanded in the callback, it will be shrunk back down by the widget. The
**int** parameter indicates distance from the focused item. If the parameter
is negative, the item is before the focused item; a positive parameter implies
that the item follows the focused item; the focused item itself is passed zero.

# RETURN VALUES

**nctree_create** returns **NULL** for invalid options. This includes a **NULL**
**items** or **nctreecb** field, or a zero **count** field.

**nctree_next** and **nctree_prev** both return the **curry** pointer from the
newly-focused item. **nctree_focused** returns the **curry** pointer from the
already-focused item.

# NOTES

**nctree** shares many properties with **notcurses_reel**. Unlike the latter,
**nctree**s support arbitrary hierarchical levels, but they do not allow
elements to come and go across the lifetime of the widget.

# SEE ALSO

**notcurses(3)**,
**notcurses_input(3)**,
**notcurses_plane(3)**,
**notcurses_reel(3)**
