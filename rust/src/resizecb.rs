//! `NcResizeCb`

use crate::{NcPlane, NcResult};

/// A callback function called when an [NcPlane] is resized.
///
/// See also [ncresizecb_to_rust] & [ncresizecb_to_c].
///
pub type NcResizeCb = fn(&mut NcPlane) -> NcResult;

/// The unsafe version of [NcResizeCb] expected by the notcurses C API.
pub type NcResizeCbUnsafe = unsafe extern "C" fn(*mut NcPlane) -> NcResult;

/// Converts [NcResizeCbUnsafe] to [NcResizeCb].
pub fn ncresizecb_to_rust(resizecb: Option<NcResizeCbUnsafe>) -> Option<NcResizeCb> {
    if let Some(cb) = resizecb {
        return Some(unsafe { core::mem::transmute(cb) });
    } else {
        None
    }
}

/// Converts [NcResizeCb] to [NcResizeCbUnsafe].
///
pub fn ncresizecb_to_c(resizecb: Option<NcResizeCb>) -> Option<NcResizeCbUnsafe> {
    if let Some(cb) = resizecb {
        return Some(unsafe { core::mem::transmute(cb) });
    } else {
        None
    }
}

/// Enables the [NcResizeCb] methods.
pub trait NcResizeCbMethods {
    fn to_rust(&self) -> Option<NcResizeCb>;
    fn to_c(&self) -> Option<NcResizeCbUnsafe>;
}

impl NcResizeCbMethods for NcResizeCb {
    /// Returns [NcResizeCbUnsafe].
    ///
    /// *C style function: [ncresizecb_to_c()][ncresizecb_to_c].*
    fn to_c(&self) -> Option<NcResizeCbUnsafe> {
        ncresizecb_to_c(Some(*self))
    }
    /// no-op.
    fn to_rust(&self) -> Option<NcResizeCb> {
        Some(*self)
    }
}

impl NcResizeCbMethods for NcResizeCbUnsafe {
    /// no-op.
    fn to_c(&self) -> Option<NcResizeCbUnsafe> {
        Some(*self)
    }

    /// Returns [NcResizeCb].
    ///
    /// *C style function: [ncresizecb_to_rust()][ncresizecb_to_rust].*
    fn to_rust(&self) -> Option<NcResizeCb> {
        ncresizecb_to_rust(Some(*self))
    }
}
