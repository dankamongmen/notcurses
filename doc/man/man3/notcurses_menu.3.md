% notcurses_menu(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses_menu - operations on ncmenu objects

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
struct ncmenu;
struct ncplane;
struct notcurses;

struct ncmenu_section {
  const char* name;   // utf-8 c string
  struct ncmenu_item {
    const char* desc; // utf-8 menu item, NULL for horizontal separator
    ncinput shortcut; // shortcut, all should be distinct
  }* items;
  int itemcount;
};

#define NCMENU_OPTION_BOTTOM 0x0001 // bottom row (as opposed to top row)
#define NCMENU_OPTION_HIDING 0x0002 // hide the menu when not unrolled

typedef struct ncmenu_options {
  struct ncmenu_section* sections; // 'sectioncount' menu_sections
  int sectioncount;         // must be positive
  uint64_t headerchannels;  // styling for header
  uint64_t sectionchannels; // styling for sections
  uint64_t flags;           // bitfield on NCMENU_OPTION_*
} ncmenu_options;
```

**struct ncmenu* ncmenu_create(struct notcurses* ***nc***, const menu_options* ***opts***);**

**int ncmenu_unroll(struct ncmenu* ***n***, int ***sectionidx***);**

**int ncmenu_rollup(struct ncmenu* ***n***);**

**int ncmenu_nextsection(struct ncmenu* ***n***);**

**int ncmenu_prevsection(struct ncmenu* ***n***);**

**int ncmenu_nextitem(struct ncmenu* ***n***);**

**int ncmenu_previtem(struct ncmenu* ***n***);**

**int ncmenu_item_set_status(struct ncmenu* ***n***, const char* ***section***, const char* ***item***, bool ***enabled***);**

**const char* ncmenu_selected(const struct ncmenu* ***n***, ncinput* ***ni***);**

**const char* ncmenu_mouse_selected(const struct ncmenu* ***n***, const ncinput* ***click***, ncinput* ***ni***);**

**struct ncplane* ncmenu_plane(struct ncmenu* ***n***);**

**bool ncmenu_offer_input(struct ncmenu* ***n***, const ncinput* ***nc***);**

**void ncmenu_destroy(struct ncmenu* ***n***);**

# DESCRIPTION

A notcurses instance supports menu bars on the top or bottom row of a plane.
A menu is composed of sections, which are in turn composed of items. Either no
sections are visible, and the menu is *rolled up*, or exactly one section is
*unrolled*. **ncmenu_rollup** places an ncmenu in the rolled up state.
**ncmenu_unroll** rolls up any unrolled section, and unrolls the specified one.
**ncmenu_destroy** removes a menu bar, and frees all associated resources.

**ncmenu_selected** return the selected item description, or NULL if no section
is unrolled, or no valid item is selected. **ncmenu_mouse_selected** returns
the item selected by a mouse click (described in **click**), which must be on
the actively unrolled section. In either case, if there is a shortcut for the
item and **ni** is not **NULL**, **ni** will be filled in with the shortcut.

The menu can be driven either entirely by the application, via direct calls to
**ncmenu_previtem**, **ncmenu_prevsection**, and the like, or **ncinput**
objects can be handed to **ncmenu_offer_input**. In the latter case, the menu
will largely manage itself. The application must handle item selection (usually
via the Enter key and/or mouse click) itself, since the menu cannot arbitrarily
call into the application. **ncmenu_offer_input** will handle clicks to unroll
menu sections, clicks to dismiss the menu, Escape to dismiss the menu, left
and right to change menu sections, and up and down to change menu items (these
latter are only consumed if there is an actively unrolled section).

Items can be disabled with **ncmenu_item_set_status**. Pass **false** to
disable the item, or **true** to enable it. All items are enabled by
default. A disabled item will not be returned with **ncmenu_mouse_selected**,
nor reached with **ncmenu_nextsection** nor **ncmenu_prevsection**.

# RETURN VALUES

**ncmenu_create** returns **NULL** on error, or a pointer to a valid new ncmenu.
Other functions return non-zero on error, or zero on success. Almost all errors
are due to invalid parameters.

**ncmenu_offer_input** returns **true** if the menu "consumed" the input, i.e.
found it relevant and took an action. Otherwise, **false** is returned, and the
**ncinput** should be considered irrelevant to the menu.

# SEE ALSO

**notcurses(3)**,
**notcurses_input(3)**,
**notcurses_plane(3)**
