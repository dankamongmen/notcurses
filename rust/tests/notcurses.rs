use core::ptr::{null, null_mut};
use std::ffi::{CStr, CString};

use serial_test::serial; // serialize tests w/ nc::notcurses_init()

use libnotcurses_sys as nc;

#[test]
#[serial]
fn get_notcurses_version() {
    let c_str = unsafe {
        let s = nc::notcurses_version();
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
        let opts = nc::NotcursesOptions {
            loglevel: 0,
            termtype: null(),
            renderfp: null_mut(),
            margin_t: 0,
            margin_r: 0,
            margin_b: 0,
            margin_l: 0,
            flags: (nc::types::NCOPTION_NO_ALTERNATE_SCREEN | nc::types::NCOPTION_INHIBIT_SETLOCALE),
        };
        let nc = nc::notcurses_init(&opts, null_mut());
        nc::notcurses_stop(nc);
    }
}

#[test]
#[serial]
fn create_direct_context() {
    unsafe {
        let _ = libc::setlocale(libc::LC_ALL, CString::new("").unwrap().as_ptr());
        let nc = nc::ncdirect_init(null_mut(), null_mut(), 0);
        nc::ncdirect_stop(nc);
    }
}
