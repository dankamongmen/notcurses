//! `NcD` wrapper struct and traits implementations.

use std::ops::{Deref, DerefMut};

use crate::{raw_wrap, NcDirect, NcResult};

/// Safe wrapper around [NcDirect], minimal notcurses instance for styling text.
#[derive(Debug)]
pub struct NcD<'a> {
    pub(crate) raw: &'a mut NcDirect,
}

impl<'a> AsRef<NcDirect> for NcD<'a> {
    fn as_ref(&self) -> &NcDirect {
        self.raw
    }
}

impl<'a> AsMut<NcDirect> for NcD<'a> {
    fn as_mut(&mut self) -> &mut NcDirect {
        self.raw
    }
}

impl<'a> Deref for NcD<'a> {
    type Target = NcDirect;

    fn deref(&self) -> &Self::Target {
        self.as_ref()
    }
}

impl<'a> DerefMut for NcD<'a> {
    fn deref_mut(&mut self) -> &mut Self::Target {
        self.as_mut()
    }
}

impl<'a> Drop for NcD<'a> {
    /// Destroys the NcD context.
    fn drop(&mut self) {
        let _ = self.raw.stop();
    }
}

/// # Constructors and methods overriden from NcDirect
impl<'a> NcD<'a> {
    // wrap constructors

    /// New NcD (without banners).
    pub fn new() -> NcResult<Self> {
        raw_wrap![NcDirect::new()]
    }

    /// New NcD, expects `NCOPTION_*` flags.
    pub fn with_flags(flags: u64) -> NcResult<Self> {
        raw_wrap![NcDirect::with_flags(flags)]
    }

    // disable destructor

    /// Since NcD already implements [Drop](#impl-Drop),
    /// this function is made no-op.
    pub fn stop(&mut self) -> NcResult<()> {
        Ok(())
    }
}
