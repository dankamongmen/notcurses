//! `NcStats`

use crate::Notcurses;

/// notcurses runtime statistics
pub type NcStats = crate::bindings::ffi::ncstats;

/// # `NcStats` Methods.
impl NcStats {
    /// Allocates an NcStats object.
    pub fn new<'a>(nc: &'a Notcurses) -> &'a mut Self {
        unsafe { &mut *crate::notcurses_stats_alloc(nc) }
    }

    /// Acquires an atomic snapshot of the Notcurses object's stats.
    pub fn stats(&mut self, nc: &Notcurses) {
        unsafe { crate::notcurses_stats(nc, self) }
    }

    /// Resets all cumulative stats (immediate ones are not reset).
    pub fn reset(&mut self, nc: &mut Notcurses) {
        unsafe { crate::notcurses_stats_reset(nc, self) }
    }
}
