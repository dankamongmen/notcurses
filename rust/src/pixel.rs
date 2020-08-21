//! The ncpixel API facilitates direct management of the pixels within an
//! ncvisual (ncvisuals keep a backing store of 32-bit RGBA pixels, and render
//! them down to terminal graphics in ncvisual_render()).
//
// - NOTE: The pixel color & alpha components are u8 instead of u32.
//   Because of type enforcing, some runtime checks are now unnecessary.
//
// - NOTE: None of the functions can't fail anymore and don't have to return an error.
//
//
// functions already exported by bindgen : 0
// -----------------------------------------
//
// static inline functions to reimplement: 10
// ------------------------------------------ (done / (x) wont / remaining)
// (+) implement : 10 /  0 /  0
// (#) unit tests:  0 /  0 / 10
// ------------------------------------------
//+ncpixel
//+ncpixel_a
//+ncpixel_b
//+ncpixel_g
//+ncpixel_r
//+ncpixel_set_a
//+ncpixel_set_b
//+ncpixel_set_g
//+ncpixel_set_r
//+ncpixel_set_rgb

use crate as nc;
use nc::types::{Color, Pixel};

// Pixel Structure:
//
// 0xff000000 8 bit Alpha
// 0x00ff0000 8 bit Green
// 0x0000ff00 8 bit Blue
// 0x000000ff 8 bit Red

/// Get an RGB pixel from RGB values
pub fn ncpixel(r: Color, g: Color, b: Color) -> Pixel {
    0xff000000 as Pixel | r as Pixel | (b as Pixel) << 8 | (g as Pixel) << 16
}

/// Extract the 8-bit alpha component from a pixel
pub fn ncpixel_a(pixel: Pixel) -> Color {
    ((pixel & 0xff000000) >> 24) as Color
}

/// Extract the 8 bit green component from a pixel
pub fn ncpixel_g(pixel: Pixel) -> Color {
    ((pixel & 0x00ff0000) >> 16) as Color
}

/// Extract the 8 bit blue component from a pixel
pub fn ncpixel_b(pixel: Pixel) -> Color {
    ((pixel & 0x0000ff00) >> 8) as Color
}

/// Extract the 8 bit red component from a pixel
pub fn ncpixel_r(pixel: Pixel) -> Color {
    (pixel & 0x000000ff) as Color
}

/// Set the 8-bit alpha component of a pixel
pub fn ncpixel_set_a(pixel: &mut Pixel, alpha: Color) {
    *pixel = (*pixel & 0x00ffffff) | ((alpha as Pixel) << 24);
}

/// Set the 8-bit green component of a pixel
pub fn ncpixel_set_g(pixel: &mut Pixel, green: Color) {
    *pixel = (*pixel & 0xff00ffff) | ((green as Pixel) << 16);
}

/// Set the 8-bit blue component of a pixel
pub fn ncpixel_set_b(pixel: &mut Pixel, blue: Color) {
    *pixel = (*pixel & 0xffff00ff) | ((blue as Pixel) << 8);
}

/// Set the 8-bit red component of a pixel
pub fn ncpixel_set_r(pixel: &mut Pixel, red: Color) {
    *pixel = (*pixel & 0xffffff00) | red as Pixel;
}

/// set the RGB values of an RGB pixel
pub fn ncpixel_set_rgb(pixel: &mut Pixel, red: Color, green: Color, blue: Color) {
    ncpixel_set_r(pixel, red);
    ncpixel_set_g(pixel, green);
    ncpixel_set_b(pixel, blue);
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
