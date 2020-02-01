% notcurses_resize(3)
% nick black <nickblack@linux.com>
% v1.1.3

# NAME

notcurses_resize - resizeialize a notcurses instance

# SYNOPSIS

**#include <notcurses.h>**

**int notcurses_resize(const struct notcurses* nc, int* rows, int* cols);**

# DESCRIPTION

**notcurses_resize** causes notcurses to retrieve the screen's current size
using **termios(3)**. If it has changed, notcurses will resize the standard
plane appropriately. Like any other **ncplane_resize(3)** operation, lost
space will be culled from the standard plane, while new space will be
populated by the standard plane's base cell. Other planes are unaffected.
**notcurses_resize** does *not* result in a rendering operation.

**notcurses_render(3)** calls this function following a render+raster cycle. It
is thus not necessary to call it yourself unless both of the following are
true:

* Your program is in an event loop rather than a rendering loop (i.e. it calls **notcurses_render(3)** only based on external events), and
* Your program makes use of the standard plane, or the standard plane's dimensions.

If this is the case, you might call **notcurses_resize** based on an
**NCKEY_RESIZE** event on the input channel (see **notcurses_input(3)**),
so that you can write to the standard plane using its new size prior to a
render. If you have no changes, and just want to render what you have (with
more or less now visible), it is sufficient to simply call **notcurses_render(3)**.

If **rows** and/or **cols** is not NULL, they receive the new geometry.

# NOTES

If your program **is** in a render loop (i.e. rendering as quickly as
possible, or at least at the refresh rate), there's not much point in
erecting the machinery to trigger **notcurses_resize** based off
**NCKEY_RESIZE**. The latter is generated based upon receipt of the **SIGWINCH**
signal, which is fundamentally racy with regards to the rest of the program.
If your program truly relies on timely invocation of **notcurses_resize()**,
it's a broken program. If you don't rely on it in a causal fashion, then just
wait for the upcoming render.

Highest performance in a rendering loop would actually call for disabling
notcurses SIGWINCH handling in the call to **notcurses_init(3)**, so that no
time is spent handling a signal you're not going to use.

# RETURN VALUES

Returns 0 on success, and -1 on failure. The causes for failure include system
error, programming error, closing of output, or allocation failure. None
of these are particularly good things, and the most reasonable response to a
**notcurses_resize** failure is probably to weep.

# SEE ALSO

**notcurses_init(3)**, **notcurses_input(3)**, **notcurses_ncplane(3)**,
**notcurses_render(3)**, **termios(3)**, **signal(7)**
