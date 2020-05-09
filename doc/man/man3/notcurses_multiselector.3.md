% notcurses_multiselector(3)
% nick black <nickblack@linux.com>
% v1.3.4

# NAME

notcurses_multiselector - high level widget for selecting from a set

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
struct ncinput;
struct ncplane;
struct notcurses;
struct ncmultiselector;

struct ncmselector_item {
  char* option;
  char* desc;
  bool selected;
};

typedef struct ncmultiselector_options {
  char* title; // title may be NULL, inhibiting riser
  char* secondary; // secondary may be NULL
  char* footer; // footer may be NULL
  struct ncmselector_item* items; // initial items, statuses
  unsigned itemcount; // number of initial items
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
  uint64_t bgchannels;   // background channels for body
} ncmultiselector_options;
```

**struct ncmultiselector* ncmultiselector_create(struct notcurses* n, int y, int x, const ncmultiselector_options* opts);**

**int ncselector_selected(bool* selected, unsigned n);**

**struct ncplane* ncmultiselector_plane(struct ncmultiselector* n);**

**bool ncmultiselector_offer_input(struct ncmultiselector* n, const struct ncinput* nc);**

**void ncmultiselector_destroy(struct ncmultiselector* n, char** item);**

# DESCRIPTION

# NOTES

# RETURN VALUES

# SEE ALSO

**notcurses(3)**,
**notcurses_input(3)**,
**notcurses_plane(3)**
**notcurses_selector(3)**
