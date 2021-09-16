//! `Nc*` methods and associated functions.

use core::ptr::{null, null_mut};

use crate::{
    cstring, error, error_ref_mut, notcurses_init, rstring, Nc, NcAlign, NcBlitter, NcChannels,
    NcDim, NcError, NcFile, NcInput, NcLogLevel, NcOptions, NcPixelImpl, NcPlane, NcResult,
    NcScale, NcStats, NcStyle, NcStyleMethods, NcTime, NCOPTION_NO_ALTERNATE_SCREEN,
    NCOPTION_SUPPRESS_BANNERS, NCRESULT_ERR, NCSTYLE_BOLD, NCSTYLE_ITALIC, NCSTYLE_NONE,
    NCSTYLE_STRUCK, NCSTYLE_UNDERCURL, NCSTYLE_UNDERLINE,
};

/// # `NcOptions` Constructors
impl NcOptions {
    /// New NcOptions.
    pub const fn new() -> Self {
        Self::with_all_options(0, 0, 0, 0, 0, 0)
    }

    /// New NcOptions, with margins.
    pub const fn with_margins(top: NcDim, right: NcDim, bottom: NcDim, left: NcDim) -> Self {
        Self::with_all_options(0, top, right, bottom, left, 0)
    }

    /// New NcOptions, with flags.
    pub const fn with_flags(flags: u64) -> Self {
        Self::with_all_options(0, 0, 0, 0, 0, flags)
    }

    /// New NcOptions, with all the options.
    ///
    /// ## Arguments
    ///
    /// - loglevel
    ///
    ///   Progressively higher log levels result in more logging to stderr. By
    ///   default, nothing is printed to stderr once fullscreen service begins.
    ///
    /// - margin_t, margin_r, margin_b, margin_l
    ///
    ///   Desirable margins (top, right, bottom, left).
    ///
    ///   If all are 0 (default), we will render to the entirety of the screen.
    ///   If the screen is too small, we do what we can.
    ///   Absolute coordinates are relative to the rendering area
    ///   ((0, 0) is always the origin of the rendering area).
    ///
    /// - flags
    ///
    ///   General flags; This is expressed as a bitfield so that future options
    ///   can be added without reshaping the struct.
    ///   Undefined bits must be set to 0.
    ///
    ///   - [`NCOPTION_INHIBIT_SETLOCALE`][crate::NCOPTION_INHIBIT_SETLOCALE]
    ///   - [`NCOPTION_NO_ALTERNATE_SCREEN`]
    ///   - [`NCOPTION_NO_FONT_CHANGES`][crate::NCOPTION_NO_FONT_CHANGES]
    ///   - [`NCOPTION_NO_QUIT_SIGHANDLERS`][crate::NCOPTION_NO_QUIT_SIGHANDLERS]
    ///   - [`NCOPTION_NO_WINCH_SIGHANDLER`][crate::NCOPTION_NO_WINCH_SIGHANDLER]
    ///   - [`NCOPTION_SUPPRESS_BANNERS`]
    ///
    pub const fn with_all_options(
        loglevel: NcLogLevel,
        margin_t: NcDim,
        margin_r: NcDim,
        margin_b: NcDim,
        margin_l: NcDim,
        flags: u64,
    ) -> Self {
        Self {
            termtype: null(),
            loglevel,
            renderfp: null_mut(),
            margin_t: margin_t as i32,
            margin_r: margin_r as i32,
            margin_b: margin_b as i32,
            margin_l: margin_l as i32,
            flags,
        }
    }
}

/// # `Nc` Constructors
impl Nc {
    /// New notcurses context (without banners).
    pub fn new<'a>() -> NcResult<&'a mut Nc> {
        Self::with_flags(NCOPTION_SUPPRESS_BANNERS)
    }

    /// New notcurses context, with banners.
    ///
    /// This is the default in the C library.
    pub fn with_banners<'a>() -> NcResult<&'a mut Nc> {
        Self::with_flags(0)
    }

    /// New notcurses context, without an alternate screen (nor banners).
    pub fn without_altscreen<'a>() -> NcResult<&'a mut Nc> {
        Self::with_flags(NCOPTION_NO_ALTERNATE_SCREEN | NCOPTION_SUPPRESS_BANNERS)
    }

    /// New notcurses context, expects `NCOPTION_*` flags.
    pub fn with_flags<'a>(flags: u64) -> NcResult<&'a mut Nc> {
        Self::with_options(NcOptions::with_flags(flags))
    }

    /// New notcurses context, expects [NcOptions].
    pub fn with_options<'a>(options: NcOptions) -> NcResult<&'a mut Nc> {
        let res = unsafe { notcurses_init(&options, null_mut()) };
        error_ref_mut![res, "Notcurses.with_options()"]
    }

    /// New notcurses context, expects [NcLogLevel] and flags.
    pub fn with_debug<'a>(loglevel: NcLogLevel, flags: u64) -> NcResult<&'a mut Nc> {
        Self::with_options(NcOptions::with_all_options(loglevel, 0, 0, 0, 0, flags))
    }
}

/// # `Nc` methods
impl Nc {
    /// Returns the offset into `availcols` at which `cols` ought be output given
    /// the requirements of `align`.
    ///
    /// Returns `-`[`NCRESULT_MAX`][crate::NCRESULT_MAX] if
    /// [NCALIGN_UNALIGNED][crate::NCALIGN_UNALIGNED] or invalid [NcAlign].
    ///
    /// *C style function: [notcurses_align()][crate::notcurses_align].*
    //
    // TODO: handle error rightfully.
    pub fn align(availcols: NcDim, align: NcAlign, cols: NcDim) -> NcResult<()> {
        error![crate::notcurses_align(availcols, align, cols)]
    }

    /// Retrieves the current contents of the specified [NcCell][crate::NcCell]
    /// as last rendered, returning the `EGC` (or None on error) and writing
    /// out the [`NcStyle`] and the [`NcChannels`].
    ///
    // possible BUG? CHECK:
    /// This `EGC` must be freed by the caller.
    ///
    /// *C style function: [notcurses_at_yx()][crate::notcurses_at_yx].*
    pub fn at_yx(
        &mut self,
        y: NcDim,
        x: NcDim,
        stylemask: &mut NcStyle,
        channels: &mut NcChannels,
    ) -> Option<String> {
        let egc = unsafe { crate::notcurses_at_yx(self, x as i32, y as i32, stylemask, channels) };
        if egc.is_null() {
            return None;
        }
        Some(rstring![egc].into())
    }

    /// Returns the bottommost [`NcPlane`] on the standard pile,
    /// of which there is always at least one.
    ///
    /// *C style function: [notcurses_bottom()][crate::notcurses_bottom].*
    pub fn bottom(&mut self) -> &mut NcPlane {
        unsafe { &mut *crate::notcurses_bottom(self) }
    }

    /// Returns true if we can reliably use Unicode Braille.
    ///
    /// See also [NCBLIT_BRAILLE][crate::NCBLIT_BRAILLE].
    ///
    /// *C style function: [notcurses_canbraille()][crate::notcurses_canbraille].*
    pub fn canbraille(&self) -> bool {
        unsafe { crate::notcurses_canbraille(self) }
    }

    /// Returns true if it's possible to set the "hardware" palette.
    ///
    /// Requires the "ccc" terminfo capability.
    ///
    /// *C style function: [notcurses_canchangecolor()][crate::notcurses_canchangecolor].*
    pub fn canchangecolor(&self) -> bool {
        unsafe { crate::notcurses_canchangecolor(self) }
    }

    /// Returns true if fading is possible.
    ///
    /// Fading requires either the "rgb" or "ccc" terminfo capability.
    ///
    /// *C style function: [notcurses_canfade()][crate::notcurses_canfade].*
    pub fn canfade(&self) -> bool {
        unsafe { crate::notcurses_canfade(self) }
    }

    /// Returns true if we can reliably use Unicode half blocks.
    ///
    /// See also [NCBLIT_2x1][crate::NCBLIT_2x1].
    ///
    /// *C style function: [notcurses_canhalfblock()][crate::notcurses_canhalfblock].*
    pub fn canhalfblock(&self) -> bool {
        unsafe { crate::notcurses_canhalfblock(self) }
    }

    /// Returns true if loading images is possible.
    ///
    /// This requires being built against FFmpeg/OIIO.
    ///
    /// *C style function: [notcurses_canopen_images()][crate::notcurses_canopen_images].*
    pub fn canopen_images(&self) -> bool {
        unsafe { crate::notcurses_canopen_images(self) }
    }

    /// Returns true if loading videos is possible.
    ///
    /// This requires being built against FFmpeg.
    ///
    /// *C style function: [notcurses_canopen_videos()][crate::notcurses_canopen_videos].*
    pub fn canopen_videos(&self) -> bool {
        unsafe { crate::notcurses_canopen_videos(self) }
    }

    /// Returns true if we can reliably use Unicode quadrant blocks.
    ///
    /// See also [NCBLIT_2x2][crate::NCBLIT_2x2].
    ///
    /// *C style function: [notcurses_canquadrant()][crate::notcurses_canquadrant].*
    pub fn canquadrant(&self) -> bool {
        unsafe { crate::notcurses_canquadrant(self) }
    }

    /// Returns true if we can reliably use Unicode 13 sextants.
    ///
    /// See also [NCBLIT_3x2][crate::NCBLIT_3x2].
    ///
    /// *C style function: [notcurses_cansextant()][crate::notcurses_cansextant].*
    pub fn cansextant(&self) -> bool {
        unsafe { crate::notcurses_cansextant(self) }
    }

    /// Returns true if it's possible to directly specify RGB values per cell,
    /// or false if it's only possible to use palettes.
    ///
    /// *C style function: [notcurses_cantruecolor()][crate::notcurses_cantruecolor].*
    pub fn cantruecolor(&self) -> bool {
        unsafe { crate::notcurses_cantruecolor(self) }
    }

    /// Returns true if the encoding is UTF-8.
    ///
    /// Requires `LANG` being set to a UTF-8 locale.
    ///
    /// *C style function: [notcurses_canutf8()][crate::notcurses_canutf8].*
    pub fn canutf8(&self) -> bool {
        unsafe { crate::notcurses_canutf8(self) }
    }

    /// Checks for pixel support.
    ///
    /// Returns [`NcPixelImpl`] with a non-zero constant corresponding to some
    /// pixel-blitting mechanism if bitmap support (via any mechanism) has been
    /// detected, or else 0 (NCPIXEL_NONE).
    ///
    /// *C style function: [notcurses_check_pixel_support()][crate::notcurses_check-pixel_support].*
    #[allow(clippy::wildcard_in_or_patterns)]
    pub fn check_pixel_support(&self) -> NcPixelImpl {
        unsafe { crate::notcurses_check_pixel_support(self) }
    }

    /// Disables the terminal's cursor, if supported.
    ///
    /// Immediate effect (no need for a call to notcurses_render()).
    ///
    /// *C style function: [notcurses_cursor_disable()][crate::notcurses_cursor_disable].*
    pub fn cursor_disable(&mut self) -> NcResult<()> {
        error![unsafe { crate::notcurses_cursor_disable(self) }]
    }

    /// Enables the terminal's cursor, if supported, placing it at `y`, `x`.
    ///
    /// Immediate effect (no need for a call to notcurses_render()).
    /// It is an error if `y`, `x` lies outside the standard plane.
    ///
    /// *C style function: [notcurses_cursor_enable()][crate::notcurses_cursor_enable].*
    pub fn cursor_enable(&mut self, y: NcDim, x: NcDim) -> NcResult<()> {
        error![unsafe { crate::notcurses_cursor_enable(self, y as i32, x as i32) }]
    }

    /// Dumps notcurses state to the supplied `debugfp`.
    ///
    /// Output is freeform, and subject to change. It includes geometry of all
    /// planes, from all piles.
    ///
    /// *C style function: [notcurses_debug()][crate::notcurses_debug].*
    pub fn debug(&mut self, debugfp: &mut NcFile) {
        unsafe {
            crate::notcurses_debug(self, debugfp.as_nc_ptr());
        }
    }

    /// Returns the name of the detected terminal.
    ///
    /// *C style function: [notcurses_detected_terminal()][crate::notcurses_detected_terminal].*
    pub fn detected_terminal(&self) -> String {
        rstring![crate::notcurses_detected_terminal(self)].to_string()
    }

    /// Destroys all [`NcPlane`]s other than the stdplane.
    ///
    /// *C style function: [notcurses_drop_planes()][crate::notcurses_drop_planes].*
    pub fn drop_planes(&mut self) {
        unsafe {
            crate::notcurses_drop_planes(self);
        }
    }

    /// Returns a [char] representing a single unicode point.
    ///
    /// If an event is processed, the return value is the `id` field from that
    /// event.
    ///
    /// Provide a None `time` to block at length, a `time` of 0 for non-blocking
    /// operation, and otherwise a timespec to bound blocking.
    ///
    /// *C style function: [notcurses_getc()][crate::notcurses_getc].*
    pub fn getc(&mut self, time: Option<NcTime>, input: Option<&mut NcInput>) -> NcResult<char> {
        let ntime;
        if let Some(time) = time {
            ntime = &time as *const _;
        } else {
            ntime = null();
        }

        let ninput;
        if let Some(input) = input {
            ninput = input as *mut _;
        } else {
            ninput = null_mut();
        }
        let c =
            unsafe { core::char::from_u32_unchecked(crate::notcurses_get(self, ntime, ninput)) };
        if c as u32 as i32 == NCRESULT_ERR {
            return Err(NcError::new());
        }
        Ok(c)
    }

    /// If no event is ready, returns 0.
    ///
    /// *C style function: [notcurses_getc_nblock()][crate::notcurses_getc_nblock].*
    pub fn getc_nblock(&mut self, input: &mut NcInput) -> char {
        crate::notcurses_getc_nblock(self, input)
    }

    /// Blocks until a codepoint or special key is read,
    /// or until interrupted by a signal.
    ///
    /// In the case of a valid read, a 32-bit Unicode codepoint is returned.
    ///
    /// Optionally writes the event details in `input`.
    ///
    /// *C style function: [notcurses_getc_blocking()][crate::notcurses_getc_blocking].*
    pub fn getc_blocking(&mut self, input: Option<&mut NcInput>) -> NcResult<char> {
        let input_txt;
        if cfg!(debug_assertions) {
            input_txt = format!("{:?}", input);
        } else {
            input_txt = String::from("");
        }

        let res = crate::notcurses_getc_blocking(self, input);

        // An invalid read is indicated with -1
        if res as u32 as i32 != -1 {
            Ok(res)
        } else {
            error![-1, &format!("Nc.getc_blocking({:?})", input_txt), res]
        }
    }

    /// Gets a file descriptor suitable for input event poll()ing.
    ///
    /// When this descriptor becomes available, you can call
    /// [getc_nblock()][Nc#method.getc_nblock], and input ought be ready.
    ///
    /// This file descriptor is not necessarily the file descriptor associated
    /// with stdin (but it might be!).
    ///
    /// *C style function: [notcurses_inputready_fd()][crate::notcurses_inputready_fd].*
    pub fn inputready_fd(&mut self) -> NcResult<()> {
        error![unsafe { crate::notcurses_inputready_fd(self) }]
    }

    /// Returns an [`NcBlitter`] from a string representation.
    ///
    /// *C style function: [notcurses_lex_blitter()][crate::notcurses_lex_blitter].*
    pub fn lex_blitter(blitter_str: &str) -> NcResult<NcBlitter> {
        let mut blitter = 0;
        error![
            unsafe { crate::notcurses_lex_blitter(cstring![blitter_str], &mut blitter) },
            "Invalid blitter name", blitter
        ]
    }

    /// Lexes a margin argument according to the standard notcurses definition.
    ///
    /// There can be either a single number, which will define all margins equally,
    /// or there can be four numbers separated by commas.
    ///
    /// *C style function: [notcurses_lex_margins()][crate::notcurses_lex_margins].*
    pub fn lex_margins(margins_str: &str, options: &mut NcOptions) -> NcResult<()> {
        error![unsafe { crate::notcurses_lex_margins(cstring![margins_str], options) }]
    }

    /// Returns an [`NcScale`] from a string representation.
    ///
    /// *C style function: [notcurses_lex_scalemode()][crate::notcurses_lex_scalemode].*
    pub fn lex_scalemode(scalemode_str: &str) -> NcResult<NcScale> {
        let mut scalemode = 0;
        error![
            unsafe { crate::notcurses_lex_scalemode(cstring![scalemode_str], &mut scalemode) },
            "", scalemode
        ]
    }

    /// Returns an [`NcStyle`] from a string representation.
    ///
    /// It is case-insensitive, and supports multiple styles separated by
    /// spaces.
    ///
    /// The supported styles are: `italic`, `underline`, `undercurl`,
    /// `struck`, `bold`, and `none`.
    ///
    /// If a style is are not recognized returns an error.
    ///
    /// *(No equivalent C style function)*
    pub fn lex_styles(styles_str: &str) -> NcResult<NcStyle> {
        let mut style = NCSTYLE_NONE;
        let mut errstr = String::new();

        for s in styles_str.split(' ') {
            match s.to_lowercase().as_str() {
                "italic" => style.add(NCSTYLE_ITALIC),
                "underline" => style.add(NCSTYLE_UNDERLINE),
                "undercurl" => style.add(NCSTYLE_UNDERCURL),
                "struck" => style.add(NCSTYLE_STRUCK),
                "bold" => style.add(NCSTYLE_BOLD),
                "none" => (),
                _ => {
                    errstr.push_str(s);
                    errstr.push(' ');
                }
            }
        }
        if errstr.is_empty() {
            Ok(style)
        } else {
            let _ = errstr.pop();
            Err(NcError::new_msg(&format![
                "the following styles are not recognized: '{}'",
                errstr
            ]))
        }
    }

    /// Disables signals originating from the terminal's line discipline, i.e.
    /// SIGINT (^C), SIGQUIT (^), and SIGTSTP (^Z). They are enabled by default.
    ///
    /// *C style function: [notcurses_linesigs_disable()][crate::notcurses_linesigs_disable].*
    pub fn linesigs_disable(&mut self) -> NcResult<()> {
        error![unsafe { crate::notcurses_linesigs_disable(self) }]
    }

    /// Restores signals originating from the terminal's line discipline, i.e.
    /// SIGINT (^C), SIGQUIT (^), and SIGTSTP (^Z), if disabled.
    ///
    /// *C style function: [notcurses_linesigs_enable()][crate::notcurses_linesigs_enable].*
    pub fn linesigs_enable(&mut self) -> NcResult<()> {
        error![unsafe { crate::notcurses_linesigs_enable(self) }]
    }

    /// Disables mouse events.
    ///
    /// Any events in the input queue can still be delivered.
    ///
    /// *C style function: [notcurses_mouse_disable()][crate::notcurses_mouse_disable].*
    pub fn mouse_disable(&mut self) -> NcResult<()> {
        error![unsafe { crate::notcurses_mouse_disable(self) }]
    }

    /// Enable the mouse in "button-event tracking" mode with focus detection
    /// and UTF8-style extended coordinates.
    ///
    /// On success, mouse events will be published to [getc()][Nc#method.getc].
    ///
    /// *C style function: [notcurses_mouse_enable()][crate::notcurses_mouse_enable].*
    pub fn mouse_enable(&mut self) -> NcResult<()> {
        error![
            unsafe { crate::notcurses_mouse_enable(self) },
            "Nc.mouse_enable()"
        ]
    }

    /// Returns the number of simultaneous colors claimed to be supported,
    /// if there is color support.
    ///
    /// Note that several terminal emulators advertise more colors than they
    /// actually support, downsampling internally.
    ///
    /// *C style function: [notcurses_palette_size()][crate::notcurses_palette_size].*
    pub fn palette_size(&self) -> NcResult<u32> {
        let res = unsafe { crate::notcurses_palette_size(self) };
        if res == 1 {
            return Err(NcError::with_msg(1, "No color support ← Nc.palette_size()"));
        }
        Ok(res)
    }

    /// Refreshes the physical screen to match what was last rendered (i.e.,
    /// without reflecting any changes since the last call to
    /// [`render`][crate::Nc#method.render]).
    ///
    /// Returns the current screen geometry (`y`, `x`).
    ///
    /// This is primarily useful if the screen is externally corrupted, or if an
    /// [NCKEY_RESIZE][crate::NCKEY_RESIZE] event has been read and you're not
    /// yet ready to render.
    ///
    /// *C style function: [notcurses_refresh()][crate::notcurses_refresh].*
    //
    pub fn refresh(&mut self) -> NcResult<(NcDim, NcDim)> {
        let (mut y, mut x) = (0, 0);
        error![
            unsafe { crate::notcurses_refresh(self, &mut y, &mut x) },
            "",
            (y as NcDim, x as NcDim)
        ]
    }

    /// Renders and rasterizes the standard pile in one shot. Blocking call.
    ///
    /// *C style function: [notcurses_render()][crate::notcurses_render].*
    pub fn render(&mut self) -> NcResult<()> {
        error![unsafe { crate::notcurses_render(self) }, "Nc.render()"]
    }

    /// Performs the rendering and rasterization portion of
    /// [`render`][Nc#method.render] but do not write the resulting buffer
    /// out to the terminal.
    ///
    /// Using this function, the user can control the writeout process,
    /// and render a second frame while writing another.
    ///
    // possible BUG? CHECK:
    /// The returned buffer must be freed by the caller.
    ///
    /// *C style function: [notcurses_render_to_buffer()][crate::notcurses_render_to_buffer].*
    //
    // CHECK that this works.
    pub fn render_to_buffer(&mut self, buffer: &mut Vec<u8>) -> NcResult<()> {
        #[allow(unused_mut)] // CHECK whether it actually works without the mut
        let mut len = buffer.len() as u32;

        // https://github.com/dankamongmen/notcurses/issues/1339
        #[cfg(any(target_arch = "x86_64", target_arch = "i686", target_arch = "x86"))]
        let mut buf = buffer.as_mut_ptr() as *mut i8;
        #[cfg(not(any(target_arch = "x86_64", target_arch = "i686", target_arch = "x86")))]
        let mut buf = buffer.as_mut_ptr() as *mut u8;

        error![unsafe { crate::notcurses_render_to_buffer(self, &mut buf, &mut len.into()) }]
    }

    /// Writes the last rendered frame, in its entirety, to 'fp'.
    ///
    /// If [`render`][Nc#method.render] has not yet been called,
    /// nothing will be written.
    ///
    /// *C style function: [notcurses_render_to_file()][crate::notcurses_render_to_file].*
    pub fn render_to_file(&mut self, fp: &mut NcFile) -> NcResult<()> {
        error![unsafe { crate::notcurses_render_to_file(self, fp.as_nc_ptr()) }]
    }

    /// Acquires an atomic snapshot of the notcurses object's stats.
    ///
    /// *C style function: [notcurses_stats()][crate::notcurses_stats].*
    pub fn stats(&mut self, stats: &mut NcStats) {
        unsafe {
            crate::notcurses_stats(self, stats);
        }
    }

    /// Allocates an [`NcStats`] object.
    ///
    /// Use this rather than allocating your own, since future versions of
    /// notcurses might enlarge this structure.
    ///
    /// *C style function: [notcurses_stats_alloc()][crate::notcurses_stats_alloc].*
    pub fn stats_alloc(&mut self) -> &mut NcStats {
        unsafe { &mut *crate::notcurses_stats_alloc(self) }
    }

    /// Resets all cumulative stats (immediate ones, such as fbbytes, are not reset).
    ///
    /// *C style function: [notcurses_stats_reset()][crate::notcurses_stats_reset].*
    pub fn stats_reset(&mut self, stats: &mut NcStats) {
        unsafe {
            crate::notcurses_stats_reset(self, stats);
        }
    }

    // TODO: decide what to do with these two:
    //
    // /// [notcurses_stdplane()][crate::notcurses_stdplane], plus free bonus
    // /// dimensions written to non-NULL y/x!
    // ///
    // /// *C style function: [notcurses_stddim_yx()][crate::notcurses_stddim_yx].*
    // #[inline]
    // pub fn stddim_yx<'a>(
    //     &'a mut self,
    //     y: &mut NcDim,
    //     x: &mut NcDim,
    // ) -> NcResult<&'a mut NcPlane> {
    //     crate::notcurses_stddim_yx(self, y, x)
    // }

    // /// [stdplane_const()][Notcurses#method.stdplane_const], plus free
    // /// bonus dimensions written to non-NULL y/x!
    // ///
    // /// *C style function: [notcurses_stddim_yx()][crate::notcurses_stddim_yx].*
    // #[inline]
    // pub fn stddim_yx_const<'a>(
    //     &'a self,
    //     y: &mut NcDim,
    //     x: &mut NcDim,
    // ) -> NcResult<&'a NcPlane> {
    //     crate::notcurses_stddim_yx_const(self, y, x)
    // }

    /// Returns a mutable reference to the standard [`NcPlane`] for this terminal.
    ///
    /// The standard plane always exists, and its origin is always at the
    /// uppermost, leftmost cell.
    ///
    /// *C style function: [notcurses_stdplane()][crate::notcurses_stdplane].*
    pub fn stdplane<'a>(&mut self) -> &'a mut NcPlane {
        unsafe { &mut *crate::notcurses_stdplane(self) }
    }

    /// Returns a reference to the standard [`NcPlane`] for this terminal.
    ///
    /// The standard plane always exists, and its origin is always at the
    /// uppermost, leftmost cell.
    ///
    /// *C style function: [notcurses_stdplane_const()][crate::notcurses_stdplane_const].*
    pub fn stdplane_const<'a>(&self) -> &'a NcPlane {
        unsafe { &*crate::notcurses_stdplane_const(self) }
    }

    /// Destroys the notcurses context.
    ///
    /// *C style function: [notcurses_stop()][crate::notcurses_stop].*
    pub fn stop(&mut self) -> NcResult<()> {
        error![unsafe { crate::notcurses_stop(self) }]
    }

    /// Gets the name of an [`NcBlitter`] blitter.
    ///
    /// *C style function: [notcurses_str_blitter()][crate::notcurses_str_blitter].*
    pub fn str_blitter(blitter: NcBlitter) -> String {
        rstring![crate::notcurses_str_blitter(blitter)].to_string()
    }

    /// Gets the name of an [`NcScale`] scaling mode.
    ///
    /// *C style function: [notcurses_str_scalemode()][crate::notcurses_str_scalemode].*
    pub fn str_scalemode(scalemode: NcScale) -> String {
        rstring![crate::notcurses_str_scalemode(scalemode)].to_string()
    }

    /// Gets the lowercase name (or names) of the styles included in an [`NcStyle`].
    ///
    /// *(No equivalent C style function)*
    pub fn str_styles(style: NcStyle) -> String {
        let mut string = String::new();
        for s in style.to_vec() {
            string.push_str(match s {
                NCSTYLE_ITALIC => "italic",
                NCSTYLE_UNDERLINE => "underline",
                NCSTYLE_UNDERCURL => "undercurl",
                NCSTYLE_STRUCK => "struck",
                NCSTYLE_BOLD => "bold",
                #[allow(unreachable_patterns)] // FIXME
                NCSTYLE_NONE => "none",
                _ => "none",
            });
            string.push(' ');
        }
        let _ = string.pop();
        string
    }

    /// Returns an [`NcStyle`] with the supported curses-style attributes.
    ///
    /// The attribute is only indicated as supported if the terminal can support
    /// it together with color.
    ///
    /// For more information, see the "ncv" capability in terminfo(5).
    ///
    /// *C style function: [notcurses_supported_styles()][crate::notcurses_supported_styles].*
    pub fn supported_styles(&self) -> NcStyle {
        unsafe { crate::notcurses_supported_styles(self) as NcStyle }
    }

    /// Returns our current idea of the terminal dimensions in rows and cols.
    ///
    /// *C style function: [notcurses_term_dim_yx()][crate::notcurses_term_dim_yx].*
    pub fn term_dim_yx(&self) -> (NcDim, NcDim) {
        crate::notcurses_term_dim_yx(self)
    }

    /// Returns the topmost [`NcPlane`], of which there is always at least one.
    ///
    /// *C style function: [notcurses_top()][crate::notcurses_top].*
    pub fn top(&mut self) -> &mut NcPlane {
        unsafe { &mut *crate::notcurses_top(self) }
    }

    /// Returns a human-readable string describing the running notcurses version.
    ///
    /// *C style function: [notcurses_version()][crate::notcurses_version].*
    pub fn version() -> String {
        rstring![crate::notcurses_version()].to_string()
    }

    /// Returns the running notcurses version components
    /// (major, minor, patch, tweak).
    ///
    /// *C style function: [notcurses_version_components()][crate::notcurses_version_components].*
    pub fn version_components() -> (u32, u32, u32, u32) {
        let (mut major, mut minor, mut patch, mut tweak) = (0, 0, 0, 0);
        unsafe {
            crate::notcurses_version_components(&mut major, &mut minor, &mut patch, &mut tweak);
        }
        (major as u32, minor as u32, patch as u32, tweak as u32)
    }
}
