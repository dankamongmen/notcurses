//! `NcReader` widget

// functions already exported by bindgen : 11
// ------------------------------------------
// ncreader_clear
// ncreader_contents
// ncreader_create
// ncreader_destroy
// ncreader_move_down
// ncreader_move_left
// ncreader_move_right
// ncreader_move_up
// ncreader_offer_input
// ncreader_plane
// ncreader_write_egc
//
// static inline functions total: 0
// -------------------------------------------

mod types;
pub use types::{NcReader, NcReaderOptions};
pub use types::{
    NCREADER_OPTION_CURSOR, NCREADER_OPTION_HORSCROLL, NCREADER_OPTION_NOCMDKEYS,
    NCREADER_OPTION_VERSCROLL,
};

use crate::{ncreader_create, NcPlane};

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
