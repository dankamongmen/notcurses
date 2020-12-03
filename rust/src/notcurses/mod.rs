// functions already exported by bindgen : 39
// ----------------------------------------- (done / remaining)
// (#) unit tests: 10 / 29
// ------------------------------------------
//  notcurses_at_yx
//  notcurses_bottom
//# notcurses_canchangecolor
//# notcurses_canfade
//# notcurses_canopen_images
//# notcurses_canopen_videos
//# notcurses_cansixel
//# notcurses_cantruecolor
//# notcurses_canutf8
//  notcurses_cursor_disable
//  notcurses_cursor_enable
//# notcurses_debug
//# notcurses_drop_planes
//  notcurses_getc
//# notcurses_init
//  notcurses_inputready_fd
//  notcurses_lex_blitter
//  notcurses_lex_margins
//  notcurses_lex_scalemode
//  notcurses_mouse_disable
//  notcurses_mouse_enable
//  notcurses_palette_size
//  notcurses_refresh
//  notcurses_render
//  notcurses_render_to_buffer
//  notcurses_render_to_file
//  notcurses_stats
//  notcurses_stats_alloc
//  notcurses_stats_reset
//  notcurses_stdplane
//  notcurses_stdplane_const
//# notcurses_stop
//  notcurses_str_blitter
//  notcurses_str_scalemode
//  notcurses_supported_styles
//  notcurses_top
//  notcurses_ucs32_to_utf8
//  notcurses_version
//  notcurses_version_components
//
// static inline functions total: 6
// ----------------------------------------- (done / remaining)
// (+) implement : 6 / 0
// (#) unit tests: 0 / 6
// -----------------------------------------
//# notcurses_align
//+ notcurses_getc_blocking
//+ notcurses_getc_nblock
//+ notcurses_stddim_yx
//+ notcurses_stddim_yx_const
//+ notcurses_term_dim_yx

#[cfg(test)]
mod tests;

mod types;
pub use types::{
    NcLogLevel, Notcurses, NotcursesOptions, NCLOGLEVEL_DEBUG, NCLOGLEVEL_ERROR, NCLOGLEVEL_FATAL,
    NCLOGLEVEL_INFO, NCLOGLEVEL_PANIC, NCLOGLEVEL_SILENT, NCLOGLEVEL_TRACE, NCLOGLEVEL_VERBOSE,
    NCLOGLEVEL_WARNING, NCOPTION_INHIBIT_SETLOCALE, NCOPTION_NO_ALTERNATE_SCREEN,
    NCOPTION_NO_FONT_CHANGES, NCOPTION_NO_QUIT_SIGHANDLERS, NCOPTION_NO_WINCH_SIGHANDLER,
    NCOPTION_SUPPRESS_BANNERS, NCOPTION_VERIFY_SIXEL,
};

mod wrapped;
pub use wrapped::*;

use core::ptr::null;

use crate::{
    // NOTE: can't use libc::sigset_t with notcurses_getc(()
    bindings::{sigemptyset, sigfillset, sigset_t},
    ncplane_dim_yx,
    notcurses_getc,
    notcurses_stdplane,
    notcurses_stdplane_const,
    NcAlign,
    NcInput,
    NcPlane,
    NcTime,
    NCALIGN_CENTER,
    NCALIGN_LEFT,
};

/// return the offset into 'availcols' at which 'cols' ought be output given the requirements of 'align'
#[inline]
pub fn notcurses_align(availcols: i32, align: NcAlign, cols: i32) -> i32 {
    if align == NCALIGN_LEFT {
        return 0;
    }
    if cols > availcols {
        return 0;
    }
    if align == NCALIGN_CENTER {
        return (availcols - cols) / 2;
    }
    availcols - cols // NCALIGN_RIGHT
}

/// 'input' may be NULL if the caller is uninterested in event details.
/// If no event is ready, returns 0.
// TODO: use pakr-signals
#[inline]
pub fn notcurses_getc_nblock(nc: &mut Notcurses, input: &mut NcInput) -> char {
    unsafe {
        let mut sigmask = sigset_t { __val: [0; 16] };
        sigfillset(&mut sigmask);
        let ts = NcTime {
            tv_sec: 0,
            tv_nsec: 0,
        };
        core::char::from_u32_unchecked(notcurses_getc(nc, &ts, &mut sigmask, input))
    }
}

/// 'input' may be NULL if the caller is uninterested in event details.
/// Blocks until an event is processed or a signal is received.
#[inline]
pub fn notcurses_getc_nblocking(nc: &mut Notcurses, input: &mut NcInput) -> char {
    unsafe {
        let mut sigmask = sigset_t { __val: [0; 16] };
        sigemptyset(&mut sigmask);
        core::char::from_u32_unchecked(notcurses_getc(nc, null(), &mut sigmask, input))
    }
}

/// notcurses_stdplane(), plus free bonus dimensions written to non-NULL y/x!
#[inline]
pub fn notcurses_stddim_yx(nc: &mut Notcurses, y: &mut i32, x: &mut i32) -> NcPlane {
    unsafe {
        let s = notcurses_stdplane(nc);
        ncplane_dim_yx(s, y, x);
        *s
    }
}

/// notcurses_stdplane_const(), plus free bonus dimensions written to non-NULL y/x!
#[inline]
pub fn notcurses_stddim_yx_const(nc: &Notcurses, y: &mut i32, x: &mut i32) -> NcPlane {
    unsafe {
        let s = notcurses_stdplane_const(nc);
        ncplane_dim_yx(s, y, x);
        *s
    }
}

/// Return our current idea of the terminal dimensions in rows and cols.
#[inline]
pub fn notcurses_term_dim_yx(nc: &Notcurses, rows: &mut u32, cols: &mut u32) {
    unsafe {
        let mut irows = *rows as i32;
        let mut icols = *cols as i32;
        ncplane_dim_yx(notcurses_stdplane_const(nc), &mut irows, &mut icols);
    }
}
