// functions already exported by bindgen : 38
// ------------------------------------------
// ncdirect_bg_default
// ncdirect_bg_palindex
// ncdirect_bg_rgb
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
// ncdirect_fg_default
// ncdirect_fg_palindex
// ncdirect_fg_rgb
// ncdirect_flush
// ncdirect_getc
// ncdirect_hline_interp
// ncdirect_init                // wrapped at _new() & _with_flags()
// ncdirect_inputready_fd
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

use crate::{
    ncdirect_init,
    types::{NcDirect, NcDirectFlags},
};
use core::ptr::{null, null_mut};

impl NcDirect {
    /// A simple ncdirect_init() wrapper using the default options.
    ///
    /// Initialize a direct-mode notcurses context on the tty.
    ///
    /// Direct mode supports a limited subset of notcurses routines,
    /// and neither supports nor requires notcurses_render(). This can be
    /// used to add color and styling to text in the standard output paradigm.
    //
    // Returns NULL on error, including any failure initializing terminfo.
    pub unsafe fn new<'a>() -> &'a mut NcDirect {
        Self::with_flags(0)
    }

    /// A simple ncdirect_init() wrapper with optional flags.
    ///
    /// `flags` is a bitmask over:
    /// - NCDIRECT_OPTION_INHIBIT_CBREAK
    /// - NCDIRECT_OPTION_INHIBIT_SETLOCALE
    ///
    pub unsafe fn with_flags<'a>(flags: NcDirectFlags) -> &'a mut NcDirect {
        &mut *ncdirect_init(null(), null_mut(), flags)
    }
}

// Explicitly implementing both `Drop` and `Copy` trait on a type is currently
// disallowed (rustc --explain E0184)
// https://github.com/rust-lang/rust/issues/20126
//
// impl Drop for NcDirect {
//     fn drop(&mut self) {
//         unsafe {
//             ncdirect_stop(self);
//         }
//     }
// }
