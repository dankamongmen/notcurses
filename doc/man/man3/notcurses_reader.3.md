% notcurses_reader(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses_reader - high level widget for collecting input

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
struct ncplane;
struct ncreader;
struct notcurses;

#define NCREADER_OPTION_HORSCROLL 0x0001
#define NCREADER_OPTION_VERSCROLL 0x0002
#define NCREADER_OPTION_NOCMDKEYS 0x0004
#define NCREADER_OPTION_CURSOR    0x0008

typedef struct ncreader_options {
  uint64_t tchannels; // channels used for input
  uint32_t tattrword; // attributes used for input
  uint64_t flags;     // bitfield over NCREADER_OPTION_*
} ncreader_options;
```

**struct ncreader* ncreader_create(struct notcurses* ***nc***, const ncreader_options* ***opts***);**

**int ncreader_clear(struct ncreader* ***n***);**

**struct ncplane* ncreader_plane(struct ncreader* ***n***);**

**int ncreader_move_left(struct ncreader* ***n***);**

**int ncreader_move_right(struct ncreader* ***n***);**

**int ncreader_move_up(struct ncreader* ***n***);**

**int ncreader_move_down(struct ncreader* ***n***);**

**int ncreader_write_egc(struct ncreader* ***n***, const char* ***egc***);**

**bool ncreader_offer_input(struct ncreader* ***n***, const ncinput* ***ni***);**

**char* ncreader_contents(const struct ncreader* ***n***);**

**void ncreader_destroy(struct ncreader* ***n***, char** ***contents***);**

# DESCRIPTION

The **ncreader** widget supports free-form, multi-line input. It supports
navigation with the arrow keys and scrolling. While the visible portion of
the **ncreader** is always the same size (defined by the provided **ncplane**),
the actual text backing this visible region can grow arbitrarily large if
scrolling is enabled.

The following option flags are supported:

* **NCREADER_OPTION_HORSCROLL**: The visual area will be backed by a plane
     that can grow arbitrarily wide.
* **NCREADER_OPTION_VERSCROLL**: The visual area will be backed by a plane
     that can grow arbitrarily high.
* **NCREADER_OPTION_NOCMDKEYS**: The typical keyboard shortcuts (see below)
     will not be honored.
* **NCREADER_OPTION_CURSOR**: The terminal's cursor will be made visible across
     the life of the **ncreader**, and its location will be managed.

The contents of the **ncreader** can be retrieved with **ncreader_contents**.

The **ncreader** consists of at least one **ncplane** (the visible editing
area). If the **ncreader** supports scrolling, it will consist of two
**ncplanes**, one of which will be kept outside the rendering area (and will
thus be invisible). **ncreader_plane** always returns the visible plane.

**ncreader_clear** drops all input from the **ncreader**, restoring it to
the same pristine condition in which it was returned by **ncreader_create**.

Unlike most widgets' input handlers, **ncreader_offer_input** will consume most
inputs. The arrow keys navigate. Backspace consumes the EGC to the left of the
cursor, if one exists. Enter moves to the first column of the next line (if
the cursor is already on the bottom line, the plane must have scrolling
enabled, or the cursor will not advance, though the Enter will still be
consumed). Otherwise, most inputs will be reproduced onto the **ncreader**
(though see NOTES below).

All the **ncreader**'s content is returned from **ncreader_contents**,
preserving whitespace.

"Emacs-style" keyboard shortcuts similar to those supported by **readline(3)**
and most shells are supported unless **NCREADER_OPTION_NOCMDKEYS** is provided
to **ncreader_create**.

# NOTES

Support for **NCREADER_OPTION_VERSCROLL** is not yet implemented.

**ncreader** does not buffer inputs in order to assemble EGCs from them. If
inputs are to be processed as EGCs (as they should), the caller would need
assemble the grapheme clusters. Of course, there is not yet any API through
which multi-codepoint EGCs could be supplied to **ncreader**.

There is not yet any support for word-wrapping. This will likely come
in the future.

# RETURN VALUES

# SEE ALSO

**notcurses(3)**,
**notcurses_input(3)**,
**notcurses_plane(3)**,
**readline(3)**
