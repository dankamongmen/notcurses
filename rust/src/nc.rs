// functions already exported by bindgen : 35
// ------------------------------------------
// notcurses_at_yx
// notcurses_canchangecolor
// notcurses_canfade
// notcurses_canopen_images
// notcurses_canopen_videos
// notcurses_cansixel
// notcurses_cantruecolor
// notcurses_canutf8
// notcurses_cursor_disable
// notcurses_cursor_enable
// notcurses_debug
// notcurses_drop_planes
// notcurses_getc
// notcurses_init
// notcurses_inputready_fd
// notcurses_lex_blitter
// notcurses_lex_margins
// notcurses_lex_scalemode
// notcurses_mouse_disable
// notcurses_mouse_enable
// notcurses_palette_size
// notcurses_refresh
// notcurses_render
// notcurses_render_to_file
// notcurses_reset_stats
// notcurses_stats
// notcurses_stdplane
// notcurses_stdplane_const
// notcurses_stop
// notcurses_str_blitter
// notcurses_str_scalemode
// notcurses_supported_styles
// notcurses_top
// notcurses_version
// notcurses_version_components
//
// static inline functions to reimplement: 4
// ----------------------------------------- (done / (x) wont / remaining)
// (+) implement : 4 / 0 / 0
// (#) unit tests: 0 / 0 / 4
// -----------------------------------------
//+notcurses_getc_blocking
//+notcurses_getc_nblock
//+notcurses_stddim_yx
//+notcurses_term_dim_yx

use core::ptr::null;

use crate as ffi;
use ffi::{ncinput, ncplane, notcurses};

/// 'input' may be NULL if the caller is uninterested in event details.
/// If no event is ready, returns 0.
// TODO: TEST
#[inline]
pub fn notcurses_getc_nblock(nc: &mut notcurses, input: &mut ncinput) -> ffi::char32_t {
    unsafe {
        let mut sigmask = ffi::sigset_t { __val: [0; 16] };
        ffi::sigfillset(&mut sigmask);
        let ts = ffi::timespec {
            tv_sec: 0,
            tv_nsec: 0,
        };
        ffi::notcurses_getc(nc, &ts, &mut sigmask, input)
    }
}

/// 'input' may be NULL if the caller is uninterested in event details.
/// Blocks until an event is processed or a signal is received.
// TODO: TEST
#[inline]
pub fn notcurses_getc_nblocking(nc: &mut notcurses, input: &mut ncinput) -> ffi::char32_t {
    unsafe {
        let mut sigmask = ffi::sigset_t { __val: [0; 16] };
        ffi::sigemptyset(&mut sigmask);
        ffi::notcurses_getc(nc, null(), &mut sigmask, input)
    }
}

/// notcurses_stdplane(), plus free bonus dimensions written to non-NULL y/x!
// TODO: TEST
#[inline]
pub fn notcurses_stddim_yx(nc: &mut notcurses, y: &mut i32, x: &mut i32) -> ncplane {
    unsafe {
        let s = ffi::notcurses_stdplane(nc);
        ffi::ncplane_dim_yx(s, y, x);
        *s
    }
}

/// Return our current idea of the terminal dimensions in rows and cols.
// TODO: TEST
#[inline]
pub fn notcurses_term_dim_yx(nc: &notcurses, rows: &mut i32, cols: &mut i32) {
    unsafe {
        ffi::ncplane_dim_yx(ffi::notcurses_stdplane_const(nc), rows, cols);
    }
}

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
