This document attempts to list user-visible changes and any major internal
rearrangements of Notcurses.

* 1.3.2 (not yet released)
  * `ncdirect_cursor_push()`, `notcurses_cursor_pop()`, and
    `ncdirect_cursor_yx()` have been added. These are not supported on all
    terminals. `ncdirect_cursor_yx()` ought be considered experimental; it
    must read a response from the terminal, and this can interact poorly with
    other uses of standard input.

* 1.3.1
  * The C++ `Notcurses::render()` function now returns non-zero on failure,
  mirroring the behavior of the core C `notcurses_render()`. This is an
  inversion of its previous behavior.

* 1.2.8
  * `notcurses-tetris` now happily continues if it can't load its background.

* 1.2.7
  * Plots now always keep the most recent data to their far right (i.e., the
    gap that is initially filled is on the left, rather than the right).

* 1.2.6
  * `ncplane_putsimple_yx()` and `ncplane_putstr_yx()` have been exported as
    static inline functions.
  * `ncplane_set_scrolling()` has been added, allowing control over whether a
    plane scrolls. All planes, including the standard plane, do not scroll by
    default. If scrolling is enabled, text output via the `*put*` family of
    functions continues onto the next line when encountering the end of a row.
    This does not apply to e.g. boxes or lines.
  * `ncplane_putstr_yx()` now always returns the inverse of the number of
    columns advanced on an error (it used to return the positive short count so
    long as the error was due to plane geometry, not bad input).
  * `ncplot_add_sample()` and `ncplot_set_sample()` have been changed to accept
    a `uint64_t` rather than `int64_t`, since negative samples do not
    currently make sense. Plots were made more accurate in general.
  * `notcurses_term_dim_yx()` now accepts a `const struct notcurses*`.
  * `notcurses_resize()` is no longer exported. It was never necessary to call
    this in response to a resize, despite confusing documentation that could
    have been read to suggest otherwise. If you're in a long block on input, and
    get an `NCKEY_RESIZE`, just call `notcurses_refresh()` (which now calls
    `notcurses_resize()` internally, as `notcurses_render()` always has).
  * First Fedora packaging.

* 1.2.5
  * Add ncplot, with support for sliding-windowed horizontal histograms.
  * gradient, polyfill, `ncplane_format()` and `ncplane_stain()` all now return
    the number of cells written on success. Failure still sees -1 returned.
  * `ncvisual_render()` now returns the number of cells emitted on success, as
    opposed to 0. Failure still sees -1 returned.
  * `ncvisual_render()` now interprets length parameters of -1 to mean "to the
    end along this axis", and no longer interprets 0 to mean this. 0 now means
   "a length of 0", resulting in a zero-area rendering.
  * `notcurses_at_yx()` no longer accepts a `cell*` as its last parameter.
    Instead, it accepts a `uint32_t*` and a `uint64_t*`, and writes the
    attribute and channels to these parameters. This was done because the
    `gcluster` field of the `cell*` was always set to 0, which was surprising
    and a source of blunders. The EGC is returned via the `char*` return
    value. https://github.com/dankamongmen/notcurses/issues/410

* 1.2.4 2020-03-24
  * Add ncmultiselector
  * Add `ncdirect_cursor_enable()` and `ncdirect_cursor_disable()`.
