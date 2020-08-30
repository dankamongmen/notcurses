// functions already exported by bindgen : 30
// ------------------------------------------
// ncdirect_bg
// ncdirect_bg_default
// ncdirect_bg_palindex
// ncdirect_box
// ncdirect_canopen_images
// ncdirect_canutf8
// ncdirect_clear
// ncdirect_cursor_disable
// ncdirect_cursor_down
// ncdirect_cursor_enable
// ncdirect_cursor_left
// ncdirect_cursor_move_yx
// ncdirect_cursor_pop
// ncdirect_cursor_push
// ncdirect_cursor_right
// ncdirect_cursor_up
// ncdirect_cursor_yx
// ncdirect_dim_x
// ncdirect_dim_y
// ncdirect_double_box
// ncdirect_fg
// ncdirect_fg_default
// ncdirect_fg_palindex
// ncdirect_hline_interp
// ncdirect_init
// ncdirect_palette_size
// ncdirect_printf_aligned
// ncdirect_putstr
// ncdirect_render_image
// ncdirect_rounded_box
// ncdirect_stop
// ncdirect_styles_off
// ncdirect_styles_on
// ncdirect_styles_set
// ncdirect_vline_interp
//

use crate as nc;
use nc::types::DirectMode;

extern "C" {
    fn libc_stdout() -> *mut nc::_IO_FILE;
}

/// A simple ncdirect_init() wrapper
///
/// Initialize a direct-mode notcurses context on the tty.
///
/// Direct mode supportes a limited subset of notcurses routines,
/// and neither supports nor requires notcurses_render().
/// This can be used to add color and styling to text in the standard output paradigm.
/// Returns NULL on error, including any failure initializing terminfo.
pub unsafe fn ncdirect_start() -> *mut DirectMode {
    nc::ncdirect_init(core::ptr::null(), libc_stdout(), 0)
}

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
