//! Test `notcurses_*` reimplemented functions.

use serial_test::serial;
use std::io::Read;

use crate::{notcurses_init_test, notcurses_stop, NCRESULT_MAX};

use crate::NcFile;

#[test]
#[serial]
fn notcurses_align() {
    unsafe {
        let nc = notcurses_init_test();
        assert_eq![0, crate::notcurses_align(30, crate::NCALIGN_LEFT, 20)];
        assert_eq![5, crate::notcurses_align(30, crate::NCALIGN_CENTER, 20)];
        assert_eq![10, crate::notcurses_align(30, crate::NCALIGN_RIGHT, 20)];
        assert_eq![
            -NCRESULT_MAX,
            crate::notcurses_align(30, crate::NCALIGN_UNALIGNED, 20)
        ];
        notcurses_stop(nc);
    }
}

#[test]
#[serial]
fn notcurses_canchangecolor() {
    unsafe {
        let nc = notcurses_init_test();
        let res = crate::notcurses_canchangecolor(nc);
        notcurses_stop(nc);
        print!("[{}] ", res);
    }
}

#[test]
#[serial]
fn notcurses_canfade() {
    unsafe {
        let nc = notcurses_init_test();
        let res = crate::notcurses_canfade(nc);
        notcurses_stop(nc);
        print!("[{}] ", res);
    }
}

#[test]
#[serial]
fn notcurses_canopen_images() {
    unsafe {
        let nc = notcurses_init_test();
        let res = crate::notcurses_canopen_images(nc);
        notcurses_stop(nc);
        print!("[{}] ", res);
    }
}

#[test]
#[serial]
fn notcurses_canopen_videos() {
    unsafe {
        let nc = notcurses_init_test();
        let res = crate::notcurses_canopen_videos(nc);
        notcurses_stop(nc);
        print!("[{}] ", res);
    }
}

#[test]
#[serial]
fn notcurses_cansextant() {
    unsafe {
        let nc = notcurses_init_test();
        let res = crate::notcurses_cansextant(nc);
        notcurses_stop(nc);
        print!("[{}] ", res);
    }
}

#[test]
#[serial]
fn notcurses_cantruecolor() {
    unsafe {
        let nc = notcurses_init_test();
        let res = crate::notcurses_cantruecolor(nc);
        notcurses_stop(nc);
        print!("[{}] ", res);
    }
}

#[test]
#[serial]
fn notcurses_canutf8() {
    unsafe {
        let nc = notcurses_init_test();
        let res = crate::notcurses_canutf8(nc);
        notcurses_stop(nc);
        print!("[{}] ", res);
    }
}

#[test]
#[serial]
fn notcurses_drop_planes() {
    unsafe {
        let nc = notcurses_init_test();
        let stdplane = crate::notcurses_stdplane(nc);
        let plane1 = crate::ncplane_new_bound_test(&mut *stdplane, 0, 0, 10, 10);
        let _plane2 = crate::ncplane_new_bound_test(plane1, 0, 0, 10, 10);

        crate::notcurses_drop_planes(nc);
        // TODO: CHECK that planes are really dropped.

        notcurses_stop(nc);
    }
}

#[test]
#[serial]
fn notcurses_initialization() {
    unsafe {
        let nc = notcurses_init_test();
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
        let nc = notcurses_init_test();
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
        let nc = notcurses_init_test();

        #[cfg(any(
            target_arch = "s390x",
            target_arch = "powerpc64le",
            target_arch = "armv7hl",
            target_arch = "aarch64"
        ))]
        let mut _p: *mut u8 = &mut 0;
        #[cfg(not(any(
            target_arch = "s390x",
            target_arch = "powerpc64le",
            target_arch = "armv7hl",
            target_arch = "aarch64"
        )))]
        let mut _p: *mut i8 = &mut 0;

        let mut _size: *mut usize = &mut 0;
        let mut file = NcFile::from_libc(libc::open_memstream(&mut _p, _size));
        crate::notcurses_debug(nc, file.as_nc_ptr());
        notcurses_stop(nc);

        let mut string1 = String::new();
        let _result = file.read_to_string(&mut string1);

        let string2 =
            " -------------------------- notcurses debug state -----------------------------";

        assert_eq![&string1[0..string2.len()], &string2[..]];
    }
}

#[test]
#[serial]
// TODO test version_components
fn notcurses_version() {
    let c_str = unsafe { crate::notcurses_version() };
    assert!(!c_str.is_null());
    print!("v{} ", crate::rstring![c_str]);
}
