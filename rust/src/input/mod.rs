//! `NcInput` & `NcKey`

// functions manually reimplemented: 1
// ------------------------------------------
// (+) done: 3 / 0
// (#) test: 0 / 3
// ------------------------------------------
// + ncinput_equal_p
// + nckey_mouse_p
// + nckey_supppuab_p

use crate::NcDimension;

mod keycodes;
pub use keycodes::*;

/// Reads and decodes input events
///
/// Reads from stdin and decodes the input to stdout,
/// including synthesized events and mouse events.
///
///
/// Notcurses provides input from keyboards and mice.
/// Single Unicode codepoints are received from the keyboard, directly encoded
/// as `u32`.
///
/// The input system must deal with numerous keyboard signals which do not map
/// to Unicode code points. This includes the keypad arrows and function keys.
/// These "synthesized" codepoints are enumerated in , and mapped into the
/// Supplementary Private Use Area-B (U+100000..U+10FFFD).
/// Mouse button events are similarly mapped into the SPUA-B.
///
/// All events carry a ncinput structure with them.
/// For mouse events, the x and y coordinates are reported within this struct.
/// For all events, modifiers (e.g. "Alt") are carried as bools in this struct.
pub type NcInput = crate::bindings::ffi::ncinput;

/// Compares two ncinput structs for data equality by doing a field-by-field
/// comparison for equality (excepting seqnum).
///
/// Returns true if the two are data-equivalent.
pub const fn ncinput_equal_p(n1: NcInput, n2: NcInput) -> bool {
    if n1.id != n2.id {
        return false;
    }
    if n1.y != n2.y || n1.x != n2.x {
        return false;
    }
    // do not check seqnum
    true
}

/// New NcInput.
impl NcInput {
    /// New empty NcInput.
    pub const fn new_empty() -> NcInput {
        NcInput {
            id: 0,
            y: 0,
            x: 0,
            alt: false,
            shift: false,
            ctrl: false,
            seqnum: 0,
        }
    }

    /// New NcInput, expecting all the arguments.
    pub const fn with_all_args(
        id: char,
        x: Option<NcDimension>,
        y: Option<NcDimension>,
        alt: bool,
        shift: bool,
        ctrl: bool,
        seqnum: u64,
    ) -> NcInput {
        let (ix, iy);
        if let Some(x) = x {
            ix = x as i32
        } else {
            ix = -1
        };
        if let Some(y) = y {
            iy = y as i32
        } else {
            iy = -1
        };

        NcInput {
            id: id as u32,
            y: ix,
            x: iy,
            alt,
            shift,
            ctrl,
            seqnum,
        }
    }

    /// New NcInput with alt key.
    pub const fn with_alt(id: char) -> NcInput {
        Self::with_all_args(id, None, None, true, false, false, 0)
    }

    /// New NcInput with shift key.
    pub const fn with_shift(id: char) -> NcInput {
        Self::with_all_args(id, None, None, false, true, false, 0)
    }

    /// New NcInput with ctrl key.
    pub const fn with_ctrl(id: char) -> NcInput {
        Self::with_all_args(id, None, None, false, false, true, 0)
    }
}

/// Is this [char] a Supplementary Private Use Area-B codepoint?
///
/// Links:
/// - https://en.wikipedia.org/wiki/Private_Use_Areas
/// - https://codepoints.net/supplementary_private_use_area-b
#[inline]
pub const fn nckey_supppuab_p(w: char) -> bool {
    w as u32 >= 0x100000_u32 && w as u32 <= 0x10fffd_u32
}

/// Is the event a synthesized mouse event?
#[inline]
pub const fn nckey_mouse_p(r: char) -> bool {
    r >= NCKEY_BUTTON1 && r <= NCKEY_RELEASE
}
