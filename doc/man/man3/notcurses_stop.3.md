% notcurses_stop(3)
% nick black <nickblack@linux.com>
% v1.4.2.2

# NAME

notcurses_stop - free up resources and restore initial terminal state

# SYNOPSIS

**#include <notcurses/notcurses.h>**

**int notcurses_stop(struct notcurses* nc);**

# DESCRIPTION

**notcurses_stop** frees up any resources associated with the
**struct notcurses** provided as **nc**, and attempts to restore the terminal to its
state prior to calling notcurses_init(3). It also unregisters any signal
handlers put into place by notcurses_init(3). **nc** must not be used following
the call, and all references to ncplanes, cells, etc. are invalidated.

# NOTES

Behavior is undefined if other threads are working with **nc** when or after
this function is called. It is unlikely to be good.

# RETURN VALUES

On success, 0 is returned. Otherwise, a negative value is returned.

# SEE ALSO

**notcurses(3)**, **notcurses_init(3)**
