#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

mod cells;
mod channels;
mod plane;
mod types;
pub use cells::*;
pub use channels::*;
pub use plane::*;
pub use types::*;

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

#[cfg(test)]
mod tests {
    use std::ffi::{CStr, CString};
    use std::ptr::{null, null_mut};

    use serial_test::serial; // serialize tests w/ ffi::notcurses_init()

    use crate as ffi;

    #[test]
    #[serial]
    fn get_notcurses_version() {
        let c_str = unsafe {
            let s = ffi::notcurses_version();
            assert!(!s.is_null());
            CStr::from_ptr(s)
        };
        let r_str = c_str.to_str().unwrap();
        println!("rust-bound notcurses v{}", r_str);
    }

    #[test]
    #[serial]
    fn create_notcurses_context() {
        unsafe {
            let _ = libc::setlocale(libc::LC_ALL, CString::new("").unwrap().as_ptr());
            let opts = ffi::notcurses_options {
                loglevel: 0,
                termtype: null(),
                renderfp: null_mut(),
                margin_t: 0,
                margin_r: 0,
                margin_b: 0,
                margin_l: 0,
                flags: (ffi::NCOPTION_NO_ALTERNATE_SCREEN | ffi::NCOPTION_INHIBIT_SETLOCALE) as u64,
            };
            let nc = ffi::notcurses_init(&opts, null_mut());
            ffi::notcurses_stop(nc);
        }
    }

    #[test]
    #[serial]
    fn create_direct_context() {
        unsafe {
            let _ = libc::setlocale(libc::LC_ALL, CString::new("").unwrap().as_ptr());
            let nc = ffi::ncdirect_init(null_mut(), null_mut());
            ffi::ncdirect_stop(nc);
        }
    }
}
