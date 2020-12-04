//! `NcPalette*`

// -----------------------------------------------------------------------------
// Now none of these functions can't fail and therefore don't return errors.
// -----------------------------------------------------------------------------
//
// functions already exported by bindgen : 3
// -----------------------------------------
// (#) unit tests: 0 / 3
// -----------------------------------------
//  palette256_free
//  palette256_new
//  palette256_use
//
// functions manually reimplemented: 3
// -----------------------------------------
// (+) implement : 3 / 0
// (#) unit tests: 0 / 3
// -----------------------------------------
// + palette256_get_rgb
// + palette256_set
// + palette256_set_rgb

mod reimplemented;
pub use reimplemented::*;

/// NcPalette structure consisting of an array of 256 [`NcChannel`]s.
///
/// See also [NcPaletteIndex].
///
/// Some terminals only support 256 colors, but allow the full
/// palette to be specified with arbitrary RGB colors. In all cases, it's more
/// performant to use indexed colors, since it's much less data to write to the
/// terminal. If you can limit yourself to 256 colors, that's probably best.
///
/// `type in C: ncpalette256 (struct)`
///
pub type NcPalette = crate::bindings::bindgen::palette256;

/// 8-bit value used for indexing into a [`NcPalette`]
///
pub type NcPaletteIndex = u8;
