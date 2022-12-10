% notcurses_fade(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses_fade - fade ncplanes in and out

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
struct ncfadectx;

// Called for each delta performed in a fade on ncp. If anything but 0 is
// returned, the fading operation ceases immediately, and that value is
// propagated out. If provided and not NULL, the faders will not themselves
// call notcurses_render().
typedef int (*fadecb)(struct notcurses* nc, struct ncplane* ncp,
                      const struct timespec*, void* curry);
```

**bool notcurses_canfade(const struct notcurses* ***nc***);**

**int ncplane_fadeout(struct ncplane* ***n***, const struct timespec* ***ts***, fadecb ***fader***, void* ***curry***);**

**int ncplane_fadein(struct ncplane* ***n***, const struct timespec* ***ts***, fadecb ***fader***, void* ***curry***);**

**int ncplane_pulse(struct ncplane* ***n***, const struct timespec* ***ts***, fadecb ***fader***, void* ***curry***);**

**struct ncfadectx* ncfadectx_setup(struct ncplane* ***n***);**

**int ncfadectx_iterations(const struct ncfadectx* ***nctx***);**

**int ncplane_fadeout_iteration(struct ncplane* ***n***, struct ncfadectx* ***nctx***, int ***iter***, fadecb ***fader***, void* ***curry***);**

**int ncplane_fadein_iteration(struct ncplane* ***n***, struct ncfadectx* ***nctx***, int ***iter***, fadecb ***fader***, void* ***curry***);**

**void ncfadectx_free(struct ncfadectx* ***nctx***);**

# DESCRIPTION

**ncplane_fadeout**, **ncplane_fadein**, and **ncplane_pulse** are simple
APIs for fading planes in and out. Fades require either RGB support or
palette reprogramming support from the terminal (the RGB method is
preferred, and will be used whenever possible). The **ts** parameter
specifies the total amount of time for the fade operation. The operation
itself is time-adaptive (i.e. if it finds itself falling behind, it will
skip iterations; if it is proceeding too quickly, it will sleep).

These are wrappers around the more flexible **ncfadectx** API. Create an
**ncfadectx** with **ncfadectx_setup**. The number of possible state changes
(iterations) can be accessed with **ncfadectx_iterations**. A state can be
reached with **ncplane_fadeout_iteration** or **ncplane_fadein_iteration**.
Finally, destroy the **ncfadectx** with **ncfadectx_free**.

# RETURN VALUES

**ncplane_fadeout_iteration** and **ncplane_fadein_iteration** will propagate
out any non-zero return value from the callback **fader**.

# BUGS

Palette reprogramming can affect other contents of the terminal in complex
ways. This is not a problem when the RGB method is used.

# SEE ALSO

**clock_nanosleep(2)**,
**notcurses(3)**,
**notcurses_plane(3)**
