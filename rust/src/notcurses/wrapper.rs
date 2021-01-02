//! `FullMode` wrapper struct and traits implementations.

use std::ops::{Deref, DerefMut};

use crate::{
    raw_wrap, NcAlign, NcBlitter, NcDimension, NcLogLevel, NcNotcurses, NcNotcursesOptions,
    NcResult, NcScale,
};

/// The main struct of the TUI library (full mode).
///
/// Safely wraps an [NcNotcurses],
/// and implements Drop, AsRef, AsMut, Deref & DerefMut around it.
pub struct FullMode<'a> {
    raw: &'a mut NcNotcurses,
}

impl<'a> AsRef<NcNotcurses> for FullMode<'a> {
    fn as_ref(&self) -> &NcNotcurses {
        self.raw
    }
}

impl<'a> AsMut<NcNotcurses> for FullMode<'a> {
    fn as_mut(&mut self) -> &mut NcNotcurses {
        self.raw
    }
}

impl<'a> Deref for FullMode<'a> {
    type Target = NcNotcurses;

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

/// # Constructors and methods overriden from NcNotcurses
impl<'a> FullMode<'a> {
    // wrap constructors

    /// New FullMode (without banners).
    pub fn new() -> NcResult<Self> {
        raw_wrap![NcNotcurses::new()]
    }

    /// New FullMode, without banners.
    pub fn with_banners() -> NcResult<Self> {
        raw_wrap![NcNotcurses::with_banners()]
    }

    /// New FullMode, without an alternate screen (nor banners).
    pub fn without_altscreen() -> NcResult<Self> {
        raw_wrap![NcNotcurses::without_altscreen()]
    }

    /// New FullMode, expects `NCOPTION_*` flags.
    pub fn with_flags(flags: u64) -> NcResult<Self> {
        raw_wrap![NcNotcurses::with_flags(flags)]
    }

    /// New FullMode, expects [NcNotcursesOptions].
    pub fn with_options(options: NcNotcursesOptions) -> NcResult<Self> {
        raw_wrap![NcNotcurses::with_options(options)]
    }

    /// New FullMode, expects [NcLogLevel] and flags.
    pub fn with_debug(loglevel: NcLogLevel, flags: u64) -> NcResult<Self> {
        raw_wrap![NcNotcurses::with_debug(loglevel, flags)]
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
    pub fn align(availcols: NcDimension, align: NcAlign, cols: NcDimension) -> NcResult<()> {
        NcNotcurses::align(availcols, align, cols)
    }

    /// Gets the name of an [NcBlitter] blitter.
    pub fn str_blitter(blitter: NcBlitter) -> String {
        NcNotcurses::str_blitter(blitter)
    }

    /// Gets the name of an [NcScale] scaling mode.
    pub fn str_scalemode(scalemode: NcScale) -> String {
        NcNotcurses::str_scalemode(scalemode)
    }

    /// Returns an [NcBlitter] from a string representation.
    pub fn lex_blitter(op: &str) -> NcResult<NcBlitter> {
        NcNotcurses::lex_blitter(op)
    }

    /// Lexes a margin argument according to the standard NcNotcurses definition.
    ///
    /// There can be either a single number, which will define all margins equally,
    /// or there can be four numbers separated by commas.
    ///
    pub fn lex_margins(op: &str, options: &mut NcNotcursesOptions) -> NcResult<()> {
        NcNotcurses::lex_margins(op, options)
    }

    /// Returns an [NcScale] from a string representation.
    pub fn lex_scalemode(op: &str) -> NcResult<NcScale> {
        NcNotcurses::lex_scalemode(op)
    }

    /// Returns a human-readable string describing the running FullMode version.
    pub fn version() -> String {
        NcNotcurses::version()
    }

    /// Returns the running NcNotcurses version components
    /// (major, minor, patch, tweak).
    pub fn version_components() -> (u32, u32, u32, u32) {
        NcNotcurses::version_components()
    }
}
