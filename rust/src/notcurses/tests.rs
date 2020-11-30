//! [`Notcurses`] tests

use serial_test::serial;
use std::io::Read;

use crate::{notcurses_stop, NcFile, Notcurses};

#[test]
#[serial]
fn notcurses_canchangecolor() {
    unsafe {
        let nc = Notcurses::new();
        let res = crate::notcurses_canchangecolor(nc);
        notcurses_stop(nc);
        print!("[{}] ", res);
    }
}

#[test]
#[serial]
fn notcurses_canfade() {
    unsafe {
        let nc = Notcurses::new();
        let res = crate::notcurses_canfade(nc);
        notcurses_stop(nc);
        print!("[{}] ", res);
    }
}

#[test]
#[serial]
fn notcurses_canopen_images() {
    unsafe {
        let nc = Notcurses::new();
        let res = crate::notcurses_canopen_images(nc);
        notcurses_stop(nc);
        print!("[{}] ", res);
    }
}

#[test]
#[serial]
fn notcurses_canopen_videos() {
    unsafe {
        let nc = Notcurses::new();
        let res = crate::notcurses_canopen_videos(nc);
        notcurses_stop(nc);
        print!("[{}] ", res);
    }
}

#[test]
#[serial]
fn notcurses_cansixel() {
    unsafe {
        let nc = Notcurses::new();
        let res = crate::notcurses_cansixel(nc);
        notcurses_stop(nc);
        print!("[{}] ", res);
    }
}

#[test]
#[serial]
fn notcurses_cantruecolor() {
    unsafe {
        let nc = Notcurses::new();
        let res = crate::notcurses_cantruecolor(nc);
        notcurses_stop(nc);
        print!("[{}] ", res);
    }
}

#[test]
#[serial]
fn notcurses_canutf8() {
    unsafe {
        let nc = Notcurses::new();
        let res = crate::notcurses_canutf8(nc);
        notcurses_stop(nc);
        print!("[{}] ", res);
    }
}

#[test]
#[serial]
fn notcurses_init() {
    unsafe {
        let nc = Notcurses::new();
        assert![nc as *mut _ != core::ptr::null_mut()];
        notcurses_stop(nc);
    }
}

#[test]
#[serial]
#[ignore]
// FIXME: always return null
fn notcurses_at_yx() {
    unsafe {
        let nc = Notcurses::new();
        let mut sm = 0;
        let mut ch = 0;
        let res = crate::notcurses_at_yx(nc, 0, 0, &mut sm, &mut ch);
        notcurses_stop(nc);
        assert![!res.is_null()];

        //print!("[{}] ", res);
    }
}

#[test]
#[serial]
fn notcurses_debug() {
    unsafe {
        let nc = Notcurses::new();
        let mut _p: *mut i8 = &mut 0;
        let mut _size: *mut usize = &mut 0;
        let mut file = NcFile::from_libc(libc::open_memstream(&mut _p, _size));
        crate::notcurses_debug(nc, file.as_nc_ptr());
        notcurses_stop(nc);

        let mut string1 = String::new();
        let _result = file.read_to_string(&mut string1);

        let string2 =
            " ************************** notcurses debug state *****************************";

        assert_eq![&string1[0..string2.len()], &string2[..]];
    }
}
