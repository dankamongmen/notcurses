//! `ncplane_*` reimplemented functions.

use core::{ffi::c_void, ptr::null_mut};
use libc::free;
use std::ffi::CString;

use crate::{
    bindgen::__va_list_tag, cell_load, cell_release, cells_double_box, cells_rounded_box,
    channels_bchannel, channels_bg_alpha, channels_bg_default_p, channels_bg_rgb, channels_bg_rgb8,
    channels_fchannel, channels_fg_alpha, channels_fg_default_p, channels_fg_rgb, channels_fg_rgb8,
    ncplane_at_cursor, ncplane_at_yx, ncplane_box, ncplane_channels, ncplane_cursor_move_yx,
    ncplane_cursor_yx, ncplane_dim_yx, ncplane_gradient, ncplane_hline_interp, ncplane_putc_yx,
    ncplane_putegc_yx, ncplane_putnstr_yx, ncplane_putstr_yx, ncplane_resize, ncplane_styles,
    ncplane_vline_interp, ncplane_vprintf_yx, notcurses_align, NcAlign, NcAlphaBits, NcCell,
    NcChannel, NcChannelPair, NcColor, NcPlane, NcResult, NcStyleMask, NCRESULT_ERR, NCRESULT_OK,
};

/// Return the column at which 'cols' columns ought start in order to be aligned
/// according to 'align' within ncplane 'n'. Returns INT_MAX on invalid 'align'.
/// Undefined behavior on negative 'cols'.
//
// NOTE: [leave cols as i32](https://github.com/dankamongmen/notcurses/issues/904)
#[inline]
pub fn ncplane_align(plane: &NcPlane, align: NcAlign, cols: i32) -> i32 {
    notcurses_align(ncplane_dim_x(plane), align, cols)
}

/// Retrieve the current contents of the cell under the cursor into 'cell'.
/// This cell is invalidated if the associated plane is destroyed.
#[inline]
pub fn ncplane_at_cursor_cell(plane: &mut NcPlane, cell: &mut NcCell) -> NcResult {
    let mut egc = unsafe { ncplane_at_cursor(plane, &mut cell.stylemask, &mut cell.channels) };
    if egc.is_null() {
        return NCRESULT_ERR;
    }
    let result: NcResult = unsafe { cell_load(plane, cell, egc) };
    if result != NCRESULT_OK {
        unsafe {
            free(&mut egc as *mut _ as *mut c_void);
        }
    }
    result
}

/// Retrieve the current contents of the specified cell into 'cell'.
/// This cell is invalidated if the associated plane is destroyed.
#[inline]
pub fn ncplane_at_yx_cell(plane: &mut NcPlane, y: i32, x: i32, cell: &mut NcCell) -> NcResult {
    let mut egc = unsafe { ncplane_at_yx(plane, y, x, &mut cell.stylemask, &mut cell.channels) };
    if egc.is_null() {
        return NCRESULT_ERR;
    }
    let channels = cell.channels; // need to preserve wide flag
    let result: NcResult = unsafe { cell_load(plane, cell, egc) };
    cell.channels = channels;
    unsafe {
        free(&mut egc as *mut _ as *mut c_void);
    }
    result
}

/// Draw a box with its upper-left corner at the current cursor position, having
/// dimensions 'ylen'x'xlen'. See ncplane_box() for more information. The
/// minimum box size is 2x2, and it cannot be drawn off-screen.
#[inline]
pub fn ncplane_box_sized(
    plane: &mut NcPlane,
    ul: &NcCell,
    ur: &NcCell,
    ll: &NcCell,
    lr: &NcCell,
    hline: &NcCell,
    vline: &NcCell,
    ylen: i32,
    xlen: i32,
    ctlword: u32,
) -> NcResult {
    let (mut y, mut x) = (0, 0);
    unsafe {
        ncplane_cursor_yx(plane, &mut y, &mut x);
        ncplane_box(
            plane,
            ul,
            ur,
            ll,
            lr,
            hline,
            vline,
            y + ylen - 1,
            x + xlen - 1,
            ctlword,
        )
    }
}

///
#[inline]
pub fn ncplane_dim_x(plane: &NcPlane) -> i32 {
    unsafe {
        let mut x = 0;
        ncplane_dim_yx(plane, null_mut(), &mut x);
        x
    }
}

///
#[inline]
pub fn ncplane_dim_y(plane: &NcPlane) -> i32 {
    unsafe {
        let mut y = 0;
        ncplane_dim_yx(plane, &mut y, null_mut());
        y
    }
}

///
#[inline]
pub fn ncplane_double_box(
    plane: &mut NcPlane,
    stylemask: NcStyleMask,
    channels: NcChannelPair,
    ystop: i32,
    xstop: i32,
    ctlword: u32,
) -> NcResult {
    #[allow(unused_assignments)]
    let mut ret = NCRESULT_OK;

    let mut ul = NcCell::new();
    let mut ur = NcCell::new();
    let mut ll = NcCell::new();
    let mut lr = NcCell::new();
    let mut hl = NcCell::new();
    let mut vl = NcCell::new();

    unsafe {
        ret = cells_double_box(
            plane,
            stylemask as u32,
            channels,
            &mut ul,
            &mut ur,
            &mut ll,
            &mut lr,
            &mut hl,
            &mut vl,
        );
        if ret == NCRESULT_OK {
            ret = ncplane_box(plane, &ul, &ur, &ll, &lr, &hl, &vl, ystop, xstop, ctlword);
        }

        cell_release(plane, &mut ul);
        cell_release(plane, &mut ur);
        cell_release(plane, &mut ll);
        cell_release(plane, &mut lr);
        cell_release(plane, &mut hl);
        cell_release(plane, &mut vl);
    }
    ret
}

///
#[inline]
pub fn ncplane_double_box_sized(
    plane: &mut NcPlane,
    stylemask: NcStyleMask,
    channels: NcChannelPair,
    ylen: i32,
    xlen: i32,
    ctlword: u32,
) -> NcResult {
    let (mut y, mut x) = (0, 0);
    unsafe {
        ncplane_cursor_yx(plane, &mut y, &mut x);
    }
    ncplane_double_box(
        plane,
        stylemask,
        channels,
        y + ylen - 1,
        x + xlen - 1,
        ctlword,
    )
}

/// On error, return the negative number of cells drawn.
#[inline]
pub fn ncplane_hline(plane: &mut NcPlane, cell: &NcCell, len: i32) -> i32 {
    unsafe { ncplane_hline_interp(plane, cell, len, cell.channels, cell.channels) }
}

///
#[inline]
pub fn ncplane_perimeter(
    plane: &mut NcPlane,
    ul: &NcCell,
    ur: &NcCell,
    ll: &NcCell,
    lr: &NcCell,
    hline: &NcCell,
    vline: &NcCell,
    ctlword: u32,
) -> NcResult {
    unsafe {
        ncplane_cursor_move_yx(plane, 0, 0);
        let (mut dimy, mut dimx) = (0, 0);
        ncplane_dim_yx(plane, &mut dimy, &mut dimx);
        ncplane_box_sized(plane, ul, ur, ll, lr, hline, vline, dimy, dimx, ctlword)
    }
}

///
#[inline]
pub fn ncplane_perimeter_double(
    plane: &mut NcPlane,
    stylemask: NcStyleMask,
    channels: NcChannelPair,
    ctlword: u32,
) -> NcResult {
    if unsafe { ncplane_cursor_move_yx(plane, 0, 0) } != NCRESULT_OK {
        return NCRESULT_ERR;
    }
    let (mut dimy, mut dimx) = (0, 0);
    unsafe {
        ncplane_dim_yx(plane, &mut dimy, &mut dimx);
    }
    let mut ul = NcCell::new();
    let mut ur = NcCell::new();
    let mut ll = NcCell::new();
    let mut lr = NcCell::new();
    let mut hl = NcCell::new();
    let mut vl = NcCell::new();
    if unsafe {
        cells_double_box(
            plane,
            stylemask as u32,
            channels,
            &mut ul,
            &mut ur,
            &mut ll,
            &mut lr,
            &mut hl,
            &mut vl,
        )
    } != NCRESULT_OK
    {
        return NCRESULT_ERR;
    }
    let ret = ncplane_box_sized(plane, &ul, &ur, &ll, &lr, &hl, &vl, dimy, dimx, ctlword);
    unsafe {
        cell_release(plane, &mut ul);
        cell_release(plane, &mut ur);
        cell_release(plane, &mut ll);
        cell_release(plane, &mut lr);
        cell_release(plane, &mut hl);
        cell_release(plane, &mut vl);
    }
    ret
}

///
#[inline]
pub fn ncplane_perimeter_rounded(
    plane: &mut NcPlane,
    stylemask: NcStyleMask,
    channels: NcChannelPair,
    ctlword: u32,
) -> NcResult {
    if unsafe { ncplane_cursor_move_yx(plane, 0, 0) } != NCRESULT_OK {
        return NCRESULT_ERR;
    }
    let (mut dimy, mut dimx) = (0, 0);
    unsafe {
        ncplane_dim_yx(plane, &mut dimy, &mut dimx);
    }
    let mut ul = NcCell::new();
    let mut ur = NcCell::new();
    let mut ll = NcCell::new();
    let mut lr = NcCell::new();
    let mut hl = NcCell::new();
    let mut vl = NcCell::new();
    if unsafe {
        cells_rounded_box(
            plane,
            stylemask as u32,
            channels,
            &mut ul,
            &mut ur,
            &mut ll,
            &mut lr,
            &mut hl,
            &mut vl,
        )
    } != NCRESULT_OK
    {
        return NCRESULT_ERR;
    }
    let ret = ncplane_box_sized(plane, &ul, &ur, &ll, &lr, &hl, &vl, dimy, dimx, ctlword);
    unsafe {
        cell_release(plane, &mut ul);
        cell_release(plane, &mut ur);
        cell_release(plane, &mut ll);
        cell_release(plane, &mut lr);
        cell_release(plane, &mut hl);
        cell_release(plane, &mut vl);
    }
    ret
}

/// Call ncplane_putc_yx() for the current cursor location.
#[inline]
pub fn ncplane_putc(plane: &mut NcPlane, cell: &NcCell) -> NcResult {
    unsafe { ncplane_putc_yx(plane, -1, -1, cell) }
}

/// Call ncplane_putchar_yx() at the current cursor location.
#[inline]
pub fn ncplane_putchar(plane: &mut NcPlane, c: char) -> NcResult {
    ncplane_putchar_yx(plane, -1, -1, c)
}

/// Replace the EGC underneath us, but retain the styling. The current styling
/// of the plane will not be changed.
///
/// Replace the cell at the specified coordinates with the provided 7-bit char
/// 'c'. Advance the cursor by 1. On success, returns 1. On failure, returns -1.
/// This works whether the underlying char is signed or unsigned.
#[inline]
// TODO: test char is < 8bit (currently 32bit)
pub fn ncplane_putchar_yx(plane: &mut NcPlane, y: i32, x: i32, c: char) -> NcResult {
    unsafe {
        let ce = NcCell::with_all(c, ncplane_styles(plane), ncplane_channels(plane));
        ncplane_putc_yx(plane, y, x, &ce)
    }
}

/// Call ncplane_putegc() at the current cursor location.
#[inline]
pub fn ncplane_putegc(plane: &mut NcPlane, gcluster: i8, sbytes: &mut i32) -> NcResult {
    unsafe { ncplane_putegc_yx(plane, -1, -1, &gcluster, sbytes) }
}

///
#[inline]
pub fn ncplane_putstr(plane: &mut NcPlane, string: &str) -> NcResult {
    unsafe {
        ncplane_putstr_yx(
            plane,
            -1,
            -1,
            CString::new(string.as_bytes())
                .expect("Bad string")
                .as_ptr(),
        )
    }
}

///
#[inline]
pub fn ncplane_putnstr(plane: &mut NcPlane, size: u64, gclustarr: &[u8]) -> NcResult {
    unsafe {
        ncplane_putnstr_yx(
            plane,
            -1,
            -1,
            size,
            CString::new(gclustarr).expect("Bad string").as_ptr(),
        )
    }
}

/// Resize the plane, retaining what data we can (everything, unless we're
/// shrinking in some dimension). Keep the origin where it is.
#[inline]
pub fn ncplane_resize_simple(plane: &mut NcPlane, ylen: i32, xlen: i32) -> NcResult {
    let (mut oldy, mut oldx) = (0, 0);
    unsafe {
        ncplane_dim_yx(plane, &mut oldy, &mut oldx);
    }
    let keepleny = {
        if oldy > ylen {
            ylen
        } else {
            oldy
        }
    };
    let keeplenx = {
        if oldx > xlen {
            xlen
        } else {
            oldx
        }
    };
    unsafe { ncplane_resize(plane, 0, 0, keepleny, keeplenx, 0, 0, ylen, xlen) }
}

///
/// On error, return the negative number of cells drawn.
#[inline]
pub fn ncplane_vline(plane: &mut NcPlane, cell: &NcCell, len: i32) -> i32 {
    unsafe { ncplane_vline_interp(plane, cell, len, cell.channels, cell.channels) }
}

///
#[inline]
pub fn ncplane_vprintf(plane: &mut NcPlane, format: &str, ap: &mut __va_list_tag) -> NcResult {
    unsafe {
        ncplane_vprintf_yx(
            plane,
            -1,
            -1,
            CString::new(format).expect("Bad string").as_ptr(),
            ap,
        )
    }
}

/// Draw a gradient with its upper-left corner at the current cursor position,
/// having dimensions 'ylen'x'xlen'. See ncplane_gradient for more information.
/// static inline int
// XXX receive cells as u32? https://github.com/dankamongmen/notcurses/issues/920
#[inline]
pub fn ncplane_gradient_sized(
    plane: &mut NcPlane,
    egc: &[u8],
    stylemask: NcStyleMask,
    ul: u64,
    ur: u64,
    ll: u64,
    lr: u64,
    ylen: i32,
    xlen: i32,
) -> NcResult {
    if ylen < 1 || xlen < 1 {
        return NCRESULT_ERR;
    }
    let (mut y, mut x) = (0, 0);
    unsafe {
        ncplane_cursor_yx(plane, &mut y, &mut x);
        ncplane_gradient(
            plane,
            CString::new(egc).expect("Bad EGC").as_ptr(),
            stylemask as u32,
            ul,
            ur,
            ll,
            lr,
            y + ylen - 1,
            x + xlen - 1,
        )
    }
}

/// Extract the 32-bit working foreground channel from an ncplane.
#[inline]
pub fn ncplane_fchannel(plane: &NcPlane) -> NcChannel {
    channels_fchannel(unsafe { ncplane_channels(plane) })
}

/// Extract the 32-bit working background channel from an ncplane.
#[inline]
pub fn ncplane_bchannel(plane: &NcPlane) -> NcChannel {
    channels_bchannel(unsafe { ncplane_channels(plane) })
}

/// Extract 24 bits of working foreground RGB from an ncplane, shifted to LSBs.
#[inline]
pub fn ncplane_fg_rgb(plane: &NcPlane) -> NcChannel {
    channels_fg_rgb(unsafe { ncplane_channels(plane) })
}

/// Extract 24 bits of working background RGB from an ncplane, shifted to LSBs.
#[inline]
pub fn ncplane_bg_rgb(plane: &NcPlane) -> NcChannel {
    channels_bg_rgb(unsafe { ncplane_channels(plane) })
}

/// Extract 2 bits of foreground alpha from 'struct ncplane', shifted to LSBs.
#[inline]
pub fn ncplane_fg_alpha(plane: &NcPlane) -> NcAlphaBits {
    channels_fg_alpha(unsafe { ncplane_channels(plane) })
}

/// Extract 2 bits of background alpha from 'struct ncplane', shifted to LSBs.
#[inline]
pub fn ncplane_bg_alpha(plane: &NcPlane) -> NcAlphaBits {
    channels_bg_alpha(unsafe { ncplane_channels(plane) })
}

/// Is the plane's foreground using the "default foreground color"?
#[inline]
pub fn ncplane_fg_default_p(plane: &NcPlane) -> bool {
    channels_fg_default_p(unsafe { ncplane_channels(plane) })
}

/// Is the plane's background using the "default background color"?
#[inline]
pub fn ncplane_bg_default_p(plane: &NcPlane) -> bool {
    channels_bg_default_p(unsafe { ncplane_channels(plane) })
}

/// Extract 24 bits of foreground RGB from a plane, split into components.
#[inline]
pub fn ncplane_fg_rgb8(
    plane: &NcPlane,
    red: &mut NcColor,
    green: &mut NcColor,
    blue: &mut NcColor,
) -> NcChannel {
    channels_fg_rgb8(unsafe { ncplane_channels(plane) }, red, green, blue)
}

/// Extract 24 bits of background RGB from a plane, split into components.
#[inline]
pub fn ncplane_bg_rgb8(
    plane: &NcPlane,
    red: &mut NcColor,
    green: &mut NcColor,
    blue: &mut NcColor,
) -> NcChannel {
    channels_bg_rgb8(unsafe { ncplane_channels(plane) }, red, green, blue)
}

///
#[inline]
pub fn ncplane_rounded_box(
    plane: &mut NcPlane,
    stylemask: NcStyleMask,
    channels: NcChannelPair,
    ystop: i32,
    xstop: i32,
    ctlword: u32,
) -> NcResult {
    #[allow(unused_assignments)]
    let mut ret = NCRESULT_OK;

    let mut ul = NcCell::new();
    let mut ur = NcCell::new();
    let mut ll = NcCell::new();
    let mut lr = NcCell::new();
    let mut hl = NcCell::new();
    let mut vl = NcCell::new();

    unsafe {
        ret = cells_rounded_box(
            plane,
            stylemask as u32,
            channels,
            &mut ul,
            &mut ur,
            &mut ll,
            &mut lr,
            &mut hl,
            &mut vl,
        );
        if ret == NCRESULT_OK {
            ret = ncplane_box(plane, &ul, &ur, &ll, &lr, &hl, &vl, ystop, xstop, ctlword);
        }
        cell_release(plane, &mut ul);
        cell_release(plane, &mut ur);
        cell_release(plane, &mut ll);
        cell_release(plane, &mut lr);
        cell_release(plane, &mut hl);
        cell_release(plane, &mut vl);
    }
    ret
}

///
#[inline]
pub fn ncplane_rounded_box_sized(
    plane: &mut NcPlane,
    stylemask: NcStyleMask,
    channels: NcChannelPair,
    ylen: i32,
    xlen: i32,
    ctlword: u32,
) -> NcResult {
    let (mut y, mut x) = (0, 0);
    unsafe {
        ncplane_cursor_yx(plane, &mut y, &mut x);
    }
    ncplane_rounded_box(
        plane,
        stylemask,
        channels,
        y + ylen - 1,
        x + xlen - 1,
        ctlword,
    )
}

// static inline int
// ncplane_printf(struct ncplane* n, const char* format, ...)
//   __attribute__ ((format (printf, 2, 3)));

// static inline int
// ncplane_printf(struct ncplane* n, const char* format, ...){
//   va_list va;
//   va_start(va, format);
//   int ret = ncplane_vprintf(n, format, va);
//   va_end(va);
//   return ret;
// }

// static inline int
// ncplane_printf_yx(struct ncplane* n, int y, int x, const char* format, ...)
//   __attribute__ ((format (printf, 4, 5)));

// static inline int
// ncplane_printf_yx(struct ncplane* n, int y, int x, const char* format, ...){
//   va_list va;
//   va_start(va, format);
//   int ret = ncplane_vprintf_yx(n, y, x, format, va);
//   va_end(va);
//   return ret;
// }

// static inline int
// ncplane_printf_aligned(struct ncplane* n, int y, ncalign_e align,
//                        const char* format, ...)
//   __attribute__ ((format (printf, 4, 5)));

// static inline int
// ncplane_printf_aligned(struct ncplane* n, int y, ncalign_e align, const char* format, ...){
//   va_list va;
//   va_start(va, format);
//   int ret = ncplane_vprintf_aligned(n, y, align, format, va);
//   va_end(va);
//   return ret;
// }

// static inline int
// ncplane_printf_stained(struct ncplane* n, const char* format, ...)
//   __attribute__ ((format (printf, 2, 3)));

// static inline int
// ncplane_printf_stained(struct ncplane* n, const char* format, ...){
//   va_list va;
//   va_start(va, format);
//   int ret = ncplane_vprintf_stained(n, format, va);
//   va_end(va);
//   return ret;
// }
