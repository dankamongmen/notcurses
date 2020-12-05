//! `NcReader*` methods and associated functions.

use crate::{ncreader_create, NcPlane, NcReader, NcReaderOptions};

/// # `NcReaderOptions` Constructors
impl NcReaderOptions {
    /// `NcReaderOptions` simple constructor
    pub fn new() -> Self {
        Self {
            // channels used for input
            tchannels: 0,
            // attributes used for input
            tattrword: 0,
            // bitfield of NCREADER_OPTION_*
            flags: 0,
        }
    }
}

/// # `NcReader` Constructors
impl NcReader {
    /// `NcReader` simple constructor
    pub unsafe fn new<'a>(plane: &mut NcPlane) -> &'a mut Self {
        Self::with_options(plane, &NcReaderOptions::new())
    }

    /// `NcReader` constructor with options
    pub unsafe fn with_options<'a>(plane: &mut NcPlane, options: &NcReaderOptions) -> &'a mut Self {
        &mut *ncreader_create(plane, options)
    }
}

impl Drop for NcReader {
    /// Destroys the NcReader and its bound [NcPlane].
    /// 
    /// See the `destroy` method or `ncreader_destroy` for more options.
    fn drop(&mut self) {
        unsafe { crate::ncreader_destroy(self, core::ptr::null_mut()); }
    }
}
