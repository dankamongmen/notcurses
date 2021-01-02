//! `Direct` wrapper struct and traits implementations.

use std::ops::{Deref, DerefMut};

use crate::{
    raw_wrap, NcDirect, NcResult
};

/// Minimal notcurses instance for styling text.
///
/// Safely wraps an [NcDirect],
/// and implements Drop, AsRef, AsMut, Deref & DerefMut around it.
pub struct Direct<'a> {
    raw: &'a mut NcDirect,
}

impl<'a> AsRef<NcDirect> for Direct<'a> {
    fn as_ref(&self) -> &NcDirect {
        self.raw
    }
}

impl<'a> AsMut<NcDirect> for Direct<'a> {
    fn as_mut(&mut self) -> &mut NcDirect {
        self.raw
    }
}

impl<'a> Deref for Direct<'a> {
    type Target = NcDirect;

    fn deref(&self) -> &Self::Target {
        self.as_ref()
    }
}

impl<'a> DerefMut for Direct<'a> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        self.as_mut()
    }
}

impl<'a> Drop for Direct<'a> {
    /// Destroys the Direct context.
    fn drop(&mut self) {
        let _ = self.raw.stop();
    }
}

/// # Constructors and methods overriden from NcDirect
impl<'a> Direct<'a> {
    // wrap constructors

    /// New Direct (without banners).
    pub fn new() -> NcResult<Self> {
        raw_wrap![NcDirect::new()]
    }

    /// New Direct, expects `NCOPTION_*` flags.
    pub fn with_flags(flags: u64) -> NcResult<Self> {
        raw_wrap![NcDirect::with_flags(flags)]
    }

    // disable destructor

    /// Since Direct already implements [Drop](#impl-Drop),
    /// this function is made no-op.
    pub fn stop(&mut self) -> NcResult<()> {
        Ok(())
    }
}
