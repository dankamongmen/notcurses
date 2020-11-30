use core::ptr::{null, null_mut};
use std::ffi::{CStr, CString};

use serial_test::serial; // serialize tests w/ sys::notcurses_init()

use libnotcurses_sys as sys;

#[test]
#[serial]
fn get_notcurses_version() {
    let c_str = unsafe {
        let s = sys::notcurses_version();
        assert!(!s.is_null());
        CStr::from_ptr(s)
    };
    let r_str = c_str.to_str().unwrap();
    print!("rust-bound notcurses v{} ", r_str);
}

#[test]
#[serial]
fn create_notcurses_context() {
    unsafe {
        let _ = libc::setlocale(libc::LC_ALL, CString::new("").unwrap().as_ptr());
        let opts = sys::NotcursesOptions {
            loglevel: 0,
            termtype: null(),
            renderfp: null_mut(),
            margin_t: 0,
            margin_r: 0,
            margin_b: 0,
            margin_l: 0,
            flags: (sys::types::NCOPTION_NO_ALTERNATE_SCREEN | sys::types::NCOPTION_INHIBIT_SETLOCALE | sys::types::NCOPTION_SUPPRESS_BANNERS),
        };
        let nc = sys::notcurses_init(&opts, null_mut());
        sys::notcurses_stop(nc);
    }
}

#[test]
#[serial]
fn create_direct_context() {
    unsafe {
        let _ = libc::setlocale(libc::LC_ALL, CString::new("").unwrap().as_ptr());
        let nc = sys::ncdirect_init(null_mut(), null_mut(), 0);
        sys::ncdirect_stop(nc);
    }
}
