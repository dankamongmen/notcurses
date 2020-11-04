use cstr_core::CString;

use libnotcurses_sys as nc;

fn main() {
    unsafe {
        let ncd = nc::ncdirect_new();

        render_image(&mut *ncd, nc::ncblitter_e_NCBLIT_1x1);
        render_image(&mut *ncd, nc::ncblitter_e_NCBLIT_2x1);
        render_image(&mut *ncd, nc::ncblitter_e_NCBLIT_BRAILLE);

        nc::ncdirect_stop(ncd);
    }
}

fn render_image(ncd: &mut nc::ncdirect, blit: nc::ncblitter_e) {
    unsafe {
        if nc::ncdirect_render_image(
            ncd,
            CString::new("image-16x16.png").unwrap().as_ptr(),
            nc::ncalign_e_NCALIGN_CENTER,
            blit,
            nc::ncscale_e_NCSCALE_NONE,
        ) != 0
        {
            panic!("ERR: ncdirect_render_image. \
                Make sure you are running this example from the examples folder");
        }
    }
}
