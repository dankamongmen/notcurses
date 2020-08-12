// ---------------------------------------------------------------------------------------
// NOTE: Now none of these functions can't fail and therefore don't return errors.
// ---------------------------------------------------------------------------------------
//
// functions already exported by bindgen : 3
// -----------------------------------------
// palette256_free
// palette256_new
// palette256_use
//
// static inline functions to reimplement: 3
// ----------------------------------------- (done / wont / remaining)
// - implement : 1 / 0 / 2
// - unit tests: 0 / 0 / 3
// --------------- (+) implemented (#) + unit test (x) wont implement
//+palette256_get_rgb
//+palette256_set
//+palette256_set_rgb

use crate as ffi;
use crate::types::Color;
use ffi::{Channel, PaletteIndex, Rgb};

/// Set the different color components of an entry inside a palette store.
// TODO: TEST
#[inline]
pub fn palette256_set_rgb(
    palette: &mut ffi::palette256,
    idx: PaletteIndex,
    red: Color,
    green: Color,
    blue: Color,
) {
    ffi::channel_set_rgb(&mut palette.chans[idx as usize], red, green, blue)
}

/// Same as `palette256_set_rgb()` but set an assembled 24 bit channel at once.
// TODO: TEST
#[inline]
pub fn palette256_set(palette: &mut ffi::palette256, idx: PaletteIndex, rgb: Rgb) {
    ffi::channel_set(&mut palette.chans[idx as usize], rgb);
}

/// Extract the three 8-bit R/G/B components from an entry inside a palette store.
// TODO: TEST
#[inline]
pub fn palette256_get_rgb(
    palette: &ffi::palette256,
    idx: PaletteIndex,
    red: &mut Color,
    green: &mut Color,
    blue: &mut Color,
) -> Channel {
    ffi::channel_rgb(palette.chans[idx as usize], red, green, blue)
}

#[cfg(test)]
mod test {
    // use super::ffi;
    // use serial_test::serial;
    /*
    #[test]
    #[serial]
    fn () {
    }
    */
}
