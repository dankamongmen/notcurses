extern crate libnotcurses_sys as ffi;

pub fn getc_blocking(_n: *mut ffi::notcurses, _ni: &mut ffi::ncinput) -> u32 {
    unsafe {
        let mut sigmask: ffi::sigset_t = std::mem::zeroed();
        ffi::sigemptyset(&mut sigmask);
        return ffi::notcurses_getc(_n, std::ptr::null(), &mut sigmask, _ni);
    }
}

pub fn ncplane_putstr_yx(_n: *mut ffi::ncplane, mut _y: i32, mut _x: i32, _str: &str) -> usize {
    let mut ret = 0;
    while ret < _str.len() {
        let mut wcs = 0;
        unsafe {
            let col = ffi::ncplane_putegc_yx(_n, -1, -1,  std::ffi::CString::new(_str).expect("Bad string").as_ptr(), &mut wcs);
            if col < 0 {
                return ret; // FIXME return error result
            }
            ret += col as usize;
        }
    }
    return ret;
}

pub fn ncplane_putstr(_n: *mut ffi::ncplane, _str: &str) -> usize {
    return ncplane_putstr_yx(_n, -1, -1, _str);
}

pub fn ncplane_dim_y(_n: *const ffi::ncplane) -> i32 {
    unsafe {
        let mut y = 0;
        ffi::ncplane_dim_yx(_n, &mut y, std::ptr::null_mut());
        return y;
    }
}

pub fn ncplane_dim_x(_n: *const ffi::ncplane) -> i32 {
    unsafe {
        let mut x = 0;
        ffi::ncplane_dim_yx(_n, std::ptr::null_mut(), &mut x);
        return x;
    }
}

pub fn render(_n: *mut ffi::notcurses) -> std::result::Result<(), std::io::Error> {
    unsafe {
        let r = ffi::notcurses_render(_n);
        if r != 0 {
            return Err(std::io::Error::new(std::io::ErrorKind::Other, "error rendering"));
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
    use serial_test_derive::serial; // serialize tests w/ ffi::notcurses_init()

    extern {
        static stdout: *mut ffi::_IO_FILE;
    }

    #[test]
    #[serial]
    fn create_context() {
        unsafe {
            let _ = libc::setlocale(libc::LC_ALL, std::ffi::CString::new("").unwrap().as_ptr());
            let opts: ffi::notcurses_options = ffi::notcurses_options {
                inhibit_alternate_screen: false,
                loglevel: 0,
                termtype: std::ptr::null(),
                retain_cursor: false,
                suppress_banner: false,
                no_winch_sighandler: false,
                no_quit_sighandlers: false,
                renderfp: std::ptr::null_mut(),
                margin_t: 0,
                margin_r: 0,
                margin_b: 0,
                margin_l: 0,
            };
            let nc = ffi::notcurses_init(&opts, stdout);
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
            let nc = ffi::notcurses_init(std::ptr::null(), stdout);
            assert_ne!(std::ptr::null(), nc);
            let mut dimsy = 0;
            let mut dimsx = 0;
            let _stdplane = stddim_yx(nc, &mut dimsy, &mut dimsx);
            all_asserts::assert_lt!(0, dimsy);
            all_asserts::assert_lt!(0, dimsx);
            let dimy = ncplane_dim_y(_stdplane);
            let dimx = ncplane_dim_x(_stdplane);
            assert_eq!(dimy, dimsy);
            assert_eq!(dimx, dimsx);
            assert_eq!(0, ffi::notcurses_stop(nc));
        }
    }
}
