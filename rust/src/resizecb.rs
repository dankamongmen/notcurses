//! `NcResizeCb`

use crate::{NcPlane, NcResult};

/// A callback function called when an [`NcPlane`] is resized.
///
/// See also [ncresizecb_to_rust] & [ncresizecb_to_c].
pub type NcResizeCb = fn(&mut NcPlane) -> NcResult;

pub(crate) type NcResizeCbUnsafe = unsafe extern "C" fn(*mut NcPlane) -> NcResult;

/// Returns [NcResizeCb] from the resizecb type in the notcurses C API.
pub fn ncresizecb_to_rust(resizecb: Option<NcResizeCbUnsafe>) -> Option<NcResizeCb> {
    if let Some(cb) = resizecb {
        return Some(unsafe { core::mem::transmute(cb) });
    } else {
        None
    }
}

/// Converts [NcResizeCb] to the resizecb type expected by notcurses C API.
pub fn ncresizecb_to_c(resizecb: Option<NcResizeCb>) -> Option<NcResizeCbUnsafe> {
    if let Some(cb) = resizecb {
        return Some(unsafe { core::mem::transmute(cb) });
    } else {
        None
    }
}
