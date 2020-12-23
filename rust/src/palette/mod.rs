//! `NcPalette*`

// -----------------------------------------------------------------------------
// Now none of these functions can't fail and therefore don't return errors.
// -----------------------------------------------------------------------------
//
// functions already exported by bindgen : 3
// -----------------------------------------
// (#) test: 0
// (W) wrap: 3 / 0
// -----------------------------------------
//W  palette256_free
//W  palette256_new
//W  palette256_use
//
// functions manually reimplemented: 3
// -----------------------------------------
// (+) done: 3 / 0
// (#) test: 0
// (W) wrap: 3 / 0
// -----------------------------------------
//W+ palette256_get_rgb
//W+ palette256_set
//W+ palette256_set_rgb

mod methods;
mod reimplemented;
pub use methods::*;
pub use reimplemented::*;

/// NcPalette structure consisting of an array of 256
/// [`NcChannel`][crate::NcChannel]s.
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
pub type NcPalette = crate::bindings::ffi::palette256;

/// 8-bit value used for indexing into a [`NcPalette`]
///
pub type NcPaletteIndex = u8;
