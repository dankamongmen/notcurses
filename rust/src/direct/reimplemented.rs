//! `ncdirect_*` reimplemented functions.

use core::ptr::{null, null_mut};

use crate::{
    cstring, NcCapabilities, NcChannels, NcComponent, NcDim, NcDirect, NcInput, NcIntResult, NcRgb,
    NcTime,
};

/// Can we directly specify RGB values per cell, or only use palettes?
#[inline]
pub fn ncdirect_cantruecolor(ncd: &NcDirect) -> bool {
    ncdirect_capabilities(ncd).rgb
}

/// Can we set the "hardware" palette? Requires the "ccc" terminfo capability.
#[inline]
pub fn ncdirect_canchangecolor(ncd: &NcDirect) -> bool {
    crate::nccapability_canchangecolor(&ncdirect_capabilities(ncd))
}

/// Can we fade? Fading requires either the "rgb" or "ccc" terminfo capability.
#[inline]
pub fn ncdirect_canfade(ncd: &NcDirect) -> bool {
    ncdirect_canchangecolor(ncd) || ncdirect_cantruecolor(ncd)
}

/// Can we load videos? This requires being built against FFmpeg.
#[inline]
pub fn ncdirect_canopen_videos(_ncd: &NcDirect) -> bool {
    unsafe { crate::notcurses_canopen_videos(null()) }
}

/// Can we reliably use Unicode halfblocks?
#[inline]
pub fn ncdirect_canhalfblock(ncd: &NcDirect) -> bool {
    unsafe { crate::ncdirect_canutf8(ncd) }
}

/// Can we reliably use Unicode quadrants?
#[inline]
pub fn ncdirect_canquadrant(ncd: &NcDirect) -> bool {
    (unsafe { crate::ncdirect_canutf8(ncd) }) && ncdirect_capabilities(ncd).quadrants
}

/// Can we reliably use Unicode 13 sextants?
#[inline]
pub fn ncdirect_cansextant(ncd: &NcDirect) -> bool {
    (unsafe { crate::ncdirect_canutf8(ncd) }) && ncdirect_capabilities(ncd).sextants
}

/// Can we reliably use Unicode Braille?
#[inline]
pub fn ncdirect_canbraille(_ncd: &NcDirect) -> bool {
    unsafe { crate::notcurses_canbraille(null()) }
}

/// Returns the detected [`NcCapabilities`].
#[inline]
pub fn ncdirect_capabilities(ncd: &NcDirect) -> NcCapabilities {
    unsafe { *crate::bindings::ffi::ncdirect_capabilities(ncd) }
}

/// 'input' may be NULL if the caller is uninterested in event details.
/// Blocks until an event is processed or a signal is received.
///
/// *Method: NcDirect.[getc_blocking()][NcDirect#method.getc_blocking].*
// TODO: use from_u32 & return Option.
#[inline]
pub fn ncdirect_getc_blocking(ncd: &mut NcDirect, input: &mut NcInput) -> char {
    unsafe { core::char::from_u32_unchecked(crate::ncdirect_getc(ncd, null(), null_mut(), input)) }
}

///
/// If no event is ready, returns 0.
///
/// *Method: NcDirect.[getc_nblock()][NcDirect#method.getc_nblock].*
//
// `input` may be NULL if the caller is uninterested in event details.
#[inline]
pub fn ncdirect_getc_nblock(ncd: &mut NcDirect, input: &mut NcInput) -> char {
    unsafe {
        let ts = NcTime::new();
        core::char::from_u32_unchecked(crate::ncdirect_getc(ncd, &ts, null_mut(), input))
    }
}

/// Sets the foreground [NcComponent] components.
///
/// *Method: NcDirect.[set_fg_rgb8()][NcDirect#method.set_fg_rgb8].*
#[inline]
pub fn ncdirect_set_fg_rgb8(
    ncd: &mut NcDirect,
    red: NcComponent,
    green: NcComponent,
    blue: NcComponent,
) -> NcIntResult {
    let rgb = (red as NcRgb) << 16 | (green as NcRgb) << 8 | blue as NcRgb;
    unsafe { crate::ncdirect_set_fg_rgb(ncd, rgb) }
}

/// Sets the background [NcComponent] components.
///
/// *Method: NcDirect.[set_bg_rgb8()][NcDirect#method.set_bg_rgb8].*
#[inline]
pub fn ncdirect_set_bg_rgb8(
    ncd: &mut NcDirect,
    red: NcComponent,
    green: NcComponent,
    blue: NcComponent,
) -> NcIntResult {
    let rgb = (red as NcRgb) << 16 | (green as NcRgb) << 8 | blue as NcRgb;
    unsafe { crate::ncdirect_set_bg_rgb(ncd, rgb) }
}

/// Draws horizontal lines using the specified [NcChannels]s, interpolating
/// between them as we go.
///
/// The string at `egc` may not use more than one column.
///
/// All lines start at the current cursor position.
///
/// For a horizontal line, `len` cannot exceed the screen width minus the
/// cursor's offset.
// TODO:MAYBE saturate the `len` value
///
/// *Method: NcDirect.[hline_interp()][NcDirect#method.hline_interp].*
#[inline]
pub fn ncdirect_hline_interp(
    ncd: &mut NcDirect,
    egc: &str,
    len: NcDim,
    h1: NcChannels,
    h2: NcChannels,
) -> NcIntResult {
    #[cfg(any(target_arch = "armv7l", target_arch = "i686"))]
    let egc_ptr = cstring![egc] as *const i8;
    #[cfg(not(any(target_arch = "armv7l", target_arch = "i686")))]
    let egc_ptr = cstring![egc];

    unsafe { crate::bindings::ffi::ncdirect_hline_interp(ncd, egc_ptr, len as i32, h1, h2) }
}

/// Draws horizontal lines using the specified [NcChannels]s, interpolating
/// between them as we go.
///
/// The string at `egc` may not use more than one column.
///
/// All lines start at the current cursor position.
///
/// For a vertical line, `len` may be as long as you'd like; the screen
/// will scroll as necessary.
///
/// *Method: NcDirect.[vline_interp()][NcDirect#method.vline_interp].*
#[inline]
pub fn ncdirect_vline_interp(
    ncd: &mut NcDirect,
    egc: &str,
    len: NcDim,
    h1: NcChannels,
    h2: NcChannels,
) -> NcIntResult {
    #[cfg(any(target_arch = "armv7l", target_arch = "i686"))]
    let egc_ptr = cstring![egc] as *const i8;
    #[cfg(not(any(target_arch = "armv7l", target_arch = "i686")))]
    let egc_ptr = cstring![egc];

    unsafe { crate::bindings::ffi::ncdirect_vline_interp(ncd, egc_ptr, len as i32, h1, h2) }
}
