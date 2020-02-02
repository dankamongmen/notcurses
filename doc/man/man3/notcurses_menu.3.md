% notcurses_menu(3)
% nick black <nickblack@linux.com>
% v1.1.3

# NAME

notcurses_menu - operations on menus

# SYNOPSIS

**#include <notcurses.h>**

```c
struct ncmenu;

struct ncmenu_section {
  char* name;  // utf-8 c string
  struct ncmenu_item {
    char* desc;  // utf-8 menu item, NULL for horizontal separator
    ncinput shortcut;  // shortcut, all should be distinct
  }* items;
  int itemcount;
};

typedef struct ncmenu_options {
  bool bottom;  // on the bottom row, as opposed to top row
  bool hiding;  // hide the menu when not being used
  struct ncmenu_section* sections; // 'sectioncount' menu_sections
  int sectioncount;         // must be positive
  uint64_t headerchannels;  // styling for header
  uint64_t sectionchannels; // styling for sections
} ncmenu_options;
```

**struct ncmenu* ncmenu_create(struct notcurses* nc, const menu_options* opts);**

**int ncmenu_unroll(struct ncmenu* n, int sectionidx);**

**int ncmenu_rollup(struct ncmenu* n);**

**int ncmenu_destroy(struct ncmenu* n);**

# DESCRIPTION

A notcurses instance supports up to two menu bars, with one on the top and/or
bottom rows of the true screen. They are logically above other ncplanes on the
z-axis. Attempting to create a menu where one already exists is an error.

A menu is composed of sections, which are in turn composed of items. Either no
sections are visible, and the menu is *rolled up*, or exactly one section is
*unrolled*. **ncmenu_rollup** places an ncmenu in the rolled up state.
**ncmenu_unroll** rolls up any unrolled section, and unrolls the specified one.
**ncmenu_destroy** removes a menu bar, and frees all associated resources.

# RETURN VALUES

**ncmenu_create** returns NULL on error, or a pointer to a valid new ncmenu.
Other functions return non-zero on error, or zero on success. Almost all errors
are due to invalid parameters.

# SEE ALSO

**notcurses(3)**, **notcurses_input(3)**
