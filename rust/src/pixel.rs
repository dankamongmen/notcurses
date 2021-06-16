//! The ncpixel API facilitates direct management of the pixels within an
//! ncvisual (ncvisuals keep a backing store of 32-bit RGBA pixels, and render
//! them down to terminal graphics in ncvisual_render()).
//
// - NOTE: The pixel color & alpha [`NcComponent`]s are u8 instead of u32.
//   Because of type enforcing, some runtime checks are now unnecessary.
//
// - NOTE: None of the functions can't fail anymore and don't have to return an error.
//
// functions manually reimplemented: 10
// ------------------------------------------
// (+) done: 10 /  0
// (#) test:  0
// (W) wrap: 10
// ------------------------------------------
//W+ ncpixel
//W+ ncpixel_a
//W+ ncpixel_b
//W+ ncpixel_g
//W+ ncpixel_r
//W+ ncpixel_set_a
//W+ ncpixel_set_b
//W+ ncpixel_set_g
//W+ ncpixel_set_r
//W+ ncpixel_set_rgb8

use crate::NcComponent;

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

/// Constructs a libav-compatible ABGR pixel from RGB [`NcComponent`]s.
#[inline]
#[allow(clippy::unnecessary_cast)]
pub const fn ncpixel(red: NcComponent, green: NcComponent, blue: NcComponent) -> NcPixel {
    0xff000000 as NcPixel | red as NcPixel | (blue as NcPixel) << 8 | (green as NcPixel) << 16
}

/// Extracts the 8-bit alpha [`NcComponent`] from an ABGR pixel.
#[inline]
pub const fn ncpixel_a(pixel: NcPixel) -> NcComponent {
    ((pixel.to_le() & 0xff000000) >> 24) as NcComponent
}

/// Extracts the 8 bit blue [`NcComponent`] from an ABGR pixel.
#[inline]
pub const fn ncpixel_b(pixel: NcPixel) -> NcComponent {
    ((pixel.to_le() & 0x00ff0000) >> 16) as NcComponent
}

/// Extracts the 8 bit green [`NcComponent`] from an ABGR pixel.
#[inline]
pub const fn ncpixel_g(pixel: NcPixel) -> NcComponent {
    ((pixel.to_le() & 0x0000ff00) >> 8) as NcComponent
}

/// Extracts the 8 bit red [`NcComponent`] from an ABGR pixel.
#[inline]
pub const fn ncpixel_r(pixel: NcPixel) -> NcComponent {
    (pixel.to_le() & 0x000000ff) as NcComponent
}

/// Sets the 8-bit alpha [`NcComponent`] of an ABGR pixel.
#[inline]
pub fn ncpixel_set_a(pixel: &mut NcPixel, alpha: NcComponent) {
    *pixel = (((*pixel).to_le() & 0x00ffffff) | ((alpha as NcPixel) << 24)).to_le();
}

/// Sets the 8-bit blue [`NcComponent`] of an ABGR pixel.
#[inline]
pub fn ncpixel_set_b(pixel: &mut NcPixel, blue: NcComponent) {
    *pixel = (((*pixel).to_le() & 0xffff00ff) | ((blue as NcPixel) << 8)).to_le();
}

/// Sets the 8-bit green [`NcComponent`] of an ABGR pixel.
#[inline]
pub fn ncpixel_set_g(pixel: &mut NcPixel, green: NcComponent) {
    *pixel = (((*pixel).to_le() & 0xff00ffff) | ((green as NcPixel) << 16)).to_le();
}

/// Sets the 8-bit red [`NcComponent`] of an ABGR pixel.
#[inline]
pub fn ncpixel_set_r(pixel: &mut NcPixel, red: NcComponent) {
    *pixel = (((*pixel).to_le() & 0xffffff00) | red as NcPixel).to_le();
}

/// Sets the RGB [`NcComponent`]s of an ABGR pixel.
#[inline]
pub fn ncpixel_set_rgb8(
    pixel: &mut NcPixel,
    red: NcComponent,
    green: NcComponent,
    blue: NcComponent,
) {
    ncpixel_set_r(pixel, red);
    ncpixel_set_g(pixel, green);
    ncpixel_set_b(pixel, blue);
}

/// Enables the [NcPixel] methods.
//
// NOTE: waiting for: https://github.com/rust-lang/rust/issues/56546
// to move doc comments to the trait and appear unhidden at the implementation.
pub trait NcPixelMethods {
    fn new(r: NcComponent, g: NcComponent, b: NcComponent) -> Self;
    fn a(self) -> NcComponent;
    fn b(self) -> NcComponent;
    fn g(self) -> NcComponent;
    fn r(self) -> NcComponent;
    fn set_a(&mut self, green: NcComponent);
    fn set_b(&mut self, blue: NcComponent);
    fn set_g(&mut self, green: NcComponent);
    fn set_r(&mut self, red: NcComponent);
    fn set_rgb8(&mut self, red: NcComponent, green: NcComponent, blue: NcComponent);
}

impl NcPixelMethods for NcPixel {
    /// Constructs a libav-compatible ABGR pixel from RGB [`NcComponent`]s.
    fn new(red: NcComponent, green: NcComponent, blue: NcComponent) -> Self {
        ncpixel(red, green, blue)
    }

    /// Extracts the 8-bit alpha [`NcComponent`] from an ABGR pixel.
    fn a(self) -> NcComponent {
        ncpixel_a(self)
    }

    /// Extracts the 8 bit blue [`NcComponent`] from an ABGR pixel.
    fn b(self) -> NcComponent {
        ncpixel_b(self)
    }

    /// Extracts the 8 bit green [`NcComponent`] from an ABGR pixel.
    fn g(self) -> NcComponent {
        ncpixel_g(self)
    }

    /// Extracts the 8 bit red [`NcComponent`] from an ABGR pixel.
    fn r(self) -> NcComponent {
        ncpixel_r(self)
    }

    /// Sets the 8-bit alpha [`NcComponent`] of an ABGR pixel.
    fn set_a(&mut self, alpha: NcComponent) {
        ncpixel_set_a(self, alpha)
    }

    /// Sets the 8-bit green [`NcComponent`] of an ABGR pixel.
    fn set_g(&mut self, green: NcComponent) {
        ncpixel_set_b(self, green)
    }

    /// Sets the 8-bit blue [`NcComponent`] of an ABGR pixel.
    fn set_b(&mut self, blue: NcComponent) {
        ncpixel_set_b(self, blue)
    }

    /// Sets the 8-bit red [`NcComponent`] of an ABGR pixel.
    fn set_r(&mut self, red: NcComponent) {
        ncpixel_set_r(self, red)
    }

    /// Sets the RGB [`NcComponent`]s of an ABGR pixel.
    fn set_rgb8(&mut self, red: NcComponent, green: NcComponent, blue: NcComponent) {
        ncpixel_set_rgb8(self, red, green, blue);
    }
}
