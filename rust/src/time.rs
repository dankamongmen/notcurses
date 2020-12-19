//! `NcTime`

///
// Expected by [`notcurses_getc`] & [`notcurses_getc_nblock`], that can't use
// libc::timespec
pub type NcTime = crate::bindings::ffi::timespec;
