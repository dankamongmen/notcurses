% notcurses_stats(3)
% nick black <nickblack@linux.com>
% v1.5.1

# NAME

notcurses_stats - notcurses runtime statistics

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
typedef struct ncstats {
  // purely increasing stats
  uint64_t renders;          // number of successful renders
  uint64_t failed_renders;   // aborted renders, should be 0
  uint64_t render_bytes;     // bytes emitted to ttyfp
  int64_t render_max_bytes;  // max bytes emitted for a frame
  int64_t render_min_bytes;  // min bytes emitted for a frame
  uint64_t render_ns;        // nanoseconds spent rendering
  int64_t render_max_ns;     // max ns spent rendering
  int64_t render_min_ns;     // min ns spent rendering
  uint64_t cellelisions;     // cells elided entirely
  uint64_t cellemissions;    // cells emitted
  uint64_t fgelisions;       // RGB fg elision count
  uint64_t fgemissions;      // RGB fg emissions
  uint64_t bgelisions;       // RGB bg elision count
  uint64_t bgemissions;      // RGB bg emissions
  uint64_t defaultelisions;  // default color was emitted
  uint64_t defaultemissions; // default color was elided

  // current state -- these can decrease
  uint64_t fbbytes;          // bytes devoted to framebuffers
  unsigned planes;           // planes currently in existence
} ncstats;
```

**void notcurses_stats(struct notcurses* nc, ncstats* stats);**

**void notcurses_reset_stats(struct notcurses* nc, ncstats* stats);**

# DESCRIPTION

**notcurses_stats** acquires an atomic snapshot of statistics, primarily
related to notcurses_render(3). **notcurses_reset_stats** does the same, but
also resets all cumulative stats (immediate stats such as **fbbytes** are not
reset).

# NOTES

Unsuccessful render operations do not contribute to the render timing stats.

# RETURN VALUES

Neither of these functions can fail. Neither returns any value.

# SEE ALSO

**notcurses(3)**, **notcurses_render(3)**
