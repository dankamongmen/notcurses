//! `NcProgBar` & `NcProgBarOptions` methods and associated functions.

use crate::{NcPlane, NcProgBar, NcProgBarOptions, NcResult};

/// # `NcProgBarOptions` Methods
impl NcProgBarOptions {
    /// New NcProgBarOptions for [NcProgBar].
    pub fn new() -> Self {
        Self {
            ulchannel: 0,
            urchannel: 0,
            blchannel: 0,
            brchannel: 0,
            flags: 0,
        }
    }
}

/// # `NcProgBar` Methods
impl NcProgBar {
    /// New NcProgBar.
    ///
    /// Takes ownership of the `plane`, which will be destroyed by
    /// [destroy][NcProgBar#method.destroy](). The progress bar is initially at 0%.
    pub fn new<'a>(plane: &mut NcPlane) -> &'a mut Self {
        Self::with_options(plane, &NcProgBarOptions::new())
    }

    /// New NcProgBar. Expects an [NcProgBarOptions] struct.
    ///
    /// C style function: [ncprogbar_create][crate::ncprogbar_create]
    pub fn with_options<'a>(plane: &mut NcPlane, options: &NcProgBarOptions) -> &'a mut Self {
        unsafe { &mut *crate::ncprogbar_create(plane, options) }
    }

    /// Destroy the progress bar and its underlying ncplane.
    ///
    /// C style function: [ncprogbar_destroy][crate::ncprogbar_destroy]
    pub fn destroy(&mut self) {
        unsafe {
            crate::ncprogbar_destroy(self);
        }
    }

    /// Return a reference to the ncprogbar's underlying ncplane.
    ///
    /// C style function: [ncprogbar_plane][crate::ncprogbar_plane]
    pub fn plane<'a>(&'a mut self) -> &'a mut NcPlane {
        unsafe { &mut *crate::ncprogbar_plane(self) }
    }

    /// Get the progress bar's completion, an [f64] on [0, 1].
    ///
    /// C style function: [ncprogbar_progress][crate::ncprogbar_progress]
    pub fn progress(&self) -> f64 {
        unsafe { crate::ncprogbar_progress(self) }
    }

    /// Sets the progress bar's completion, an 0 <= [f64] <= 1.
    ///
    /// Returns [NCRESULT_ERR][crate::NCRESULT_ERR] if progress is < 0 || > 1.
    ///
    /// C style function: [ncprogbar_set_progress][crate::ncprogbar_set_progress]
    pub fn set_progress(&mut self, progress: f64) -> NcResult {
        unsafe { crate::ncprogbar_set_progress(self, progress) }
    }
}
