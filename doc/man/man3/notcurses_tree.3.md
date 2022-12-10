% notcurses_tree(3)
% nick black <nickblack@linux.com>
% v3.0.9

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

**void* nctree_goto(struct nctree* ***n***, const int* ***path***, int* ***failpath***);**

**int nctree_add(struct nctree* ***n***, const unsigned* ***path***, const nctree_item* ***add***);**

**int nctree_del(struct nctree* ***n***, const unsigned* ***path***);**

**void nctree_destroy(struct nctree* ***n***);**

# DESCRIPTION

**nctree**s organize hierarchical items, and allow them to be browsed. Each
item can have arbitrary subitems. Items can be collapsed and expanded.
The display supports scrolling and searching.

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

**nctree_goto**, **nctree_add**, and **nctree_del** all use the concept of a
***path***. A path is an array of **unsigned** values, terminated by
**UINT_MAX**, with each successive value indexing into the hierarchy thus far.
The root node of the **nctree** thus always has a 2-element path of
[0, **UINT_MAX**].

# RETURN VALUES

**nctree_create** returns **NULL** for invalid options. This includes a **NULL**
**items** or **nctreecb** field, or a zero **count** field.

**nctree_next** and **nctree_prev** both return the **curry** pointer from the
newly-focused item. **nctree_focused** returns the **curry** pointer from the
already-focused item.

**nctree_goto** returns the **curry** pointer from the newly-focused item. If
***path*** is invalid, the first invalid hierarchy level is written to
***failpath***, and **NULL** is returned. If ***path*** is valid, the value -1
is written to ***failpath***. Since it is possible for a ***curry***
pointer to be **NULL**, one should check ***failpath*** before assuming the
operation failed.

**nctree_add** and **nctree_del** both return -1 for an invalid ***path***, and
0 otherwise.

# NOTES

**nctree** shares many properties with **notcurses_reel**. Unlike the latter,
**nctree**s support arbitrary hierarchical levels.

**nctree** used to only handle static data, but **nctree_add** and
**nctree_del** were added in Notcurses 3.0.1.

# SEE ALSO

**notcurses(3)**,
**notcurses_input(3)**,
**notcurses_plane(3)**,
**notcurses_reel(3)**
