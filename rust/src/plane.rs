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
// (+) implement: 24 / … / 18
// (#) unit test:  0 / … / 42
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
// ncplane_double_box
// ncplane_double_box_sized
//+ncplane_fchannel
//+ncplane_fg
//+ncplane_fg_alpha
//+ncplane_fg_default_p
//+ncplane_fg_rgb
// ncplane_gradient_sized
// ncplane_highgradient_sized
//+ncplane_hline
//+ncplane_perimeter
// ncplane_perimeter_double
//+ncplane_perimeter_rounded
// ncplane_putc
//+ncplane_putegc
// ncplane_putnstr
//+ncplane_putsimple
//+ncplane_putsimple_yx
//+ncplane_putstr
// ncplane_putwc
// ncplane_putwc_yx
// ncplane_putwegc
// ncplane_putwegc_yx
// ncplane_putwstr
// ncplane_putwstr_aligned
// ncplane_putwstr_yx
// ncplane_resize_simple
// ncplane_rounded_box
// ncplane_rounded_box_sized
//+ncplane_vline
// ncplane_vprintf

use core::ffi::c_void;
use core::ptr::null_mut;
use cstr_core::CString;

use crate as ffi;
use ffi::types::{
    AlphaBits, Channel, ChannelPair, Color, EGC, EGCBackstop, IntResult,
    StyleMask,
};
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
    ctrlword: u32,
) -> IntResult {

    let (mut y, mut x) = (0, 0);
    unsafe {
        ffi::ncplane_cursor_yx(plane, &mut y, &mut x);
        ffi::ncplane_box(plane, ul, ur, ll, lr, hline, vline, y + ylen - 1, x + xlen - 1, ctrlword,)
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

///
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

// static inline int
// ncplane_perimeter_double(struct ncplane* n, uint32_t stylemask,
//                          uint64_t channels, unsigned ctlword){
//   if(ncplane_cursor_move_yx(n, 0, 0)){
//     return -1;
//   }
//   int dimy, dimx;
//   ncplane_dim_yx(n, &dimy, &dimx);
//   cell ul = CELL_TRIVIAL_INITIALIZER;
//   cell ur = CELL_TRIVIAL_INITIALIZER;
//   cell ll = CELL_TRIVIAL_INITIALIZER;
//   cell lr = CELL_TRIVIAL_INITIALIZER;
//   cell vl = CELL_TRIVIAL_INITIALIZER;
//   cell hl = CELL_TRIVIAL_INITIALIZER;
//   if(cells_double_box(n, stylemask, channels, &ul, &ur, &ll, &lr, &hl, &vl)){
//     return -1;
//   }
//   int r = ncplane_box_sized(n, &ul, &ur, &ll, &lr, &hl, &vl, dimy, dimx, ctlword);
//   cell_release(n, &ul); cell_release(n, &ur);
//   cell_release(n, &ll); cell_release(n, &lr);
//   cell_release(n, &hl); cell_release(n, &vl);
//   return r;
// }

// TODO: TEST!
#[inline]
pub fn ncplane_perimeter_rounded(
    plane: &mut ncplane,
    stylemask: StyleMask,
    channels: ChannelPair,
    ctrlword: u32,
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
        ffi::cells_rounded_box(plane, stylemask as u32, channels,
        &mut ul, &mut ur, &mut ll, &mut lr, &mut hl, &mut vl) } != 0 {
        return -1;
    }
    let ret = ncplane_box_sized(plane, &ul, &ur, &ll, &lr, &hl, &vl, dimy, dimx, ctrlword);
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
pub fn ncplane_putsimple(plane: &mut ncplane, char: i8) -> IntResult {
    ffi::ncplane_putsimple_yx(plane, -1, -1, char)
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
/// 'c'. Advance the cursor by 1. On success, returns 1. On failure, returns -1.
/// This works whether the underlying char is signed or unsigned.
// TODO: TEST
#[inline]
pub fn ncplane_putsimple_yx(plane: &mut ncplane, y: i32, x: i32, char: i8) -> IntResult {
    let newcell = cell_initializer![char, unsafe { ffi::ncplane_attr(plane) }, unsafe {
        ffi::ncplane_channels(plane)
    }];
    unsafe { ffi::ncplane_putc_yx(plane, y, x, &newcell) }
}

///
// TODO: TEST
#[inline]
pub fn ncplane_putstr(plane: &mut ncplane, _str: &str) -> i32 {
    unsafe {
        ffi::ncplane_putstr_yx(
            plane,
            -1,
            -1,
            CString::new(_str).expect("Bad string").as_ptr(),
        )
    }
}

// static inline int
// ncplane_putstr(struct ncplane* n, const char* gclustarr){
//   return ncplane_putstr_yx(n, -1, -1, gclustarr);
// }
//
// static inline int
// ncplane_putnstr(struct ncplane* n, size_t s, const char* gclustarr){
//   return ncplane_putnstr_yx(n, -1, -1, s, gclustarr);
// }

// // ncplane_putstr(), but following a conversion from wchar_t to UTF-8 multibyte.
// static inline int
// ncplane_putwstr_yx(struct ncplane* n, int y, int x, const wchar_t* gclustarr){
//   // maximum of six UTF8-encoded bytes per wchar_t
//   const size_t mbytes = (wcslen(gclustarr) * WCHAR_MAX_UTF8BYTES) + 1;
//   char* mbstr = (char*)malloc(mbytes); // need cast for c++ callers
//   if(mbstr == NULL){
//     return -1;
//   }
//   size_t s = wcstombs(mbstr, gclustarr, mbytes);
//   if(s == (size_t)-1){
//     free(mbstr);
//     return -1;
//   }
//   int ret = ncplane_putstr_yx(n, y, x, mbstr);
//   free(mbstr);
//   return ret;
// }

// // ncplane_putegc(), but following a conversion from wchar_t to UTF-8 multibyte.
// static inline int
// ncplane_putwegc(struct ncplane* n, const wchar_t* gclust, int* sbytes){
//   // maximum of six UTF8-encoded bytes per wchar_t
//   const size_t mbytes = (wcslen(gclust) * WCHAR_MAX_UTF8BYTES) + 1;
//   char* mbstr = (char*)malloc(mbytes); // need cast for c++ callers
//   if(mbstr == NULL){
//     return -1;
//   }
//   size_t s = wcstombs(mbstr, gclust, mbytes);
//   if(s == (size_t)-1){
//     free(mbstr);
//     return -1;
//   }
//   int ret = ncplane_putegc(n, mbstr, sbytes);
//   free(mbstr);
//   return ret;
// }

// Call ncplane_putwegc() after successfully moving to y, x.
// static inline int
// ncplane_putwegc_yx(struct ncplane* n, int y, int x, const wchar_t* gclust,
//                    int* sbytes){
//   if(ncplane_cursor_move_yx(n, y, x)){
//     return -1;
//   }
//   return ncplane_putwegc(n, gclust, sbytes);
// }

// static inline int
// ncplane_putwstr_aligned(struct ncplane* n, int y, ncalign_e align,
//                         const wchar_t* gclustarr){
//   int width = wcswidth(gclustarr, INT_MAX);
//   int xpos = ncplane_align(n, align, width);
//   return ncplane_putwstr_yx(n, y, xpos, gclustarr);
// }

// static inline int
// ncplane_putwstr(struct ncplane* n, const wchar_t* gclustarr){
//   return ncplane_putwstr_yx(n, -1, -1, gclustarr);
// }

// Replace the cell at the specified coordinates with the provided wide char
// 'w'. Advance the cursor by the character's width as reported by wcwidth().
// On success, returns 1. On failure, returns -1.
// static inline int
// ncplane_putwc_yx(struct ncplane* n, int y, int x, wchar_t w){
//   wchar_t warr[2] = { w, L'\0' };
//   return ncplane_putwstr_yx(n, y, x, warr);
// }

// Call ncplane_putwc() at the current cursor position.
// static inline int
// ncplane_putwc(struct ncplane* n, wchar_t w){
//   return ncplane_putwc_yx(n, -1, -1, w);
// }

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

// // Resize the plane, retaining what data we can (everything, unless we're
// // shrinking in some dimension). Keep the origin where it is.
// static inline int
// ncplane_resize_simple(struct ncplane* n, int ylen, int xlen){
//   int oldy, oldx;
//   ncplane_dim_yx(n, &oldy, &oldx); // current dimensions of 'n'
//   int keepleny = oldy > ylen ? ylen : oldy;
//   int keeplenx = oldx > xlen ? xlen : oldx;
//   return ncplane_resize(n, 0, 0, keepleny, keeplenx, 0, 0, ylen, xlen);
// }

///
/// On error, return the negative number of cells drawn.
// TODO: TEST
#[inline]
pub fn ncplane_vline(plane: &mut ncplane, cell: &cell, len: i32) -> i32 {
    unsafe { ffi::ncplane_vline_interp(plane, cell, len, cell.channels, cell.channels) }
}

// static inline int
// ncplane_vprintf(struct ncplane* n, const char* format, va_list ap){
//   return ncplane_vprintf_yx(n, -1, -1, format, ap);
// }

// // Draw a gradient with its upper-left corner at the current cursor position,
// // having dimensions 'ylen'x'xlen'. See ncplane_gradient for more information.
// static inline int
// ncplane_gradient_sized(struct ncplane* n, const char* egc, uint32_t attrword,
//                        uint64_t ul, uint64_t ur, uint64_t ll, uint64_t lr,
//                        int ylen, int xlen){
//   if(ylen < 1 || xlen < 1){
//     return -1;
//   }
//   int y, x;
//   ncplane_cursor_yx(n, &y, &x);
//   return ncplane_gradient(n, egc, attrword, ul, ur, ll, lr, y + ylen - 1, x + xlen - 1);
// }

// static inline int
// ncplane_highgradient_sized(struct ncplane* n, uint32_t ul, uint32_t ur,
//                            uint32_t ll, uint32_t lr, int ylen, int xlen){
//   if(ylen < 1 || xlen < 1){
//     return -1;
//   }
//   int y, x;
//   if(!notcurses_canutf8(ncplane_notcurses_const(n))){
//     // this works because the uin32_ts we pass in will be promoted to uint64_ts
//     // via extension, and the space will employ the background. mwahh!
//     return ncplane_gradient_sized(n, " ", 0, ul, ur, ll, lr, ylen, xlen);
//   }
//   ncplane_cursor_yx(n, &y, &x);
//   return ncplane_highgradient(n, ul, ur, ll, lr, y + ylen - 1, x + xlen - 1);
// }

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

// static inline int
// ncplane_rounded_box(struct ncplane* n, uint32_t attr, uint64_t channels,
//                     int ystop, int xstop, unsigned ctlword){
//   int ret = 0;
//   cell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
//   cell ll = CELL_TRIVIAL_INITIALIZER, lr = CELL_TRIVIAL_INITIALIZER;
//   cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
//   if((ret = cells_rounded_box(n, attr, channels, &ul, &ur, &ll, &lr, &hl, &vl)) == 0){
//     ret = ncplane_box(n, &ul, &ur, &ll, &lr, &hl, &vl, ystop, xstop, ctlword);
//   }
//   cell_release(n, &ul); cell_release(n, &ur);
//   cell_release(n, &ll); cell_release(n, &lr);
//   cell_release(n, &hl); cell_release(n, &vl);
//   return ret;
// }

// static inline int
// ncplane_rounded_box_sized(struct ncplane* n, uint32_t attr, uint64_t channels,
//                           int ylen, int xlen, unsigned ctlword){
//   int y, x;
//   ncplane_cursor_yx(n, &y, &x);
//   return ncplane_rounded_box(n, attr, channels, y + ylen - 1,
//                              x + xlen - 1, ctlword);
// }

// static inline int
// ncplane_double_box(struct ncplane* n, uint32_t attr, uint64_t channels,
//                    int ystop, int xstop, unsigned ctlword){
//   int ret = 0;
//   cell ul = CELL_TRIVIAL_INITIALIZER, ur = CELL_TRIVIAL_INITIALIZER;
//   cell ll = CELL_TRIVIAL_INITIALIZER, lr = CELL_TRIVIAL_INITIALIZER;
//   cell hl = CELL_TRIVIAL_INITIALIZER, vl = CELL_TRIVIAL_INITIALIZER;
//   if((ret = cells_double_box(n, attr, channels, &ul, &ur, &ll, &lr, &hl, &vl)) == 0){
//     ret = ncplane_box(n, &ul, &ur, &ll, &lr, &hl, &vl, ystop, xstop, ctlword);
//   }
//   cell_release(n, &ul); cell_release(n, &ur);
//   cell_release(n, &ll); cell_release(n, &lr);
//   cell_release(n, &hl); cell_release(n, &vl);
//   return ret;
// }

// static inline int
// ncplane_double_box_sized(struct ncplane* n, uint32_t attr, uint64_t channels,
//                          int ylen, int xlen, unsigned ctlword){
//   int y, x;
//   ncplane_cursor_yx(n, &y, &x);
//   return ncplane_double_box(n, attr, channels, y + ylen - 1,
//                             x + xlen - 1, ctlword);
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
