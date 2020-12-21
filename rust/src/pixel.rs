//! The ncpixel API facilitates direct management of the pixels within an
//! ncvisual (ncvisuals keep a backing store of 32-bit RGBA pixels, and render
//! them down to terminal graphics in ncvisual_render()).
//
// - NOTE: The pixel color & alpha components are u8 instead of u32.
//   Because of type enforcing, some runtime checks are now unnecessary.
//
// - NOTE: None of the functions can't fail anymore and don't have to return an error.
//
// functions manually reimplemented: 10
// ------------------------------------------
// (+) implement : 10 /  0
// (#) unit tests:  0 / 10
// ------------------------------------------
// + ncpixel
// + ncpixel_a
// + ncpixel_b
// + ncpixel_g
// + ncpixel_r
// + ncpixel_set_a
// + ncpixel_set_b
// + ncpixel_set_g
// + ncpixel_set_r
// + ncpixel_set_rgb8

use crate::NcColor;

// NcPixel (RGBA)
/// An ABGR pixel.
///
/// ## Diagram
///
/// ```txt
/// AAAAAAAA GGGGGGGG BBBBBBBB RRRRRRRR
/// ```
///
/// `type in C: ncpixel (uint32_t)`
///
/// NcPixel has 8 bits of alpha,  more or less linear, contributing
/// directly to the usual alpha blending equation.
///
/// We map the 8 bits of alpha to 2 bits of alpha via a [level
/// function](https://nick-black.com/dankwiki/index.php?title=Notcurses#Transparency.2FContrasting)
///
/// The ncpixel API facilitates direct management of the pixels within an
/// ncvisual (ncvisuals keep a backing store of 32-bit RGBA pixels, and render
/// them down to terminal graphics in ncvisual_render()).
///
/// Per libav, we "store as BGRA on little-endian, and ARGB on big-endian".
/// This is an RGBA *byte-order* scheme. libav emits bytes, not words. Those
/// bytes are R-G-B-A. When read as words, on little endian this will be ABGR,
/// and on big-endian this will be RGBA. force everything to LE ABGR.
///
pub type NcPixel = u32;

/// Constructs a libav-compatible ABGR pixel from [NcColor] RGB components.
#[inline]
pub const fn ncpixel(r: NcColor, g: NcColor, b: NcColor) -> NcPixel {
    0xff000000 as NcPixel | r as NcPixel | (b as NcPixel) << 8 | (g as NcPixel) << 16
}

/// Extracts the 8-bit alpha component from an ABGR pixel.
#[inline]
pub const fn ncpixel_a(pixel: NcPixel) -> NcColor {
    ((pixel.to_le() & 0xff000000) >> 24) as NcColor
}

/// Extracts the 8 bit green component from an ABGR pixel.
#[inline]
pub const fn ncpixel_g(pixel: NcPixel) -> NcColor {
    ((pixel.to_le() & 0x00ff0000) >> 16) as NcColor
}

/// Extracts the 8 bit blue component from an ABGR pixel.
#[inline]
pub const fn ncpixel_b(pixel: NcPixel) -> NcColor {
    ((pixel.to_le() & 0x0000ff00) >> 8) as NcColor
}

/// Extracts the 8 bit red component from an ABGR pixel.
#[inline]
pub const fn ncpixel_r(pixel: NcPixel) -> NcColor {
    (pixel.to_le() & 0x000000ff) as NcColor
}

/// Sets the 8-bit alpha component of an ABGR pixel.
#[inline]
pub fn ncpixel_set_a(pixel: &mut NcPixel, alpha: NcColor) {
    *pixel = (((*pixel).to_le() & 0x00ffffff) | ((alpha as NcPixel) << 24)).to_le();
}

/// Sets the 8-bit green component of an ABGR pixel.
#[inline]
pub fn ncpixel_set_g(pixel: &mut NcPixel, green: NcColor) {
    *pixel = (((*pixel).to_le() & 0xff00ffff) | ((green as NcPixel) << 16)).to_le();
}

/// Sets the 8-bit blue component of an ABGR pixel.
#[inline]
pub fn ncpixel_set_b(pixel: &mut NcPixel, blue: NcColor) {
    *pixel = (((*pixel).to_le() & 0xffff00ff) | ((blue as NcPixel) << 8)).to_le();
}

/// Sets the 8-bit red component of an ABGR pixel.
#[inline]
pub fn ncpixel_set_r(pixel: &mut NcPixel, red: NcColor) {
    *pixel = (((*pixel).to_le() & 0xffffff00) | red as NcPixel).to_le();
}

/// Sets the [NcColor] RGB components of an ABGR pixel.
#[inline]
pub fn ncpixel_set_rgb8(pixel: &mut NcPixel, red: NcColor, green: NcColor, blue: NcColor) {
    ncpixel_set_r(pixel, red);
    ncpixel_set_g(pixel, green);
    ncpixel_set_b(pixel, blue);
}
