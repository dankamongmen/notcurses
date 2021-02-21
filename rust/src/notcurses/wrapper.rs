//! `FullMode` wrapper struct and traits implementations.

use std::ops::{Deref, DerefMut};

use crate::{
    raw_wrap, NcAlign, NcBlitter, NcDim, NcLogLevel, NcResult, NcScale, Notcurses, NotcursesOptions,
};

/// Safe wrapper around [Notcurses], the main struct of the TUI library.
pub struct FullMode<'a> {
    pub(crate) raw: &'a mut Notcurses,
}

impl<'a> AsRef<Notcurses> for FullMode<'a> {
    fn as_ref(&self) -> &Notcurses {
        self.raw
    }
}

impl<'a> AsMut<Notcurses> for FullMode<'a> {
    fn as_mut(&mut self) -> &mut Notcurses {
        self.raw
    }
}

impl<'a> Deref for FullMode<'a> {
    type Target = Notcurses;

    fn deref(&self) -> &Self::Target {
        self.as_ref()
    }
}

impl<'a> DerefMut for FullMode<'a> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        self.as_mut()
    }
}

impl<'a> Drop for FullMode<'a> {
    /// Destroys the FullMode context.
    fn drop(&mut self) {
        let _ = self.raw.stop();
    }
}

/// # Constructors and methods overriden from Notcurses
impl<'a> FullMode<'a> {
    // wrap constructors

    /// New FullMode (without banners).
    pub fn new() -> NcResult<Self> {
        raw_wrap![Notcurses::new()]
    }

    /// New FullMode, with banners.
    pub fn with_banners() -> NcResult<Self> {
        raw_wrap![Notcurses::with_banners()]
    }

    /// New FullMode, without an alternate screen (nor banners).
    pub fn without_altscreen() -> NcResult<Self> {
        raw_wrap![Notcurses::without_altscreen()]
    }

    /// New FullMode, expects `NCOPTION_*` flags.
    pub fn with_flags(flags: u64) -> NcResult<Self> {
        raw_wrap![Notcurses::with_flags(flags)]
    }

    /// New FullMode, expects [NotcursesOptions].
    pub fn with_options(options: NotcursesOptions) -> NcResult<Self> {
        raw_wrap![Notcurses::with_options(options)]
    }

    /// New FullMode, expects [NcLogLevel] and flags.
    pub fn with_debug(loglevel: NcLogLevel, flags: u64) -> NcResult<Self> {
        raw_wrap![Notcurses::with_debug(loglevel, flags)]
    }

    // disable destructor

    /// Since FullMode already implements [Drop](#impl-Drop),
    /// this function is made no-op.
    pub fn stop(&mut self) -> NcResult<()> {
        Ok(())
    }

    // wrap associated functions

    /// Returns the offset into `availcols` at which `cols` ought be output given
    /// the requirements of `align`.
    pub fn align(availcols: NcDim, align: NcAlign, cols: NcDim) -> NcResult<()> {
        Notcurses::align(availcols, align, cols)
    }

    /// Gets the name of an [NcBlitter] blitter.
    pub fn str_blitter(blitter: NcBlitter) -> String {
        Notcurses::str_blitter(blitter)
    }

    /// Gets the name of an [NcScale] scaling mode.
    pub fn str_scalemode(scalemode: NcScale) -> String {
        Notcurses::str_scalemode(scalemode)
    }

    /// Returns an [NcBlitter] from a string representation.
    pub fn lex_blitter(op: &str) -> NcResult<NcBlitter> {
        Notcurses::lex_blitter(op)
    }

    /// Lexes a margin argument according to the standard Notcurses definition.
    ///
    /// There can be either a single number, which will define all margins equally,
    /// or there can be four numbers separated by commas.
    ///
    pub fn lex_margins(op: &str, options: &mut NotcursesOptions) -> NcResult<()> {
        Notcurses::lex_margins(op, options)
    }

    /// Returns an [NcScale] from a string representation.
    pub fn lex_scalemode(op: &str) -> NcResult<NcScale> {
        Notcurses::lex_scalemode(op)
    }

    /// Returns a human-readable string describing the running FullMode version.
    pub fn version() -> String {
        Notcurses::version()
    }

    /// Returns the running Notcurses version components
    /// (major, minor, patch, tweak).
    pub fn version_components() -> (u32, u32, u32, u32) {
        Notcurses::version_components()
    }
}
