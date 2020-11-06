#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]
#![no_std]
#![allow(clippy::too_many_arguments)]

pub mod bindings;
pub mod types;

pub use bindings::*;
pub use types::*;

#[macro_use]
mod macros;

mod cells;
mod channel;
mod direct;
mod input;
mod key;
mod keycodes;
mod notcurses;
mod palette;
mod pixel;
mod plane;
mod visual;

pub use cells::*;
pub use channel::*;
pub use direct::*;
pub use input::*;
pub use key::*;
pub use keycodes::*;
pub use notcurses::*;
pub use palette::*;
pub use pixel::*;
pub use plane::*;
pub use visual::*;

// TODO: move tests out
#[cfg(test)]
mod tests {
    use core::ptr::{null, null_mut};
    use cstr_core::{CStr, CString};

    use libc_print::*;
    use serial_test::serial; // serialize tests w/ nc::notcurses_init()

    use crate as nc;

    #[test]
    #[serial]
    fn get_notcurses_version() {
        let c_str = unsafe {
            let s = nc::notcurses_version();
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
            let opts = nc::notcurses_options {
                loglevel: 0,
                termtype: null(),
                renderfp: null_mut(),
                margin_t: 0,
                margin_r: 0,
                margin_b: 0,
                margin_l: 0,
                flags: (nc::NCOPTION_NO_ALTERNATE_SCREEN | nc::NCOPTION_INHIBIT_SETLOCALE) as u64,
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
}
