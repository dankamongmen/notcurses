//! `ncplane_*` reimplemented functions.

use core::{ffi::c_void, ptr::null_mut};
use libc::free;

use crate::ffi::__va_list_tag;
use crate::{
    cell_release, cstring, ncplane_channels, NcAlign, NcAlphaBits, NcBoxMask, NcCell, NcChannel,
    NcChannelPair, NcColor, NcDimension, NcEgc, NcPlane, NcResult, NcRgb, NcStyleMask,
    NCRESULT_ERR, NCRESULT_OK,
};

// Alpha -----------------------------------------------------------------------

/// Gets the foreground [NcAlphaBits] from the [NcPlane], shifted to LSBs.
///
/// *Method: NcPlane.[fg_alpha()][NcPlane#method.fg_alpha].*
#[inline]
pub fn ncplane_fg_alpha(plane: &NcPlane) -> NcAlphaBits {
    crate::channels_fg_alpha(unsafe { ncplane_channels(plane) })
}

/// Gets the background [NcAlphaBits] from the [NcPlane], shifted to LSBs.
///
/// *Method: NcPlane.[bg_alpha()][NcPlane#method.bg_alpha].*
#[inline]
pub fn ncplane_bg_alpha(plane: &NcPlane) -> NcAlphaBits {
    crate::channels_bg_alpha(unsafe { ncplane_channels(plane) })
}

// NcChannel -------------------------------------------------------------------

/// Gets the foreground [NcChannel] from an [NcPlane].
///
/// *Method: NcPlane.[fchannel()][NcPlane#method.fchannel].*
#[inline]
pub fn ncplane_fchannel(plane: &NcPlane) -> NcChannel {
    crate::channels_fchannel(unsafe { ncplane_channels(plane) })
}

/// Gets the background [NcChannel] from an [NcPlane].
///
/// *Method: NcPlane.[bchannel()][NcPlane#method.bchannel].*
#[inline]
pub fn ncplane_bchannel(plane: &NcPlane) -> NcChannel {
    crate::channels_bchannel(unsafe { ncplane_channels(plane) })
}

// NcColor ---------------------------------------------------------------------

/// Gets the foreground [NcColor] RGB components from an [NcPlane].
/// and returns the background [NcChannel].
///
/// *Method: NcPlane.[fg_rgb8()][NcPlane#method.fg_rgb8].*
#[inline]
pub fn ncplane_fg_rgb8(
    plane: &NcPlane,
    red: &mut NcColor,
    green: &mut NcColor,
    blue: &mut NcColor,
) -> NcChannel {
    crate::channels_fg_rgb8(unsafe { ncplane_channels(plane) }, red, green, blue)
}

/// Gets the background [NcColor] RGB components from an [NcPlane],
/// and returns the background [NcChannel].
///
/// *Method: NcPlane.[bg_rgb8()][NcPlane#method.bg_rgb8].*
#[inline]
pub fn ncplane_bg_rgb8(
    plane: &NcPlane,
    red: &mut NcColor,
    green: &mut NcColor,
    blue: &mut NcColor,
) -> NcChannel {
    crate::channels_bg_rgb8(unsafe { ncplane_channels(plane) }, red, green, blue)
}

// NcRgb -----------------------------------------------------------------------

/// Gets the foreground [NcRgb] from an [NcPlane], shifted to LSBs.
///
/// *Method: NcPlane.[fg_rgb()][NcPlane#method.fg_rgb].*
#[inline]
pub fn ncplane_fg_rgb(plane: &NcPlane) -> NcRgb {
    crate::channels_fg_rgb(unsafe { ncplane_channels(plane) })
}

/// Gets the background [NcRgb] from an [NcPlane], shifted to LSBs.
///
/// *Method: NcPlane.[bg_rgb()][NcPlane#method.bg_rgb].*
#[inline]
pub fn ncplane_bg_rgb(plane: &NcPlane) -> NcRgb {
    crate::channels_bg_rgb(unsafe { ncplane_channels(plane) })
}

// Default ---------------------------------------------------------------------

/// Is the plane's foreground using the "default foreground color"?
///
/// *Method: NcPlane.[fg_default_p()][NcPlane#method.fg_default_p].*
#[inline]
pub fn ncplane_fg_default_p(plane: &NcPlane) -> bool {
    crate::channels_fg_default_p(unsafe { ncplane_channels(plane) })
}

/// Is the plane's background using the "default background color"?
///
/// *Method: NcPlane.[bg_default_p()][NcPlane#method.bg_default_p].*
#[inline]
pub fn ncplane_bg_default_p(plane: &NcPlane) -> bool {
    crate::channels_bg_default_p(unsafe { ncplane_channels(plane) })
}

// put & print -----------------------------------------------------------------

/// Calls [ncplane_putc_yx()][crate::ncplane_putc_yx] for the current cursor location.
///
/// *Method: NcPlane.[putc()][NcPlane#method.putc].*
#[inline]
pub fn ncplane_putc(plane: &mut NcPlane, cell: &NcCell) -> NcResult {
    unsafe { crate::ncplane_putc_yx(plane, -1, -1, cell) }
}

/// Calls ncplane_putchar_yx() at the current cursor location.
///
/// *Method: NcPlane.[putchar()][NcPlane#method.putchar].*
#[inline]
pub fn ncplane_putchar(plane: &mut NcPlane, ch: char) -> NcResult {
    unsafe {
        let cell = NcCell::with_char(plane, ch);
        crate::ncplane_putc_yx(plane, -1, -1, &cell)
    }
}

/// Replaces the [NcCell] at the specified coordinates with the provided char.
/// Advances the cursor by 1.
///
/// *Method: NcPlane.[putchar_yx()][NcPlane#method.putchar_yx].*
#[inline]
pub fn ncplane_putchar_yx(
    plane: &mut NcPlane,
    y: NcDimension,
    x: NcDimension,
    ch: char,
) -> NcResult {
    unsafe {
        let cell = NcCell::with_char(plane, ch);
        crate::ncplane_putc_yx(plane, y as i32, x as i32, &cell)
    }
}

/// Writes a series of [NcEgc]s to the current location, using the current style.
///
/// *Method: NcPlane.[putstr()][NcPlane#method.putstr].*
#[inline]
pub fn ncplane_putstr(plane: &mut NcPlane, string: &str) -> NcResult {
    unsafe { crate::ncplane_putstr_yx(plane, -1, -1, cstring![string]) }
}

///
///
/// *Method: NcPlane.[putnstr()][NcPlane#method.putnstr].*
#[inline]
pub fn ncplane_putnstr(plane: &mut NcPlane, size: u64, gclustarr: &[u8]) -> NcResult {
    unsafe { crate::ncplane_putnstr_yx(plane, -1, -1, size, cstring![gclustarr]) }
}

/// The [NcPlane] equivalent of `vprintf(3)`.
///
/// *Method: NcPlane.[vprintf()][NcPlane#method.vprintf].*
#[inline]
pub fn ncplane_vprintf(plane: &mut NcPlane, format: &str, ap: &mut __va_list_tag) -> NcResult {
    unsafe { crate::ncplane_vprintf_yx(plane, -1, -1, cstring![format], ap) }
}

// NcCell ----------------------------------------------------------------------

/// Retrieves the current contents of the [NcCell] under the cursor.
///
/// This NcCell is invalidated if the associated NcPlane is destroyed.
///
/// *Method: NcPlane.[at_cursor_cell()][NcPlane#method.at_cursor_cell].*
#[inline]
pub fn ncplane_at_cursor_cell(plane: &mut NcPlane, cell: &mut NcCell) -> NcResult {
    let mut egc =
        unsafe { crate::ncplane_at_cursor(plane, &mut cell.stylemask, &mut cell.channels) };
    if egc.is_null() {
        return NCRESULT_ERR;
    }
    let result: NcResult = unsafe { crate::cell_load(plane, cell, egc) };
    if result != NCRESULT_OK {
        unsafe {
            free(&mut egc as *mut _ as *mut c_void);
        }
    }
    result
}

/// Retrieves the current contents of the specified cell into `cell`.
/// This cell is invalidated if the associated plane is destroyed.
///
/// *Method: NcPlane.[at_yx_cell()][NcPlane#method.at_yx_cell].*
#[inline]
pub fn ncplane_at_yx_cell(
    plane: &mut NcPlane,
    y: NcDimension,
    x: NcDimension,
    cell: &mut NcCell,
) -> NcResult {
    let mut egc = unsafe {
        crate::ncplane_at_yx(
            plane,
            y as i32,
            x as i32,
            &mut cell.stylemask,
            &mut cell.channels,
        )
    };
    if egc.is_null() {
        return NCRESULT_ERR;
    }
    let channels = cell.channels; // need to preserve wide flag
    let result: NcResult = unsafe { crate::cell_load(plane, cell, egc) };
    cell.channels = channels;
    unsafe {
        free(&mut egc as *mut _ as *mut c_void);
    }
    result
}

// size & alignment ------------------------------------------------------------

/// Gets the columns of the [NcPlane].
///
/// *Method: NcPlane.[dim_x()][NcPlane#method.dim_x].*
#[inline]
pub fn ncplane_dim_x(plane: &NcPlane) -> NcDimension {
    unsafe {
        let mut x = 0;
        crate::ncplane_dim_yx(plane, null_mut(), &mut x);
        x as NcDimension
    }
}

/// Gets the rows of the [NcPlane].
///
/// *Method: NcPlane.[dim_y()][NcPlane#method.dim_y].*
#[inline]
#[inline]
pub fn ncplane_dim_y(plane: &NcPlane) -> NcDimension {
    unsafe {
        let mut y = 0;
        crate::ncplane_dim_yx(plane, &mut y, null_mut());
        y as NcDimension
    }
}

/// Resizes the plane, retaining what data we can (everything, unless we're
/// shrinking in some dimension). Keep the origin where it is.
///
/// *Method: NcPlane.[resize_simple()][NcPlane#method.resize_simple].*
#[inline]
pub fn ncplane_resize_simple(
    plane: &mut NcPlane,
    y_len: NcDimension,
    x_len: NcDimension,
) -> NcResult {
    let (mut old_y, mut old_x) = (0, 0);
    unsafe {
        crate::ncplane_dim_yx(plane, &mut old_y, &mut old_x);
    }
    let keep_len_y = {
        if old_y > y_len as i32 {
            y_len as i32
        } else {
            old_y
        }
    };
    let keep_len_x = {
        if old_x > x_len as i32 {
            x_len as i32
        } else {
            old_x
        }
    };
    unsafe {
        crate::ncplane_resize(
            plane,
            0,
            0,
            keep_len_y,
            keep_len_x,
            0,
            0,
            y_len as i32,
            x_len as i32,
        )
    }
}

/// Returns the column at which `cols` columns ought start in order to be aligned
/// according to `align` within this NcPlane.
///
/// Returns `-`[`NCRESULT_MAX`][crate::NCRESULT_MAX] if
/// [NCALIGN_UNALIGNED][crate::NCALIGN_UNALIGNED] or invalid [NcAlign].
///
/// *Method: NcPlane.[align()][NcPlane#method.align].*
#[inline]
pub fn ncplane_align(plane: &NcPlane, align: NcAlign, cols: NcDimension) -> NcResult {
    crate::notcurses_align(ncplane_dim_x(plane), align, cols)
}

// line ------------------------------------------------------------------------

/// Draws horizontal lines using the specified NcCell, starting at the current
/// cursor position.
///
/// The cursor will end at the cell following the last cell output,
/// just as if ncplane_putc() was called at that spot.
///
/// Returns the number of cells drawn on success. On error, returns the negative
/// number of cells drawn.
///
/// *Method: NcPlane.[hline()][NcPlane#method.hline].*
#[inline]
pub fn ncplane_hline(plane: &mut NcPlane, cell: &NcCell, len: NcDimension) -> NcResult {
    unsafe { crate::ncplane_hline_interp(plane, cell, len as i32, cell.channels, cell.channels) }
}

/// Draws vertical lines using the specified NcCell, starting at the current
/// cursor position.
///
/// The cursor will end at the cell following the last cell output,
/// just as if ncplane_putc() was called at that spot.
///
/// Returns the number of cells drawn on success. On error, returns the negative
/// number of cells drawn.
///
/// *Method: NcPlane.[vline()][NcPlane#method.vline].*
#[inline]
pub fn ncplane_vline(plane: &mut NcPlane, cell: &NcCell, len: NcDimension) -> NcResult {
    unsafe { crate::ncplane_vline_interp(plane, cell, len as i32, cell.channels, cell.channels) }
}

// perimeter -------------------------------------------------------------------

///
///
/// *Method: NcPlane.[perimeter()][NcPlane#method.perimeter].*
#[inline]
pub fn ncplane_perimeter(
    plane: &mut NcPlane,
    ul: &NcCell,
    ur: &NcCell,
    ll: &NcCell,
    lr: &NcCell,
    hline: &NcCell,
    vline: &NcCell,
    boxmask: NcBoxMask,
) -> NcResult {
    unsafe {
        crate::ncplane_cursor_move_yx(plane, 0, 0);
        let (mut dimy, mut dimx) = (0, 0);
        crate::ncplane_dim_yx(plane, &mut dimy, &mut dimx);
        ncplane_box_sized(
            plane,
            ul,
            ur,
            ll,
            lr,
            hline,
            vline,
            dimy as NcDimension,
            dimx as NcDimension,
            boxmask,
        )
    }
}

///
///
/// *Method: NcPlane.[perimeter_double()][NcPlane#method.perimeter_double].*
#[inline]
pub fn ncplane_perimeter_double(
    plane: &mut NcPlane,
    stylemask: NcStyleMask,
    channels: NcChannelPair,
    boxmask: NcBoxMask,
) -> NcResult {
    if unsafe { crate::ncplane_cursor_move_yx(plane, 0, 0) } != NCRESULT_OK {
        return NCRESULT_ERR;
    }
    let (mut dimy, mut dimx) = (0, 0);
    unsafe {
        crate::ncplane_dim_yx(plane, &mut dimy, &mut dimx);
    }
    let mut ul = NcCell::new();
    let mut ur = NcCell::new();
    let mut ll = NcCell::new();
    let mut lr = NcCell::new();
    let mut hl = NcCell::new();
    let mut vl = NcCell::new();
    if unsafe {
        crate::cells_double_box(
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
    let ret = ncplane_box_sized(
        plane,
        &ul,
        &ur,
        &ll,
        &lr,
        &hl,
        &vl,
        dimy as NcDimension,
        dimx as NcDimension,
        boxmask,
    );
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
///
/// *Method: NcPlane.[perimeter_rounded()][NcPlane#method.perimeter_rounded].*
#[inline]
pub fn ncplane_perimeter_rounded(
    plane: &mut NcPlane,
    stylemask: NcStyleMask,
    channels: NcChannelPair,
    boxmask: NcBoxMask,
) -> NcResult {
    if unsafe { crate::ncplane_cursor_move_yx(plane, 0, 0) } != NCRESULT_OK {
        return NCRESULT_ERR;
    }
    let (mut dimy, mut dimx) = (0, 0);
    unsafe {
        crate::ncplane_dim_yx(plane, &mut dimy, &mut dimx);
    }
    let mut ul = NcCell::new();
    let mut ur = NcCell::new();
    let mut ll = NcCell::new();
    let mut lr = NcCell::new();
    let mut hl = NcCell::new();
    let mut vl = NcCell::new();
    if unsafe {
        crate::cells_rounded_box(
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
    let ret = ncplane_box_sized(
        plane,
        &ul,
        &ur,
        &ll,
        &lr,
        &hl,
        &vl,
        dimy as NcDimension,
        dimx as NcDimension,
        boxmask,
    );
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

// box -------------------------------------------------------------------------

/// Draws a box with its upper-left corner at the current cursor position,
/// having dimensions `y_len` * `x_len`.
///
/// The minimum box size is 2x2, and it cannot be drawn off-screen.
///
/// See [ncplane_box()](crate::ncplane_box) for more information.
///
/// *Method: NcPlane.[box_sized()][NcPlane#method.box_sized].*
#[inline]
pub fn ncplane_box_sized(
    plane: &mut NcPlane,
    ul: &NcCell,
    ur: &NcCell,
    ll: &NcCell,
    lr: &NcCell,
    hline: &NcCell,
    vline: &NcCell,
    y_len: NcDimension,
    x_len: NcDimension,
    boxmask: NcBoxMask,
) -> NcResult {
    let (mut y, mut x) = (0, 0);
    unsafe {
        crate::ncplane_cursor_yx(plane, &mut y, &mut x);
        crate::ncplane_box(
            plane,
            ul,
            ur,
            ll,
            lr,
            hline,
            vline,
            y + y_len as i32 - 1,
            x + x_len as i32 - 1,
            boxmask,
        )
    }
}

///
///
/// *Method: NcPlane.[double_box()][NcPlane#method.double_box].*
#[inline]
pub fn ncplane_double_box(
    plane: &mut NcPlane,
    stylemask: NcStyleMask,
    channels: NcChannelPair,
    y_stop: NcDimension,
    x_stop: NcDimension,
    boxmask: NcBoxMask,
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
        ret = crate::cells_double_box(
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
            ret = crate::ncplane_box(
                plane,
                &ul,
                &ur,
                &ll,
                &lr,
                &hl,
                &vl,
                y_stop as i32,
                x_stop as i32,
                boxmask,
            );
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
///
/// *Method: NcPlane.[double_box_sized()][NcPlane#method.double_box_sized].*
#[inline]
pub fn ncplane_double_box_sized(
    plane: &mut NcPlane,
    stylemask: NcStyleMask,
    channels: NcChannelPair,
    y_len: NcDimension,
    x_len: NcDimension,
    boxmask: NcBoxMask,
) -> NcResult {
    let (mut y, mut x) = (0, 0);
    unsafe {
        crate::ncplane_cursor_yx(plane, &mut y, &mut x);
    }
    crate::ncplane_double_box(
        plane,
        stylemask,
        channels,
        y as NcDimension + y_len - 1,
        x as NcDimension + x_len - 1,
        boxmask,
    )
}

///
///
/// *Method: NcPlane.[rounded_box()][NcPlane#method.rounded_box].*
#[inline]
pub fn ncplane_rounded_box(
    plane: &mut NcPlane,
    stylemask: NcStyleMask,
    channels: NcChannelPair,
    y_stop: NcDimension,
    x_stop: NcDimension,
    boxmask: NcBoxMask,
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
        ret = crate::cells_rounded_box(
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
            ret = crate::ncplane_box(
                plane,
                &ul,
                &ur,
                &ll,
                &lr,
                &hl,
                &vl,
                y_stop as i32,
                x_stop as i32,
                boxmask,
            );
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
///
/// *Method: NcPlane.[rounded_box_sized()][NcPlane#method.rounded_box_sized].*
#[inline]
pub fn ncplane_rounded_box_sized(
    plane: &mut NcPlane,
    stylemask: NcStyleMask,
    channels: NcChannelPair,
    y_len: NcDimension,
    x_len: NcDimension,
    boxmask: NcBoxMask,
) -> NcResult {
    let (mut y, mut x) = (0, 0);
    unsafe {
        crate::ncplane_cursor_yx(plane, &mut y, &mut x);
    }
    ncplane_rounded_box(
        plane,
        stylemask,
        channels,
        y as NcDimension + y_len - 1,
        x as NcDimension + x_len - 1,
        boxmask,
    )
}

// gradient --------------------------------------------------------------------

/// Draw a gradient with its upper-left corner at the current cursor position,
/// having dimensions `y_len` * `x_len`.
///
/// See [ncplane_gradient][crate::ncplane_gradient] for more information.
///
/// *Method: NcPlane.[gradient_sized()][NcPlane#method.gradient_sized].*
#[inline]
pub fn ncplane_gradient_sized(
    plane: &mut NcPlane,
    egc: &NcEgc,
    stylemask: NcStyleMask,
    ul: NcChannel,
    ur: NcChannel,
    ll: NcChannel,
    lr: NcChannel,
    y_len: NcDimension,
    x_len: NcDimension,
) -> NcResult {
    if y_len < 1 || x_len < 1 {
        return NCRESULT_ERR;
    }
    let (mut y, mut x) = (0, 0);
    unsafe {
        crate::ncplane_cursor_yx(plane, &mut y, &mut x);
        crate::ncplane_gradient(
            plane,
            &(*egc as i8),
            stylemask as u32,
            ul as u64,
            ur as u64,
            ll as u64,
            lr as u64,
            y + y_len as i32 - 1,
            x + x_len as i32 - 1,
        )
    }
}
