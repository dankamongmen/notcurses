//! `DirectMode` wrapper struct and traits implementations.

use std::ops::{Deref, DerefMut};

use crate::{
    raw_wrap, NcDirect, NcResult
};

/// Minimal notcurses instance for styling text.
///
/// Safely wraps an [NcDirect],
/// and implements Drop, AsRef, AsMut, Deref & DerefMut around it.
pub struct DirectMode<'a> {
    pub(crate) raw: &'a mut NcDirect,
}

impl<'a> AsRef<NcDirect> for DirectMode<'a> {
    fn as_ref(&self) -> &NcDirect {
        self.raw
    }
}

impl<'a> AsMut<NcDirect> for DirectMode<'a> {
    fn as_mut(&mut self) -> &mut NcDirect {
        self.raw
    }
}

impl<'a> Deref for DirectMode<'a> {
    type Target = NcDirect;

    fn deref(&self) -> &Self::Target {
        self.as_ref()
    }
}

impl<'a> DerefMut for DirectMode<'a> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        self.as_mut()
    }
}

impl<'a> Drop for DirectMode<'a> {
    /// Destroys the DirectMode context.
    fn drop(&mut self) {
        let _ = self.raw.stop();
    }
}

/// # Constructors and methods overriden from NcDirect
impl<'a> DirectMode<'a> {
    // wrap constructors

    /// New DirectMode (without banners).
    pub fn new() -> NcResult<Self> {
        raw_wrap![NcDirect::new()]
    }

    /// New DirectMode, expects `NCOPTION_*` flags.
    pub fn with_flags(flags: u64) -> NcResult<Self> {
        raw_wrap![NcDirect::with_flags(flags)]
    }

    // disable destructor

    /// Since DirectMode already implements [Drop](#impl-Drop),
    /// this function is made no-op.
    pub fn stop(&mut self) -> NcResult<()> {
        Ok(())
    }
}
