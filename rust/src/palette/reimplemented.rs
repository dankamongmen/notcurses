//! `ncpalette_*` reimplemented functions.

use crate::{NcChannel, NcColor, NcPalette, NcPaletteIndex, NcRgb};

/// Extracts the [NcColor] RGB components from an [NcChannel] entry inside
/// an [NcPalette], and returns the NcChannel.
///
/// *Method: NcPalette.[get_rgb()][NcPalette#method.get_rgb].*
/// *Method: NcPalette.[get_rgb8()][NcPalette#method.get_rgb8].*
#[inline]
pub fn ncpalette_get_rgb8(
    palette: &NcPalette,
    index: NcPaletteIndex,
    red: &mut NcColor,
    green: &mut NcColor,
    blue: &mut NcColor,
) -> NcChannel {
    crate::ncchannel_rgb8(palette.chans[index as usize], red, green, blue)
}

/// Sets the [NcRgb] value of the [NcChannel] entry inside an [NcPalette].
///
/// *Method: NcPalette.[set()][NcPalette#method.set].*
#[inline]
pub fn ncpalette_set(palette: &mut NcPalette, index: NcPaletteIndex, rgb: NcRgb) {
    crate::ncchannel_set(&mut palette.chans[index as usize], rgb);
}

/// Sets the [NcColor] components of the [NcChannel] entry inside an [NcPalette].
///
/// *Method: NcPalette.[set_rgb()][NcPalette#method.set_rgb].*
#[inline]
pub fn ncpalette_set_rgb8(
    palette: &mut NcPalette,
    index: NcPaletteIndex,
    red: NcColor,
    green: NcColor,
    blue: NcColor,
) {
    crate::ncchannel_set_rgb8(&mut palette.chans[index as usize], red, green, blue)
}
