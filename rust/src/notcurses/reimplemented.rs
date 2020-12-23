//! `notcurses_*` reimplemented functions.

use core::ptr::null;

use crate::{
    NcAlign, NcDimension, NcInput, NcOffset, NcPlane, NcTime, Notcurses, NCALIGN_CENTER,
    NCALIGN_LEFT, NCALIGN_RIGHT, NCRESULT_MAX,
};

// can't use libc::sigset_t with notcurses_getc(()
use crate::bindings::{sigemptyset, sigfillset, sigset_t};

/// Returns the offset into 'availcols' at which 'cols' ought be output given
/// the requirements of 'align'.
///
/// Returns -[`NCRESULT_MAX`] if [NCALIGN_UNALIGNED][crate::NCALIGN_UNALIGNED]
/// or invalid [NcAlign].
#[inline]
pub fn notcurses_align(availcols: NcDimension, align: NcAlign, cols: NcDimension) -> NcOffset {
    if align == NCALIGN_LEFT {
        return 0;
    }
    if cols > availcols {
        return 0;
    }
    if align == NCALIGN_CENTER {
        return ((availcols - cols) / 2) as NcOffset;
    }
    if align == NCALIGN_RIGHT {
        return (availcols - cols) as NcOffset;
    }
    -NCRESULT_MAX // NCALIGN_UNALIGNED
}

/// 'input' may be NULL if the caller is uninterested in event details.
/// If no event is ready, returns 0.
///
/// *Method: Notcurses.[getc_nblock()][Notcurses#method.getc_nblock].*
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
        core::char::from_u32_unchecked(crate::notcurses_getc(nc, &ts, &mut sigmask, input))
    }
}

/// 'input' may be NULL if the caller is uninterested in event details.
/// Blocks until an event is processed or a signal is received.
///
/// *Method: Notcurses.[getc_nblocking()][Notcurses#method.getc_nblocking].*
#[inline]
pub fn notcurses_getc_nblocking(nc: &mut Notcurses, input: &mut NcInput) -> char {
    unsafe {
        let mut sigmask = sigset_t { __val: [0; 16] };
        sigemptyset(&mut sigmask);
        core::char::from_u32_unchecked(crate::notcurses_getc(nc, null(), &mut sigmask, input))
    }
}

/// notcurses_stdplane(), plus free bonus dimensions written to non-NULL y/x!
///
/// *Method: Notcurses.[getc_stddim_yx()][Notcurses#method.stddim_yx].*
#[inline]
pub fn notcurses_stddim_yx<'a>(
    nc: &mut Notcurses,
    y: &mut NcDimension,
    x: &mut NcDimension,
) -> &'a mut NcPlane {
    unsafe {
        let s = crate::notcurses_stdplane(nc);
        crate::ncplane_dim_yx(s, &mut (*y as i32), &mut (*x as i32));
        &mut *s
    }
}

/// notcurses_stdplane_const(), plus free bonus dimensions written to non-NULL y/x!
///
/// *Method: Notcurses.[getc_stddim_yx_const()][Notcurses#method.stddim_yx_const].*
#[inline]
pub fn notcurses_stddim_yx_const<'a>(nc: &'a Notcurses, y: &mut i32, x: &mut i32) -> &'a NcPlane {
    unsafe {
        let s = crate::notcurses_stdplane_const(nc);
        crate::ncplane_dim_yx(s, y, x);
        &*s
    }
}

/// Returns our current idea of the terminal dimensions in rows and cols.
///
/// *Method: Notcurses.[getc_term_yx()][Notcurses#method.term_yx].*
#[inline]
pub fn notcurses_term_dim_yx(nc: &Notcurses) -> (NcDimension, NcDimension) {
    let (mut y, mut x) = (0, 0);
    unsafe {
        crate::ncplane_dim_yx(crate::notcurses_stdplane_const(nc), &mut y, &mut x);
    }
    (y as NcDimension, x as NcDimension)
}
