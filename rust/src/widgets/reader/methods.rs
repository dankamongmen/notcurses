//! `NcReader*` methods and associated functions.

use crate::{ncreader_create, NcPlane, NcReader, NcReaderOptions};

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
