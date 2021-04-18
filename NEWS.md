This document attempts to list user-visible changes and any major internal
rearrangements of Notcurses.

* 2.2.8 (2021-04-18)
  * All remaining functions prefixed with `cell_` or `cells_` have been
    deprecated in favor of versions prefixed with `nccell_` or `nccell_`,
    respectively, which the former now wrap. The old versions will be
    removed in ABI3.
  * `ncvisual_inflate()` has been added to perform non-interpolative
    enlarging. It is intended for use with pixel art.

* 2.2.6 (2021-04-12)
  * `ncplane_rgba()` has been deprecated in favor of the new function
    `ncplane_as_rgba()`, which the former now wraps. It will be removed
    in ABI3. The new function can report the synthesized pixel geometry.
  * `ncvisual_geom()` has been deprecated in favor of the new function
    `ncvisual_blitter_geom()`, which the former now wraps. It will be
    removed in ABI3. The new function can report the chosen blitter.
  * `ncplane_pixelgeom()` has been added, allowing callers to determine the
    size of the plane and cells in pixels, as well as the maximum bitmap
    size that can be displayed.
  * Added new function `ncdirect_stream()`, which does what you'd think.
  * `cell_release()` and `cell_duplicate()` have been migrated to
    `nccell_release()` and `nccell_duplicate()`, respectively. The former
    forms have been deprecated, and will be removed in API3.
  * Added `NCVISUAL_OPTION_ADDALPHA`, and the `transcolor` field to
    `ncvisual_options`. If the former flag is used, the latter color
    will be treated as transparent.

* 2.2.5 (2021-04-04)
  * Bugfix release, no user-visible changes.

* 2.2.4 (2021-03-29)
  * Implemented **EXPERIMENTAL** `NCBLIT_PIXEL` for terminals reporting the
    Kitty pixel graphics protocol.
  * Added `notcurses_debug_caps()` to dump terminal properties, both those
    reported and those inferred, to a `FILE*`.
  * Added `NCOPTION_NO_CLEAR_BITMAPS` option for `notcurses_init()`.
  * Added `ncplane_valign()` and `ncplane_halign()`. `ncplane_align()` is now
    an alias for `ncplane_halign()`, and deprecated.
  * Added `NCVISUAL_OPTION_HORALIGNED` and `NCVISUAL_OPTION_VERALIGNED` flags
    for `ncvisual_render()`.
  * Added `NCPLANE_OPTION_VERALIGNED` flag for `ncplane_create()`.
  * Added the `nctabbed` widget for multiplexing planes data with navigational
    tabs. Courtesy Łukasz Drukała, in his first contribution.
  * Removed **notcurses_canpixel()**, which was obsoleted by
    **notcurses_check_pixel_support()**.
  * Added `NCPLANE_OPTION_MARGINALIZED` flag for `ncplane_create()`. Added
    the `ncplane_resize_marginalized()` resize callback. This allows you to
    have automatic resizing with a margin relative to some parent plane.

* 2.2.3 (2021-03-08)
  * Implemented **EXPERIMENTAL** `NCBLIT_PIXEL` for terminals reporting Sixel
    support. Added `notcurses_check_pixel_support()` and its companion
    `ncdirect_check_pixel_support()`, which must be called (and must return
    success) before `NCBLIT_PIXEL` will be available. `NCBLIT_PIXEL` degrades
    to `NCBLIT_3x2` until support is verified. This functionality is not yet
    well integrated into general rendering; it will not play nicely with other
    intersecting planes. Do not rely on current behavior.
  * Add the `nctree` widget for line-oriented hierarchical data. See
    the new `notcurses_tree(3)` man page for complete information.
  * Ceased exporting `cell_fchannel()`, `cell_bchannel()`,
    `cell_set_fchannel()`, and `cell_set_bchannel()`. These functions were
    never safe for users. Everything a user might want to manipulate can be
    manipulated with more granular functions.
  * Add `SIGILL` to the set of fatal signals we handle.
  * Added `NCKEY_SIGNAL`. `NCKEY_RESIZE` is now an alias for `NCKEY_SIGNAL`.
  * `SIGCONT` now synthesizes a `NCKEY_SIGNAL`, just like `SIGWINCH`.

* 2.2.2 (2021-02-18):
  * `notcurses_stats()` no longer qualifies its `notcurses*` argument with
    `const`, since it now takes a lock. I'm sorry about that, though on the
    plus side, data races can no longer result in invalid stats.
  * `ncplane_qrcode()` no longer accepts a blitter argument, since `NCBLIT_2x1`
    is the only one that actually works with qr code scanners. I'm unaware of
    any external `ncplane_qrcode()` users, so hopefully this isn't a problem.

* 2.2.1 (2021-02-09):
  * Brown-bag release: fix UTF8 discovery in direct mode. Sorry!

* 2.2.0 (2021-02-08):
  * Add `notcurses_canbraille()` capability predicate.

* 2.1.8 (2021-02-03):
  * The `notcurses-tetris` binary has been renamed `nctetris`.
  * The new function `channel_set_palindex()` has been added.
  * `NCDIRECT_OPTION_NO_READLINE` has been removed after a short life.
  * `ncdirect_readline()` has been added. The first time used, it initializes
    Readline. Readline will be destroyed by ncdirect_stop() if it was ever
    initialized.

* 2.1.7 (2021-01-21):
  * Notcurses has been split into two libraries, `notcurses-core` and
    `notcurses`. The latter contains the heavyweight multimedia code,
    so that applications which don't need this functionality can link against
    only the former. `pkg-config` support is present for both. If using only
    `notcurses_core`, use the new functions `notcurses_core_init()` and/or
    `ncdirect_core_init()` in place of `ncdirect_init()` and
    `notcurses_init()`, or your program is unlikely to link.
  * The `notcurses-view` binary has been renamed `ncplayer`.

* 2.1.5 (2021-01-15):
  * Notcurses **now depends on GNU Readline at build and runtime**, entirely
    for the benefit of direct mode, which now prepares GNU Readline for safe
    use (unless the new `NCDIRECT_OPTIONS_NO_READLINE` is used).
  * `ncplane_putstr_yx()`, `ncplane_putstr_stained()`, and
    `ncplane_putnstr_yx()` now return the number of columns output, as
    long documented (they were mistakenly returning the number of bytes).
  * `ncplane_abs_yx()` has been added, returning the absolute coordinates of
    the plane's origin (i.e. coordinates relative to its pile).

* 2.1.4 (2021-01-03):
  * Direct mode now supports `NCDIRECT_OPTION_NO_QUIT_SIGHANDLERS`, and by
    default installs signal handlers similar to those of fullscreen mode.
    They will attempt to reset the terminal, and propagate the signal.
  * Add `channels_fg_palindex()` and `channels_bg_palindex()`.

* 2.1.3 (2020-12-31)
  * `ncdirect_styles_{set, on, off}()` have been deprecated in favor of
    `ncdirect_{set, on, off}_styles()`, to match `ncplane_` equivalents.
  * `ncdirect_raster_frame()` no longer requires `blitter` nor `scale`.
  * `ncdirect_{fg, bg}_{default, rgb}()` have been deprecated in favor of
    `ncdirect_set_{fg, bg}_{default, rgb}()`, to match `ncplane`.

* 2.1.2 (2020-12-25)
  * Add `notcurses_linesigs_enable()` and `notcurses_linesigs_disable()`.
  * Divide `ncdirect_render_image()` into component `ncdirect_render_frame()`
    and `ncdirect_raster_frame()` (the original remains), allowing multiple
    threads to decode images concurrently.
  * Sextants are now considered supported for certain values of `TERM`.
  * `ncvisual_default_blitter()` has been deprecated in favor of the new
    function `ncvisual_media_defblitter()`. This function's opaque logic
    accepts a `struct notcurses *`, providing some future-proofing against
    blitter changes. This function is necessary to get `NCBLIT_3x2` from
    `NCBLIT_DEFAULT`.

* 2.1.1 (2020-12-16)
  * Progress bars via `ncprogbar`, using the standard widget API.

* 2.1.0 (2020-12-13)
  * `cell` has been renamed `nccell`. The old name has been kept as an alias,
    but ought be considered deprecated. It will be removed in Notcurses 3.0.

* 2.0.12 (2020-12-12)
  * `ncplane_resize_maximize()` has been added, suitable for use as a
    `resizecb`. It resizes the plane to the visual area's size, and is
    the resizecb used by the standard plane.

* 2.0.11 (2020-12-09)
  * Added `ncplane_descendant_p()` predicate.
 
* 2.0.10 (2020-12-06)
  * `ncpile_top()` and `ncpile_bottom()` have been added, returning the top
    or bottommost plane, respectively, of the pile containing their argument.
  * Added `cell_load_egc32()`, allowing a cell to be released and then reloaded
    with a UTF-8 EGC of up to 4 bytes, passed as a `uint32_t` (as opposed to a
    `const char *`).

* 2.0.9 (2020-12-01)
  * `ncmenu`s now automatically expand or shrink to match their binding plane.

* 2.0.8 (2020-11-27)
  * The major, minor, and patch versions are now available as preprocessor
    numeric defines, fit for comparisons at the cpp level. The
    `NOTCURSES_VERSION_COMPARABLE` macro has been added, to form a comparable
    version ID from a provided major, minor, and patch level. The
    `NOTCURSES_VERNUM_ORDERED` macro has been added, defined as a comparable
    version ID for the current version of Notcurses.
  * Add new function `ncplane_reparent_family()`, which reparents a plane and
    its bindtree (all planes bound to the plane, recursively).
    `ncplane_reparent()` now reparents only the specified plane; any planes
    bound to it are reparented to its old parent.
  * Move to a multipile model. For full details, consult
      https://groups.google.com/g/notcurses/c/knB4ojndv8A and
      https://github.com/dankamongmen/notcurses/issues/1078 and
      `notcurses_plane(3)`. In short:
    * A `struct notcurses` is now made up of one or more piles. A pile is one
      or more `ncplane`s, with a bindtree and a z-axis. Different piles can be
      mutated or rendered concurrently. There is no new user-visible type: a
      `struct notcurses` can be treated as a single pile.
    * To create a new pile from a new plane, use the new function
      `ncpile_create()`. The returned plane will be the top, bottom, and root
      of a new plane. Alternatively, use `ncplane_reparent()` or
      `ncplane_reparent_family()` with the source equal to the destination.
    * Add new function `ncpile_render()`, which renders the pile containing the
      specified plane to the specified buffer. Add new function
      `ncpile_rasterize()` to rasterize the specified buffer to output.
  * Added `NCSTYLE_STRUCK` for strikethrough. Note that this is not supported
    by terminfo, and we instead just hardcode the control sequence. Use at your
    own risk! If your terminal doesn't support this control sequence, behavior
    is undefined.

* 2.0.7 (2020-11-21)
  * The `horiz` union of `ncplane_options` has been discarded; the `int x`
    within has been promoted. This union brought no actual type safety, and was
    annoying for callers to deal with otherwise. Sorry for the inconvenience.
  * Added `ncplane_set_resizecb()` and `ncplane_resizecb()`.

* 2.0.3 (2020-11-09)
  * Add `NCBLIT_3x2` aka the SEXBLITTER, making use of Unicode 13's sextant
    glyphs. `notcurses_lex_blitter()` now recognizes `sexblitter`.
  * Blitting functions no longer count transparent cells towards the total
    returned number of cells written, but since these are not directly
    callable by the user, this ought not lead to any user-visible changes.
  * Added (k)eller demo to `notcurses-demo`.
  * `ncreader` now supports Alt+'b' to move one word back, Alt+'f' to move one
    word forward, Ctrl+'A' to move to the beginning of the line, Ctrl+'E' to
    move to the end of the line, Ctrl+'U' to clear the line before the cursor,
    and Ctrl+'W' to clear the word before the cursor (when
    `NCREADER_OPTION_NOCMDKEYS` has not been specified).

* 2.0.2 (2020-10-25)
  * Add `ncvisual_decode_loop()`, which returns to the first frame upon
    reaching the end of a file.

* 2.0.1 (2020-10-19)
  * Add `ncmenu_item_set_status()` for disabling or enabling menu items.
    * Disabled menu items cannot be selected.
    * Menu sections consisting only of disabled items are themselves disabled,
      and cannot be unrolled.
  * Add `ncinput_equal_p()` for comparison of `ncinput` structure data.
  * `ncmenu_offer_input()` now recognizes the shortcuts for registered
    sections, and will unroll the appropriate section when given input.
  * Added `notcurses_stddim_yx_const()` (`notcurses_stddim_yx()` `const` form).

* 2.0.0 (2020-10-12) **"Stankonia"**
  * **API STABILITY!** The API expressed in 2.0.0 will be maintained throughout
    at least 2.x.x. A program compiled against 2.0.0 will continue to compile
    and function properly against all 2.x.x releases. Thanks for putting up
    with the freewheeling API breakage until now.
  * `NOTCURSES_VERSION_{MAJOR, MINOR, PATCH, TWEAK}` are now available from
    `notcurses/version.h`. These represent the version your program was
    *compiled against*. The version your program is *linked to* can still be
    acquired with `notcurses_version_components()` (or as a human-readable
    string via `notcurses_version()`).

* 1.7.6 (2020-10-09)
  * `ncstats` added the new stats `writeout_ns`, `writeout_min_ns`, and
    `writeout_max_ns`. The `render_*ns` stats now only cover the rendering
    and rasterizing process. The `writeout*ns` stats cover the time spent
    writing data out to the terminal. `notcurses_render()` involves both of
    these processes.
  * `notcurses_render_to_buffer()` has been added, allowing user control of
    the process of writing frames out to the terminal.
  * `notcurses_stats_alloc()` has been added, to allocate an `ncstats` object.
    `notcurses_reset_stats()` has been renamed `notcurses_stats_reset()`.
  * Two flags have been defined for `ncdirect_init()`:
    `NCDIRECT_OPTION_INHIBIT_SETLOCALE` and `NCDIRECT_OPTION_INHIBIT_CBREAK`.
    The former is similar to `NCOPTION_INHIBIT_SETLOCALE`. The latter keeps
    `ncdirect_init()` from touching the termios and entering cbreak mode.
  * The C++ wrapper `Ncplane::putwc()` has been renamed `Ncplane::putwch()`, so
    as not to clash with standard libraries implementing `putwc()` as a macro.

* 1.7.5 (2020-09-29)
  * `ncreel_destroy()` now returns `void` rather than `int`.
  * `nctablet_ncplane()` has been renamed `nctablet_plane()`.
  * The standard plane now has the name `std`.
  * Removed long-deprecated `ncplane_set_attrs()` and `ncplane_attrs()`.
  * Renamed `ncplane_styles_*()` to `ncplane_*_styles()`, to conform with
    every other `ncplane_set_*()` function, but retained the old versions as
    (deprecated) aliases.
  * Renamed `cell_styles_*()` to `cell_*_styles()`, to conform with every other
    `cell_set_*()` function. Since these were inline functions, I've not
    bothered to retain the old versions.

* 1.7.4 (2020-09-20)
  * All `_rgb_clipped()` functions have been renamed `_rgb8_clipped()`, to
    match the changes made in 1.7.2. Sorry, I ought have done this before.
  * `ncplane_create()` has been introduced, taking a `struct ncplane_options`
    parameter. This replaces `ncplane_aligned()`, and will replace
    `ncplane_new()`. The latter ought be considered deprecated, and will be
    removed in the future. To align a place as previously done with
    `ncplane_aligned()`, use the `NCPLANE_OPTION_HORALIGNED` flag.
  * The `ncplane_options` struct includes a function pointer member,
    `resizecb`. If not `NULL`, this function will be called after the parent
    plane is resized. See `notcurses_plane.3` for more information.
  * `ncplane_resize_realign()` has been added, suitable for use as a
    `resizecb`. It realigns the plane against its parent.
  * `NCCHANNEL_ALPHA_MASK` has been renamed `CHANNEL_ALPHA_MASK`, to match
    the other declarations.

* 1.7.3 (2020-09-19)
  * API changes pursuant to 2.0 API finalization:
  * `mbswidth()` has been renamed `ncstrwidth()`.
  * The long-promised/dreaded Great Widget Review, normalizing behavior across
    all widgets, has been effected. Sorry, there was no getting around this
    one. Pretty much all widgets have slightly changed, because pretty much all
    widgets previously behaved slightly differently:
     * `ncselector_create()` and `ncmultiselector_create()` now take ownership
       of the provided `ncplane`. On an error in these functions, the `ncplane`
       will be destroyed. Otherwise, the `ncplane` is destroyed by
       `ncselector_destroy()` or `ncmultiselector_destroy()`.
     * `ncselector_create()`, `ncmultiselector_create()`, and
       `ncreader_create()` no longer accept `int y, int x` placement
       parameters. Just place the `ncplane`.
     * `ncselector_options`, `ncmultiselector_options`, and `ncreel_options`
       have lost their `bgchannels` members. Just set the base character for
       the `ncplane`.
     * `ncreader_options` has lost its `echannels`, `eattrword`, `egc`,
       `physrows`, and `physcols` fields. Just set the base character and size
       for the `ncplane`.
  * Functions which set a 24-bit RGB value have had the suffix `g` replaced
    with `g_rgb`. Functions which set three 8-bit RGB components have had the
    suffix `rgb` replaced with `rgb8`. This was done because e.g.
    `channels_set_fg()` and `channels_set_fchannel()` were indistinguishable on
    sight. Failure to make the necessary conversions will result in compiler
    errors. See https://github.com/dankamongmen/notcurses/issues/985.
  * Functions ending in `_stainable()` now end in `_stained()`.
  * `ncplane_putwc_stained()` and `ncplane_putwstr_stained()` have been
    added in the interest of orthogonality.
  * `ncplane_new_named()` has been eliminated. `ncplane_new()` now takes a
    `const char* name` argument. `ncplane_bound()` and `ncplane_bound_named()`
    have both been eliminated. `ncplane_new()` now accepts an `ncplane*`
    instead of a `notcurses*`. All functionality exposed by the removed
    functions is thus now present in `ncplane_new()`.
  * `ncplane_aligned_named()` has been removed. `ncplane_aligned()` now accepts
    a `const char* name` argument.

* 1.7.2 (2020-09-09)
  * Exported `ncvisual_default_blitter()`, so that the effective value of
    `NCBLIT_DEFAULT` can be determined.
  * Added `NCREADER_OPTION_CURSOR`, instructing the `ncreader` to make the
    terminal cursor visible, and manage the cursor's placement.

* 1.7.1 (2020-08-31)
  * Renamed `CELL_SIMPLE_INITIALIZER` to `CELL_CHAR_INITIALIZER`, and
    `cell_load_simple()` to `cell_load_char()`.
  * Renamed `ncplane_putsimple()` to `ncplane_putchar()`,
    `ncplane_putsimple_stainable()` to `ncplane_putchar_stainable()`,
    and `ncplane_putsimple_yx()` to `ncplane_putchar_yx()`.

* 1.7.0 (2020-08-30)
  * Added `notcurses_ucs32_to_utf8()` conversion helper.
  * `ncdirect_init()` now takes a third `uint64_t flags` parameter. No flags
    have been defined, and this parameter ought be set to 0.

* 1.6.20 (2020-08-30)
  * Added convenience functions `ncplane_y()` and `ncplane_x()`, components
    of longstanding `ncplane_yx()`.
  * `ncreel` functions now generally call `ncreel_redraw()` themselves. This
    includes `ncreel_add()`, `ncreel_del()`, `ncreel_next()`, and
    `ncreel_prev()`. `ncreel_redraw()` need only be called to update tablets.
  * In order to conform with CMake naming conventions, our CMake package is
    now accessed as "Notcurses" rather than "notcurses".

* 1.6.19 (2020-08-27)
  * Direct mode now places the terminal into "cbreak mode". This disables
    echo and line-buffering of input. If this is undesirable, you can restore
    the terminal state following `ncdirect_init()`, but this will break the
    semantics of `ncdirect_getc()` and derivatives (due to line buffering).
  * The notcurses input layer has been reproduced for direct mode, including
    `ncdirect_getc()`, `ncdirect_getc_nblock()`, `ncdirect_getc_blocking()`,
    and `ncdirect_inputready_fd()`. Mouse support is not yet available in
    direct mode, but becomes possible through these additions.
  * Some very subtle bugs on big-endian machines have been repaired. Be
    aware that if your execution endianness does not match the endianness
    assumed at build time, you're gonna have a rough go of it.

* 1.6.18 (2020-08-25)
  * `nc_err_e` has been taken behind the shed and shot in the face. All
    functions which once returned `nc_err_e` now return a bimodal `int`. Those
    functions which accepted a value-result `nc_err_e*` no longer take this
    argument.
  * `notcurses_cursor_enable()` now takes two `int` parameters specifying the
    desired location of the cursor. Both `notcurses_cursor_enable()` and
    `notcurses_cursor_disable()` now return `int` rather than `void`.
  * `NCOPTION_RETAIN_CURSOR` has been removed.
  * `ncreader` now implements `NCREADER_OPTION_HORSCROLL` for horizontal
    scrolling. In addition, the following functions have been added:
    * `int ncreader_move_left(struct ncreader* n)`
    * `int ncreader_move_right(struct ncreader* n)`
    * `int ncreader_move_up(struct ncreader* n)`
    * `int ncreader_move_down(struct ncreader* n)`
    * `int ncreader_write_egc(struct ncreader* n, const char* egc)`.
  * Added `ncplane_above()` and `notcurses_bottom()`.
  * Added `ncplane_set_fchannel()` and `ncplane_set_bchannel()`.

* 1.6.17 (2020-08-22)
  * `ncdirect_flush()` now takes a `const struct ncdirect*`.
  * A `const char* title` field has been added to `ncplot_options`. If not
    `NULL`, this title will be displayed to the right of any labels. Plot
    data will cover the title, if present.
  * `ncplot` no longer inverts `maxchannel` and `minchannel`. Speaking
    of which, both of these fields are now plural, `maxchannels` etc.

* 1.6.16 (2020-08-22)
  * `cell_simple_p()` has been removed. It is no longer a useful concept for
    user code, and its presence is indicative of a likely error.
  * `channels_blend()` has been removed. It wasn't really useful to users,
    and was difficult to explain.
  * `ncplane_mergedown()` has been renamed `ncplane_mergedown_simple()`. A
    more general form, capable of projecting arbitrary subregions of the source
    plane down to the destination plane. The source argument to
    `ncplane_mergedown_simple()` is now `const`.
  * `iprefix()` has been added, corresponding to `IPREFIXSTRLEN`. This ought
    be used if you want binary prefixes without the 'i' suffix indicating
    binary prefixes, which I predict will endear you to exactly no one.
  * Added `channels_set_fg_palindex()` and `channels_set_bg_palindex()`.
    Rewrote `cell_set_fg_palindex()` and `cell_set_bg_palindex()` in terms
    of these two. This is possible because the palette index now overlaps the
    RGB in a channel (they were originally in the attrword).
  * Added `ncdirect_flush()`, mainly for the benefit of FFI that might not
    have a native interface to `fflush(3)`.
  * The `ncplot_options` struct has a new field, `legendstyle`. If the
    dependent variable is being labeled, this style will be applied to the
    legend. Without NCPLOT_OPTION_LABELTICKSD, this value is ignored.

* 1.6.15 (2020-08-16)
  * Styles now work properly with `ncdirect`, which apparently has never
    been the case until now :/.
  * EGCs occupying four bytes or fewer when encoded as UTF8 are now
    inlined directly into the `cell` structure. This should mean nothing
    for you save less memory consumption per plane, and faster operation.
    In the course of doing so, the `attrword` field of the `cell` structure
    was renamed `stylemask`, and reduced from 32 to 16 bits.
  * `notcurses_palette_size()` now returns `unsigned`.

* 1.6.12 (2020-08-12)
  * `ncreel`s `tabletcb` callback function semantics are radically simplified.
    No more worrying about borders that might or might not have been drawn;
    simply fill up the plane that you're handed. This eliminates four of the
    seven arguments to these callbacks. I hope the inconvenience of adapting
    them is worth the elimination of complexity therein; I obviously think
    it is =].
  * `ncselector_redraw()` and `ncmultiselector_redraw()` no longer call
    `notcurses_render()`. You will need to call `notcurses_render()` for the
    display to reflect any changes. `ncselector_create` now binds the plane
    it creates to the plane it was provided, and no longer checks to ensure
    the widget can be fit within the borders of this binding plane.
  * Added `ncplane_new_named()`, `ncplane_bound_named()`, and
    `ncplane_aligned_named()`. These would be the defaults, but I didn't want
    to break existing code. They might become the defaults by 2.0. Names are
    used only for debugging (`notcurses_debug()`) at this time.
  * Added `ncplane_parent()` and `ncplane_parent_const()` for accessing the
    plane to which a plane is bound.
  * The `notcurses` Rust crate (`rust/notcurses`) has been moved to
    `dankamongmen/notcurses-rs` on GitHub, and removed from the tree.
    Jose Luis will be leading development on this high-level wrapper.

* 1.6.11 (2020-08-03)
  * `cell_egc_idx()` is no longer exported; it was never intended to be.

* 1.6.10 (2020-08-01)
  * The `egc` member of `ncreader_options` is now `const`.

* 1.6.7 (2020-07-26)
  * GNU libunistring is now required to build/load Notcurses.
  * Added `ncmenu_mouse_selection()`. Escape now closes an unrolled menu
    when processed by `ncmenu_offer_input()`.

* 1.6.6 (2020-07-19)
  * `notcurses-pydemo` is now only installed alongside the Python module,
    using setuptools. CMake no longer installs it.
  * Added `notcurses_lex_blitter()` and `notcurses_str_scalemode()`.

* 1.6.4 (2020-07-19)
  * Added `notcurses_str_blitter()`.

* 1.6.2 (2020-07-15)
  * The option `NCOPTION_NO_FONT_CHANGES` has been added. This will cause
    Notcurses to not muck with the current font. Because...
  * Notcurses now detects a Linux text console, and reprograms its Unicode
    to glyph tables and font data tables to include certain Box-Drawing and
    Block-Drawing glyphs. This vastly improves multimedia rendering and
    line/box art in the Linux console.

* 1.6.1 (2020-07-12)
  * Added `notcurses_version_components()` to get the numeric components of
    the loaded Notcurses version.
  * Added `notcurses_render_file()` to dump last rendered frame to a `FILE*`.
  * The `ncreel` widget has been overhauled to bring it in line with the
    others (`ncreel` began life in another project, predating Notcurses).
    The `toff`, `boff`, `roff`, and `loff` fields of `ncreel_options` have
    been purged, as have `min_` and `max_supported_rows` and `_cols`. There
    is no longer any need to provide a pipe/eventfd. `ncreel_touch()`,
    `ncreel_del_focused()`, and `ncreel_move()` have been removed.
  * Added `ncdirect_hline_interp()`, `ncdirect_vline_interp()`,
    `ncdirect_rounded_box()`, `ncdirect_double_box()`, and the ridiculously
    flexible `ncdirect_box()`.
  * Added `ncplane_putstr_stainable()`.

* 1.6.0 (2020-07-04)
  * Behavior has changed regarding use of the provided `FILE*` (which, when
    `NULL`, is assumed to be `stdout`). Both Notcurses and `ncdirect` now
    try to open a handle to the controlling TTY, **unless** the provided
    `FILE` is a TTY, in which case it is used directly. Certain interactions
    now only go to a TTY, in particular `ncdirect_cursor_yx()` and various
    `ioctl()`s used internally. Furthermore, when no true TTY is found (true
    for e.g. daemonized processes and those in a Docker launched without
    `-t`), Notcurses (in both full mode and direct mode) will return a virtual
    screen size of 80x24. This greatly improves behavior when redirecting to a
    file or lacking a TTY; one upshot is that we now have much-expanded unit
    test coverage in the Docker+Drone autobuilders.
  * `ncdirect_render_image()` has been added, allowing images (but not
    videos or animated images) to be rendered directly into the standard I/O
    streams. It begins drawing from the current cursor position, running
    through the right-hand side of the screen, and scrolling as much content
    as is necessary.
  * `ncneofetch` has been rewritten to use `ncdirect`, and thus no longer
    clobbers your entire terminal, and scrolls like standard I/O.

* 1.5.3 (2020-06-28)
  * The default blitter when `NCSCALE_STRETCH` is used is now `NCBLIT_2x2`,
    replacing `NCBLIT_2x1`. It is not the default for `NCSCALE_NONE` and
    `NCSCALE_SCALE` because it does not preserve aspect ratio.
  * The values of `CELL_ALPHA_OPAQUE` and friends have been redefined to
    match their values within a channel representation. If you've been
    using the named constants, this should have no effect on you; they sort
    the same, subtract the same, and a zero initialization remains just as
    opaque as it ever was. If you weren't using their named constants, now's
    an excellent time to revise that policy. `CELL_ALPHA_SHIFT` has been
    eliminated; if you happened to be using this, the redefinition of the
    other `CELL_*` constants (probably) means you no longer need to.

* 1.5.2 (2020-06-19)
  * The `ncneofetch` program has been added, of no great consequence.
  * A `NULL` value can now be passed as `sbytes` to `ncplane_puttext()`.
  * `ncvisual_geom()` now takes scaling into account.
  * `notcurses_cantruecolor()` has been added, allowing clients to
    determine whether the full RGB space is available to us. If not,
    we only have palette-indexed pseudocolor.

* 1.5.1 (2020-06-15)
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
  * Added `ncuplot_sample()` and `ncdplot_sample()`, allowing retrieval of
    sample data from `ncuplot`s and `ncdplot`s, respectively.
  * Added convenience function `ncplane_home()`, which sets the cursor
    to the plane's origin (and returns `void`, since it cannot fail).
  * `ncplane_qrcode()` now accepts an `ncblitter_e`, and two value-result
    `int*`s `ymax` and `xmax`. The actual size of the drawn code is
    returned in these parameters.

* 1.5.0 (2020-06-08)
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
