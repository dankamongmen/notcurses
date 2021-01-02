//! Example 'direct-image'
//!
//! Explore image rendering in direct mode
//!
//! NOTE: This example uses the C style with functions.

use core::ptr::{null, null_mut};
use libnotcurses_sys::*;

fn main() {
    unsafe {
        let ncd = ncdirect_init(null(), null_mut(), 0);

        render_image(&mut *ncd, NCBLIT_1x1);
        render_image(&mut *ncd, NCBLIT_2x1);
        render_image(&mut *ncd, NCBLIT_BRAILLE);

        ncdirect_stop(ncd);
    }
}

fn render_image(ncd: &mut NcDirect, blit: NcBlitter) {
    unsafe {
        if ncdirect_render_image(
            ncd,
            cstring!["image-16x16.png"],
            NCALIGN_CENTER,
            blit,
            NCSCALE_NONE,
        ) != 0
        {
            panic!(
                "ERROR: ncdirect_render_image(). Make sure you \
                are running this example from the examples folder"
            );
        }
    }
}
