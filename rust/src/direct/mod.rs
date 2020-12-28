//! `NcDirect`

// functions already exported by bindgen : 43
// ------------------------------------------
// (X) wont:  4
// (#) test:  0
// (W) wrap: 39 / 0
// ------------------------------------------
//W  ncdirect_bg_default
//W  ncdirect_bg_palindex
//W  ncdirect_bg_rgb
//W  ncdirect_box
//W  ncdirect_canopen_images
//W  ncdirect_canutf8
//W  ncdirect_clear
//W  ncdirect_cursor_disable
//W  ncdirect_cursor_down
//W  ncdirect_cursor_enable
//W  ncdirect_cursor_left
//W  ncdirect_cursor_move_yx
//W  ncdirect_cursor_pop
//W  ncdirect_cursor_push
//W  ncdirect_cursor_right
//W  ncdirect_cursor_up
//W  ncdirect_cursor_yx
//W  ncdirect_dim_x
//W  ncdirect_dim_y
//W  ncdirect_double_box
//W  ncdirect_fg_default
//W  ncdirect_fg_palindex
//W  ncdirect_fg_rgb
//W  ncdirect_flush
//W  ncdirect_getc
//W  ncdirect_hline_interp
//W  ncdirect_init
//W  ncdirect_inputready_fd
//W  ncplane_on_styles
//W  ncplane_off_styles
//W  ncdirect_palette_size
// X ncdirect_printf_aligned
//W  ncdirect_putstr
//W  ncdirect_raster_frame
//W  ncdirect_render_frame
//W  ncdirect_render_image
//W  ncdirect_rounded_box
//W  ncplane_set_styles
//W  ncdirect_stop
// X ncdirect_styles_off     // deprecated
// X ncdirect_styles_on      // deprecated
// X ncdirect_styles_set     // deprecated
//W  ncdirect_vline_interp
//
// functions manually reimplemented: 4
// ------------------------------------------
// (+) done: 4 / 0
// (W) wrap: 4 / 0
// (#) test: 0
// ------------------------------------------
//W+ ncdirect_bg_rgb8
//W+ ncdirect_fg_rgb8
//W+ ncdirect_getc_nblock
//W+ ncdirect_getc_nblocking

#[cfg(test)]
mod test;

mod methods;
mod reimplemented;
pub use reimplemented::*;

/// Minimal notcurses instances for styling text
pub type NcDirect = crate::bindings::ffi::ncdirect;

/// Flags (options) for [`NcDirect`]
pub type NcDirectFlags = u64;

/// Flag that avoids placing the terminal into cbreak mode
/// (disabling echo and line buffering)
///
pub const NCDIRECT_OPTION_INHIBIT_CBREAK: NcDirectFlags =
    crate::bindings::ffi::NCDIRECT_OPTION_INHIBIT_CBREAK as NcDirectFlags;

/// Flag that avoids calling setlocale(LC_ALL, NULL)
///
/// If the result is either "C" or "POSIX", it will print a
/// diagnostic to stderr, and then call setlocale(LC_ALL, "").
///
/// This will attempt to set the locale based off the LANG
/// environment variable. Your program should call setlocale(3)
/// itself, usually as one of the first lines.
///
pub const NCDIRECT_OPTION_INHIBIT_SETLOCALE: NcDirectFlags =
    crate::bindings::ffi::NCDIRECT_OPTION_INHIBIT_SETLOCALE as NcDirectFlags;
