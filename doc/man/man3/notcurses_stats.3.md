% notcurses_stats(3)
% nick black <nickblack@linux.com>
% v2.0.11

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
  uint64_t render_ns;        // nanoseconds spent in render+raster
  int64_t render_max_ns;     // max ns spent for a frame
  int64_t render_min_ns;     // min ns spent for a frame
  uint64_t writeout_ns;      // ns spent writing frames to terminal
  int64_t writeout_max_ns;   // max ns spent writing out a frame
  int64_t writeout_min_ns;   // min ns spent writing out a frame
  uint64_t cellelisions;     // cells elided entirely
  uint64_t cellemissions;    // cells emitted
  uint64_t fgelisions;       // RGB fg elision count
  uint64_t fgemissions;      // RGB fg emissions
  uint64_t bgelisions;       // RGB bg elision count
  uint64_t bgemissions;      // RGB bg emissions
  uint64_t defaultelisions;  // default color was emitted
  uint64_t defaultemissions; // default color was elided
  uint64_t refreshes;        // refreshes (unoptimized redraws)

  // current state -- these can decrease
  uint64_t fbbytes;          // bytes devoted to framebuffers
  unsigned planes;           // planes currently in existence
} ncstats;
```

**ncstats* notcurses_stats_alloc(struct notcurses* ***nc***);**

**void notcurses_stats(struct notcurses* ***nc***, ncstats* ***stats***);**

**void notcurses_stats_reset(struct notcurses* ***nc***, ncstats* ***stats***);**

# DESCRIPTION

**notcurses_stats_alloc** allocates an **ncstats** object. This should be used
rather than allocating the object in client code, to future-proof against
the struct being enlarged by later Notcurses versions.

**notcurses_stats** acquires an atomic snapshot of statistics, primarily
related to notcurses_render(3). **notcurses_stats_reset** does the same, but
also resets all cumulative stats (immediate stats such as **fbbytes** are not
reset).

**renders** is the number of successful calls to **notcurses_render(3)**
or **notcurses_render_to_buffer(3)**. **failed_renders** is the number of
unsuccessful calls to these functions. **failed_renders** should be 0;
renders are not expected to fail except under exceptional circumstances.
should **notcurses_render(3)** fail while writing out a frame to the terminal,
it counts as a failed render.

**render_max_bytes** and **render_min_bytes** track the maximum and minimum
number of bytes used rasterizing a frame. A given state of Notcurses does not
correspond to a unique number of bytes; the size is also dependent on the
existing terminal state. As a first approximation, the time a terminal takes to
ingest and reflect a frame is dependent on the size of the rasterized frame.

**render_ns**, **render_max_ns**, and **render_min_ns** track the total
amount of time spent rendering and rasterizing frames in nanoseconds. Rendering
and rasterizing takes place in **notcurses_render(3)**, and is the entirety of
**notcurses_render_to_buffer(3)**. These steps are independent of the terminal.

**writeout_ns**, **writeout_max_ns**, and **writeout_min_ns** track the total
amount of time spent writing frames to the terminal. This takes place in only
**notcurses_render(3)**. If **notcurses_render_to_buffer(3)** is used, the
user is responsible for writing out the frame, and it will not be tracked by
any stat.

**cellemissions** reflects the number of EGCs written to the terminal.
**cellelisions** reflects the number of cells which were not written, due to
damage detection.

**refreshes** is the number of times **notcurses_refresh** has been
successfully executed.

**fbbytes** is the total number of bytes devoted to framebuffers throughout
the **struct notcurses** context. **planes** is the number of planes in the
context. Neither of these stats can reach 0, due to the mandatory standard
plane.

# NOTES

Unsuccessful render operations do not contribute to the render timing stats.

# RETURN VALUES

Neither **notcurses_stats** nor **notcurses_stats_reset** can fail. Neither
returns any value. **notcurses_stats_alloc** returns a valid **ncstats**
object on success, or **NULL** on failure.

# SEE ALSO

**notcurses(3)**, **notcurses_render(3)**
