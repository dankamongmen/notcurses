% notcurses_fade(3)
% nick black <nickblack@linux.com>
% v1.2.3

# NAME

notcurses_fade - fade ncplanes in and out

# SYNOPSIS

**#include <notcurses.h>**

```c
// Called for each delta performed in a fade on ncp. If anything but 0 is
// returned, the fading operation ceases immediately, and that value is
// propagated out. If provided and not NULL, the faders will not themselves
// call notcurses_render().
typedef int (*fadecb)(struct notcurses* nc, struct ncplane* ncp);
```

**bool notcurses_canfade(const struct notcurses* nc);**

**int ncplane_fadeout(struct ncplane* n, const struct timespec* ts, fadecb fader, void* curry);**

**int ncplane_fadein(struct ncplane* n, const struct timespec* ts, fadecb fader, void* curry);**

**int ncplane_pulse(struct ncplane* n, const struct timespec* ts, fadecb fader, void* curry);**

# DESCRIPTION

# RETURN VALUES

# BUGS

# SEE ALSO

**notcurses(3)**, **notcurses_ncplane(3)**
