//! `NcTime`

///
// Expected by [`notcurses_getc`] & [`notcurses_getc_nblock`], that can't use
// libc::timespec
pub type NcTime = crate::bindings::ffi::timespec;

impl NcTime {
    pub fn new() -> Self {
        Self {
            tv_sec: 0,
            tv_nsec: 0,
        }
    }
}
