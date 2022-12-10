% notcurses_selector(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses_selector - high level widget for selecting from a set

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
struct ncplane;
struct notcurses;
struct ncselector;

struct ncselector_item {
  const char* option;
  const char* desc;
};

typedef struct ncselector_options {
  const char* title; // title may be NULL, inhibiting riser
  const char* secondary; // secondary may be NULL
  const char* footer; // footer may be NULL
  struct ncselector_item* items; // initial items and descriptions
  // default item (selected at start)
  unsigned defidx;
  // maximum number of options to display at once
  unsigned maxdisplay;
  // exhaustive styling options
  uint64_t opchannels;   // option channels
  uint64_t descchannels; // description channels
  uint64_t titlechannels;// title channels
  uint64_t footchannels; // secondary and footer channels
  uint64_t boxchannels;  // border channels
  uint64_t flags;        // bitfield over NCSELECTOR_OPTION_*
} ncselector_options;
```

**struct ncselector* ncselector_create(struct ncplane* ***n***, const ncselector_options* ***opts***);**

**int ncselector_additem(struct ncselector* ***n***, const struct ncselector_item* ***item***);**

**int ncselector_delitem(struct ncselector* ***n***, const char* ***item***);**

**const char* ncselector_selected(const struct ncselector* ***n***);**

**struct ncplane* ncselector_plane(struct ncselector* ***n***);**

**const char* ncselector_previtem(struct ncselector* ***n***);**

**const char* ncselector_nextitem(struct ncselector* ***n***);**

**bool ncselector_offer_input(struct ncselector* ***n***, const ncinput* ***nc***);**

**void ncselector_destroy(struct ncselector* ***n***, char** ***item***);**

# DESCRIPTION

A selector widget presents a list of items (possibly more than can be
displayed at once). It facilitates a choice of zero or one item from the
list.

# NOTES

The **ncplane** **n** provided to **ncselector_create** must not be **NULL**.
It will be freely resized by the new **ncselector**, and thus cannot be
the standard plane. **ncselector_selected** returns the currently-selected
option. **ncselector_additem** and **ncselector_delitem** allow items to be
added and deleted on the fly (a static set of items can all be provided in the
**ncselector_create** call). The backing plane will be resized as necessary for
item changes.

**ncselector_nextitem** and **ncselector_previtem** select the next (down) or
previous (up) option, scrolling if necessary. It is safe to call these
functions even if no options are present.

**ncselector_plane** will return the **ncplane** on which the widget is
drawn.

While the **ncselector** can be driven entirely by client code, input can
be run through **ncselector_offer_input** to take advantage of common
controls. It will handle the up and down arrows, along with PageUp and
PageDown. If the mouse is enabled, the mouse scrollwheel and mouse clicks
on the scroll arrows will be handled.

**ncselector_destroy** destroys the backing **ncplane**, as does
**ncselector_create** in the event of any error.

As no flags exist, the **flags** option ought always be 0.

# RETURN VALUES

**ncselector_create** returns **NULL** on an error, in which case the passed
**ncplane** is destroyed.

**ncselector_selected** returns a reference to the **option** part of the
selected **ncselector_item**. If there are no items, it returns **NULL**.

**ncselector_previtem** and **ncselector_nextitem** return references to the
**option** part of the newly-selected **ncselector_item**. If there are no
items, they return **NULL**.

# SEE ALSO

**notcurses(3)**,
**notcurses_input(3)**,
**notcurses_multiselector(3)**,
**notcurses_plane(3)**,
**notcurses_stdplane(3)**
