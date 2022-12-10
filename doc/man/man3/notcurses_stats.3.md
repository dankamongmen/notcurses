% notcurses_stats(3)
% nick black <nickblack@linux.com>
% v3.0.9

# NAME

notcurses_stats - notcurses runtime statistics

# SYNOPSIS

**#include <notcurses/notcurses.h>**

```c
typedef struct ncstats {
  // purely increasing stats
  uint64_t renders;          // successful ncpile_render() runs
  uint64_t writeouts;        // successful ncpile_rasterize() runs
  uint64_t failed_renders;   // aborted renders, should be 0
  uint64_t failed_writeouts; // aborted writes
  uint64_t raster_bytes;     // bytes emitted to ttyfp
  int64_t raster_max_bytes;  // max bytes emitted for a frame
  int64_t raster_min_bytes;  // min bytes emitted for a frame
  uint64_t render_ns;        // nanoseconds spent rendering
  int64_t render_max_ns;     // max ns spent for a frame
  int64_t render_min_ns;     // min ns spent for a frame
  uint64_t raster_ns;        // nanoseconds spent rasterizing
  int64_t raster_max_ns;     // max ns spent in raster for a frame
  int64_t raster_min_ns;     // min ns spent in raster for a frame
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
  uint64_t sprixelemissions; // sprixel draw count
  uint64_t sprixelelisions;  // sprixel elision count
  uint64_t sprixelbytes;     // sprixel bytes emitted
  uint64_t appsync_updates;  // application-synchronized updates
  uint64_t input_events;     // inputs received or synthesized
  uint64_t input_errors;     // errors processing input
  uint64_t hpa_gratuitous;   // gratuitous HPAs issued
  uint64_t cell_geo_changes; // cell geometry changes (resizes)
  uint64_t pixel_geo_changes;// pixel geometry changes (font resize)

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
or **ncpile_render_to_buffer(3)**. **failed_renders** is the number of
unsuccessful calls to these functions. **failed_renders** should be 0;
renders are not expected to fail except under exceptional circumstances.
should **notcurses_render(3)** fail while writing out a frame to the terminal,
it counts as a failed render.

**raster_max_bytes** and **raster_min_bytes** track the maximum and minimum
number of bytes used rasterizing a frame. A given state of Notcurses does not
correspond to a unique number of bytes; the size is also dependent on the
existing terminal state. As a first approximation, the time a terminal takes to
ingest and reflect a frame is dependent on the size of the rasterized frame.

**render_ns**, **render_max_ns**, and **render_min_ns** track the total
amount of time spent rendering frames in nanoseconds. Rendering
takes place in **ncpile_render** (called by **notcurses_render(3)** and
**ncpile_render_to_buffer**). This step is independent of the terminal.

**raster_ns**, **raster_max_ns**, and **raster_min_ns** track the total
amount of time spent rasterizing frames in nanoseconds. Rasterizing
takes place in **ncpile_raster** (called by **notcurses_raster(3)** and
**ncpile_render_to_buffer**). This step depends on the terminal definitions.
The same frame might not rasterize to the same bytes for different terminals.

**writeout_ns**, **writeout_max_ns**, and **writeout_min_ns** track the total
amount of time spent writing frames to the terminal. This takes place in
**ncpile_rasterize** (called by **notcurses_render(3)**).

**cellemissions** reflects the number of EGCs written to the terminal.
**cellelisions** reflects the number of cells which were not written, due to
damage detection.

**refreshes** is the number of times **notcurses_refresh** has been
successfully executed.

**fbbytes** is the total number of bytes devoted to framebuffers throughout
the **struct notcurses** context. **planes** is the number of planes in the
context. Neither of these stats can reach 0, due to the mandatory standard
plane.

**sprixelemissions** is the number of sprixel draws. **sprixelelisions** is
the number of times a sprixel was elided--essentially, the number of times
a sprixel appeared in a rendered frame without freshly drawing it.
**sprixelbytes** is the number of bytes used for sprixel drawing. It does not
include move/delete operations, nor glyphs used to erase sprixels.

**input_errors** is the number of errors while processing input, e.g.
malformed control sequences or invalid UTF-8 (see **utf8(7)**).

**hpa_gratuitous** is the number of **hpa** (horizontal position absolute,
see **terminfo(5)**) control sequences issued where not strictly necessary.
This is done to cope with fundamental ambiguities regarding glyph
width. It is not generally possible to know how wide a glyph will be rendered
on a given combination of font, font rendering engine, and terminal. Indeed, it
is not even generally possible to know how many glyphs will result from a
sequence of EGCs. As a result, Notcurses sometimes issues "gratuitous" **hpa**
controls.

**cell_geo_changes** is the number of changes to the visible area's cell
geometry. The cell geometry changes whenever the visible area is resized
without a corresponding cell-pixel geometry change. **pixel_geo_changes**
is the number of changes to cells' pixel geometry (i.e. the height and
width of each cell), and changes whenever the font size changes. Both can
change at the same time if e.g. a terminal undergoes a font size change
without changing its total size.

# NOTES

Unsuccessful render operations do not contribute to the render timing stats.

Linux framebuffer bitmaps are not written through the terminal device, but
instead directly into the memory-mapped framebuffer (see **mmap(2)**). Bytes
used for framebuffer graphics are thus independent of bytes written to the
terminal. This explains why **sprixelbytes** may be surprising given the
value of **raster_bytes**.

# RETURN VALUES

Neither **notcurses_stats** nor **notcurses_stats_reset** can fail. Neither
returns any value. **notcurses_stats_alloc** returns a valid **ncstats**
object on success, or **NULL** on allocation failure.

# SEE ALSO

**mmap(2)**,
**notcurses(3)**,
**notcurses_render(3)**,
**terminfo(5)**,
**utf8(7)**
