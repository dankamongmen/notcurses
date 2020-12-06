//! `NcDirect`

// functions already exported by bindgen : 38
// ------------------------------------------
// (W) wrap: 1 / 37
// (#) test: 0 / 38
// ------------------------------------------
//  ncdirect_bg_default
//  ncdirect_bg_palindex
//  ncdirect_bg_rgb
//  ncdirect_box
//  ncdirect_canopen_images
//  ncdirect_canutf8
//  ncdirect_clear
//  ncdirect_cursor_disable
//  ncdirect_cursor_down
//  ncdirect_cursor_enable
//  ncdirect_cursor_left
//  ncdirect_cursor_move_yx
//  ncdirect_cursor_pop
//  ncdirect_cursor_push
//  ncdirect_cursor_right
//  ncdirect_cursor_up
//  ncdirect_cursor_yx
//  ncdirect_dim_x
//  ncdirect_dim_y
//  ncdirect_double_box
//  ncdirect_fg_default
//  ncdirect_fg_palindex
//  ncdirect_fg_rgb
//  ncdirect_flush
//  ncdirect_getc
//  ncdirect_hline_interp
//W ncdirect_init
//  ncdirect_inputready_fd
//  ncdirect_palette_size
//  ncdirect_printf_aligned
//  ncdirect_putstr
//  ncdirect_render_image
//  ncdirect_rounded_box
//  ncdirect_stop
//  ncdirect_styles_off
//  ncdirect_styles_on
//  ncdirect_styles_set
//  ncdirect_vline_interp

#[cfg(test)]
mod test;

mod methods;

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
