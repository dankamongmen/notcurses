% notcurses_tree(3)
% nick black <nickblack@linux.com>
% v2.2.1

# NAME

notcurses_tree - high-level hierarchical line-based data

# SYNOPSIS

**#include <notcurses/notcurses.h>**

# DESCRIPTION

**nctree**s organize static hierarchical items, and allow them to be browsed.
Each item can have arbitrary subitems. Items can be collapsed and expanded.
The display supports scrolling and searching. Items cannot be added or removed,
however; they must be provided in their entirety at creation time.

# RETURN VALUES

# NOTES

**nctree** shares many properties with **notcurses_reel**. Unlike the latter,
**nctree**s support arbitrary hierarchical levels, but they do not allow
elements to come and go across the lifetime of the widget.

# SEE ALSO

**notcurses(3)**,
**notcurses_input(3)**,
**notcurses_plane(3)**,
**notcurses_reel(3)**
