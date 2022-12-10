% notcurses_refresh(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses_refresh - redraw an externally-damaged display

# SYNOPSIS

**#include <notcurses/notcurses.h>**

**int notcurses_refresh(const struct notcurses* ***nc***, unsigned* restrict ***rows***, unsigned* restrict ***cols***);**

# DESCRIPTION

**notcurses_refresh** clears the screen, homes the cursor, checks the current
terminal geometry, and repaints the most recently rendered frame. It can be
called concurrently with all other Notcurses functions save
**notcurses_render**. **notcurses_refresh** ought be called when the screen is
externally damaged (as occurs when another program writes to the terminal, or
if your program writes to the terminal using standard I/O). It is necessary to
use **notcurses_refresh** in such a case (as opposed to simply calling
**notcurses_render**), since **notcurses_render** optimizes its output by only
writing internally-damaged cells. Notcurses has no way of knowing about
external corruption; by tradition, Ctrl+L is bound to **notcurses_refresh**,
and the user is responsible for requesting a hard redraw.

A secondary use of this function is when the program is blocking on input (and
perhaps not ready to render), and receives an **NCKEY_RESIZE** event (see
**notcurses_input**). In this case, **notcurses_refresh** will acquire the new
screen parameters, and repaint what it can. If you're prepared to call
**notcurses_render**, it's better to do that in this case, and thus avoid
unnecessary screen redrawing.

If **rows** and/or **cols** is not NULL, they receive the new geometry.

# NOTES

If your program **is** in a render loop (i.e. rendering as quickly as
possible, or at least at the refresh rate), there's not much point in
erecting the machinery to trigger **notcurses_refresh** based off
**NCKEY_RESIZE**. The latter is generated based upon receipt of the **SIGWINCH**
signal, which is fundamentally racy with regards to the rest of the program.
If your program truly relies on timely invocation of **notcurses_refresh**,
it's a broken program. If you don't rely on it in a causal fashion, then just
wait for the upcoming render.

Highest performance in a rendering loop would actually call for disabling
Notcurses's **SIGWINCH** handling in the call to **notcurses_init**, so that no
time is spent handling a signal you're not going to use.

Each time **notcurses_refresh** is successfully executed, the **refreshes**
stat is incremented by 1.

# RETURN VALUES

Returns 0 on success, and -1 on failure. The causes for failure include system
error, programming error, closing of output, or allocation failure. None
of these are particularly good things, and the most reasonable response to a
**notcurses_refresh** failure is either to ignore it, or to weep and exit.

# SEE ALSO

**notcurses_init(3)**,
**notcurses_input(3)**,
**notcurses_render(3)**,
**notcurses_stats(3)**,
**termios(3)**,
**signal(7)**
