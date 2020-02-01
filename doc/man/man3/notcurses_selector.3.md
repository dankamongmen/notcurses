% notcurses_selector(3)
% nick black <nickblack@linux.com>
% v1.1.3

# NAME

notcurses_selector - high level widget for selecting from a set

# SYNOPSIS

**#include <notcurses.h>**

```c
struct selector_item {
  char* option;
  char* desc;
};

typedef struct selector_options {
  char* title; // title may be NULL, inhibiting riser
  char* secondary; // secondary may be NULL
  char* footer; // footer may be NULL
  struct selector_item* items; // initial items and descriptions
  unsigned itemcount; // number of initial items and descriptions
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
} selector_options;
```

**struct ncselector;**

**struct ncselector* ncselector_create(struct ncplane* n, int y, int x, const selector_options* opts);**

**struct ncselector* ncselector_aligned(struct ncplane* n, int y, ncalign_e align, const selector_options* opts);**

**int ncselector_additem(struct ncselector* n, const struct selector_item* item);**

**int ncselector_delitem(struct ncselector* n, const char* item);**

**char* ncselector_selected(const struct ncselector* n);**

**struct ncplane* ncselector_plane(struct ncselector* n);**

**void ncselector_previtem(struct ncselector* n, char\*\* newitem);**

**void ncselector_nextitem(struct ncselector* n, char\*\* newitem);**

**void ncselector_destroy(struct ncselector* n, char\*\* item);**

# DESCRIPTION

# NOTES

# RETURN VALUES

# SEE ALSO

**notcurses(3)**, **notcurses_ncplane(3)**
