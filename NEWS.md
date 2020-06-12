This document attempts to list user-visible changes and any major internal
rearrangements of Notcurses.

* 1.5.1 (not yet released)
  * The semantics of rendering have changed slightly. In 1.5.0 and prior
    versions, a cell without a glyph was replaced *in toto* by that plane's
    base cell at rendering time. The replacement is now tripartite: if there
    is no glyph, the base cell's glyph is used; if there is a default
    foreground, the base cell's foreground is used; if there is a default
    background, the base cell's background is used. This will hopefully be
    more intuitive, and allows a plane to effect overlays of varying colors
    without needing to override glyphs (#395).
  * `ncvisual_geom()`'s `ncblitter_e` argument has been replaced with a
    `const struct ncvisual_options*`, so that `NCVISUAL_OPTIONS_NODEGRADE`
    can be taken into account (the latter contains a `blitter_e` field).

* 1.5.0 (2020-07-08)
  * The various `bool`s of `struct notcurses_options` have been folded into
    that `struct`'s `flags` field. Each `bool` has its own `NCOPTION_`.
  * Added a Pixel API for working directly with the contents of `ncvisual`s,
    including `ncvisual_at_yx()` and `ncvisual_set_yx()`.
  * Added `ncplane_puttext()` for writing multiline, line-broken text.
  * Added `ncplane_putnstr()`, `ncplane_putnstr_yx()`, and
    `ncplane_putnstr_aligned()` for byte-limited output of UTF-8.

* 1.4.5 (2020-06-04)
  * `ncblit_rgba()` and `ncblit_bgrx()` have replaced most of their arguments
    with a `const struct ncvisual_options*`. `NCBLIT_DEFAULT` will use
    `NCBLITTER_2x1` (with fallback) in this context. The `->n` field must
    be non-`NULL`--new planes will not be created.
  * Added `ncplane_notcurses_const()`.

* 1.4.4.1 (2020-06-01)
  * Got the `ncvisual` API ready for API freeze: `ncvisual_render()` and
    `ncvisual_stream()` now take a `struct ncvisual_options`. `ncstyle_e`
    and a few other parameters have been moved within. Both functions now
    take a `struct notcurses*`. The `struct ncvisual_options` includes a
    `ncblitter_e` field, allowing visuals to be mapped to various plotting
    paradigms including Sixel, Braille and quadrants. Not all backends have
    been implemented, and not all implementations are in their final form.
    `CELL_ALPHA_BLEND` can now be used for translucent visuals.
  * Added `ncvisual_geom()`, providing access to an `ncvisual` size and
    its pixel-to-cell blitting ratios.
  * Deprecated functions `ncvisual_open_plane()` and `ncplane_visual_open()`
    have been removed. Their functionality is present in
    `ncvisual_from_file()`. The function `ncvisual_plane()` no longer has
    any meaning, and has been removed.
  * The `fadecb` typedef now accepts as its third argument a `const struct
    timespec`. This is the absolute deadline through which the frame ought
    be displayed. New functions have been added to the Fade API: like the
    changes to `ncvisual_stream()`, this gives more flexibility, and allows
    more precise timing. All old functions remain available.

* 1.4.3 (2020-05-22)
  * Plot: make 8x1 the default, instead of 1x1.
  * Add `PREFIXFMT`, `BPREFIXFMT`, and `IPREFIXFMT` macros for `ncmetric()`.
    In order to properly use `printf(3)`'s field width capability, these
    macros must be used. This is necessary to support 'µ' (micro).
  * C++'s NotCurses constructor now passes a `nullptr` directly through to
    `notcurses_init()`, rather than replacing it with `stdout`.
  * Added `USE_STATIC` CMake option, defaulting to `ON`. If turned `OFF`,
    static libraries will not be built.

* 1.4.2.4 (2020-05-20)
  * Removed `ncplane_move_above_unsafe()` and `ncplane_move_below_unsafe()`;
    all z-axis moves are now safe. Z-axis moves are all now O(1), rather
    than the previous O(N).

* 1.4.2.3 (2020-05-17)
  * Added `notcurses_canutf8()`, to verify use of UTF-8 encoding.
  * Fixed bug in `ncvisual_from_plane()` when invoked on the standard plane.
  * `ncvisual_from_plane()` now accepts the same four geometric parameters
    as other plane selectors. To reproduce the old behavior, for `ncv`, call
    it as `ncvisual_from_plane(ncv, 0, 0, -1, -1)`.
  * `ncvisual_from_plane()`, `ncplane_move_below_unsafe()`, `ncplane_dup()`,
    and `ncplane_move_above_unsafe()` now accept `const` arguments where they
    did not before.
  * `notcurses_canopen()` has been split into `notcurses_canopen_images()` and
    `notcurses_canopen_videos()`.
  * `ncmetric()` now uses multibyte suffixes (particularly for the case of
    'µ', i.e. micro). This has changed the values of `PREFIXSTRLEN` and
    friends. So long as you were using `PREFIXSTRLEN`, this should require
    only a recompile. If you were using `PREFIXSTRLEN` in a formatted output
    context to count columns, you must change to `PREFIXCOLUMNS` etc.
  * The `streamcb` type definition now accepts a `const struct timespec*` as
    its third argument. This is the absolute time viz `CLOCK_MONOTONIC` through
    which the frame ought be displayed. The callback must now effect delay.
  * Mouse coordinates are now properly translated for any margins.
  * `qprefix()` and `bprefix()` now take a `uintmax_t` in place of an
    `unsigned`, to match `ncprefix`.

* 1.4.1 (2020-05-11)
  * No user-visible changes (fixed two unit tests).

* 1.4.0 (2020-05-10)
  * `ncplane_content()` was added. It allows all non-null glyphs of a plane to
    be returned as a nul-terminated, heap-allocated string.
  * `ncreader` was added. This widget allows freeform input to be edited in a
    block, and collected into a string.
  * `selector_options` has been renamed to `ncselector_options`, and
    `multiselector_options` has been renamed to `ncmultiselector_options`.
    This matches the other widget option struct's nomenclature.
  * `ncplane_set_channels()` and `ncplane_set_attr()` have been added to allow
    `ncplane` attributes to be set directly and in toto.
  * `NULL` can now be passed as the `FILE*` argument to `notcurses_init()` and
    `ncdirect_init()`. In this case, a new `FILE*` will be created using
    `/dev/tty`. If the `FILE*` cannot be created, an error will be returned.
  * A `flags` field has been added to `notcurses_options`. This will allow new
    boolean options to be added in the future without resizing the structure.
    Define `NCOPTION_INHIBIT_SETLOCALE` bit. If it's not set, and the "C" or
    "POSIX" locale is in use, `notcurses_init()` will invoke
    `setlocale(LC_ALL, "")`.
  * All widgets now take an `ncplane*` as their first argument (some took
    `notcurses*` before). All widgets' `options` structs now have an `unsigned
    flags` bitfield. This future-proofs the widget API, to a degree.

* 1.3.4 (2020-05-07)
  * `notcurses_lex_margins()` has been added to lex margins expressed in either
    of two canonical formats. Hopefully this will lead to more programs
    supporting margins.
  * `ncvisual_open_plane()` has been renamed `ncvisual_from_file()`. The former
    has been retained as a deprecated alias. It will be removed by 1.6/2.0.
  * `ncvisual_from_rgba()` and `ncvisual_from_bgra()` have been added to
    support creation of `ncvisual`s from memory, requiring no file.
  * `ncvisual_rotate()` has been added, supporting rotations of arbitrary
    radians on `ncvisual` objects.
  * `ncvisual_from_plane()` has been added to support "promotion" of an
    `ncplane` to an `ncvisual`. The source plane may contain only spaces,
    half blocks, and full blocks. This builds atop the new function
    `ncplane_rgba()`, which makes an RGBA flat array from an `ncplane`.
  * The `ncplane` argument to `ncplane_at_yx()` is now `const`.

* 1.3.3 (2020-04-26)
  * The `ncdplot` type has been added for plots based on `double`s rather than
    `uint64_t`s. The `ncplot` type and all `ncplot_*` functions were renamed
    `ncuplot` for symmetry.
  * FFMpeg types are no longer leaked through the Notcurses API. `AVERROR`
    is no longer applicable, and `ncvisual_decode()` no longer returns a
    `struct AVframe*`. Instead, the `nc_err_e` enumeration has been introduced.
    Functions which once accepted a value-result `AVERROR` now accept a value-
    result `nc_err_e`. The relevant constants can be found in
    `notcurses/ncerrs.h`.
  * OpenImageIO 2.1+ is now supported as an experimental multimedia backend.
    FFmpeg remains recommended. Video support with OIIO is spotty thus far.
  * CMake no longer uses the `USE_FFMPEG` option. Instead, the `USE_MULTIMEDIA`
    option can be defined as `ffmpeg`, `oiio`, or `none`. In `cmake-gui`, this
    item will now appear as an option selector. `oiio` selects OpenImageIO.

* 1.3.2 (2020-04-19)
  * `ncdirect_cursor_push()`, `notcurses_cursor_pop()`, and
    `ncdirect_cursor_yx()` have been added. These are not supported on all
    terminals. `ncdirect_cursor_yx()` ought be considered experimental; it
    must read a response from the terminal, and this can interact poorly with
    other uses of standard input.
  * 1.3.1 unintentionally inverted the C++ `Notcurses::render()` wrapper's
    return code. The previous semantics have been restored.

* 1.3.1 (2020-04-18)
  * `ncplane_at_yx()` and `ncplane_at_cursor()` have been changed to return a
    heap-allocated EGC, and write the attributes and channels to value-result
    `uint32_t*` and `uint64_t*` parameters, instead of to a `cell*`. This
    matches `notcurses_at_yx()`, and means they're no longer invalidated if the
    plane in question is destroyed. The previous functionality is available as
    new functions `ncplane_at_yx_cell()` and `ncplane_at_cursor_cell()`.
  * `ncplane_set_base()` inverted its `uint32_t attrword` and `uint64_t channels`
    parameters, thus matching every other function with these two parameters.
    It moved `const char* egc` before either, to force a type error, as the
    change would otherwise be likely to go overlooked.
  * Scrolling is now completely implemented. When a plane has scrolling enabled
    through use of `ncplane_set_scrolling(true)`, output past the end of the
    last line will now result in the top line of the plane being lost, all
    other lines moved up one, and the bottom line cleared.

* 1.3.0 (2020-04-12)
  * No user-visible changes

* 1.2.9 (2020-04-11)
  * No user-visible changes

* 1.2.8 (2020-04-10)
  * `notcurses-tetris` now happily continues if it can't load its background.

* 1.2.7 (2020-04-10)
  * Plots now always keep the most recent data to their far right (i.e., the
    gap that is initially filled is on the left, rather than the right).

* 1.2.6 (2020-04-08)
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

* 1.2.5 (2020-04-05)
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

* 1.2.4 (2020-03-24)
  * Add ncmultiselector
  * Add `ncdirect_cursor_enable()` and `ncdirect_cursor_disable()`.
