// Already done by dankamongmen:
//
// - ncplane_putstr
// - ncplane_dim_y
// - ncplane_dim_x
// - ncplane_perimeter

use std::ffi::CString;
use std::ptr::null_mut;

use crate as ffi;
use ffi::types::IntResult;

pub fn ncplane_putstr(plane: *mut ffi::ncplane, _str: &str) -> i32 {
    unsafe {
        ffi::ncplane_putstr_yx(
            plane,
            -1,
            -1,
            CString::new(_str).expect("Bad string").as_ptr(),
        )
    }
}

pub fn ncplane_dim_y(plane: *const ffi::ncplane) -> i32 {
    unsafe {
        let mut y = 0;
        ffi::ncplane_dim_yx(plane, &mut y, null_mut());
        return y;
    }
}

pub fn ncplane_dim_x(plane: *const ffi::ncplane) -> i32 {
    unsafe {
        let mut x = 0;
        ffi::ncplane_dim_yx(plane, null_mut(), &mut x);
        return x;
    }
}

pub fn ncplane_perimeter(
    plane: *mut ffi::ncplane,
    _ul: *const ffi::cell,
    _ur: *const ffi::cell,
    _ll: *const ffi::cell,
    _lr: *const ffi::cell,
    _hl: *const ffi::cell,
    _vl: *const ffi::cell,
    _ctlword: u32,
) -> IntResult {
    unsafe { ffi::ncplane_cursor_move_yx(plane, 0, 0) }
}
