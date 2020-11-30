//! Handy [`Notcurses`] and [`NotcursesOptions`] constructors

use core::ptr::{null, null_mut};

use crate::{
    notcurses_init, NcLogLevel, Notcurses, NotcursesOptions, NCOPTION_NO_ALTERNATE_SCREEN,
    NCOPTION_SUPPRESS_BANNERS,
};

impl NotcursesOptions {
    /// Simple `NotcursesOptions` constructor
    pub fn new() -> Self {
        Self::with_all_options(0, 0, 0, 0, 0, 0)
    }

    /// `NotcursesOptions` constructor with customizable margins
    pub fn with_margins(top: i32, right: i32, bottom: i32, left: i32) -> Self {
        Self::with_all_options(0, top, right, bottom, left, 0)
    }

    /// `NotcursesOptions` constructor with customizable flags
    pub fn with_flags(flags: u64) -> Self {
        Self::with_all_options(0, 0, 0, 0, 0, flags)
    }

    /// `NotcursesOptions` constructor with all the options available
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
    ///   - [`NCOPTION_INHIBIT_SETLOCALE`]
    ///   - [`NCOPTION_NO_ALTERNATE_SCREEN`]
    ///   - [`NCOPTION_NO_FONT_CHANGES`]
    ///   - [`NCOPTION_NO_QUIT_SIGHANDLERS`]
    ///   - [`NCOPTION_NO_WINCH_SIGHANDLER`]
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

impl Notcurses {
    /// `Notcurses` simple constructor with clean output
    pub unsafe fn new<'a>() -> &'a mut Notcurses {
        let options = NotcursesOptions::with_flags(NCOPTION_SUPPRESS_BANNERS);
        &mut *notcurses_init(&options, null_mut())
    }

    /// `Notcurses` simple constructor, showing banners
    pub unsafe fn with_banners<'a>() -> &'a mut Notcurses {
        &mut *notcurses_init(&NotcursesOptions::new(), null_mut())
    }

    /// `Notcurses` simple constructor without an alternate screen
    pub unsafe fn without_altscreen<'a>() -> &'a mut Notcurses {
        let options = NotcursesOptions::with_flags(NCOPTION_NO_ALTERNATE_SCREEN);
        &mut *notcurses_init(&options, null_mut())
    }

    /// `Notcurses` simple constructor without an alternate screen
    pub unsafe fn without_altscreen_nor_banners<'a>() -> &'a mut Notcurses {
        let options =
            NotcursesOptions::with_flags(NCOPTION_NO_ALTERNATE_SCREEN | NCOPTION_SUPPRESS_BANNERS);
        &mut *notcurses_init(&options, null_mut())
    }

    /// `Notcurses` constructor with options
    pub unsafe fn with_options<'a>(options: &NotcursesOptions) -> &'a mut Notcurses {
        &mut *notcurses_init(options, null_mut())
    }
}
