% notcurses_multiselector(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses_multiselector - high level widget for selecting from a set

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
struct ncplane;
struct notcurses;
struct ncmultiselector;

struct ncmselector_item {
  const char* option;
  const char* desc;
};

typedef struct ncmultiselector_options {
  const char* title; // title may be NULL, inhibiting riser
  const char* secondary; // secondary may be NULL
  const char* footer; // footer may be NULL
  struct ncmselector_item* items; // initial items, statuses
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
  uint64_t flags;        // bitfield over NCMULTISELECTOR_OPTION_*
} ncmultiselector_options;
```

**struct ncmultiselector* ncmultiselector_create(struct ncplane* ***n***, const ncmultiselector_options* ***opts***);**

**int ncmultiselector_selected(bool* ***selected***, unsigned ***n***);**

**struct ncplane* ncmultiselector_plane(struct ncmultiselector* ***n***);**

**bool ncmultiselector_offer_input(struct ncmultiselector* ***n***, const ncinput* ***nc***);**

**void ncmultiselector_destroy(struct ncmultiselector* ***n***);**

# DESCRIPTION

A multiselector widget presents a list of items (possibly more than can be
displayed at once). It facilitates a choice of zero, one, or multiple items
from the list. Items can be selected and deselected before a final choice
is made.

# NOTES

The **ncplane** **n** provided to **ncmultiselector_create** must not be
**NULL**. It will be freely resized by the new **ncmultiselector**.

**ncmultiselector_selected** returns the index of the option currently
highlighted. It stores to the **n**-ary bitmap pointed to by **selected**
based on the currently-selected options.

**ncmultiselector_plane** will return the **ncplane** on which the widget is
drawn.

Input should be run through **ncmultiselector_offer_input** to take advantage
of common controls. It will handle the up and down arrows, along with PageUp
and PageDown. If the mouse is enabled, the mouse scrollwheel and mouse clicks
on the scroll arrows will be handled.

**ncmultiselector_destroy** destroys the backing **ncplane**, as does
**ncmultiselector_create** in the event of any error.

As no flags exist, the **flags** option ought always be 0.

# RETURN VALUES

**ncmultiselector_create** returns **NULL** on an error, in which case the
passed **ncplane** is destroyed.

**ncmultiselector_selected** returns -1 if there are no items, or if **n** is
not equal to the number of items. It otherwise returns the index of the
currently highlighted option, and writes a bitmap to **selected** based off
the selected items.

# SEE ALSO

**notcurses(3)**,
**notcurses_input(3)**,
**notcurses_plane(3)**
**notcurses_selector(3)**
