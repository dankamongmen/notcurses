#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![no_std]

#![allow(clippy::too_many_arguments)]

// see https://github.com/rust-lang/rust-bindgen/issues/1470
#[allow(clippy::all)]
mod bindings {
    include!(concat!(env!("OUT_DIR"), "/bindings.rs"));
}
pub use bindings::*;

mod cells;
mod channel;
mod key;
mod keycodes;
mod nc;
mod palette;
mod pixel;
mod plane;
mod types;
pub use cells::*;
pub use channel::*;
pub use key::*;
pub use keycodes::*;
pub use nc::*;
pub use palette::*;
pub use pixel::*;
pub use plane::*;
pub use types::*;

#[cfg(test)]
mod tests {
    use core::ptr::{null, null_mut};
    use cstr_core::{CStr, CString};

    use libc_print::*;
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
        libc_println!("rust-bound notcurses v{}", r_str);
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
