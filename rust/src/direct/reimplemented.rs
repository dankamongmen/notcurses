//! `ncdirect_*` reimplemented functions.

use core::ptr::null;

use crate::{
    NcCapabilities, NcComponent, NcDirect, NcInput, NcIntResult, NcRgb, NcSignalSet, NcTime,
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
    unsafe {
        let mut sigmask = NcSignalSet::new();
        sigmask.emptyset();
        core::char::from_u32_unchecked(crate::ncdirect_getc(ncd, null(), &mut sigmask, input))
    }
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
        let mut sigmask = NcSignalSet::new();
        sigmask.fillset();
        let ts = NcTime::new();
        core::char::from_u32_unchecked(crate::ncdirect_getc(ncd, &ts, &mut sigmask, input))
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
