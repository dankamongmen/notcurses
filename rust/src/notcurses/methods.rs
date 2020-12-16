//! `Notcurses*` methods and associated functions.

use core::ptr::{null, null_mut};

use crate::{
    notcurses_init, NcLogLevel, NcPlane, NcResult, Notcurses, NotcursesOptions,
    NCOPTION_NO_ALTERNATE_SCREEN, NCOPTION_SUPPRESS_BANNERS,
};

/// # `NotcursesOptions` Constructors
impl NotcursesOptions {
    /// New NotcursesOptions.
    pub fn new() -> Self {
        Self::with_all_options(0, 0, 0, 0, 0, 0)
    }

    /// New NotcursesOptions, with margins.
    pub fn with_margins(top: i32, right: i32, bottom: i32, left: i32) -> Self {
        Self::with_all_options(0, top, right, bottom, left, 0)
    }

    /// New NotcursesOptions, with flags.
    pub fn with_flags(flags: u64) -> Self {
        Self::with_all_options(0, 0, 0, 0, 0, flags)
    }

    /// New NotcursesOptions, with all the options.
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
    pub fn with_all_options(
        loglevel: NcLogLevel,
        margin_t: i32,
        margin_r: i32,
        margin_b: i32,
        margin_l: i32,
        flags: u64,
    ) -> Self {
        Self {
            termtype: null(),
            renderfp: null_mut(),
            loglevel,
            margin_t,
            margin_r,
            margin_b,
            margin_l,
            flags,
        }
    }
}

/// # `Notcurses` Constructors
impl Notcurses {
    /// Returns a Notcurses context (without banners).
    pub fn new<'a>() -> &'a mut Notcurses {
        let options = NotcursesOptions::with_flags(NCOPTION_SUPPRESS_BANNERS);
        unsafe { &mut *notcurses_init(&options, null_mut()) }
    }

    /// Returns a Notcurses context, with banners. The default in the C library.
    pub fn with_banners<'a>() -> &'a mut Notcurses {
        unsafe { &mut *notcurses_init(&NotcursesOptions::new(), null_mut()) }
    }

    /// Returns a Notcurses context, without an alternate screen (nor banners).
    pub fn without_altscreen<'a>() -> &'a mut Notcurses {
        let options =
            NotcursesOptions::with_flags(NCOPTION_NO_ALTERNATE_SCREEN | NCOPTION_SUPPRESS_BANNERS);
        unsafe { &mut *notcurses_init(&options, null_mut()) }
    }

    /// Returns a Notcurses context, without an alternate screen, with banners.
    pub fn without_altscreen_nor_banners<'a>() -> &'a mut Notcurses {
        let options = NotcursesOptions::with_flags(NCOPTION_NO_ALTERNATE_SCREEN);
        unsafe { &mut *notcurses_init(&options, null_mut()) }
    }

    /// Returns a Notcurses context, expects [NotcursesOptions].
    pub fn with_flags<'a>(flags: u64) -> &'a mut Notcurses {
        let options = NotcursesOptions::with_flags(flags);
        unsafe { &mut *notcurses_init(&options, null_mut()) }
    }

    /// Returns a Notcurses context, expects [NotcursesOptions].
    pub fn with_options<'a>(options: &NotcursesOptions) -> &'a mut Notcurses {
        unsafe { &mut *notcurses_init(options, null_mut()) }
    }
}

/// # `Notcurses` methods
impl Notcurses {
    /// Returns a mutable reference to the standard [NcPlane] for this terminal.
    ///
    /// The standard plane always exists, and its origin is always at the
    /// uppermost, leftmost cell.
    pub fn stdplane<'a>(&mut self) -> &'a mut NcPlane {
        unsafe { &mut *crate::notcurses_stdplane(self) }
    }

    /// Returns a reference to the standard [NcPlane] for this terminal.
    ///
    /// The standard plane always exists, and its origin is always at the
    /// uppermost, leftmost cell.
    pub fn stdplane_const<'a>(&self) -> &'a NcPlane {
        unsafe { &*crate::notcurses_stdplane_const(self) }
    }

    /// Destroy the Notcurses context.
    pub fn stop(&mut self) -> NcResult {
        unsafe { crate::notcurses_stop(self) }
    }

    ///
    pub fn render(&mut self) -> NcResult {
        unsafe { crate::notcurses_render(self) }
    }
}
