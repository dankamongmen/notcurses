% notcurses_reel(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses_reel - high-level widget for hierarchical data

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
#define NCREEL_OPTION_INFINITESCROLL 0x0001
#define NCREEL_OPTION_CIRCULAR       0x0002

struct ncreel;
struct ncplane;
struct nctablet;

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
  uint64_t flags;      // bitfield over NCREEL_OPTION_*
} ncreel_options;
```

**struct ncreel* ncreel_create(struct ncplane* ***nc***, const ncreel_options* ***popts***);**

**struct ncplane* ncreel_plane(struct ncreel* ***nr***);**

**typedef int (*tabletcb)(struct nctablet* ***t***, bool ***cliptop***);**

**struct nctablet* ncreel_add(struct ncreel* ***nr***, struct nctablet* ***after***, struct nctablet* ***before***, tabletcb ***cb***, void* ***opaque***);**

**int ncreel_tabletcount(const struct ncreel* ***nr***);**

**int ncreel_del(struct ncreel* ***nr***, struct nctablet* ***t***);**

**int ncreel_redraw(struct ncreel* ***nr***);**

**struct nctablet* ncreel_focused(struct ncreel* ***nr***);**

**struct nctablet* ncreel_next(struct ncreel* ***nr***);**

**struct nctablet* ncreel_prev(struct ncreel* ***nr***);**

**bool ncreel_offer_input(struct ncreel* ***nr***, const ncinput* ***ni***);**

**void ncreel_destroy(struct ncreel* ***nr***);**

**void* nctablet_userptr(struct nctablet* ***t***);**

**struct ncplane* nctablet_plane(struct nctablet* ***t***);**

# DESCRIPTION

An **ncreel** is a widget for display and manipulation of hierarchical data,
intended to make effective use of the display area while supporting keyboards,
mice, and haptic interfaces. A series of **nctablet**s are ordered on a
virtual cylinder; the tablets can grow and shrink freely. Moving among the
tablets "spins" the cylinder. **ncreel**s support optional borders around
the reel and/or tablets.

**ncreel_redraw** arranges the tablets, invoking the **tabletcb** defined by
each. It will invoke the callbacks of only those tablets currently visible.
This function ought be called whenever the data within a tablet need be
refreshed. The return value of this callback is the number of lines drawn into
the **ncplane**. The tablet will be grown or shrunk as necessary to reflect
this return value.

Unless the reel is devoid of tablets, there is always a "focused" tablet (the
first tablet added to an empty reel becomes focused). The focused tablet can
change via **ncreel_next** and **ncreel_prev**. If **ncreel_del** is called on
the focused tablet, and at least one other tablet remains, some tablet receives
the focus.

Calling functions which change the reel, including **ncreel_next**,
**ncreel_prev**, **ncreel_del**, and **ncreel_add**, implicitly calls
**ncreel_redraw**. This behavior **must not be relied upon**, as it is likely
to go away.

## LAYOUT

When the user invokes **ncreel_redraw**, Notcurses can't assume it knows the
size of any tablets--one or more might have changed since the last draw. Only
after a callback does **ncreel_redraw** know how many rows a tablet will
occupy.

A redraw operation starts with the focused tablet. Its callback is invoked with
a plane as large as the reel, i.e. the focused tablet can occupy the entire
reel, to the exclusion of any other tablets. The focused tablet will be kept
where it was, if possible; growth might force it to move. There is now one
tablet locked into place, and zero, one, or two areas of empty space. Tablets
are successively lain out in these spaces until the reel is filled.

In general, if the reel is not full, tablets will be drawn towards the top, but
this ought not be relied on.

## THE TABLET CALLBACK

The tablet callback (of type **tabletcb**) is called with an **ncplane** and a
**bool**. The callback function ought not rely on the absolute position of the
plane, as it might be moved. The **bool** indicates whether the plane ought be
filled in from the bottom, or from the top (this is only meaningful if the
plane is insufficiently large to contain all the tablet's available data). The
callback ought not resize the plane (it will be resized following return). The
callback must return the number of rows used (it is perfectly valid to use zero
rows), or a negative number if there was an error.

Returning more rows than the plane has available is an error.

# RETURN VALUES

**ncreel_focused**, **ncreel_prev**, and **ncreel_next** all return the focused
tablet, unless no tablets exist, in which case they return **NULL**.

**ncreel_add** returns the newly-added **nctablet**.

# BUGS

I can't decide whether to require the user to explicitly call **ncreel_redraw**.
Doing so means changes can be batched up without a redraw, but it also makes
things more complicated for both me and the user.

# SEE ALSO

**notcurses(3)**,
**notcurses_input(3)**,
**notcurses_plane(3)**
