// -----------------------------------------------------------------------------
// Now none of these functions can't fail and therefore don't return errors.
// -----------------------------------------------------------------------------
//
// functions already exported by bindgen : 3
// -----------------------------------------
// palette256_free
// palette256_new
// palette256_use
//
// static inline functions total: 3
// ----------------------------------------- (done / remaining)
// (+) implement : 3 / 0
// (#) unit tests: 0 / 3
// -----------------------------------------
//+ palette256_get_rgb
//+ palette256_set
//+ palette256_set_rgb

use crate::{
    channel_rgb8, channel_set, channel_set_rgb8,
    types::{NcChannel, NcColor, NcPalette, NcPaletteIndex, NcRgb},
};

/// Set the different color components of an entry inside a palette store.
#[inline]
pub fn palette256_set_rgb(
    palette: &mut NcPalette,
    idx: NcPaletteIndex,
    red: NcColor,
    green: NcColor,
    blue: NcColor,
) {
    channel_set_rgb8(&mut palette.chans[idx as usize], red, green, blue)
}

/// Same as `palette256_set_rgb()` but set an assembled 24 bit channel at once.
#[inline]
pub fn palette256_set(palette: &mut NcPalette, idx: NcPaletteIndex, rgb: NcRgb) {
    channel_set(&mut palette.chans[idx as usize], rgb);
}

/// Extract the three 8-bit R/G/B components from an entry inside a palette store.
#[inline]
pub fn palette256_get_rgb(
    palette: &NcPalette,
    idx: NcPaletteIndex,
    red: &mut NcColor,
    green: &mut NcColor,
    blue: &mut NcColor,
) -> NcChannel {
    channel_rgb8(palette.chans[idx as usize], red, green, blue)
}

#[cfg(test)]
mod test {
    // use super::nc;
    // use serial_test::serial;
    /*
    #[test]
    #[serial]
    fn () {
    }
    */
}
