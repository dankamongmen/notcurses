% notcurses_core(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses_core - minimal Notcurses linkage

# SYNOPSIS

**#include <notcurses/notcurses.h>**

**-lnotcurses-core**

**struct notcurses* notcurses_core_init(const notcurses_options* ***opts***, FILE* ***fp***);**

**struct ncdirect* ncdirect_core_init(const char* ***termtype***, FILE* ***fp***, uint64_t ***flags***);**

# DESCRIPTION

If your binary has no use for the multimedia capabilities of Notcurses,
consider linking directly to **libnotcurses-core** rather than
**libnotcurses**. This ought greatly reduce the dependency burden of
Notcurses.

If using **libnotcurses-core**, **notcurses_core_init** must be
used in the place of **notcurses_init**, and **ncdirect_core_init** must
be used in the place of **ncdirect_init**. Failure to do will usually
result in an error during linking. At worst, you'll end up with the
unnecessary dependencies in your binary after all.

# BUGS

This all ought be handled by the toolchain. It's stupid for users to have
to think about this.

# NOTES

If Notcurses was built with **USE_MULTIMEDIA=none**, **libnotcurses** will
have no multimedia dependencies, and thus this won't save anything. It's
still best to explicitly use **libnotcurses-core** when appropriate, to
avoid picking up the dependency chain on systems where it *is* being used.

# RETURN VALUES

The same as **notcurses_init** and **ncdirect_init**.

# SEE ALSO

**notcurses(3)**,
**notcurses_direct(3)**,
**notcurses_init(3)**,
**utf8(7)**
