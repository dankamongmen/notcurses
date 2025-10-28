This document attempts to list user-visible changes and any major internal
rearrangements of Notcurses.

* 3.0.17 (2025-10-28)
  * Fix build problems on Windows and Mac OSX.

* 3.0.16 (2025-05-04)
  * Fix several bugs in input handling that could lead to invalid
    memory reads (#2875)

* 3.0.15 (2025-04-29)
  * Fix bug on FreeBSD where we could stop processing input (#2873).

* 3.0.14 (2025-04-26)
  * `ncplane_family_destroy()` has been added to the API.
  * Added some `foot` capabilities. Recognize `ghostty` and bless its
     quadrants/sextants implementations.
  * A bug introduced sometime in 2022 that caused unpredictable
    hangs on exit was resolved (#2837), yay!
  * Fix bug identified by @kmarius on github where we core out if printing
     a tab at the end of an ncplane whose x dimension is not a multiple
     of the tab stop (#2870), thanks for the report!
  * We now require CMake 3.21 (vs 3.14) and C17 (vs C11).

* 3.0.13 (2025-01-11)
  * Fix regression when building with `USE_CXX=off`.
  * Use `distutils` from its own Python component rather than assuming
    it's in `setuputils`, from which it was removed in Python 3.12.
  * Properly check for UTF8 before calling ncmetric_use_utf8(), fixing
    a bug when using ncmetric without UTF8 support.

* 3.0.12 (2025-01-09)
  * Fixed a bug when rendering QR codes into a small area. QR codes now
    require `NCBLIT_2x1`, as that is the only blitter which can generate a
    proper aspect ratio. It thus no longer works in an ASCII environment.
  * The `NCBLIT_4x2` "octant" blitter has been added, making use of new
    characters from Unicode 16. `notcurses_canoctant()` has been added to
    check for `NCBLIT_4x2` support at runtime. If present, octants will be
    used for `NCBLIT_DEFAULT` when used with `NCSCALE_NONE_HIRES`,
    `NCSCALE_SCALE_HIRES`, or `NCSCALE_STRETCH`. Thanks, eschnett! Note
    that octants are not supported by GNU libc until 2.41.
  * Fixed coredump on exit when using musl as libc (alpine, some gentoo).

* 3.0.11 (2024-10-02)
  * We now normalize the return of `nl_langinfo()` according to the behavior
    of glibc's `_nl_normalize_charset()`, supporting some atypical synonyms
    of `UTF-8`.
  * Fixed a bug in `ncsixel_as_rgba()` (called by `ncvisual_from_sixel()`)
    that broke loading of sixels having more than 12 rows (sixel generation
    from images worked fine). Thanks, waveplate!
  * Reject illegal geometries in `ncvisual_from_*()`.
  * We build with FFMPEG 7.1.

* 3.0.10 (2024-10-02)
  * Cursed. See https://github.com/dankamongmen/notcurses/issues/2795.

* 3.0.9 (2022-12-10)
  * Eliminated infinite loop in `ncplane_move_family_above()`.
    Thanks, drewt!

* 3.0.8 (2022-04-06)
  * Bugfix release, but also support curled underlines in Contour.

* 3.0.7 (2022-02-20)
  * Tab characters may now be used with `ncplane_put*()`. See
     `ncplane_output.3` for more information.

* 3.0.6 (2022-02-09)
  * `ncplane_set_[fb]channel()`, `ncplane_[fb]channel()`,
    `ncplane_channels()`, `ncplane_set_channels()`,
    `ncchannels_set_[fb]channel()`, and `ncchannels_[fb]channel()` now
    function only on the 28 alpha + coloring bits of their respective
    channels, which is almost certainly what you wanted in the first place.
  * Restore `nccell_set_[fb]channel()` and friends, using these semantics.

* 3.0.5 (2022-01-21)
  * The Hyper and Super modifiers are now supported. CapsLock and NumLock
    can further be disambiguated when using the Kitty protocol. The
    new functions `ncinput_super_p()` and friends have been added.
  * `ncinput` has a new field, `modifiers`. The old `alt`, `shift`, and
    `ctrl` booleans are now deprecated, and will be removed in 4.0.
  * `ncmenu_section` **must** now specify any expected modifiers for their
    shortucts using `modifiers`. Setting any of `alt`, `shift`, or
    `ctrl` will see `ncmenu_create()` fail.
  * `ncinput_equal_p()` considers `NCTYPE_UNKNOWN` equal to `NCTYPE_PRESS`.
  * Added `ncpalette_get()` for orthogonality's sake.

* 3.0.4 (2022-01-08)
  * We now use level 2 of `XTMODKEYS`, providing better differentiation
    of keyboard modifiers. We now unpack the Meta modifier.
  * Added `ncinput_shift_p()`, `ncinput_alt_p()`, `ncinput_ctrl_p()`,
    and `ncinput_meta_p()` to test for various modifiers in `ncinput`s.
  * When support is advertised, the 1016 mouse protocol will be used
    to provide pixel-level detail. See the `{yx}px` fields of `ncinput`.

* 3.0.3 (2022-01-02)
  * No user-visible changes to the API, but Sixel quantization has been
    rewritten. It is now substantially faster, though quality has gone
    down for some images. I'll be working on bringing it back for 3.0.4.

* 3.0.2 (2021-12-21)
  * Added `ncplane_cursor_y()` and `ncplane_cursor_x()`.
  * Added `NCOPTION_SCROLLING`, equivalent to calling
    `ncplane_set_scrolling(true)` on the standard plane.
  * Added `NCOPTION_CLI_MODE`, an alias for the bitwise OR of
    `NCOPTION_SCROLLING`, `NCOPTION_NO_CLEAR_BITMAPS`,
    `NCOPTION_NO_ALTERNATE_SCREEN`, and `NCOPTION_PRESERVE_CURSOR`.
  * Added `ncvisual_from_sixel()`.
  * The control sequence corresponding to a pixel-blitted `ncvisual()`
    can be retrieved by using `ncplane_at_yx()` on the sprixel plane.

* 3.0.1 (2021-12-14)
  * Added the `NCPLANE_OPTION_VSCROLL` flag. Creating an `ncplane` with this
    flag is equivalent to immediately calling `ncplane_set_scrolling(true)`.
  * Added the `NCPLANE_OPTION_AUTOGROW` flag and the `ncplane_set_autogrow()`
    and `ncplane_autogrow_p()` functions. When autogrow is enabled, the plane
    is automatically enlarged to accommodate output at its right (no scrolling)
    or bottom (scrolling enabled) boundaries.
  * Added `notcurses_default_background()` and `notcurses_default_foreground()`.
  * Added `nccell_load_ucs32()`.
  * Added `nctree_add()` and `nctree_del()`. `nctree` is now dynamic.

* 3.0.0 (2021-12-01) **"In the A"**
  * Made the ABI/API changes that have been planned/collected during 2.x
    development. This primarily involved removing deprecated functions,
    and making some `static inline` (and thus no longer linkable symbols).
    There have been a few small renamings (i.e. `ncplane_pixelgeom()` to
    `ncplane_pixel_geom()`) for purposes of regularity. The only thing removed
    without an obvious replacement is the `renderfp` field of
    `notcurses_options`, for which I make no apology. If you've been avoiding
    deprecated functionality, ABI3 ought require small changes, if any.
  * `notcurses_get()` and `ncdirect_get()` now require an absolute deadline
    rather than a delay bound; it ought be calculated using `CLOCK_MONOTONIC`.
  * The handling of geometry and distance has been normalized across all
    functions. Lengths are now `unsigned` as opposed to `int`. Where -1 was
    being used to indicate "everything", 0 is now required. This affects
    `ncplane_as_rgba()`, `ncplane_contents()`, and `ncvisual_from_plane()`,
    which all used -1. A length of zero passed to line-drawing functions is
    now an error. Several line-drawing functions now reliably return errors
    as opposed to short successes. Dimensions of 0 to `ncplane_mergedown()`
    now mean "everything". Almost all coordinates now accept -1 to indicate the
    current cursor position in that dimension. `ncplane_highgradient()` has
    been deprecated in favor of the new `ncplane_gradient2x1()`, which takes
    origin coordinates. `ncplane_format()` and `ncplane_stain()` now take
    origin coordinates. All now interpret their `unsigned` argument as
    lengths rather than closing coordinates, observing the same semantics as
    outlined above.
  * `ncstrwidth_valid()`, introduced in 2.4.1, has replaced `ncstrwidth()`
    entirely, and been renamed to reflect this. For the old behavior,
    simply add two `NULL`s to your `ncstrwidth()` invocations.
  * `ncplayer` now defaults to pixel blitting.
  * `NCKEY_SIGNAL` is no longer a synonym for `NCKEY_RESIZE`, but instead
    indicates receipt of `SIGCONT`.
  * `CELL_TRIVIAL_INITIALIZER`, `CELL_CHAR_INITIALIZER`, and
    `CELL_INITIALIZER` are all now prefixed with `NC`.
  * A new resize callback, `ncplane_resize_placewithin()`, has been added.
  * The `ncinput` struct has a new field, `utf8`. `notcurses_get()` will fill
    in this array of `char` with the NUL-terminated UTF-8 representation of
    the input whenever one exists.

* 2.4.9 (2021-11-11)
  * Added `ncnmetric()`, which uses `snprintf()` internally. `ncmetric()`
    was reimplemented as a trivial wrapper around `ncnmetric()`.
  * `qprefix()`, `bprefix()`, and `iprefix()` have been renamed
    `ncqprefix()`, `ncbprefix()`, and `nciprefix()`, respectively.
    All related constants have been prefixed with `NC`, and the old
    definitions will be removed for abi3.
  * `notcurses_mice_enable()` and `notcurses_mouse_disable()` replace
    `notcurses_mouse_enable()` and `notcurses_mouse_disable()`, which
    have been deprecated, and will be removed in ABI3.
    `notcurses_mice_enable()` takes an additional `unsigned eventmask`
    parameter, a bitmask union over `NCMICE_*_EVENT` (`NCMICE_ALL_EVENTS`
    is provided for convenience and future-proofing).
    `notcurses_mice_disable()` is now a `static inline` wrapper around the
    former, passing 0 as the event mask. This can be used to get mouse
    movement buttons and focus events, which were previously unavailable.
  * `ncvisual_geom()` has been introduced, using the `ncvgeom` struct
    introduced for direct mode. This allows complete statement of geometry
    for an `ncvisual`. It replaces `ncvisual_blitter_geom()`, which has been
    deprecated, and will be removed in ABI3. It furthermore exposes some of
    the information previously available only from `ncplane_pixelgeom()`,
    though that function continues to be supported.
  * `ncvgeom`'s `rcelly` and `rcellx` fields are now (finally) filled in
    by `ncvisual_geom()` (and thus `ncdirectf_geom()`), and suitable for use.
  * On transition between `ncplane`s (on terminals implementing complex wide
    glyphs), Notcurses now always issues an `hpa` sequence to force horizontal
    positioning. This fixes a number of longstanding bugs in e.g. the
    `[uniblock]` and `[whiteout]` demos at the cost of some extra control
    sequences. For more information, see
    [issue 2199](https://github.com/dankamongmen/notcurses/issues/2199). The
    number of `hpa`s issued in this manner is tracked in a new stat,
    `hpa_gratuitous`.

* 2.4.8 (2021-10-23)
  * Added new functions `notcurses_canpixel()` and `notcurses_osversion()`.
  * `notcurses_get()` now evaluates its timeout against `CLOCK_MONOTONIC`
    instead of `CLOCK_REALTIME`.
  * `SIGBUS` is now included among the signals for which a handler is
    by default installed.

* 2.4.7 (2021-10-16)
  * Features 1, 2, and 8 of the Kitty keyboard protocol are now supported. This
    provides much more detailed and fine-grained keyboard reports, including
    key repeat and release events, and modifier events (i.e. pressing Shift by
    itself now generates an event; it is not required to press another key
    along with the modifier). Only Kitty supports this protocol at this time.
  * `XTMODKEYS` is now used where supported. This is essentially a subset of
    the Kitty protocol discussed above.
  * In the absence of any `XTSMGRAPHICS` replies, advertising feature 4 in a
    DA1 response will be considered as claiming support for Sixel with 256
    color registers. If you're a terminal author, please do `XTSMGRAPHICS`.
    Actually, please implement the vastly superior Kitty graphics protocol.
  * `ncvisualplane_create()` now allows for a new pile to be created, by
    passing a `NULL` ancestor `ncplane` in `vopts`. The first argument is
    now a `struct notcurses*` rather than a `struct ncplane*`.
  * `ncvisual_render()` has been deprecated in favor of the new function
    `ncvisual_blit()`. When a `NULL` `vopts->n` is passed to `ncvisual_blit()`,
    a new plane is created (as it was in `ncvisual_render()`), but that plane
    is the root of a new pile, rather than a child of the standard plane.
    The only tricky conversion is if you previously had `vopts.n` as `NULL`,
    and were not using `NCVISUAL_OPTION_CHILDPLANE` (or were passing `NULL`
    as `vopts`). This would result in a new plane bound to the standard plane
    with `ncvisual_render()`, but with `ncvisual_blit()` it will create a new
    pile. To keep the behavior, explicitly pass the standard plane as
    `vopts->n`, and include `NCVISUAL_OPTION_CHILDPLANE` in `vopts->flags`.
    All other cases will continue to work as they did before.

* 2.4.5 (2021-10-06)
  * The poorly-considered function `ncplane_boundlist()`, added in 2.3.17, has
    been removed, having never ought have been born.
  * Added functions `ncplane_move_family_top()`, `ncplane_move_family_bottom()`,
    `ncplane_move_family_above()`, and `ncplane_move_family_below()`.
  * Added functions `ncplane_set_name()` and `ncplane_name()`.

* 2.4.4 (2021-10-03)
  * Notcurses no longer uses libreadline, as it was realized to be incompatible
    with the new input system. `ncdirect_readline()` has been rewritten to
    work without libreadline, which means it's now always available (readline
    was an optional dependency). `NCDIRECT_OPTION_INHIBIT_CBREAK` should *not*
    be used with `ncdirect_readline()`, or else it can't implement line-editing
    keybindings.
  * Helper function `ncwcsrtombs()` is now available for converting a
    `wchar_t *` to a heap-allocated UTF-8 `char *`.
  * Building without a C++ compiler is now supported using `-DUSE_CPP=off`. See
    the FAQs for restrictions.

* 2.4.3 (2021-09-26)
  * `ncplane_erase_region()` has been made much more general, and can now
    operate relative to the current cursor.
  * Several terminal emulators have recently changed their semantics regarding
    DECSDM. These changes correctly match the real VT340 behavior.
    Unfortunately, this means we always draw Sixels in the upper left corner of
    the screen. Code has been added to deal with XTerm 369, the ayosec/graphics
    branch of Alacritty 15.1, foot 1.8.2, and MinTTY 3.5.2.

* 2.4.2 (2021-09-19)
  * The Rust wrappers have been moved to
     [dankamongmen/libnotcurses-sys](https://github.com/dankamongmen/libnotcurses-sys),
     under the continued stewardship of @joseluis.
  * You can now set a resize callback on the standard plane.
  * Added `notcurses_getvec()`, providing batched input.
  * Added `NCOPTION_DRAIN_INPUT`. Notcurses now launches a thread to process
    input, so that it can respond to terminal messages with minimal latency.
    Input read from `stdin` intended for the client is buffered until
    retrieved. If your client never intends to read this input, provide this
    flag to eliminate unnecessary processing, and ensure Notcurses can always
    retrieve terminal messages (if buffers are full, Notcurses cannot
    continue reading). Likewise added `NCDIRECT_OPTION_DRAIN_INPUT`.
  * Removed a bunch of deprecated `static inline` functions from the headers.
  * A new field, `evtype`, has been added to `ncinput`. It takes a value
    from among `NCTYPE_{UNKNOWN, PRESS, REPEAT, RELEASE}.`. Where possible,
    Notcurses will distinguish between a press, repeat, and release. This
    cannot be done in all environments, nor with all inputs. The
    `NCKEY_RELEASE` definition is no longer returned; instead, the
    appropriate `NCKEY_BUTTONx` synthesized key is returned, with
    `EVTYPE_RELEASE` set.
  * `NCKEY_EOF` now indicates the end of input.

* 2.4.1 (2021-09-12)
  * `notcurses_check_pixel_support()` still returns 0 if there is no support
    for bitmap graphics, but now returns an `ncpixelimple_e` to differentiate
    the pixel backend otherwise. This result is strictly informative.
  * Added `ncstrwidth_valid()`, which is like `ncstrwidth()` except that it
    returns partial results in the case of an invalid character. `ncstrwidth()`
    will become a `static line` wrapper of `ncstrwidth_valid()` in ABI3.

* 2.4.0 (2021-09-06)
  * Mouse events in the Linux console are now reported from GPM when built
    with `-DUSE_GPM=on`.

* 2.3.18 (2021-08-31)
  * No user-visible changes.

* 2.3.17 (2021-08-22)
  * Added `notcurses_enter_alternate_screen()` and
    `notcurses_leave_alternate_screen()`.
  * Added `ncplane_boundlist()`.
  * Plots now support `NCBLIT_PIXEL`!

* 2.3.16 (2021-08-19)
  * Fix `ncdirect_set_*_rgb()` for the case where an emulator has fewer than
    8 colors, i.e. vt100. This release exists to make unit tests work again
    on the Alpine and Fedora buildservers.

* 2.3.15 (2021-08-17)
  * `ncneofetch` has been changed to use "CLI mode" instead of Direct Mode,
    as a proof of concept. It is very likely that Direct Mode will be
    deprecated for ABI3. New code ought not be written using it.
  * Added `ncplane_scrollup()` and `ncplane_scrollup_child()`.
  * Fixed grotesque errors in `ncplane_set_*_palindex()`.
  * Removed support for the iTerm2 graphics protocol, which is unsuitable for
    the Notcurses model. macOS users who want graphics are recommended to use
    Kitty or WezTerm. It will be added back if it gains necessary capabilities.

* 2.3.13 (2021-08-04)
  * Added the portable utility functions `notcurses_accountname()` and
    `notcurses_hostname()`.

* 2.3.12 (2021-07-29)
  * `notcurses_getc()` and `ncdirect_getc()` no longer accept a `sigset_t*`
    as their third argument. Instead, they accept a `void*`, with which
    they will do nothing. This is due to POSIX signals being unportable in
    addition to terrible, and this one wart complicating wrappers a great
    deal. If you were using this functionality, you were probably using it
    incorrectly, no offense. If you're certain you were doing it right,
    roll your own with `pthread_sigmask()`, and accept the race condition.
    For ABI3, these functions will be dropped entirely; for now they have
    only been marked deprecated. New functions `ncdirect_get()` and
    `notcurses_get()` elide this parameter entirely, and ought be used in
    new code. All callers have been updated.
  * Added `nccell_cols()`, which is just `nccell_width()` except it doesn't
    require the first `const ncplane*` argument, and it's `static inline`.
    `nccell_width()` has been deprecated, and will be removed in ABI3.
  * `ncvisual_subtitle_plane()` now handles all LibAV subtitle types,
    including Type-1 DVB (bitmap subtitles), so long as a pixel blitter is
    available. `ncvisual_subtitle()` has been deprecated, and will be
    removed in ABI3.
  * Add `ncvisual_from_palidx()`, which does what you would think.

* 2.3.11 (2021-07-20)
  * Notcurses now requires libz to build. In exchange, it can now generate
    PNGs on the fly, necessary for driving iTerm2's graphics protocol.
  * Experimental code has been added to draw graphics using both the iTerm2
    protocol and directly to the Linux console framebuffer. This functionality
    is still quite raw, but can be played with.
  * Added `NCPLANE_OPTION_FIXED`, to prevent a plane bound to a scrolling
    plane from scrolling along with it. Otherwise, bound planes will scroll
    along with the parent plane so long as the planes intersect.
  * Added `input_errors` and `input_events` stats.
  * `NCALPHA_HIGHCONTRAST` now works properly atop default backgrounds.
  * `SIGFPE` is now included among the fatal signals for which handlers are
    by default installed. Unsure how I overlooked it this long.

* 2.3.10 (2021-07-14)
  * Notcurses now builds and works, so far as I can tell, on OS X 11.4+.
  * Emit XTPUSHCOLORS and XTPOPCOLORS where supported (XTerm and Kitty).
  * `notcurses-info` now works around Unicode unsupported by the local
    platform, so that other output remains available.

* 2.3.9 (2021-07-12)
  * Fixed major regressions from 2.3.8: menu highlighting is working once
    more, as are pointer inputs (mice) and the 8x1 plotter. Sorry about that!
  * `notcurses_detected_terminal()` and `ncdirect_detected_terminal()` now
    both return a heap-allocated string, which will contain the terminal
    version if Notcurses was able to detect it. This result ought be free()d.
  * Added `ncplane_move_rel()`.
  * Documented `ncplane_move_yx()` in `notcurses_plane.3`, and removed the
    false comment that "passing -1 as a coordinate will hold that axis
    constant" from `USAGE.md` and `notcurses.h`. This has never been true.
  * Added `ncdirect_putegc()` to perform Unicode segmentation. It returns
    the number of columns consumed, and makes available the number of bytes
    used by the EGC.
  * `ncmenu`s can now be used with any plane, not just the standard plane.
  * Added `ncchannels_reverse()`, which reverses the color aspects of the
    two channels, while keeping other elements constant.
  * `CHANNELS_RGB_INITIALIZER` and `CHANNEL_RGB_INITIALIZER` have been renamed
    `NCCHANNELS_INITIALIZER` and `NCCHANNEL_INITIALIZER`. The former two are
    now deprecated, and will be removed for ABI3.

* 2.3.8 (2021-07-04)
  * Marked all capability functions `__attribute__ ((pure))`. If you were
    calling `notcurses_check_pixel_support()` before in order to enable pixel
    blitting (unnecessary since 2.3.5), you might get compiler warnings about
    statements without effects. Just remove the call if so.
  * Fixed bugs in `ncvisual_blitset_geom()` and `ncvisual_render()` when using
    `NCVISUAL_OPTION_CHILDPLANE` in certain configurations.
  * Fixed some serious bugs in the OpenImageIO backend.
  * Disabled Synchronized Update Mode for Kitty in response to upstream bugs.

* 2.3.7 (2021-06-29)
  * Deprecated `NCSTYLE_REVERSE` and `NCSTYLE_DIM`. The remainder are safe,
    and I added back `NCSTYLE_BLINK` according to popular demand.
  * Added `NCOPTION_PRESERVE_CURSOR`. If used, the standard plane's virtual
    cursor will be initialized to match its position at startup, rather than
    starting in the upper-left corner. Together with a scrolling standard
    plane and inhibition of the alternate screen, this allows rendered mode
    to easily be used for scrolling shell environment programs.
  * Control characters from C0 and C1 are now rejected when loading `nccell`s
    or writing to a plane (except for newline, when using a scrolling plane).
    This was always intended, but never enforced. Horizontal tabs might be
    enabled anew sometime in the future.
  * `ncls` now defaults to `NCBLIT_PIXEL`.
  * Added `ncplane_scrolling_p()` to retrieve a plane's scrolling status.
  * Greatly expanded `notcurses-info`.

* 2.3.6 (2021-06-23)
  * Fixed (harmless) warning with `-Wformat-security`.
  * Remove `NCSTYLE_{INVIS,BLINK,STANDOUT}` with extreme prejudice. They
    remain defined for now, but will be removed for ABI3.
  * Deprecated `notcurses_debug_caps()`, which no longer generates output.
    Hey, I explicitly commented that its output was "subject to change".

* 2.3.5 (2021-06-23)
  * Happy day! The terminal interrogation routines in the initialization code 
    have been completely revamped. The first outcome of this is that Sixel
    parameters are now opportunistically read at startup, and thus there is
    no longer any need to call `notcurses_check_pixel_support()` before
    using `NCBLIT_PIXEL`. If it's there, it'll be used; if not, it'll degrade
    or fail. The new routines rely on the terminal answering the Send Device
    Attributes escape; if it does not, Notcurses may refuse to start, or even
    hang. Please report a bug if you run into this.
    It is still necessary to supply a correct `TERM` environment variable,
    because this is used to index into the `terminfo(5)` database, which
    seeds most common escapes. The extended capabilities of some modern
    terminals, however, will be retrieved independently of `TERM`; they'll
    be made available for use if supported by the connected terminal, and
    others will not, even if your `TERM` variable implies they ought.
  * `ncplane_as_rgba()`/`ncvisual_from_plane()` now support `NCBLIT_BRAILLE`.
  * `CELL_ALPHA_*` macros are now `NCALPHA_*`. The former will remain
    `#define`d until ABI3.
  * Filled out the complete set of `ncdirect_can*()` capability functions,
    which now match the `notcurses_can*()` API. Added
    `ncdirect_canget_cursor()` to check if the cursor can be located.
  * `ncdirect_dim_y()` and `ncdirect_dim_x()` no longer accept a
    `const ncdirect*`, since they update the term parameters. Sorry!
  * Added `NCDIRECT_OPTION_VERBOSE` and `NCDIRECT_OPTION_VERY_VERBOSE`.
    They map to `NCLOGLEVEL_WARNING` and `NCLOGLEVEL_TRACE`, respectively.
  * New functions `ncvisual_from_rgb_packed()` and `ncvisual_from_rgb_loose()`.
  * New stat `sprixelbytes`.
  * Added new functions `ncpile_render_to_buffer()` and
    `ncpile_render_to_file()`. Rewrote `notcurses_render_to_buffer()` and
    `notcurses_render_to_file()` as trivial wrappers around these functions,
    and deprecated the latter. They will be removed in ABI3.
  * Added support for application-synchronized updates, and a new stat.

* 2.3.4 (2021-06-12)
  * Added the flag `NCVISUAL_OPTION_NOINTERPOLATE` to use non-interpolative
    scaling in `ncvisual_render()`. `ncvisual_render()` without a multimedia
    engine will now use this method for any requested scaling (previously,
    scaling was not performed without a linked multimedia backend).
  * `NCVISUAL_OPTION_BLEND` used with `NCBLIT_PIXEL` will now, when the Kitty
    graphics protocol is in use, cut the alpha of each pixel in half.
  * `ncvisual_inflate()` has been rewritten as a wrapper around the new
    function `ncvisual_resize_noninterpolative()`, and deprecated. It will be
    removed for ABI3. Godspeed, `ncvisual_inflate()`; we hardly knew ye.
  * `ncdirectf_render()` has been changed to accept a `ncvisual_options`,
    replacing and extending its four final arguments. Sorry about the breakage
    here, but `ncdirectf_render()` was introduced pretty recently (2.3.1).
    As a result, `ncdirectf_render()` and `ncdirect_stream()` now honor
    `NCVISUAL_OPTION_BLEND` and `NCVISUAL_OPTION_NOINTERPOLATE`. All of this
    also applies to `ncdirect_geomf()`.
  * `ncplayer` now accepts `-n` to force non-interpolative scaling.
  * A new binary is installed, `notcurses-info`. It prints information about
    the terminal environment in which it runs. More information is available
    from its man page, `notcurses-info(1)`.
  * Added `ncdirect_light_box()`, `ncdirect_heavy_box()`,
    `ncdirect_ascii_box()`, `nccells_light_box()`, and `nccells_heavy_box()`.
    Publicized `nccells_ascii_box()`. All are `static inline`.
  * A bug was fixed in `ncplane_move_yx()`: root planes were being moved
    relatively instead of absolutely. This was never the intended behavior.
  * It used to be possible to pass `NULL` as the second parameter of
    `ncplane_mergedown_simple()`, and have the standard plane be used as
    the destination. This is no longer supported, since the source plane
    could be in another pile. An error will instead be returned.
  * Fixed a bug in `ncdirect_box()` where default/palette-indexed colors
    weren't properly used on the top and bottom borders.
  * Added `notcurses_detected_terminal()` and `ncdirect_detected_terminal()`.

* 2.3.2 (2021-06-03)
  * Fixed a bug affecting certain scalings of `ncvisual` objects created from
    memory (e.g. `ncvisual_from_rgba()`).
  * Fixed a bug where setting a style in direct mode reset color. Shocked that
    such a bug could exist for so long, ugh.
  * Fixed memory leaks in the `ffmpeg` and `none` implementations of the
    `ncvisual` API, and also the `libnotcurses-core` implementation.
  * `ncinput_nomod_p()` has been added. This function returns `true` if and
    only if its `ncinput` argument has no modifiers active.
  * Added `notcurses_cursor_yx()` to get the current location of the cursor.
  * Added `ncdirect_supported_styles()`.
  * `ncplane_at_yx()` now properly integrates the plane's base cell when
    appropriate, and thus represents the cell as it will be used during
    rendering. This change cascades, affecting e.g. `ncplane_contents()`.
  * `ncplane_at_yx()` now returns the EGC when called on any column of a
    wide glyph. `ncplane_at_yx_cell()` continues to duplicate the exact
    `nccell`, and can thus continue to be used to distinguish between primary
    and secondary columns of a wide glyph. Likewise, `notcurses_at_yx()`
    now returns the EGC when called on any column of a wide glyph.
  * Sadly, `ncplane_contents()` no longer accepts a `const ncplane*`, since it
    might write temporaries to the plane's EGCpool during operation.
  * Added `ncdirect_styles()`, to retrieve the current styling.
  * In previous versions of Notcurses, a rendered-mode context
    (`struct notcurses`) and a direct-mode context (`struct ncdirect`) could
    be open at the same time. This was never intended, and is no longer
    possible.

* 2.3.1 (2021-05-18)
  * Sprixels no longer interact with their associated plane's framebuffer. This
    means plane contents are maintainted across blitting a sprixel and then
    independently destroying that sprixel (i.e. without destroying the plane).
    While the sprixel is bound to the plane, these contents are ignored (save
    that they will be reported by `ncplane_at_yx()`). Since no method currently
    exists to destroy a sprixel without destroying its plane, I don't think
    this will impact anyone.
  * 8bpc RGB is unconditionally enabled if the terminal emulator is determined
    to be Kitty, Alacritty, or foot; there is no longer any need to export
    `COLORTERM` on these terminals.
  * Fixed bad bug in `ncvisual_resize()` when growing an image. This isn't
    relevant to enlarging an `ncvisual` via scaling, but only when persistently
    growing one with `ncvisual_resize()`.
  * Direct mode image rendering now honors the `maxy` and `maxx` parameters,
    which specify the maximum number of cell rows and columns, respectively,
    to use for the render. They were previously ignored, contrary to
    documentation. It is now an error to pass a negative number for either of
    these values. Use 0 to specify "as much space as is necessary".
  * Added `ncdirectf_from_file()`, `ncdirectf_geom()`, and `ncdirectf_render()`,
    with the net result that you can now (efficiently) get media geometry in
    direct mode. If you don't care about media geometry, you can keep using
    `ncdirect_render_frame()` and/or `ncdirect_render_image()`, and Godspeed.
    Oh yes, and `ncdirectf_free()`. Rien n'est simple, mais tout est facile....

* 2.3.0 (2021-05-09) **"Triumph"**
  * No user-visible changes.

* 2.2.11 (2021-05-08)
  * `notcurses-core.pc` is now generated with a `Requires.private` line
    matching the local system's source of Terminfo. This ought resolve
    static linking on systems with libtinfo embedded into libncurses.
  * Added `ncblit_rgb_loose()` and `ncblit_rgb_packed()` helpers for blitting
    32bpp RGBx and 24bpp RGB.
  * Added `ncplane_erase_region()` to initialize all `nccell`s within a
    region of a plane.

* 2.2.10 (2021-05-05)
  * Added `NCVISUAL_OPTION_CHILDPLANE` to interpret the `n` field of
    `ncvisual_options` as a parent plane.
  * Reimplemented `ncdirect_cursor_down()` using vertical tabs instead
    of the `cud` capability, so that it now scrolls when on the last line.
    Thanks to Daniel Eklöf for this idea, of which I was totally ignorant!
  * Fixed several embarrassing `assert()`s which I'd not had exposed due
    to misuse of CMake.
  * Fixed a state machine bug that caused sprixels to sometimes not be
    properly redisplayed following a rebuild.

* 2.2.9 (2021-05-03)
  * Added two new stats, `sprixelemissions` and `sprixelelisions`.
  * Added `notcurses_canhalfblock()` and `notcurses_canquadrant()`.
  * The `palette256` type has been renamed `ncpalette`, and all functions
    prefixed with `palette256_` have been deprecated in favor of versions
    prefixed with `ncpalette_`, which the former now wrap. The old versions
    will be removed in ABI3.
  * All functions prefixed with `channel_` have been deprecated in favor of
    versions prefixed with `ncchannel_`, which the former now wrap. The old
    versions will be removed in ABI3.
  * All functions prefixed with `channels_` have been deprecated in favor of
    versions prefixed with `ncchannels_`, which the former now wrap. The old
    versions will be removed in ABI3.
  * `SIGINT`, `SIGQUIT`, and `SIGTERM` are now masked for the calling thread
    when writing starts, and unmasked when writing has ended. This prevents
    the writing thread from handling these signals in the middle of a write,
    which could otherwise leave the terminal locked up (if it resulted in
    aborting an escape sequence). The signal will be delivered when unblocked.
    For this to work properly, other threads ought also have these signals
    blocked. `notcurses_getc()` and friends thus no longer drop these signals
    from the provided `sigset_t`; they are instead added if not present.
  * Added `nccell_width()` to get the column length of an `nccell`.

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
  * Added `NCSTYLE_STRUCK` for strikethrough.

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
