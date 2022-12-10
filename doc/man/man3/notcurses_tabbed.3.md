% notcurses_tabbed(3)
% v3.0.9

# NAME

notcurses_tabbed – tabbed interface widget

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
#define NCTABBED_OPTION_BOTTOM 0x0001

struct nctabbed;
struct ncplane;
struct nctab;

typedef struct nctabbed_options {
  uint64_t selchan; // channel for the selected tab header
  uint64_t hdrchan; // channel for unselected tab headers
  uint64_t sepchan; // channel for the tab separator
  const char* separator; // separator string (copied by nctabbed_create())
  uint64_t flags;   // bitmask of NCTABBED_OPTION_*
} nctabbed_options;

typedef void (*tabcb)(struct nctab* t, struct ncplane* ncp, void* userptr);
```

**struct nctabbed* nctabbed_create(struct ncplane* ***ncp***, const nctabbed_options* ***opts***);**

**void nctabbed_destroy(struct nctabbed* ***nt***);**

**void nctabbed_redraw(struct nctabbed* ***nt***);**

**void nctabbed_ensure_selected_header_visible(struct nctabbed* ***nt***);**

**struct nctab* nctabbed_selected(struct nctabbed* ***nt***);**

**struct nctab* nctabbed_leftmost(struct nctabbed* ***nt***);**

**int nctabbed_tabcount(struct nctabbed* ***nt***);**

**struct ncplane* nctabbed_plane(struct nctabbed* ***nt***);**

**struct ncplane* nctabbed_content_plane(struct nctabbed* ***nt***);**

**tabcb nctab_cb(struct nctab* ***t***);**

**const char* nctab_name(struct nctab* ***t***);**

**int nctab_name_width(struct nctab* ***t***);**

**void* nctab_userptr(struct nctab* ***t***);**

**struct nctab* nctab_next(struct nctab* ***t***);**

**struct nctab* nctab_prev(struct nctab* ***t***);**

**struct nctab* nctabbed_add(struct nctabbed* ***nt***, struct nctab* ***after***, struct nctab* ***before***, tabcb ***tcb***, const char* ***name***, void* ***opaque***);

**int nctabbed_del(struct nctabbed* ***nt***, struct nctab* ***t***);**

**int nctab_move(struct nctabbed* ***nt***, struct nctab* ***t***, struct nctab* ***after***,
                   struct nctab* ***before***);**

**void nctab_move_right(struct nctabbed* ***nt***, struct nctab* ***t***);**

**void nctab_move_left(struct nctabbed* ***nt***, struct nctab* ***t***);**

**void nctabbed_rotate(struct nctabbed* ***nt***, int ***amt***);**

**struct nctab* nctabbed_next(struct nctabbed* ***nt***);**

**struct nctab* nctabbed_prev(struct nctabbed* ***nt***);**

**struct nctab* nctabbed_select(struct nctabbed* ***nt***, struct nctab* ***t***);**

**void nctabbed_channels(struct nctabbed* ***nt***, uint64_t* RESTRICT ***hdrchan***, uint64_t* RESTRICT ***selchan***, uint64_t* RESTRICT ***sepchan***);**

**uint64_t nctabbed_hdrchan(struct nctabbed* ***nt***);**

**uint64_t nctabbed_selchan(struct nctabbed* ***nt***);**

**uint64_t nctabbed_sepchan(struct nctabbed* ***nt***);**

**const char* nctabbed_separator(struct nctabbed* ***nt***);**

**int nctabbed_separator_width(struct nctabbed* ***nt***);**

**void nctabbed_set_hdrchan(struct nctabbed* ***nt***, uint64_t ***chan***);**

**void nctabbed_set_selchan(struct nctabbed* ***nt***, uint64_t ***chan***);**

**void nctabbed_set_sepchan(struct nctabbed* ***nt***, uint64_t ***chan***);**

**tabcb nctab_set_cb(struct nctab* ***t***, tabcb ***newcb***);**

**int nctab_set_name(struct nctab* ***t***, const char* ***newname***);**

**void* nctab_set_userptr(struct nctab* ***t***, void* ***newopaque***);**

**int nctabbed_set_separator(struct nctabbed* ***nt***, const char* ***separator***);**

# DESCRIPTION

An **nctabbed** is a widget with one area for data display and a bar with
names of tabs. Unless there are no tabs, exactly one tab is "selected", and
exactly one tab is "leftmost". The selected tab is the one that controls
the tab content plane. The leftmost tab is the one of which the header is
visible furthest to the left. Any tab can be moved to and from anywhere in the
list. The tabs can be "rotated", which really means the leftmost tab gets
shifted. The widget is drawn only when **nctabbed_redraw** or **ntabbed_create**
are called.

## LAYOUT

The widget has a tab list either on the top or the bottom, 1 row thick. The tab
list contains tab headers (i.e. their names), separated with the separator
specified in **nctabbed_create** or **nctabbed_set_separator**. The channels
for the selected tab's header, other tab headers and the separator can be
set independently of each other. The tab separator can be 0-length, or NULL,
in which case there is no visible separator between tab headers. The selected
tab can be made sure to be visible when drawn (by changing the leftmost tab)
by calling the very long-named **nctabbed_ensure_selected_header_visible**.
The rest of the widget is an **ncplane** housing the selected tab content. (if any)

## THE TAB CALLBACK

The tab callback (of type **tabcb**) takes a tab, the tab content plane, and
the opaque pointer given to **nctabbed_add** or **nctabbed_set_userptr**.
It is called when the tab content is supposed to be drawn, that is when
the whole widget is redrawn. It should draw the tab content and possibly
make other actions, but it should not assume anything about the current state
of the tab content plane, nor should it modify the widget's or the tab's state.

# RETURN VALUES

**nctabbed_create** returns the newly created widget, or **NULL** when the widget
failed to be created. This destroys the **ncplane** given to it even if it fails.

**nctabbed_selected** and **nctabbed_leftmost** return the selected and
leftmost tabs, respectively. If there are no tabs, these return **NULL**.

**nctab_name** returns the tab's name. This is not a copy, and it should not be
stored, since it is freed when the tab's name is changed or the tab is deleted.

**nctabbed_next**, **nctabbed_prev** and **nctabbed_select** return the newly
selected tab.

**nctabbed_separator** returns the tab separator. This is not a copy, and it
should not be stored, since it is freed when the separator is changed or the
widget is deleted.

Functions returning **int** return **-1** on failure.

# SEE ALSO

**notcurses(3)**,
**notcurses_channels(3)**,
**notcurses_plane(3)**
