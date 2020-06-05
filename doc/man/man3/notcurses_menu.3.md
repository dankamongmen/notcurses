% notcurses_menu(3)
% nick black <nickblack@linux.com>
% v1.4.5

# NAME

notcurses_menu - operations on menus

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
struct ncmenu;
struct ncplane;
struct ncinput;
struct notcurses;

struct ncmenu_section {
  char* name;  // utf-8 c string
  struct ncmenu_item {
    char* desc;  // utf-8 menu item, NULL for horizontal separator
    ncinput shortcut;  // shortcut, all should be distinct
  }* items;
  int itemcount;
};

#define NCMENU_OPTION_BOTTOM 0x0001 // bottom row (as opposed to top row)
#define NCMENU_OPTION_HIDING 0x0002 // hide the menu when not being used

typedef struct ncmenu_options {
  struct ncmenu_section* sections; // 'sectioncount' menu_sections
  int sectioncount;         // must be positive
  uint64_t headerchannels;  // styling for header
  uint64_t sectionchannels; // styling for sections
  unsigned flags;           // bitfield on NCMENU_OPTION_*
} ncmenu_options;
```

**struct ncmenu* ncmenu_create(struct notcurses* nc, const menu_options* opts);**

**int ncmenu_unroll(struct ncmenu* n, int sectionidx);**

**int ncmenu_rollup(struct ncmenu* n);**

**int ncmenu_nextsection(struct ncmenu* n);**

**int ncmenu_prevsection(struct ncmenu* n);**

**int ncmenu_nextitem(struct ncmenu* n);**

**int ncmenu_previtem(struct ncmenu* n);**

**const char* ncmenu_selected(const struct ncmenu* n, struct ncinput* ni);**

**struct ncplane* ncmenu_plane(struct ncmenu* n);**

**bool ncmenu_offer_input(struct ncmenu* n, const struct ncinput* nc);**

**int ncmenu_destroy(struct ncmenu* n);**

# DESCRIPTION

A notcurses instance supports menu bars on the top or bottom row of the true
screen. A menu is composed of sections, which are in turn composed of items.
Either no sections are visible, and the menu is *rolled up*, or exactly one
section is *unrolled*. **ncmenu_rollup** places an ncmenu in the rolled up
state. **ncmenu_unroll** rolls up any unrolled section, and unrolls the
specified one. **ncmenu_destroy** removes a menu bar, and frees all associated
resources.

**ncmenu_selected** return the selected item description, or NULL if no section
is unrolled.

The menu can be driven either entirely by the application, via direct calls to
**ncmenu_previtem**, **ncmenu_prevsection**, and the like, or **struct ncinput**
objects can be handed to **ncmenu_offer_input**. In the latter case, the menu
will largely manage itself.

# RETURN VALUES

**ncmenu_create** returns **NULL** on error, or a pointer to a valid new ncmenu.
Other functions return non-zero on error, or zero on success. Almost all errors
are due to invalid parameters.

**ncmenu_offer_input** returns **true** if the menu "consumed" the input, i.e.
found it relevant and took an action. Otherwise, **false** is returned, and the
**struct ncinput** should be considered irrelevant to the menu.

# SEE ALSO

**notcurses(3)**,
**notcurses_input(3)**,
**notcurses_plane(3)**
