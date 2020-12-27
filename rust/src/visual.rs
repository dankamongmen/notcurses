//! `ncvisual`

// TODO: implement constructors

// functions already exported by bindgen : 18
// -----------------------------------------
// (W) wrap: 0
// (#) test: 0
// -----------------------------------------
//  ncvisual_at_yx
//  ncvisual_decode
//  ncvisual_decode_loop
//  ncvisual_destroy
//  ncvisual_from_bgra
//  ncvisual_from_file
//  ncvisual_from_plane
//  ncvisual_from_rgba
//  ncvisual_geom
//  ncvisual_media_defblitter
//  ncvisual_polyfill_yx
//  ncvisual_render
//  ncvisual_resize
//  ncvisual_rotate
//  ncvisual_set_yx
//  ncvisual_simple_streamer
//  ncvisual_stream
//  ncvisual_subtitle

/// How to scale an [`NcVisual`] during rendering
///
/// - NCSCALE_NONE will apply no scaling.
/// - NCSCALE_SCALE scales a visual to the plane's size,
///   maintaining aspect ratio.
/// - NCSCALE_STRETCH stretches and scales the image in an
///   attempt to fill the entirety of the plane.
///
pub type NcScale = crate::bindings::ffi::ncscale_e;
/// Maintain original size
pub const NCSCALE_NONE: NcScale = crate::bindings::ffi::ncscale_e_NCSCALE_NONE;
/// Maintain aspect ratio
pub const NCSCALE_SCALE: NcScale = crate::bindings::ffi::ncscale_e_NCSCALE_SCALE;
/// Throw away aspect ratio
pub const NCSCALE_STRETCH: NcScale = crate::bindings::ffi::ncscale_e_NCSCALE_STRETCH;

/// A visual bit of multimedia opened with LibAV|OIIO
pub type NcVisual = crate::bindings::ffi::ncvisual;
/// Options struct for [`NcVisual`]
pub type NcVisualOptions = crate::bindings::ffi::ncvisual_options;

/// Uses [`NCCELL_ALPHA_BLEND`][crate::NCCELL_ALPHA_BLEND] with visual.
pub const NCVISUAL_OPTION_BLEND: u32 = crate::bindings::ffi::NCVISUAL_OPTION_BLEND;

/// Fails rather than degrade.
pub const NCVISUAL_OPTION_NODEGRADE: u32 = crate::bindings::ffi::NCVISUAL_OPTION_NODEGRADE;
