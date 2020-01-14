% notcurses_fade(3)
% nick black <nickblack@linux.com>
% v1.1.0

# NAME

notcurses_fade - fade ncplanes in and out

# SYNOPSIS

**#include <notcurses.h>**

```c
typedef int (*fadecb)(struct notcurses* nc, struct ncplane* ncp);
```

**int ncplane_fadeout(struct ncplane* n, const struct timespec* ts);**

**int ncplane_fadein(struct ncplane* n, const struct timespec* ts);**

**int ncplane_pulse(struct ncplane* n, const struct timespec* ts, fadecb fader);**

# DESCRIPTION

# RETURN VALUES

# BUGS

# SEE ALSO

**notcurses(3)**, **notcurses_ncplane(3)**
