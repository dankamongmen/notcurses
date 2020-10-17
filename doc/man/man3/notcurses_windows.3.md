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
supported), Notcurses provides functionality related to managing its own window
properties. These functions use the xcb asynchronous protocol to communicate
with the server, and thus ought not block. They are strictly best-effort, and
a successful return does not necessarily mean that the property change was
effected.

When compiled with xcb support, **notcurses_init** will check for the
**DISPLAY** environment variable. If this variable is defined, it will attempt
to establish a connection to the specified X.Org server.

# RETURN VALUES

The string returned by **notcurses_title** is heap-allocated and ought be
freed by the caller. It is encoded in UTF-8.

**notcurses_set_title** returns 0 on success, but this indicates only that the
request was dispatched.

If run outside a windowing environment, or without an active connection, or
without xcb support in Notcurses, these functions always return failure.

# NOTES

The **NCOPTION_NO_XCB_CONNECT** flag can be provided to **notcurses_init** to
inhibit the xcb connection. In any case, **notcurses_init** always ignores a
failure to connect (a diagnostic might be printed).

Use of this functionality might conflict with the terminal emulator in
unpredictable ways.

# BUGS

# SEE ALSO

**Xorg(1)**,
**notcurses(3)**,
**notcurses_init(3)**,
**xcb_change_property(3)**
