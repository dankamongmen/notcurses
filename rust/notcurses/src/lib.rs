extern crate libnotcurses_sys as ffi;

pub fn getc_blocking(_n: *mut ffi::notcurses, _ni: &mut ffi::ncinput) -> u32 {
    unsafe {
        let mut sigmask: ffi::sigset_t = std::mem::zeroed();
        ffi::sigemptyset(&mut sigmask);
        return ffi::notcurses_getc(_n, std::ptr::null(), &mut sigmask, _ni);
    }
}

pub fn render(_n: *mut ffi::notcurses) -> std::result::Result<(), std::io::Error> {
    unsafe {
        let r = ffi::notcurses_render(_n);
        if r != 0 {
            return Err(std::io::Error::new(
                std::io::ErrorKind::Other,
                "error rendering",
            ));
        }
        Ok(())
    }
}

pub fn stddim_yx(_n: *mut ffi::notcurses, _dimy: &mut i32, _dimx: &mut i32) -> *mut ffi::ncplane {
    unsafe {
        let stdplane = ffi::notcurses_stdplane(_n);
        ffi::ncplane_dim_yx(stdplane, _dimy, _dimx);
        return stdplane;
    }
}

pub fn dim_yx(_n: *const ffi::notcurses, _dimy: &mut i32, _dimx: &mut i32) {
    unsafe {
        ffi::ncplane_dim_yx(ffi::notcurses_stdplane_const(_n), _dimy, _dimx);
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use all_asserts;
    use serial_test::serial; // serialize tests w/ ffi::notcurses_init()

    extern "C" {
        static stdout: *mut ffi::_IO_FILE;
    }

    fn opts() -> ffi::notcurses_options {
        ffi::notcurses_options {
            loglevel: 0,
            termtype: std::ptr::null(),
            renderfp: std::ptr::null_mut(),
            margin_t: 0,
            margin_r: 0,
            margin_b: 0,
            margin_l: 0,
            flags: ffi::NCOPTION_NO_ALTERNATE_SCREEN as u64 | ffi::NCOPTION_SUPPRESS_BANNERS as u64,
        }
    }

    #[test]
    #[serial]
    fn create_context() {
        unsafe {
            let _ = libc::setlocale(libc::LC_ALL, std::ffi::CString::new("").unwrap().as_ptr());
            let nc = ffi::notcurses_init(&opts(), stdout);
            assert_ne!(std::ptr::null(), nc);
            let mut dimy = 0;
            let mut dimx = 0;
            dim_yx(nc, &mut dimy, &mut dimx);
            all_asserts::assert_lt!(0, dimy);
            all_asserts::assert_lt!(0, dimx);
            assert_eq!(0, ffi::notcurses_stop(nc));
        }
    }

    #[test]
    #[serial]
    fn stdplane_dims() {
        unsafe {
            let _ = libc::setlocale(libc::LC_ALL, std::ffi::CString::new("").unwrap().as_ptr());
            let nc = ffi::notcurses_init(&opts(), stdout);
            assert_ne!(std::ptr::null(), nc);
            let mut dimsy = 0;
            let mut dimsx = 0;
            let _stdplane = stddim_yx(nc, &mut dimsy, &mut dimsx);
            all_asserts::assert_lt!(0, dimsy);
            all_asserts::assert_lt!(0, dimsx);
            let dimy = ffi::ncplane_dim_y(_stdplane);
            let dimx = ffi::ncplane_dim_x(_stdplane);
            assert_eq!(dimy, dimsy);
            assert_eq!(dimx, dimsx);
            assert_eq!(0, ffi::notcurses_stop(nc));
        }
    }
}
