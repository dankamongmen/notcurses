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
//W  ncpalette_free
//W  ncpalette_new
//W  ncpalette_use
//
// functions manually reimplemented: 3
// -----------------------------------------
// (+) done: 3 / 0
// (#) test: 0
// (W) wrap: 3 / 0
// -----------------------------------------
//W+ ncpalette_get_rgb
//W+ ncpalette_set
//W+ ncpalette_set_rgb

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
/// `type in C: ncncpalette (struct)`
///
pub type NcPalette = crate::bindings::ffi::ncpalette;

/// 8-bit value used for indexing into a [`NcPalette`]
///
pub type NcPaletteIndex = u8;
