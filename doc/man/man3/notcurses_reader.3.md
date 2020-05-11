% notcurses_reader(3)
% nick black <nickblack@linux.com>
% v1.4.1

# NAME

notcurses_reader - high level widget for collecting input

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
struct ncinput;
struct ncplane;
struct ncreader;
struct notcurses;

#define NCREADER_OPTION_HORSCROLL  0x0001
#define NCREADER_OPTION_VERSCROLL  0x0002

typedef struct ncreader_options {
  uint64_t tchannels; // channels used for input
  uint64_t echannels; // channels used for empty space
  uint32_t tattrword; // attributes used for input
  uint32_t eattrword; // attributes used for empty space
  char* egc;          // egc used for empty space
  int physrows;
  int physcols;
  unsigned flags;     // bitfield over NCREADER_OPTIONS_*
} ncreader_options;
```

**struct ncreader* ncreader_create(struct notcurses* nc, int y, int x, const ncreader_options* opts);**

**int ncreader_clear(struct ncreader* n);**

**struct ncplane* ncreader_plane(struct ncreader* n);**

**bool ncreader_offer_input(struct ncreader* n, const struct ncinput* ni);**

**char* ncreader_contents(const struct ncreader* n);**

**void ncreader_destroy(struct ncreader* n, char** contents);**

# DESCRIPTION

# NOTES

# RETURN VALUES

# SEE ALSO

**notcurses(3)**,
**notcurses_input(3)**,
**notcurses_plane(3)**
