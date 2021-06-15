//! `notcurses_*` reimplemented functions.

use core::ptr::{null, null_mut};

use crate::{
    NcAlign, NcDim, NcError, NcInput, NcOffset, NcPlane, NcResult, NcSignalSet, NcTime, Nc,
    NCALIGN_CENTER, NCALIGN_LEFT, NCALIGN_RIGHT, NCRESULT_MAX,
};

/// Returns the offset into `availcols` at which `cols` ought be output given
/// the requirements of `align`.
///
/// Returns `-`[`NCRESULT_MAX`] if [NCALIGN_UNALIGNED][crate::NCALIGN_UNALIGNED]
/// or invalid [NcAlign].
///
/// *Method: Nc.[align()][Nc#method.align].*
#[inline]
pub fn notcurses_align(availcols: NcDim, align: NcAlign, cols: NcDim) -> NcOffset {
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

///
/// If no event is ready, returns 0.
///
/// *Method: Nc.[getc_nblock()][Nc#method.getc_nblock].*
//
// TODO: use from_u32 & return Option.
#[inline]
pub fn notcurses_getc_nblock(nc: &mut Nc, input: &mut NcInput) -> char {
    unsafe {
        let mut sigmask = NcSignalSet::new();
        sigmask.fillset();
        let ts = NcTime {
            tv_sec: 0,
            tv_nsec: 0,
        };
        core::char::from_u32_unchecked(crate::notcurses_getc(nc, &ts, &sigmask, input))
    }
}

/// Blocks until an event is processed or a signal is received.
///
/// Optionally writes the event details in `input`.
///
/// In case of an invalid read (including on EOF) *-1 as char* is returned.
///
/// *Method: Nc.[getc_blocking()][Nc#method.getc_blocking].*
#[inline]
pub fn notcurses_getc_blocking(nc: &mut Nc, input: Option<&mut NcInput>) -> char {
    let input_ptr;
    if let Some(i) = input {
        input_ptr = i as *mut _;
    } else {
        input_ptr = null_mut();
    }
    unsafe {
        let mut sigmask = NcSignalSet::new();
        sigmask.emptyset();
        core::char::from_u32_unchecked(crate::notcurses_getc(nc, null(), &sigmask, input_ptr))
    }
}

/// [notcurses_stdplane()][crate::notcurses_stdplane], plus free bonus
/// dimensions written to non-NULL y/x!
///
/// *Method: Nc.[getc_stddim_yx()][Nc#method.stddim_yx].*
#[inline]
pub fn notcurses_stddim_yx<'a>(
    nc: &'a mut Nc,
    y: &mut NcDim,
    x: &mut NcDim,
) -> NcResult<&'a mut NcPlane> {
    unsafe {
        let sp = crate::notcurses_stdplane(nc);
        if !sp.is_null() {
            crate::ncplane_dim_yx(sp, &mut (*y as i32), &mut (*x as i32));
            return Ok(&mut *sp);
        }
    }
    Err(NcError::new())
}

/// [notcurses_stdplane_const()][crate::notcurses_stdplane_const], plus free
/// bonus dimensions written to non-NULL y/x!
///
/// *Method: Nc.[getc_stddim_yx_const()][Nc#method.stddim_yx_const].*
#[inline]
pub fn notcurses_stddim_yx_const<'a>(
    nc: &'a Nc,
    y: &mut NcDim,
    x: &mut NcDim,
) -> NcResult<&'a NcPlane> {
    unsafe {
        let sp = crate::notcurses_stdplane_const(nc);
        if !sp.is_null() {
            crate::ncplane_dim_yx(sp, &mut (*y as i32), &mut (*x as i32));
            return Ok(&*sp);
        }
    }
    Err(NcError::new())
}

/// Returns our current idea of the terminal dimensions in rows and cols.
///
/// *Method: Nc.[getc_term_yx()][Nc#method.term_yx].*
#[inline]
pub fn notcurses_term_dim_yx(nc: &Nc) -> (NcDim, NcDim) {
    let (mut y, mut x) = (0, 0);
    unsafe {
        crate::ncplane_dim_yx(crate::notcurses_stdplane_const(nc), &mut y, &mut x);
    }
    (y as NcDim, x as NcDim)
}
