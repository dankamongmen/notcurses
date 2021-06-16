//! `ncpalette_*` reimplemented functions.

use crate::{NcChannel, NcComponent, NcPalette, NcPaletteIndex, NcRgb};

/// Extracts the RGB [`NcComponent`]s from an [`NcChannel`] entry inside
/// an [`NcPalette`], and returns the `NcChannel`.
///
/// *Method: NcPalette.[get_rgb()][NcPalette#method.get_rgb].*
/// *Method: NcPalette.[get_rgb8()][NcPalette#method.get_rgb8].*
#[inline]
pub fn ncpalette_get_rgb8(
    palette: &NcPalette,
    index: NcPaletteIndex,
    red: &mut NcComponent,
    green: &mut NcComponent,
    blue: &mut NcComponent,
) -> NcChannel {
    crate::ncchannel_rgb8(palette.chans[index as usize], red, green, blue)
}

/// Sets the [`NcRgb`] value of the [`NcChannel`] entry inside an [`NcPalette`].
///
/// *Method: NcPalette.[set()][NcPalette#method.set].*
#[inline]
pub fn ncpalette_set(palette: &mut NcPalette, index: NcPaletteIndex, rgb: NcRgb) {
    crate::ncchannel_set(&mut palette.chans[index as usize], rgb);
}

/// Sets the RGB [`NcComponent`]s of the [`NcChannel`] entry inside an
/// [`NcPalette`].
///
/// *Method: NcPalette.[set_rgb()][NcPalette#method.set_rgb].*
#[inline]
pub fn ncpalette_set_rgb8(
    palette: &mut NcPalette,
    index: NcPaletteIndex,
    red: NcComponent,
    green: NcComponent,
    blue: NcComponent,
) {
    crate::ncchannel_set_rgb8(&mut palette.chans[index as usize], red, green, blue)
}
