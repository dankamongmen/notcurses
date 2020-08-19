// functions already exported by bindgen : 86
// ------------------------------------------
// ncplane_aligned
// ncplane_at_cursor
// ncplane_attr
// ncplane_at_yx
// ncplane_base
// ncplane_below
// ncplane_bound
// ncplane_box
// ncplane_center_abs
// ncplane_channels
// ncplane_contents
// ncplane_cursor_move_yx
// ncplane_cursor_yx
// ncplane_destroy
// ncplane_dim_yx
// ncplane_dup
// ncplane_erase
// ncplane_fadein
// ncplane_fadein_iteration
// ncplane_fadeout
// ncplane_fadeout_iteration
// ncplane_format
// ncplane_gradient
// ncplane_greyscale
// ncplane_highgradient
// ncplane_hline_interp
// ncplane_home
// ncplane_mergedown
// ncplane_move_above
// ncplane_move_below
// ncplane_move_bottom
// ncplane_move_top
// ncplane_move_yx
// ncplane_new
// ncplane_notcurses
// ncplane_notcurses_const
// ncplane_polyfill_yx
// ncplane_pulse
// ncplane_putc_yx
// ncplane_putegc_stainable
// ncplane_putegc_yx
// ncplane_putnstr_aligned
// ncplane_putnstr_yx
// ncplane_putsimple_stainable
// ncplane_putstr_aligned
// ncplane_putstr_stainable
// ncplane_putstr_yx
// ncplane_puttext
// ncplane_putwegc_stainable
// ncplane_qrcode
// ncplane_reparent
// ncplane_resize
// ncplane_rgba
// ncplane_rotate_ccw
// ncplane_rotate_cw
// ncplane_set_attr
// ncplane_set_base
// ncplane_set_base_cell
// ncplane_set_bg
// ncplane_set_bg_alpha
// ncplane_set_bg_default
// ncplane_set_bg_palindex
// ncplane_set_bg_rgb
// ncplane_set_bg_rgb_clipped
// ncplane_set_channels
// ncplane_set_fg
// ncplane_set_fg_alpha
// ncplane_set_fg_default
// ncplane_set_fg_palindex
// ncplane_set_fg_rgb
// ncplane_set_fg_rgb_clipped
// ncplane_set_scrolling
// ncplane_set_userptr
// ncplane_stain
// ncplane_styles_off
// ncplane_styles_on
// ncplane_styles_set
// ncplane_translate
// ncplane_translate_abs
// ncplane_userptr
// ncplane_vline_interp
// ncplane_vprintf_aligned
// ncplane_vprintf_stainable
// ncplane_vprintf_yx
// ncplane_yx
//
// static inline functions to reimplement: 42
// ------------------------------------------ (done / (x) wont / remaining)
// (+) implement: 35 / 7 /  0
// (#) unit test:  0 / 7 / 35
// ------------------------------------------
//+ncplane_align
//+ncplane_at_cursor_cell
//+ncplane_at_yx_cell
//+ncplane_bchannel
//+ncplane_bg
//+ncplane_bg_alpha
//+ncplane_bg_default_p
//+ncplane_bg_rgb
//+ncplane_box_sized
//+ncplane_dim_x
//+ncplane_dim_y
//+ncplane_double_box
//+ncplane_double_box_sized
//+ncplane_fchannel
//+ncplane_fg
//+ncplane_fg_alpha
//+ncplane_fg_default_p
//+ncplane_fg_rgb
//+ncplane_gradient_sized     // u64|u32 https://github.com/dankamongmen/notcurses/issues/920
//+ncplane_highgradient_sized //
//+ncplane_hline
//+ncplane_perimeter
//+ncplane_perimeter_double
//+ncplane_perimeter_rounded
//+ncplane_putc
//+ncplane_putegc
//+ncplane_putnstr
//+ncplane_putsimple
//+ncplane_putsimple_yx
//+ncplane_putstr
//xncplane_putwc                // I don't think these will be needed from Rust. See:
//xncplane_putwc_yx             // https://locka99.gitbooks.io/a-guide-to-porting-c-to-rust/content/features_of_rust/strings.html
//xncplane_putwegc              //
//xncplane_putwegc_yx           //
//xncplane_putwstr              //
//xncplane_putwstr_aligned      //
//xncplane_putwstr_yx           //
//+ncplane_resize_simple
//+ncplane_rounded_box
//+ncplane_rounded_box_sized
//+ncplane_vline
//+ncplane_vprintf
//
// NOTE: TODO: Still remains all the ncplane_printf* functions/macros (at the end)

use core::convert::TryInto;
use core::ffi::c_void;
use core::ptr::null_mut;

use cstr_core::CString;

use crate as ffi;
use ffi::types::{AlphaBits, Channel, ChannelPair, Color, EGCBackstop, IntResult, StyleMask};
use ffi::{cell, ncalign_e, ncplane};

/// Return the column at which 'cols' columns ought start in order to be aligned
/// according to 'align' within ncplane 'n'. Returns INT_MAX on invalid 'align'.
/// Undefined behavior on negative 'cols'.
//
// NOTE: [leave cols as i32](https://github.com/dankamongmen/notcurses/issues/904)
// TODO: TEST
#[inline]
pub fn ncplane_align(plane: &ncplane, align: ncalign_e, cols: i32) -> i32 {
    if align == ffi::ncalign_e_NCALIGN_LEFT {
        return 0;
    }

    let plane_cols = ncplane_dim_x(plane);
    if cols > plane_cols {
        return 0;
    }

    if align == ffi::ncalign_e_NCALIGN_CENTER {
        return plane_cols - cols / 2;
    } else if align == ffi::ncalign_e_NCALIGN_RIGHT {
        return plane_cols - cols;
    }
    core::i32::MAX
}

/// Retrieve the current contents of the cell under the cursor into 'cell'.
/// This cell is invalidated if the associated plane is destroyed.
// TODO: TEST
#[inline]
pub fn nplane_at_cursor_cell(plane: &mut ncplane, cell: &mut cell) -> IntResult {
    let mut egc = unsafe { ffi::ncplane_at_cursor(plane, &mut cell.stylemask, &mut cell.channels) };
    if egc.is_null() {
        return -1;
    }
    let result: IntResult = unsafe { ffi::cell_load(plane, cell, egc) };
    if result < 0 {
        unsafe {
            ffi::free(&mut egc as *mut _ as *mut c_void);
        }
    }
    result
}

/// Retrieve the current contents of the specified cell into 'cell'.
/// This cell is invalidated if the associated plane is destroyed.
// TODO: TEST
#[inline]
pub fn ncplane_at_yx_cell(plane: &mut ncplane, y: i32, x: i32, cell: &mut cell) -> IntResult {
    let mut egc =
        unsafe { ffi::ncplane_at_yx(plane, y, x, &mut cell.stylemask, &mut cell.channels) };
    if egc.is_null() {
        return -1;
    }
    let channels = cell.channels; // need to preserve wide flag
    let result: IntResult = unsafe { ffi::cell_load(plane, cell, egc) };
    cell.channels = channels;
    unsafe {
        ffi::free(&mut egc as *mut _ as *mut c_void);
    }
    result
}

/// Draw a box with its upper-left corner at the current cursor position, having
/// dimensions 'ylen'x'xlen'. See ncplane_box() for more information. The
/// minimum box size is 2x2, and it cannot be drawn off-screen.
// TODO: TEST
#[inline]
pub fn ncplane_box_sized(
    plane: &mut ncplane,
    ul: &cell,
    ur: &cell,
    ll: &cell,
    lr: &cell,
    hline: &cell,
    vline: &cell,
    ylen: i32,
    xlen: i32,
    ctlword: u32,
) -> IntResult {
    let (mut y, mut x) = (0, 0);
    unsafe {
        ffi::ncplane_cursor_yx(plane, &mut y, &mut x);
        ffi::ncplane_box(
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
// TODO: TEST
#[inline]
pub fn ncplane_dim_x(plane: &ncplane) -> i32 {
    unsafe {
        let mut x = 0;
        ffi::ncplane_dim_yx(plane, null_mut(), &mut x);
        x
    }
}

///
// TODO: TEST
#[inline]
pub fn ncplane_dim_y(plane: &ncplane) -> i32 {
    unsafe {
        let mut y = 0;
        ffi::ncplane_dim_yx(plane, &mut y, null_mut());
        y
    }
}

// TODO: TEST
#[inline]
pub fn ncplane_double_box(
    plane: &mut ncplane,
    stylemask: StyleMask,
    channels: ChannelPair,
    ystop: i32,
    xstop: i32,
    ctlword: u32,
) -> IntResult {
    #[allow(unused_assignments)]
    let mut ret = 0;

    let mut ul = cell_trivial_initializer![];
    let mut ur = cell_trivial_initializer![];
    let mut ll = cell_trivial_initializer![];
    let mut lr = cell_trivial_initializer![];
    let mut hl = cell_trivial_initializer![];
    let mut vl = cell_trivial_initializer![];

    unsafe {
        ret = ffi::cells_double_box(
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
        if ret == 0 {
            ret = ffi::ncplane_box(plane, &ul, &ur, &ll, &lr, &hl, &vl, ystop, xstop, ctlword);
        }

        ffi::cell_release(plane, &mut ul);
        ffi::cell_release(plane, &mut ur);
        ffi::cell_release(plane, &mut ll);
        ffi::cell_release(plane, &mut lr);
        ffi::cell_release(plane, &mut hl);
        ffi::cell_release(plane, &mut vl);
    }
    ret
}

// TODO: TEST
#[inline]
pub fn ncplane_double_box_sized(
    plane: &mut ncplane,
    stylemask: StyleMask,
    channels: ChannelPair,
    ylen: i32,
    xlen: i32,
    ctlword: u32,
) -> IntResult {
    let (mut y, mut x) = (0, 0);
    unsafe {
        ffi::ncplane_cursor_yx(plane, &mut y, &mut x);
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
// TODO: TEST
#[inline]
pub fn ncplane_hline(plane: &mut ncplane, cell: &cell, len: i32) -> i32 {
    unsafe { ffi::ncplane_hline_interp(plane, cell, len, cell.channels, cell.channels) }
}

///
// TODO: TEST
#[inline]
pub fn ncplane_perimeter(
    plane: &mut ncplane,
    ul: &cell,
    ur: &cell,
    ll: &cell,
    lr: &cell,
    hline: &cell,
    vline: &cell,
    ctlword: u32,
) -> IntResult {
    unsafe {
        ffi::ncplane_cursor_move_yx(plane, 0, 0);
        let (mut dimy, mut dimx) = (0, 0);
        ffi::ncplane_dim_yx(plane, &mut dimy, &mut dimx);
        ncplane_box_sized(plane, ul, ur, ll, lr, hline, vline, dimy, dimx, ctlword)
    }
}

// TODO: TEST
#[inline]
pub fn ncplane_perimeter_double(
    plane: &mut ncplane,
    stylemask: StyleMask,
    channels: ChannelPair,
    ctlword: u32,
) -> IntResult {
    if unsafe { ffi::ncplane_cursor_move_yx(plane, 0, 0) } != 0 {
        return -1;
    }
    let (mut dimy, mut dimx) = (0, 0);
    unsafe {
        ffi::ncplane_dim_yx(plane, &mut dimy, &mut dimx);
    }
    let mut ul = cell_trivial_initializer![];
    let mut ur = cell_trivial_initializer![];
    let mut ll = cell_trivial_initializer![];
    let mut lr = cell_trivial_initializer![];
    let mut hl = cell_trivial_initializer![];
    let mut vl = cell_trivial_initializer![];
    if unsafe {
        ffi::cells_double_box(
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
    } != 0
    {
        return -1;
    }
    let ret = ncplane_box_sized(plane, &ul, &ur, &ll, &lr, &hl, &vl, dimy, dimx, ctlword);
    unsafe {
        ffi::cell_release(plane, &mut ul);
        ffi::cell_release(plane, &mut ur);
        ffi::cell_release(plane, &mut ll);
        ffi::cell_release(plane, &mut lr);
        ffi::cell_release(plane, &mut hl);
        ffi::cell_release(plane, &mut vl);
    }
    ret
}

// TODO: TEST!
#[inline]
pub fn ncplane_perimeter_rounded(
    plane: &mut ncplane,
    stylemask: StyleMask,
    channels: ChannelPair,
    ctlword: u32,
) -> IntResult {
    if unsafe { ffi::ncplane_cursor_move_yx(plane, 0, 0) } != 0 {
        return -1;
    }
    let (mut dimy, mut dimx) = (0, 0);
    unsafe {
        ffi::ncplane_dim_yx(plane, &mut dimy, &mut dimx);
    }
    let mut ul = cell_trivial_initializer![];
    let mut ur = cell_trivial_initializer![];
    let mut ll = cell_trivial_initializer![];
    let mut lr = cell_trivial_initializer![];
    let mut hl = cell_trivial_initializer![];
    let mut vl = cell_trivial_initializer![];
    if unsafe {
        ffi::cells_rounded_box(
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
    } != 0
    {
        return -1;
    }
    let ret = ncplane_box_sized(plane, &ul, &ur, &ll, &lr, &hl, &vl, dimy, dimx, ctlword);
    unsafe {
        ffi::cell_release(plane, &mut ul);
        ffi::cell_release(plane, &mut ur);
        ffi::cell_release(plane, &mut ll);
        ffi::cell_release(plane, &mut lr);
        ffi::cell_release(plane, &mut hl);
        ffi::cell_release(plane, &mut vl);
    }
    ret
}

/// Call ncplane_putc_yx() for the current cursor location.
// TODO: TEST
#[inline]
pub fn ncplane_putc(plane: &mut ncplane, cell: &cell) -> IntResult {
    unsafe { ffi::ncplane_putc_yx(plane, -1, -1, cell) }
}

/// Call ncplane_putsimple_yx() at the current cursor location.
// TODO: TEST
#[inline]
pub fn ncplane_putsimple(plane: &mut ncplane, ch: char) -> IntResult {
    ffi::ncplane_putsimple_yx(plane, -1, -1, ch)
}

/// Call ncplane_putegc() at the current cursor location.
// TODO: TEST
#[inline]
pub fn ncplane_putegc(plane: &mut ncplane, gcluster: i8, sbytes: &mut i32) -> IntResult {
    unsafe { ffi::ncplane_putegc_yx(plane, -1, -1, &gcluster, sbytes) }
}

/// Replace the EGC underneath us, but retain the styling. The current styling
/// of the plane will not be changed.
///
/// Replace the cell at the specified coordinates with the provided 7-bit char
/// 'ch'. Advance the cursor by 1. On success, returns 1. On failure, returns -1.
/// This works whether the underlying char is signed or unsigned.
// TODO: TEST
#[inline]
pub fn ncplane_putsimple_yx(plane: &mut ncplane, y: i32, x: i32, ch: char) -> IntResult {
    let newcell = cell_initializer![ch, unsafe { ffi::ncplane_attr(plane) }, unsafe {
        ffi::ncplane_channels(plane)
    }];
    unsafe { ffi::ncplane_putc_yx(plane, y, x, &newcell) }
}

///
// TODO: TEST
#[inline]
pub fn ncplane_putstr(plane: &mut ncplane, gclustarr: &[u8]) -> IntResult {
    unsafe {
        ffi::ncplane_putstr_yx(
            plane,
            -1,
            -1,
            CString::new(gclustarr).expect("Bad string").as_ptr(),
        )
    }
}

///
// TODO: TEST
#[inline]
pub fn ncplane_putnstr(plane: &mut ncplane, size: ffi::size_t, gclustarr: &[u8]) -> IntResult {
    unsafe {
        ffi::ncplane_putnstr_yx(
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
// TODO: TEST
#[inline]
pub fn ncplane_resize_simple(plane: &mut ncplane, ylen: i32, xlen: i32) -> IntResult {
    let (mut oldy, mut oldx) = (0, 0);
    unsafe {
        ffi::ncplane_dim_yx(plane, &mut oldy, &mut oldx);
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
    unsafe { ffi::ncplane_resize(plane, 0, 0, keepleny, keeplenx, 0, 0, ylen, xlen) }
}

///
/// On error, return the negative number of cells drawn.
// TODO: TEST
#[inline]
pub fn ncplane_vline(plane: &mut ncplane, cell: &cell, len: i32) -> i32 {
    unsafe { ffi::ncplane_vline_interp(plane, cell, len, cell.channels, cell.channels) }
}

// TODO: TEST
#[inline]
pub fn ncplane_vprintf(
    plane: &mut ncplane,
    format: &str,
    ap: &mut ffi::__va_list_tag,
) -> IntResult {
    unsafe {
        ffi::ncplane_vprintf_yx(
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
// TODO: TEST
// XXX receive cells as u32? https://github.com/dankamongmen/notcurses/issues/920
#[inline]
pub fn ncplane_gradient_sized(
    plane: &mut ncplane,
    egc: &[u8],
    stylemask: StyleMask,
    ul: u64,
    ur: u64,
    ll: u64,
    lr: u64,
    ylen: i32,
    xlen: i32,
) -> IntResult {
    if ylen < 1 || xlen < 1 {
        return -1;
    }
    let (mut y, mut x) = (0, 0);
    unsafe {
        ffi::ncplane_cursor_yx(plane, &mut y, &mut x);
        ffi::ncplane_gradient(
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

// TODO: TEST
// XXX receive cells as u32? https://github.com/dankamongmen/notcurses/issues/920
#[inline]
pub fn ncplane_highgradient_sized(
    plane: &mut ncplane,
    ul: u64,
    ur: u64,
    ll: u64,
    lr: u64,
    ylen: i32,
    xlen: i32,
) -> IntResult {
    if ylen < 1 || xlen < 1 {
        return -1;
    }
    let (mut y, mut x) = (0, 0);
    if unsafe { !ffi::notcurses_canutf8(ffi::ncplane_notcurses_const(plane)) } {
        return ncplane_gradient_sized(plane, b" ", 0, ul, ur, ll, lr, ylen, xlen);
    }
    unsafe {
        ffi::ncplane_cursor_yx(plane, &mut y, &mut x);
        ffi::ncplane_highgradient(
            plane,
            ul.try_into().unwrap(),
            ur.try_into().unwrap(),
            ll.try_into().unwrap(),
            lr.try_into().unwrap(),
            y + ylen - 1,
            x + xlen - 1,
        )
    }
}

/// Extract the 32-bit working foreground channel from an ncplane.
// TODO: TEST
#[inline]
pub fn ncplane_fchannel(plane: &ncplane) -> Channel {
    ffi::channels_fchannel(unsafe { ffi::ncplane_channels(plane) })
}

/// Extract the 32-bit working background channel from an ncplane.
// TODO: TEST
#[inline]
pub fn ncplane_bchannel(plane: &ncplane) -> Channel {
    ffi::channels_bchannel(unsafe { ffi::ncplane_channels(plane) })
}

/// Extract 24 bits of working foreground RGB from an ncplane, shifted to LSBs.
// TODO: TEST
#[inline]
pub fn ncplane_fg(plane: &ncplane) -> Channel {
    ffi::channels_fg(unsafe { ffi::ncplane_channels(plane) })
}

/// Extract 24 bits of working background RGB from an ncplane, shifted to LSBs.
// TODO: TEST
#[inline]
pub fn ncplane_bg(plane: &ncplane) -> Channel {
    ffi::channels_bg(unsafe { ffi::ncplane_channels(plane) })
}

/// Extract 2 bits of foreground alpha from 'struct ncplane', shifted to LSBs.
// TODO: TEST
#[inline]
pub fn ncplane_fg_alpha(plane: &ncplane) -> AlphaBits {
    ffi::channels_fg_alpha(unsafe { ffi::ncplane_channels(plane) })
}

/// Extract 2 bits of background alpha from 'struct ncplane', shifted to LSBs.
// TODO: TEST
#[inline]
pub fn ncplane_bg_alpha(plane: &ncplane) -> AlphaBits {
    ffi::channels_bg_alpha(unsafe { ffi::ncplane_channels(plane) })
}

/// Is the plane's foreground using the "default foreground color"?
// TODO: TEST
#[inline]
pub fn ncplane_fg_default_p(plane: &ncplane) -> bool {
    ffi::channels_fg_default_p(unsafe { ffi::ncplane_channels(plane) })
}

/// Is the plane's background using the "default background color"?
// TODO: TEST
#[inline]
pub fn ncplane_bg_default_p(plane: &ncplane) -> bool {
    ffi::channels_bg_default_p(unsafe { ffi::ncplane_channels(plane) })
}

/// Extract 24 bits of foreground RGB from a plane, split into components.
// TODO: TEST
#[inline]
pub fn ncplane_fg_rgb(
    plane: &ncplane,
    red: &mut Color,
    green: &mut Color,
    blue: &mut Color,
) -> Channel {
    ffi::channels_fg_rgb(unsafe { ffi::ncplane_channels(plane) }, red, green, blue)
}

/// Extract 24 bits of background RGB from a plane, split into components.
// TODO: TEST
#[inline]
pub fn ncplane_bg_rgb(
    plane: &ncplane,
    red: &mut Color,
    green: &mut Color,
    blue: &mut Color,
) -> Channel {
    ffi::channels_bg_rgb(unsafe { ffi::ncplane_channels(plane) }, red, green, blue)
}

// TODO: TEST
#[inline]
pub fn ncplane_rounded_box(
    plane: &mut ncplane,
    stylemask: StyleMask,
    channels: ChannelPair,
    ystop: i32,
    xstop: i32,
    ctlword: u32,
) -> IntResult {
    #[allow(unused_assignments)]
    let mut ret = 0;

    let mut ul = cell_trivial_initializer![];
    let mut ur = cell_trivial_initializer![];
    let mut ll = cell_trivial_initializer![];
    let mut lr = cell_trivial_initializer![];
    let mut hl = cell_trivial_initializer![];
    let mut vl = cell_trivial_initializer![];

    unsafe {
        ret = ffi::cells_rounded_box(
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
        if ret == 0 {
            ret = ffi::ncplane_box(plane, &ul, &ur, &ll, &lr, &hl, &vl, ystop, xstop, ctlword);
        }

        ffi::cell_release(plane, &mut ul);
        ffi::cell_release(plane, &mut ur);
        ffi::cell_release(plane, &mut ll);
        ffi::cell_release(plane, &mut lr);
        ffi::cell_release(plane, &mut hl);
        ffi::cell_release(plane, &mut vl);
    }
    ret
}

// TODO: TEST
#[inline]
pub fn ncplane_rounded_box_sized(
    plane: &mut ncplane,
    stylemask: StyleMask,
    channels: ChannelPair,
    ylen: i32,
    xlen: i32,
    ctlword: u32,
) -> IntResult {
    let (mut y, mut x) = (0, 0);
    unsafe {
        ffi::ncplane_cursor_yx(plane, &mut y, &mut x);
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
// ncplane_printf_stainable(struct ncplane* n, const char* format, ...)
//   __attribute__ ((format (printf, 2, 3)));

// static inline int
// ncplane_printf_stainable(struct ncplane* n, const char* format, ...){
//   va_list va;
//   va_start(va, format);
//   int ret = ncplane_vprintf_stainable(n, format, va);
//   va_end(va);
//   return ret;
// }

#[cfg(test)]
mod test {
    // use super::ffi;
    // use serial_test::serial;
    /*
    #[test]
    #[serial]
    fn () {
    }
    */
}
