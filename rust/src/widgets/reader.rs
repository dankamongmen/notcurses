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

use crate::{
    ncreader_create,
    types::{NcPlane, NcReader, NcReaderOptions},
};

/// `NcReader` constructor (wraps `ncreader_create`)
///
/// NcReader provides freeform input in a (possibly multiline) region, supporting
/// optional readline keybindings. takes ownership of its plane, destroying it
/// on any error (ncreader_destroy() otherwise destroys the ncplane).
impl NcReader {
    /// Simple `NcReader` constructor
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
