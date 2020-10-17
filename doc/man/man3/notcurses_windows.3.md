% notcurses_windows(3)
% nick black <nickblack@linux.com>
% v2.0.0

# NAME
notcurses_windows - interaction with windowing environments

# SYNOPSIS

**#include <notcurses/notcurses.h>**

**char* notcurses_title(const struct notcurses* nc);**

**int notcurses_set_title(struct notcurses* nc, const char* title);**

# DESCRIPTION

When used within a windowing environment (currently only the X.Org server is
supported) and a terminal emulator supporting the relevant OSC escape
sequences, Notcurses provides functionality related to managing its own
window's properties. They are strictly best-effort, and a successful return
does not necessarily mean that the property change was effected.

# RETURN VALUES

The string returned by **notcurses_title** is heap-allocated and ought be
freed by the caller. It is encoded in UTF-8.

**notcurses_set_title** returns 0 on success, but this indicates only that the
escape was written to the terminal.

If run outside a windowing environment, these functions always return failure.

# NOTES

Not all terminals support the OSC sequences used to retrieve and set window
properties (see **console_codes(4)**).

# BUGS

# SEE ALSO

**notcurses(3)**,
**console_codes(4)**
