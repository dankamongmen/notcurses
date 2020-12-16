//! `notcurses_*` reimplemented functions.

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
    Notcurses,
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
        // https://www.gnu.org/software/libc/manual/html_node/Signal-Sets.html
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
pub fn notcurses_stddim_yx<'a>(nc: &mut Notcurses, y: &mut i32, x: &mut i32) -> &'a mut NcPlane {
    unsafe {
        let s = notcurses_stdplane(nc);
        ncplane_dim_yx(s, y, x);
        &mut *s
    }
}

/// notcurses_stdplane_const(), plus free bonus dimensions written to non-NULL y/x!
#[inline]
pub fn notcurses_stddim_yx_const<'a>(nc: &'a Notcurses, y: &mut i32, x: &mut i32) -> &'a NcPlane {
    unsafe {
        let s = notcurses_stdplane_const(nc);
        ncplane_dim_yx(s, y, x);
        &*s
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
