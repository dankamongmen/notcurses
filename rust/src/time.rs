//! `NcTime`

///
// only used for now with [`notcurses_getc_nblock`], which can't use
// libc::timespec
pub type NcTime = crate::bindings::ffi::timespec;
