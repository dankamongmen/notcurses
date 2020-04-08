extern crate libnotcurses_sys as ffi;

pub fn getc_blocking(_n: *mut ffi::notcurses, _ni: &mut ffi::ncinput) -> u32 {
    unsafe {
        let mut sigmask: ffi::sigset_t = std::mem::zeroed();
        ffi::sigemptyset(&mut sigmask);
        return ffi::notcurses_getc(_n, std::ptr::null(), &mut sigmask, _ni);
    }
}

pub fn ncplane_putstr(_n: *mut ffi::ncplane, _str: &str) -> i32 {
    unsafe {
        return ffi::ncplane_putstr_yx(_n, -1, -1, std::ffi::CString::new(_str).expect("Bad string").as_ptr());
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

#[cfg(test)]
mod tests {
    use super::*;

    extern {
        static stdout: *mut ffi::_IO_FILE;
    }

    #[test]
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
            ffi::notcurses_stop(nc);
        }
    }
}
