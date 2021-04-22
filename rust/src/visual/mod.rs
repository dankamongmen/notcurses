// functions already exported by bindgen : 18
// -----------------------------------------
// (W) wrap: 18
// (#) test: 0
// -----------------------------------------
//W  ncvisual_at_yx
//W  ncvisual_decode
//W  ncvisual_decode_loop
//W  ncvisual_destroy
//W  ncvisual_from_bgra
//W  ncvisual_from_file
//W  ncvisual_from_plane
//W  ncvisual_from_rgba
//W  ncvisual_inflate
//W  ncvisual_blitter_geom
//W  ncvisual_media_defblitter
//W  ncvisual_polyfill_yx
//W  ncvisual_render
//W  ncvisual_resize
//W  ncvisual_rotate
//W  ncvisual_set_yx
//W  ncvisual_simple_streamer
//~  ncvisual_stream
//W  ncvisual_subtitle

#[allow(unused_imports)] // for the doc comments
use crate::{NcChannel, NcDim, NcRgb};

mod methods;

/// How to scale an [`NcVisual`] during rendering
///
/// - NCSCALE_NONE will apply no scaling.
/// - NCSCALE_SCALE scales a visual to the plane's size,
///   maintaining aspect ratio.
/// - NCSCALE_STRETCH stretches and scales the image in an
///   attempt to fill the entirety of the plane.
///
pub type NcScale = crate::bindings::ffi::ncscale_e;

/// Maintain original size.
pub const NCSCALE_NONE: NcScale = crate::bindings::ffi::ncscale_e_NCSCALE_NONE;

/// Maintain aspect ratio.
pub const NCSCALE_SCALE: NcScale = crate::bindings::ffi::ncscale_e_NCSCALE_SCALE;

/// Throw away aspect ratio.
pub const NCSCALE_STRETCH: NcScale = crate::bindings::ffi::ncscale_e_NCSCALE_STRETCH;

/// Maintain original size, admitting high-resolution blitters
/// that don't preserve aspect ratio.
pub const NCSCALE_NONE_HIRES: NcScale = crate::bindings::ffi::ncscale_e_NCSCALE_NONE_HIRES;

/// Maintain aspect ratio, admitting high-resolution blitters
/// that don't preserve aspect ratio.
pub const NCSCALE_SCALE_HIRES: NcScale = crate::bindings::ffi::ncscale_e_NCSCALE_SCALE_HIRES;

/// A visual bit of multimedia.
///
/// It can be constructed from a rgba or bgra buffer.
///
/// The [NcVisualOptions] structure is used only by the following methods:
/// - [.geom][NcVisual#method.render]
/// - [.render][NcVisual#method.render]
/// - [.simple_streamer][NcVisual#method.render]
pub type NcVisual = crate::bindings::ffi::ncvisual;

/// Options struct for [`NcVisual`]
///
/// If a plane is not provided, one will be created, having the exact size
/// necessary to display the visual.
///
/// A subregion of the visual can be rendered using `begx`, `begy`, `lenx`, and `leny`.
pub type NcVisualOptions = crate::bindings::ffi::ncvisual_options;

// NcRgba
//
/// 32 bits broken into 3x 8bpp RGB channels + 8ppp alpha.
///
/// Unlike with [`NcChannel`], operations involving `NcRgb` ignores the last 4th byte
///
/// ## Diagram
///
/// ```txt
/// AAAAAAAA RRRRRRRR GGGGGGGG BBBBBBBB
/// ```
/// `type in C: no data type`
///
/// See also: [NcRgb] and [NcChannel] types.
pub type NcRgba = u32;

// // NcBgra
// //
// /// 32 bits broken into 3x 8bpp BGR channels + 8ppp alpha.
// ///
// /// ## Diagram
// ///
// /// ```txt
// /// AAAAAAAA BBBBBBBB GGGGGGGG RRRRRRRR
// /// ```
// ///
// /// `type in C: no data type`
// ///
// /// See also: [NcRgba], [NcRgb] and [NcChannel] types.
// pub type NcBgra = u32;

/// Treats as transparent the color specified in the `transcolor` field.
pub const NCVISUAL_OPTION_ADDALPHA: u32 = crate::bindings::ffi::NCVISUAL_OPTION_ADDALPHA;

/// Uses [`NCCELL_ALPHA_BLEND`][crate::NCCELL_ALPHA_BLEND] with visual.
pub const NCVISUAL_OPTION_BLEND: u32 = crate::bindings::ffi::NCVISUAL_OPTION_BLEND;

/// Fails rather than gracefully degrade. See [NcBlitter].
pub const NCVISUAL_OPTION_NODEGRADE: u32 = crate::bindings::ffi::NCVISUAL_OPTION_NODEGRADE;

/// Y is an alignment, not absolute.
pub const NCVISUAL_OPTION_VERALIGNED: u32 = crate::bindings::ffi::NCVISUAL_OPTION_VERALIGNED;

/// X is an alignment, not absolute.
pub const NCVISUAL_OPTION_HORALIGNED: u32 = crate::bindings::ffi::NCVISUAL_OPTION_HORALIGNED;

/// Blitter Mode (`NCBLIT_*`)
///
/// We never blit full blocks, but instead spaces (more efficient) with the
/// background set to the desired foreground.
///
/// ## Modes
///
/// - [`NCBLIT_1x1`]
/// - [`NCBLIT_2x1`]
/// - [`NCBLIT_2x2`]
/// - [`NCBLIT_3x2`]
/// - [`NCBLIT_4x1`]
/// - [`NCBLIT_8x1`]
/// - [`NCBLIT_BRAILLE`]
/// - [`NCBLIT_DEFAULT`]
/// - [`NCBLIT_PIXEL`]
///
/// There is a mechanism of graceful degradation, that works as follows:
/// - without braille support, NCBLIT_BRAILLE decays to NCBLIT_3x2
/// - without bitmap support, NCBLIT_PIXEL decays to NCBLIT_3x2
/// - without sextant support, NCBLIT_3x2 decays to NCBLIT_2x2
/// - without quadrant support, NCBLIT_2x2 decays to NCBLIT_2x1
/// - the only viable blitters in ASCII are NCBLIT_1x1 and NCBLIT_PIXEL
/// 
/// If you don't want this behaviour you have to use [NCVISUAL_OPTION_NODEGRADE]
///
pub type NcBlitter = crate::bindings::ffi::ncblitter_e;

/// [`NcBlitter`] mode using: space, compatible with ASCII
pub const NCBLIT_1x1: NcBlitter = crate::bindings::ffi::ncblitter_e_NCBLIT_1x1;

/// [`NcBlitter`] mode using: halves + 1x1 (space)
/// ▄▀
pub const NCBLIT_2x1: NcBlitter = crate::bindings::ffi::ncblitter_e_NCBLIT_2x1;

/// [`NcBlitter`] mode using: quadrants + 2x1
/// ▗▐ ▖▀▟▌▙
pub const NCBLIT_2x2: NcBlitter = crate::bindings::ffi::ncblitter_e_NCBLIT_2x2;

/// [`NcBlitter`] mode using: sextants
/// 🬀🬁🬂🬃🬄🬅🬆🬇🬈🬉🬊🬋🬌🬍🬎🬏🬐🬑🬒🬓🬔🬕🬖🬗🬘🬙🬚🬛🬜🬝🬞🬟🬠🬡🬢🬣🬤🬥🬦🬧🬨🬩🬪🬫🬬🬭🬮🬯🬰🬱🬲🬳🬴🬵🬶🬷🬸🬹🬺🬻
pub const NCBLIT_3x2: NcBlitter = crate::bindings::ffi::ncblitter_e_NCBLIT_3x2;

/// [`NcBlitter`] mode using: four vertical levels
/// █▆▄▂
pub const NCBLIT_4x1: NcBlitter = crate::bindings::ffi::ncblitter_e_NCBLIT_4x1;

/// [`NcBlitter`] mode using: eight vertical levels
/// █▇▆▅▄▃▂▁
pub const NCBLIT_8x1: NcBlitter = crate::bindings::ffi::ncblitter_e_NCBLIT_8x1;

/// [`NcBlitter`] mode using: 4 rows, 2 cols (braille)
/// ⡀⡄⡆⡇⢀⣀⣄⣆⣇⢠⣠⣤⣦⣧⢰⣰⣴⣶⣷⢸⣸⣼⣾⣿
pub const NCBLIT_BRAILLE: NcBlitter = crate::bindings::ffi::ncblitter_e_NCBLIT_BRAILLE;

/// [`NcBlitter`] mode where the blitter is automatically chosen
pub const NCBLIT_DEFAULT: NcBlitter = crate::bindings::ffi::ncblitter_e_NCBLIT_DEFAULT;

/// Sixel/Pixel mode
///
/// NCBLIT_PIXEL
///
/// See [Sixel in Wikipedia](https://en.wikipedia.org/wiki/Sixel).
pub const NCBLIT_PIXEL: NcBlitter = crate::bindings::ffi::ncblitter_e_NCBLIT_PIXEL;

/// Contains the pixel geometry information as returned by the
/// NcPlane.[pixelgeom()][crate::NcPlane#method.pixelgeom] method.
///
/// If bitmaps are not supported, the fields `max_bitmap_*` will be 0.
#[derive(Clone, Debug)]
pub struct NcPixelGeometry {
    /// Geometry of the display region
    pub display_y: NcDim,
    pub display_x: NcDim,
    pub cell_y: NcDim,
    pub cell_x: NcDim,
    pub max_bitmap_y: NcDim,
    pub max_bitmap_x: NcDim,
}
