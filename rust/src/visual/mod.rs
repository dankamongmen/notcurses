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
use crate::{NcChannel, NcRgb};

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

/// Fails rather than degrade.
pub const NCVISUAL_OPTION_NODEGRADE: u32 = crate::bindings::ffi::NCVISUAL_OPTION_NODEGRADE;

/// Y is an alignment, not absolute.
pub const NCVISUAL_OPTION_VERALIGNED: u32 = crate::bindings::ffi::NCVISUAL_OPTION_VERALIGNED;

/// X is an alignment, not absolute.
pub const NCVISUAL_OPTION_HORALIGNED: u32 = crate::bindings::ffi::NCVISUAL_OPTION_HORALIGNED;
