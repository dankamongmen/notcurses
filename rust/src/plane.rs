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
// ncplane_styles
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
// (+) implement : 10 / … / 32
// (#) unit tests:  0 / … / 42
// ------------------------------------------
// ncplane_align
// ncplane_at_cursor_cell
// ncplane_at_yx_cell
//+ncplane_bchannel
//+ncplane_bg
//+ncplane_bg_alpha
//+ncplane_bg_default_p
//+ncplane_bg_rgb
// ncplane_box_sized
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
// ncplane_hline
//+ncplane_perimeter
// ncplane_perimeter_double
// ncplane_perimeter_rounded
// ncplane_putc
// ncplane_putegc
// ncplane_putnstr
// ncplane_putsimple
// ncplane_putsimple_yx
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
// ncplane_vline
// ncplane_vprintf

use core::ptr::null_mut;
use cstr_core::CString;

use crate as ffi;
use ffi::types::{Alpha, Channel, Color, IntResult};

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

// // Retrieve the current contents of the cell under the cursor into 'c'. This
// // cell is invalidated if the associated plane is destroyed.
// static inline int
// ncplane_at_cursor_cell(struct ncplane* n, cell* c){
//   char* egc = ncplane_at_cursor(n, &c->attrword, &c->channels);
//   if(!egc){
//     return -1;
//   }
//   uint64_t channels = c->channels; // need to preserve wide flag
//   int r = cell_load(n, c, egc);
//   c->channels = channels;
//   if(r < 0){
//     free(egc);
//   }
//   return r;
// }

// // Retrieve the current contents of the specified cell into 'c'. This cell is
// // invalidated if the associated plane is destroyed.
// static inline int
// ncplane_at_yx_cell(struct ncplane* n, int y, int x, cell* c){
//   char* egc = ncplane_at_yx(n, y, x, &c->attrword, &c->channels);
//   if(!egc){
//     return -1;
//   }
//   uint64_t channels = c->channels; // need to preserve wide flag
//   int r = cell_load(n, c, egc);
//   c->channels = channels;
//   free(egc);
//   return r;
// }

// // Return the column at which 'c' cols ought start in order to be aligned
// // according to 'align' within ncplane 'n'. Returns INT_MAX on invalid 'align'.
// // Undefined behavior on negative 'c'.
// static inline int
// ncplane_align(const struct ncplane* n, ncalign_e align, int c){
//   if(align == NCALIGN_LEFT){
//     return 0;
//   }
//   int cols = ncplane_dim_x(n);
//   if(c > cols){
//     return 0;
//   }
//   if(align == NCALIGN_CENTER){
//     return (cols - c) / 2;
//   }else if(align == NCALIGN_RIGHT){
//     return cols - c;
//   }
//   return INT_MAX;
// }

// // Call ncplane_putc_yx() for the current cursor location.
// static inline int
// ncplane_putc(struct ncplane* n, const cell* c){
//   return ncplane_putc_yx(n, -1, -1, c);
// }

// // Replace the EGC underneath us, but retain the styling. The current styling
// // of the plane will not be changed.
// //
// // Replace the cell at the specified coordinates with the provided 7-bit char
// // 'c'. Advance the cursor by 1. On success, returns 1. On failure, returns -1.
// // This works whether the underlying char is signed or unsigned.
// static inline int
// ncplane_putsimple_yx(struct ncplane* n, int y, int x, char c){
//   cell ce = CELL_INITIALIZER((uint32_t)c, ncplane_attr(n), ncplane_channels(n));
//   if(!cell_simple_p(&ce)){
//     return -1;
//   }
//   return ncplane_putc_yx(n, y, x, &ce);
// }

// // Call ncplane_putsimple_yx() at the current cursor location.
// static inline int
// ncplane_putsimple(struct ncplane* n, char c){
//   return ncplane_putsimple_yx(n, -1, -1, c);
// }

// // Call ncplane_putegc() at the current cursor location.
// static inline int
// ncplane_putegc(struct ncplane* n, const char* gclust, int* sbytes){
//   return ncplane_putegc_yx(n, -1, -1, gclust, sbytes);
// }
//
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

// // Call ncplane_putwegc() after successfully moving to y, x.
// static inline int
// ncplane_putwegc_yx(struct ncplane* n, int y, int x, const wchar_t* gclust,
//                    int* sbytes){
//   if(ncplane_cursor_move_yx(n, y, x)){
//     return -1;
//   }
//   return ncplane_putwegc(n, gclust, sbytes);
// }

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

// // Replace the cell at the specified coordinates with the provided wide char
// // 'w'. Advance the cursor by the character's width as reported by wcwidth().
// // On success, returns 1. On failure, returns -1.
// static inline int
// ncplane_putwc_yx(struct ncplane* n, int y, int x, wchar_t w){
//   wchar_t warr[2] = { w, L'\0' };
//   return ncplane_putwstr_yx(n, y, x, warr);
// }

// // Call ncplane_putwc() at the current cursor position.
// static inline int
// ncplane_putwc(struct ncplane* n, wchar_t w){
//   return ncplane_putwc_yx(n, -1, -1, w);
// }

// static inline int
// ncplane_vprintf(struct ncplane* n, const char* format, va_list ap){
//   return ncplane_vprintf_yx(n, -1, -1, format, ap);
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

// static inline int
// ncplane_hline(struct ncplane* n, const cell* c, int len){
//   return ncplane_hline_interp(n, c, len, c->channels, c->channels);
// }

// static inline int
// ncplane_vline(struct ncplane* n, const cell* c, int len){
//   return ncplane_vline_interp(n, c, len, c->channels, c->channels);
// }

// // Draw a box with its upper-left corner at the current cursor position, having
// // dimensions 'ylen'x'xlen'. See ncplane_box() for more information. The
// // minimum box size is 2x2, and it cannot be drawn off-screen.
// static inline int
// ncplane_box_sized(struct ncplane* n, const cell* ul, const cell* ur,
//                   const cell* ll, const cell* lr, const cell* hline,
//                   const cell* vline, int ylen, int xlen, unsigned ctlword){
//   int y, x;
//   ncplane_cursor_yx(n, &y, &x);
//   return ncplane_box(n, ul, ur, ll, lr, hline, vline, y + ylen - 1,
//                      x + xlen - 1, ctlword);
// }

// static inline int
// ncplane_perimeter(struct ncplane* n, const cell* ul, const cell* ur,
//                   const cell* ll, const cell* lr, const cell* hline,
//                   const cell* vline, unsigned ctlword){
//   if(ncplane_cursor_move_yx(n, 0, 0)){
//     return -1;
//   }
//   int dimy, dimx;
//   ncplane_dim_yx(n, &dimy, &dimx);
//   return ncplane_box_sized(n, ul, ur, ll, lr, hline, vline, dimy, dimx, ctlword);
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
pub fn ncplane_fchannel(plane: &ffi::ncplane) -> Channel {
    ffi::channels_fchannel(unsafe { ffi::ncplane_channels(plane)})
}

/// Extract the 32-bit working background channel from an ncplane.
// TODO: TEST
#[inline]
pub fn ncplane_bchannel(plane: &ffi::ncplane) -> Channel {
    ffi::channels_bchannel(unsafe { ffi::ncplane_channels(plane)})
}

/// Extract 24 bits of working foreground RGB from an ncplane, shifted to LSBs.
// TODO: TEST
#[inline]
pub fn ncplane_fg(plane: &ffi::ncplane) -> Channel {
    ffi::channels_fg(unsafe { ffi::ncplane_channels(plane)})
}


/// Extract 24 bits of working background RGB from an ncplane, shifted to LSBs.
// TODO: TEST
#[inline]
pub fn ncplane_bg(plane: &ffi::ncplane) -> Channel {
    ffi::channels_bg(unsafe { ffi::ncplane_channels(plane)})
}

/// Extract 2 bits of foreground alpha from 'struct ncplane', shifted to LSBs.
// TODO: TEST
#[inline]
pub fn ncplane_fg_alpha(plane: &ffi::ncplane) -> Alpha {
    ffi::channels_fg_alpha(unsafe { ffi::ncplane_channels(plane)})
}

/// Extract 2 bits of background alpha from 'struct ncplane', shifted to LSBs.
// TODO: TEST
#[inline]
pub fn ncplane_bg_alpha(plane: &ffi::ncplane) -> Alpha {
    ffi::channels_bg_alpha(unsafe { ffi::ncplane_channels(plane)})
}

/// Is the plane's foreground using the "default foreground color"?
// TODO: TEST
#[inline]
pub fn ncplane_fg_default_p(plane: &ffi::ncplane) -> bool {
    ffi::channels_fg_default_p(unsafe { ffi::ncplane_channels(plane)})
}

/// Is the plane's background using the "default background color"?
// TODO: TEST
#[inline]
pub fn ncplane_bg_default_p(plane: &ffi::ncplane) -> bool {
    ffi::channels_bg_default_p(unsafe { ffi::ncplane_channels(plane)})
}

/// Extract 24 bits of foreground RGB from a plane, split into components.
// TODO: TEST
#[inline]
pub fn ncplane_fg_rgb(
    plane: &ffi::ncplane,
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
    plane: &ffi::ncplane,
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
// ncplane_perimeter_rounded(struct ncplane* n, uint32_t attrword,
//                           uint64_t channels, unsigned ctlword){
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
//   if(cells_rounded_box(n, attrword, channels, &ul, &ur, &ll, &lr, &hl, &vl)){
//     return -1;
//   }
//   int r = ncplane_box_sized(n, &ul, &ur, &ll, &lr, &hl, &vl, dimy, dimx, ctlword);
//   cell_release(n, &ul); cell_release(n, &ur);
//   cell_release(n, &ll); cell_release(n, &lr);
//   cell_release(n, &hl); cell_release(n, &vl);
//   return r;
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
// ncplane_perimeter_double(struct ncplane* n, uint32_t attrword,
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
//   if(cells_double_box(n, attrword, channels, &ul, &ur, &ll, &lr, &hl, &vl)){
//     return -1;
//   }
//   int r = ncplane_box_sized(n, &ul, &ur, &ll, &lr, &hl, &vl, dimy, dimx, ctlword);
//   cell_release(n, &ul); cell_release(n, &ur);
//   cell_release(n, &ll); cell_release(n, &lr);
//   cell_release(n, &hl); cell_release(n, &vl);
//   return r;
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
