//! `ncdirect_*` reimplemented functions.

use core::ptr::null;

use crate::{NcComponent, NcDirect, NcInput, NcIntResult, NcRgb, NcSignalSet, NcTime};

/// 'input' may be NULL if the caller is uninterested in event details.
/// Blocks until an event is processed or a signal is received.
///
/// *Method: NcDirect.[getc_blocking()][NcDirect#method.getc_blocking].*
// TODO: use from_u32 & return Option.
#[inline]
pub fn ncdirect_getc_blocking(nc: &mut NcDirect, input: &mut NcInput) -> char {
    unsafe {
        let mut sigmask = NcSignalSet::new();
        sigmask.emptyset();
        core::char::from_u32_unchecked(crate::ncdirect_getc(nc, null(), &mut sigmask, input))
    }
}

///
/// If no event is ready, returns 0.
///
/// *Method: NcDirect.[getc_nblock()][NcDirect#method.getc_nblock].*
//
// `input` may be NULL if the caller is uninterested in event details.
#[inline]
pub fn ncdirect_getc_nblock(nc: &mut NcDirect, input: &mut NcInput) -> char {
    unsafe {
        let mut sigmask = NcSignalSet::new();
        sigmask.fillset();
        let ts = NcTime::new();
        core::char::from_u32_unchecked(crate::ncdirect_getc(nc, &ts, &mut sigmask, input))
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
