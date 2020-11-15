use std::ffi::CString;

use libnotcurses_sys as nc;

fn main() {
    unsafe {
        let ncd = nc::NcDirect::new();

        render_image(&mut *ncd, nc::NCBLIT_1x1);
        render_image(&mut *ncd, nc::NCBLIT_2x1);
        render_image(&mut *ncd, nc::NCBLIT_BRAILLE);

        nc::ncdirect_stop(ncd);
    }
}

fn render_image(ncd: &mut nc::NcDirect, blit: nc::NcBlitter) {
    unsafe {
        if nc::ncdirect_render_image(
            ncd,
            CString::new("image-16x16.png").unwrap().as_ptr(),
            nc::NCALIGN_CENTER,
            blit,
            nc::NCSCALE_NONE,
        ) != 0
        {
            panic!("ERR: ncdirect_render_image. \
                Make sure you are running this example from the examples folder");
        }
    }
}
