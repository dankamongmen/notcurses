% notcurses_progbar(3)
% nick black <nickblack@linux.com>
% v2.0.11

# NAME

notcurses_progbar - high level widget for progress bars

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
struct ncprogbar;

#define NCPROGBAR_OPTION_RETROGRADE        0x0001u // proceed left/down

typedef struct ncprogbar_options {
  uint32_t ulchannel; // upper-left channel. in the context of a progress bar,
  uint32_t urchannel; // "up" is the direction we are progressing towards, and
  uint32_t blchannel; // "bottom" is the direction of origin. for monochromatic
  uint32_t brchannel; // bar, all four channels ought be the same.
  uint64_t flags;
} ncprogbar_options;
```

**struct ncprogbar* ncprogbar_create(struct ncplane* ***n***, const ncprogbar_options* ***opts***)**

**struct ncplane* ncprogbar_plane(struct ncprogbar* ***n***)**

**int ncprogbar_set_progress(struct ncprogbar* ***n***, double ***p***)**

**double ncprogbar_progress(const struct ncprogbar* ***n***)**

**void ncprogbar_destroy(struct ncprogbar* ***n***)**

# DESCRIPTION

These functions draw progress bars in any of four directions. The progress
measure is a **double** between zero and one, inclusive, provided to
**ncprogbar_set_progress**. This will be scaled to the size of the provided
ncplane ***n***. The axis of progression is the longer element of the plane's
geometry. Horizontal bars proceed to the right by default, and vertical bars
proceed up. This can be changed with **NCPROGBAR_OPTION_RETROGRADE**.

# NOTES

**ncprogbar_create** takes ownership of ***n*** in all cases. On failure,
***n*** will be destroyed immediately. It is otherwise destroyed by
**ncprogbar_destroy**.

# RETURN VALUES

**ncprogbar_plane** returns the **ncplane** on which the progress bar is drawn.
**ncprogbar_progress** returns the current progress, a value between zero and
one, inclusive. They cannot fail.

**ncprogbar_set_progress** returns -1 if ***p*** is less than zero or greater
than one, or if there is an internal error redrawing the progress bar. It
returns 0 otherwise.

# BUGS

Whether progression is to the left or right by default probably ought be an
aspect of the current locale.

# SEE ALSO

**notcurses(3)**,
**notcurses_plane(3)**,
**notcurses_visual(3)**
