extern crate libnotcurses_sys as ffi;

pub fn getc_blocking(_n: *mut ffi::notcurses, _ni: &mut ffi::ncinput) -> u32 {
    unsafe {
      let mut sigmask: ffi::sigset_t = std::mem::zeroed();
      ffi::sigemptyset(&mut sigmask);
      return ffi::notcurses_getc(_n, std::ptr::null(), &mut sigmask, _ni);
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
