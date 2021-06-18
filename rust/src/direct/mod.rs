//! `NcDirect`

// total: 61
// ---------------------------------------------------
// (X)  1 : wont do
// (~)  3 : TODO / WIP
//
// (f) 45 : unsafe ffi function exported by bindgen
// (w)  1 : safely wrapped ffi function
// (r) 11 : static function manually reimplemented
//
// (m) 45 : method implemented
//
// (t)  0 : unit test done for the function
// (T)  0 : unit test done also for the method
// ---------------------------------------------------
// fm  ncdirect_bg_default
// fm  ncdirect_bg_palindex
// fm  ncdirect_bg_rgb
// fm  ncdirect_box
// r   ncdirect_canbraille
// r   ncdirect_canchangecolor
// r   ncdirect_canfade
// r   ncdirect_canhalfblock
// fm  ncdirect_canopen_images
// rm  ncdirect_canopen_videos
// r   ncdirect_canquadrant
// r   ncdirect_cantruecolor
// fm  ncdirect_canutf8
// wm  ncdirect_capabilities
// fm  ncdirect_check_pixel_support
// fm  ncdirect_clear
//~f   ncdirect_core_init
// fm  ncdirect_cursor_disable
// fm  ncdirect_cursor_down
// fm  ncdirect_cursor_enable
// fm  ncdirect_cursor_left
// fm  ncdirect_cursor_move_yx
// fm  ncdirect_cursor_pop
// fm  ncdirect_cursor_push
// fm  ncdirect_cursor_right
// fm  ncdirect_cursor_up
// fm  ncdirect_cursor_yx
//     ncdirect_detected_terminal
// fm  ncdirect_dim_x
// fm  ncdirect_dim_y
// fm  ncdirect_double_box
// fm  ncdirect_fg_default
// fm  ncdirect_fg_palindex
// fm  ncdirect_fg_rgb
// fm  ncdirect_flush
// fm  ncdirect_getc
//~r   ncdirect_heavy_box,
// fm  ncdirect_hline_interp
// fm  ncdirect_init
// fm  ncdirect_inputready_fd
//~r   ncdirect_light_box,
// fm  ncplane_on_styles
// fm  ncplane_off_styles
// fm  ncdirect_palette_size
//X    ncdirect_printf_aligned
// fm  ncdirect_putstr
// fm  ncdirect_raster_frame
// fm  ncdirect_readline
// fm  ncdirect_render_frame
// fm  ncdirect_render_image
// fm  ncdirect_rounded_box
// fm  ncplane_set_styles
// fm  ncdirect_stop
// f   ncdirect_stream
// f   ncdirect_styles
// f   ncdirect_supported_styles
// fm  ncdirect_vline_interp
// rm  ncdirect_bg_rgb8
// rm  ncdirect_fg_rgb8
// rm  ncdirect_getc_nblock
// rm  ncdirect_getc_nblocking

#[cfg(test)]
mod test;

mod methods;
mod reimplemented;

pub use reimplemented::*;

/// Minimal notcurses instance for styling text.
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

/// Flag that inhibits registration of the SIGINT, SIGSEGV, SIGABRT & SIGQUIT
/// signal handlers.
pub const NCDIRECT_OPTION_NO_QUIT_SIGHANDLERS: NcDirectFlags =
    crate::bindings::ffi::NCDIRECT_OPTION_NO_QUIT_SIGHANDLERS as NcDirectFlags;
