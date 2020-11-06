// functions already exported by bindgen : 97
// ------------------------------------------
// ncplane_above
// ncplane_at_cursor
// ncplane_at_yx
// ncplane_base
// ncplane_below
// ncplane_box
// ncplane_center_abs
// ncplane_channels
// ncplane_contents
// ncplane_create
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
// ncplane_highgradient_sized
// ncplane_hline_interp
// ncplane_home
// ncplane_mergedown
// ncplane_mergedown_simple
// ncplane_move_above
// ncplane_move_below
// ncplane_move_bottom
// ncplane_move_top
// ncplane_move_yx
// ncplane_new
// ncplane_notcurses
// ncplane_notcurses_const
// ncplane_off_styles
// ncplane_on_styles
// ncplane_parent
// ncplane_parent_const
// ncplane_polyfill_yx
// ncplane_pulse
// ncplane_putchar_stained
// ncplane_putc_yx
// ncplane_putegc_stained
// ncplane_putegc_yx
// ncplane_putnstr_aligned
// ncplane_putnstr_yx
// ncplane_putstr_aligned
// ncplane_putstr_stained
// ncplane_putstr_yx
// ncplane_puttext
// ncplane_putwegc_stained
// ncplane_putwstr_stained
// ncplane_qrcode
// ncplane_reparent
// ncplane_resize
// ncplane_resize_realign
// ncplane_rgba
// ncplane_rotate_ccw
// ncplane_rotate_cw
// ncplane_set_base
// ncplane_set_base_cell
// ncplane_set_bchannel
// ncplane_set_bg_alpha
// ncplane_set_bg_default
// ncplane_set_bg_palindex
// ncplane_set_bg_rgb
// ncplane_set_bg_rgb8
// ncplane_set_bg_rgb8_clipped
// ncplane_set_channels
// ncplane_set_fchannel
// ncplane_set_fg_alpha
// ncplane_set_fg_default
// ncplane_set_fg_palindex
// ncplane_set_fg_rgb
// ncplane_set_fg_rgb8
// ncplane_set_fg_rgb8_clipped
// ncplane_set_scrolling
// ncplane_set_styles
// ncplane_set_userptr
// ncplane_stain
// ncplane_styles
// ncplane_styles_off
// ncplane_styles_on
// ncplane_styles_set
// ncplane_translate
// ncplane_translate_abs
// ncplane_userptr
// ncplane_vline_interp
// ncplane_vprintf_aligned
// ncplane_vprintf_stained
// ncplane_vprintf_yx
// ncplane_x
// ncplane_y
// ncplane_yx
//
// static inline functions total: 42
// ------------------------------------------ (implement / remaining)
// (X) wont:  8
// (+) done: 32 / 2
// (#) test:  0 / 34
// ------------------------------------------
//+ ncplane_align
//+ ncplane_at_cursor_cell
//+ ncplane_at_yx_cell
//+ ncplane_bchannel
//+ ncplane_bg_alpha
//+ ncplane_bg_default_p
//+ ncplane_bg_rgb
//+ ncplane_bg_rgb8
//+ ncplane_box_sized
//+ ncplane_dim_x
//+ ncplane_dim_y
//+ ncplane_double_box
//+ ncplane_double_box_sized
//+ ncplane_fchannel
//+ ncplane_fg_alpha
//+ ncplane_fg_default_p
//+ ncplane_fg_rgb
//+ ncplane_fg_rgb8
//+ ncplane_gradient_sized       // u64|u32 https://github.com/dankamongmen/notcurses/issues/920
//+ ncplane_hline
//+ ncplane_perimeter
//+ ncplane_perimeter_double
//+ ncplane_perimeter_rounded
//+ ncplane_putc
//  ncplane_putchar
//  ncplane_putchar_yx
//+ ncplane_putegc
//+ ncplane_putnstr
//+ ncplane_putstr
//X ncplane_putwc                // I don't think these will be needed from Rust. See:
//X ncplane_putwc_stained
//X ncplane_putwc_yx             // https://locka99.gitbooks.io/a-guide-to-porting-c-to-rust/content/features_of_rust/strings.html
//X ncplane_putwegc              //
//X ncplane_putwegc_yx           //
//X ncplane_putwstr              //
//X ncplane_putwstr_aligned      //
//X ncplane_putwstr_yx           //
//+ ncplane_resize_simple
//+ ncplane_rounded_box
//+ ncplane_rounded_box_sized
//+ ncplane_vline
//+ ncplane_vprintf
//
// NOTE: TODO: Still remains all the ncplane_printf* functions/macros (at the end)

use core::ffi::c_void;
use core::ptr::null_mut;

use cstr_core::CString;
use libc::free;

use crate as nc;
use nc::types::{
    AlphaBits, Cell, Channel, ChannelPair, Color, EGCBackstop, IntResult, NcAlign, NcPlane,
    StyleMask,
};

/// Return the column at which 'cols' columns ought start in order to be aligned
/// according to 'align' within ncplane 'n'. Returns INT_MAX on invalid 'align'.
/// Undefined behavior on negative 'cols'.
//
// NOTE: [leave cols as i32](https://github.com/dankamongmen/notcurses/issues/904)
#[inline]
pub fn ncplane_align(plane: &NcPlane, align: NcAlign, cols: i32) -> i32 {
    nc::notcurses_align(nc::ncplane_dim_x(plane), align, cols)
}

/// Retrieve the current contents of the cell under the cursor into 'cell'.
/// This cell is invalidated if the associated plane is destroyed.
#[inline]
pub fn nplane_at_cursor_cell(plane: &mut NcPlane, cell: &mut Cell) -> IntResult {
    let mut egc = unsafe { nc::ncplane_at_cursor(plane, &mut cell.stylemask, &mut cell.channels) };
    if egc.is_null() {
        return -1;
    }
    let result: IntResult = unsafe { nc::cell_load(plane, cell, egc) };
    if result < 0 {
        unsafe {
            free(&mut egc as *mut _ as *mut c_void);
        }
    }
    result
}

/// Retrieve the current contents of the specified cell into 'cell'.
/// This cell is invalidated if the associated plane is destroyed.
#[inline]
pub fn ncplane_at_yx_cell(plane: &mut NcPlane, y: i32, x: i32, cell: &mut Cell) -> IntResult {
    let mut egc =
        unsafe { nc::ncplane_at_yx(plane, y, x, &mut cell.stylemask, &mut cell.channels) };
    if egc.is_null() {
        return -1;
    }
    let channels = cell.channels; // need to preserve wide flag
    let result: IntResult = unsafe { nc::cell_load(plane, cell, egc) };
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
    ul: &Cell,
    ur: &Cell,
    ll: &Cell,
    lr: &Cell,
    hline: &Cell,
    vline: &Cell,
    ylen: i32,
    xlen: i32,
    ctlword: u32,
) -> IntResult {
    let (mut y, mut x) = (0, 0);
    unsafe {
        nc::ncplane_cursor_yx(plane, &mut y, &mut x);
        nc::ncplane_box(
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
        nc::ncplane_dim_yx(plane, null_mut(), &mut x);
        x
    }
}

///
#[inline]
pub fn ncplane_dim_y(plane: &NcPlane) -> i32 {
    unsafe {
        let mut y = 0;
        nc::ncplane_dim_yx(plane, &mut y, null_mut());
        y
    }
}

///
#[inline]
pub fn ncplane_double_box(
    plane: &mut NcPlane,
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
        ret = nc::cells_double_box(
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
            ret = nc::ncplane_box(plane, &ul, &ur, &ll, &lr, &hl, &vl, ystop, xstop, ctlword);
        }

        nc::cell_release(plane, &mut ul);
        nc::cell_release(plane, &mut ur);
        nc::cell_release(plane, &mut ll);
        nc::cell_release(plane, &mut lr);
        nc::cell_release(plane, &mut hl);
        nc::cell_release(plane, &mut vl);
    }
    ret
}

///
#[inline]
pub fn ncplane_double_box_sized(
    plane: &mut NcPlane,
    stylemask: StyleMask,
    channels: ChannelPair,
    ylen: i32,
    xlen: i32,
    ctlword: u32,
) -> IntResult {
    let (mut y, mut x) = (0, 0);
    unsafe {
        nc::ncplane_cursor_yx(plane, &mut y, &mut x);
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
pub fn ncplane_hline(plane: &mut NcPlane, cell: &Cell, len: i32) -> i32 {
    unsafe { nc::ncplane_hline_interp(plane, cell, len, cell.channels, cell.channels) }
}

///
#[inline]
pub fn ncplane_perimeter(
    plane: &mut NcPlane,
    ul: &Cell,
    ur: &Cell,
    ll: &Cell,
    lr: &Cell,
    hline: &Cell,
    vline: &Cell,
    ctlword: u32,
) -> IntResult {
    unsafe {
        nc::ncplane_cursor_move_yx(plane, 0, 0);
        let (mut dimy, mut dimx) = (0, 0);
        nc::ncplane_dim_yx(plane, &mut dimy, &mut dimx);
        ncplane_box_sized(plane, ul, ur, ll, lr, hline, vline, dimy, dimx, ctlword)
    }
}

///
#[inline]
pub fn ncplane_perimeter_double(
    plane: &mut NcPlane,
    stylemask: StyleMask,
    channels: ChannelPair,
    ctlword: u32,
) -> IntResult {
    if unsafe { nc::ncplane_cursor_move_yx(plane, 0, 0) } != 0 {
        return -1;
    }
    let (mut dimy, mut dimx) = (0, 0);
    unsafe {
        nc::ncplane_dim_yx(plane, &mut dimy, &mut dimx);
    }
    let mut ul = cell_trivial_initializer![];
    let mut ur = cell_trivial_initializer![];
    let mut ll = cell_trivial_initializer![];
    let mut lr = cell_trivial_initializer![];
    let mut hl = cell_trivial_initializer![];
    let mut vl = cell_trivial_initializer![];
    if unsafe {
        nc::cells_double_box(
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
        nc::cell_release(plane, &mut ul);
        nc::cell_release(plane, &mut ur);
        nc::cell_release(plane, &mut ll);
        nc::cell_release(plane, &mut lr);
        nc::cell_release(plane, &mut hl);
        nc::cell_release(plane, &mut vl);
    }
    ret
}

///
#[inline]
pub fn ncplane_perimeter_rounded(
    plane: &mut NcPlane,
    stylemask: StyleMask,
    channels: ChannelPair,
    ctlword: u32,
) -> IntResult {
    if unsafe { nc::ncplane_cursor_move_yx(plane, 0, 0) } != 0 {
        return -1;
    }
    let (mut dimy, mut dimx) = (0, 0);
    unsafe {
        nc::ncplane_dim_yx(plane, &mut dimy, &mut dimx);
    }
    let mut ul = cell_trivial_initializer![];
    let mut ur = cell_trivial_initializer![];
    let mut ll = cell_trivial_initializer![];
    let mut lr = cell_trivial_initializer![];
    let mut hl = cell_trivial_initializer![];
    let mut vl = cell_trivial_initializer![];
    if unsafe {
        nc::cells_rounded_box(
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
        nc::cell_release(plane, &mut ul);
        nc::cell_release(plane, &mut ur);
        nc::cell_release(plane, &mut ll);
        nc::cell_release(plane, &mut lr);
        nc::cell_release(plane, &mut hl);
        nc::cell_release(plane, &mut vl);
    }
    ret
}

/// Call ncplane_putc_yx() for the current cursor location.
#[inline]
pub fn ncplane_putc(plane: &mut NcPlane, cell: &Cell) -> IntResult {
    unsafe { nc::ncplane_putc_yx(plane, -1, -1, cell) }
}

/// Call ncplane_putegc() at the current cursor location.
#[inline]
pub fn ncplane_putegc(plane: &mut NcPlane, gcluster: i8, sbytes: &mut i32) -> IntResult {
    unsafe { nc::ncplane_putegc_yx(plane, -1, -1, &gcluster, sbytes) }
}

///
#[inline]
pub fn ncplane_putstr(plane: &mut NcPlane, gclustarr: &[u8]) -> IntResult {
    unsafe {
        nc::ncplane_putstr_yx(
            plane,
            -1,
            -1,
            CString::new(gclustarr).expect("Bad string").as_ptr(),
        )
    }
}

///
#[inline]
pub fn ncplane_putnstr(plane: &mut NcPlane, size: u64, gclustarr: &[u8]) -> IntResult {
    unsafe {
        nc::ncplane_putnstr_yx(
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
pub fn ncplane_resize_simple(plane: &mut NcPlane, ylen: i32, xlen: i32) -> IntResult {
    let (mut oldy, mut oldx) = (0, 0);
    unsafe {
        nc::ncplane_dim_yx(plane, &mut oldy, &mut oldx);
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
    unsafe { nc::ncplane_resize(plane, 0, 0, keepleny, keeplenx, 0, 0, ylen, xlen) }
}

///
/// On error, return the negative number of cells drawn.
#[inline]
pub fn ncplane_vline(plane: &mut NcPlane, cell: &Cell, len: i32) -> i32 {
    unsafe { nc::ncplane_vline_interp(plane, cell, len, cell.channels, cell.channels) }
}

///
#[inline]
pub fn ncplane_vprintf(plane: &mut NcPlane, format: &str, ap: &mut nc::__va_list_tag) -> IntResult {
    unsafe {
        nc::ncplane_vprintf_yx(
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
        nc::ncplane_cursor_yx(plane, &mut y, &mut x);
        nc::ncplane_gradient(
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
pub fn ncplane_fchannel(plane: &NcPlane) -> Channel {
    nc::channels_fchannel(unsafe { nc::ncplane_channels(plane) })
}

/// Extract the 32-bit working background channel from an ncplane.
#[inline]
pub fn ncplane_bchannel(plane: &NcPlane) -> Channel {
    nc::channels_bchannel(unsafe { nc::ncplane_channels(plane) })
}

/// Extract 24 bits of working foreground RGB from an ncplane, shifted to LSBs.
#[inline]
pub fn ncplane_fg_rgb(plane: &NcPlane) -> Channel {
    nc::channels_fg_rgb(unsafe { nc::ncplane_channels(plane) })
}

/// Extract 24 bits of working background RGB from an ncplane, shifted to LSBs.
#[inline]
pub fn ncplane_bg_rgb(plane: &NcPlane) -> Channel {
    nc::channels_bg_rgb(unsafe { nc::ncplane_channels(plane) })
}

/// Extract 2 bits of foreground alpha from 'struct ncplane', shifted to LSBs.
#[inline]
pub fn ncplane_fg_alpha(plane: &NcPlane) -> AlphaBits {
    nc::channels_fg_alpha(unsafe { nc::ncplane_channels(plane) })
}

/// Extract 2 bits of background alpha from 'struct ncplane', shifted to LSBs.
#[inline]
pub fn ncplane_bg_alpha(plane: &NcPlane) -> AlphaBits {
    nc::channels_bg_alpha(unsafe { nc::ncplane_channels(plane) })
}

/// Is the plane's foreground using the "default foreground color"?
#[inline]
pub fn ncplane_fg_default_p(plane: &NcPlane) -> bool {
    nc::channels_fg_default_p(unsafe { nc::ncplane_channels(plane) })
}

/// Is the plane's background using the "default background color"?
#[inline]
pub fn ncplane_bg_default_p(plane: &NcPlane) -> bool {
    nc::channels_bg_default_p(unsafe { nc::ncplane_channels(plane) })
}

/// Extract 24 bits of foreground RGB from a plane, split into components.
#[inline]
pub fn ncplane_fg_rgb8(
    plane: &NcPlane,
    red: &mut Color,
    green: &mut Color,
    blue: &mut Color,
) -> Channel {
    nc::channels_fg_rgb8(unsafe { nc::ncplane_channels(plane) }, red, green, blue)
}

/// Extract 24 bits of background RGB from a plane, split into components.
#[inline]
pub fn ncplane_bg_rgb8(
    plane: &NcPlane,
    red: &mut Color,
    green: &mut Color,
    blue: &mut Color,
) -> Channel {
    nc::channels_bg_rgb8(unsafe { nc::ncplane_channels(plane) }, red, green, blue)
}

///
#[inline]
pub fn ncplane_rounded_box(
    plane: &mut NcPlane,
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
        ret = nc::cells_rounded_box(
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
            ret = nc::ncplane_box(plane, &ul, &ur, &ll, &lr, &hl, &vl, ystop, xstop, ctlword);
        }

        nc::cell_release(plane, &mut ul);
        nc::cell_release(plane, &mut ur);
        nc::cell_release(plane, &mut ll);
        nc::cell_release(plane, &mut lr);
        nc::cell_release(plane, &mut hl);
        nc::cell_release(plane, &mut vl);
    }
    ret
}

///
#[inline]
pub fn ncplane_rounded_box_sized(
    plane: &mut NcPlane,
    stylemask: StyleMask,
    channels: ChannelPair,
    ylen: i32,
    xlen: i32,
    ctlword: u32,
) -> IntResult {
    let (mut y, mut x) = (0, 0);
    unsafe {
        nc::ncplane_cursor_yx(plane, &mut y, &mut x);
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

#[cfg(test)]
mod test {
    // use super::nc;
    // use serial_test::serial;
    /*
    #[test]
    #[serial]
    fn () {
    }
    */
}
