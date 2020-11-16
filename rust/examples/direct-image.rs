//! Example 'direct-image'
//!
//! Explore image rendering in direct mode

use std::ffi::CString;

// This time we are gonna use the notcurses library through the `sys` namespace
use libnotcurses_sys as sys;

fn main() {
    unsafe {
        let ncd = sys::NcDirect::new();

        render_image(&mut *ncd, sys::NCBLIT_1x1);
        render_image(&mut *ncd, sys::NCBLIT_2x1);
        render_image(&mut *ncd, sys::NCBLIT_BRAILLE);

        sys::ncdirect_stop(ncd);
    }
}

fn render_image(ncd: &mut sys::NcDirect, blit: sys::NcBlitter) {
    unsafe {
        if sys::ncdirect_render_image(
            ncd,
            CString::new("image-16x16.png").unwrap().as_ptr(),
            sys::NCALIGN_CENTER,
            blit,
            sys::NCSCALE_NONE,
        ) != 0
        {
            panic!("ERR: ncdirect_render_image. Make sure \
                you are running this example from the examples folder");
        }
    }
}
