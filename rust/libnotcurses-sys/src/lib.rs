#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

include!(concat!(env!("OUT_DIR"), "/bindings.rs"));

pub fn ncplane_putstr_yx(_n: *mut ncplane, mut _y: i32, mut _x: i32, _str: &str) -> usize {
    let mut ret = 0;
    while ret < _str.len() {
        let mut wcs = 0;
        unsafe {
            let col =ncplane_putegc_yx(_n, -1, -1,  std::ffi::CString::new(&_str[ret..]).expect("Bad string").as_ptr(), &mut wcs);
            if col < 0 {
                return ret; // FIXME return error result
            }
            ret += col as usize;
        }
    }
    return ret;
}

pub fn ncplane_putstr(_n: *mut ncplane, _str: &str) -> usize {
    return ncplane_putstr_yx(_n, -1, -1, _str);
}

pub fn ncplane_dim_y(_n: *const ncplane) -> i32 {
    unsafe {
        let mut y = 0;
        ncplane_dim_yx(_n, &mut y, std::ptr::null_mut());
        return y;
    }
}

pub fn ncplane_dim_x(_n: *const ncplane) -> i32 {
    unsafe {
        let mut x = 0;
        ncplane_dim_yx(_n, std::ptr::null_mut(), &mut x);
        return x;
    }
}

pub fn ncplane_perimeter(_n: *mut ncplane, _ul: *const cell,
                         _ur: *const cell, _ll: *const cell,
                         _lr: *const cell, _hl: *const cell,
                         _vl: *const cell, _ctlword: u32)
                         -> std::result::Result<(), std::io::Error> {
    let r;
    unsafe {
        r =ncplane_cursor_move_yx(_n, 0, 0);
    }
    if r != 0 {
        return Err(std::io::Error::new(std::io::ErrorKind::Other, "error moving cursor"));
    }
    Ok(())
}

pub fn cell_prime(_n: *mut ncplane, _c: *mut cell, _egc: &str,
                  _attr: u32, _channels: u64) -> i32 {
    unsafe{
        (*_c).attrword = _attr;
        (*_c).channels = _channels;
        return cell_load(_n, _c, std::ffi::CString::new(_egc).unwrap().as_ptr());
    }
}

pub fn cells_load_box(_n: *mut ncplane, _attrs: u32, _channels: u64,
                      _ul: *mut cell, _ur: *mut cell,
                      _ll: *mut cell, _lr: *mut cell,
                      _hl: *mut cell, _vl: *mut cell, _egcs: &str)
                      -> std::result::Result<(), std::io::Error> {
    let rul = cell_prime(_n, _ul, _egcs, _attrs, _channels);
    if rul <= 0 {
        return Err(std::io::Error::new(std::io::ErrorKind::Other, "error priming cell"));
    // FIXME cell_prime()s
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    extern {
        fn libc_stdout() -> *mut _IO_FILE;
    }

    #[test]
    fn get_notcurses_version() {
        unsafe {
            let c_str = unsafe {
                let s = notcurses_version();
                assert!(!s.is_null());
                std::ffi::CStr::from_ptr(s)
            };
            let r_str = c_str.to_str().unwrap();
            println!("rust-bound notcurses v{}", r_str);
        }
    }

    #[test]
    fn create_notcurses_context() {
        unsafe {
            let _ = libc::setlocale(libc::LC_ALL, std::ffi::CString::new("").unwrap().as_ptr());
            let opts: notcurses_options = notcurses_options {
                inhibit_alternate_screen: true,
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
            let nc = notcurses_init(&opts, libc_stdout());
            notcurses_stop(nc);
        }
    }

}
