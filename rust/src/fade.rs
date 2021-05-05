//! `NcFadeCb` & `NcFadeCtx`

// functions already exported by bindgen : 3
// -------------------------------------------
// (#) test: 0
// (W) wrap: 3 / 0
// -------------------------------------------
//W  ncfadectx_free
//W  ncfadectx_iterations
//W  ncfadectx_setup

use std::ffi::c_void;

use crate::{NcIntResult, NcPlane, NcTime, Notcurses};

/// Called for each fade iteration on the NcPlane.
///
/// If anything but 0 is returned, the fading operation ceases immediately,
/// and that value is propagated out.
///
/// The recommended absolute display time target is passed in 'tspec'.
pub type NcFadeCb = Option<
    unsafe extern "C" fn(*mut Notcurses, *mut NcPlane, *const NcTime, *mut c_void) -> NcIntResult,
>;

/// Context for a palette fade operation
pub type NcFadeCtx = crate::bindings::ffi::ncfadectx;

impl NcFadeCtx {
    /// NcFadeCtx constructor.
    ///
    /// Rather than the simple ncplane_fade{in/out}(),
    /// ncfadectx_setup() can be paired with a loop over
    /// ncplane_fade{in/out}_iteration() + ncfadectx_free().
    pub fn setup(plane: &mut NcPlane) -> &mut NcFadeCtx {
        unsafe { &mut *crate::ncfadectx_setup(plane) }
    }

    /// Releases the resources associated.
    pub fn free(&mut self) {
        unsafe {
            crate::ncfadectx_free(self);
        }
    }

    /// Returns the number of iterations through which will fade.
    pub fn iterations(&self) -> u32 {
        unsafe { crate::ncfadectx_iterations(self) as u32 }
    }
}
