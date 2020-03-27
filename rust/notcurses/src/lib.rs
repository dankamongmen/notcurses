extern crate libnotcurses_sys as ffi;

extern {
    fn libc_stdout() -> *mut ffi::_IO_FILE;
}

#[cfg(test)]
mod tests {
    use super::*;

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
            let nc = ffi::notcurses_init(&opts, libc_stdout());
            ffi::notcurses_stop(nc);
        }
    }
}
