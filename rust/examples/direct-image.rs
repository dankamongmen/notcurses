use cstr_core::CString;

use libnotcurses_sys as nc;

extern "C" {
    fn libc_stdout() -> *mut nc::_IO_FILE;
}

fn main() {
    unsafe {
        let _ = libc::setlocale(libc::LC_ALL, CString::new("").unwrap().as_ptr());
        let ncd: *mut nc::ncdirect = nc::ncdirect_init(std::ptr::null(), libc_stdout());

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
            CString::new("direct-image.png").unwrap().as_ptr(),
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
