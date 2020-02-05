#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

use std::str;
use std::ffi::CStr;

extern {
    fn libc_stdout() -> *mut _IO_FILE;
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn get_notcurses_version() {
        unsafe {
            let c_str = unsafe {
                let s = notcurses_version();
                assert!(!s.is_null());
                CStr::from_ptr(s)
            };
            let r_str = c_str.to_str().unwrap();
            println!("rust-bound notcurses v{}", r_str);
        }
    }

    #[test]
    fn create_notcurses_context() {
        unsafe {
            let _ = libc::setlocale(libc::LC_ALL, std::ffi::CString::new("").unwrap().as_ptr());
            let opts: notcurses_options = notcurses_options {
                inhibit_alternate_screen: false,
                loglevel: 0,
                termtype: std::ptr::null(),
                retain_cursor: false,
                suppress_banner: false,
                no_winch_sighandler: false,
                no_quit_sighandlers: false,
                renderfp: std::ptr::null_mut(),
            };
            let nc = notcurses_init(&opts, libc_stdout());
            notcurses_stop(nc);
        }
    }

}
