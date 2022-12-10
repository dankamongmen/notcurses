% notcurses_stop(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses_stop - free up resources and restore initial terminal state

# SYNOPSIS

**#include <notcurses/notcurses.h>**

**int notcurses_stop(struct notcurses* ***nc***);**

# DESCRIPTION

**notcurses_stop** frees up any resources associated with the
**struct notcurses** provided as **nc**, and attempts to restore the terminal to its
state prior to calling **notcurses_init(3)**. It also unregisters any signal
handlers put into place by **notcurses_init(3)**. **nc** must not be used
following the call, and all references to ncplanes, cells, etc. are
invalidated.

Once the terminal has been reset, a summary of runtime and performance is
printed, unless **NCOPTION_SUPPRESS_BANNERS** was provided to
**notcurses_init(3)**.

The first step taken by **notcurses_stop** is a call to the internal function
**notcurses_stop_minimal**. This is the same function called by the fatal
signal handlers installed in the absence of **NCOPTION_NO_QUIT_SIGHANDLERS**.
This function:

* Disables the Notcurses signal handlers
* Emits the **op** terminfo capability, if supported
* Emits the **sgr0** terminfo capability, if supported
* Emits the **oc** terminfo capability, if supported
* Emits the **rmcup** terminfo capability, if supported (and if
  **NCOPTION_NO_ALTERNATE_SCREEN** was not provided).
* Emits the **cnorm** terminfo capability, if supported

Respectively, these restore the default colorpair to its original value
(**op**), turn off all text attributes (**sgr0**), restore the default
palette (**oc**), exit the alternate screen (**rmcup**), and restore the
cursor to its default appearance (**cnorm**).

It is legal to pass **NULL** to **notcurses_stop**. This is a no-op.

# NOTES

Behavior is undefined if other threads are working with **nc** when or after
this function is called. It is unlikely to be good.

# RETURN VALUES

On success, 0 is returned. Otherwise, a negative value is returned.

# SEE ALSO

**notcurses(3)**,
**notcurses_init(3)**,
**terminfo(5)**
