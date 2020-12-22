// functions manually reimplemented: 2
// ------------------------------------------
// (+) done: 2 / 0
// (#) test: 0 / 2
// ------------------------------------------
// + nckey_mouse_p
// + nckey_supppuab_p

use crate::{NCKEY_BUTTON1, NCKEY_RELEASE};

/// Is this [char] a Supplementary Private Use Area-B codepoint?
///
/// Links:
/// - https://en.wikipedia.org/wiki/Private_Use_Areas
/// - https://codepoints.net/supplementary_private_use_area-b
#[inline]
pub fn nckey_supppuab_p(w: char) -> bool {
    w as u32 >= 0x100000_u32 && w as u32 <= 0x10fffd_u32
}

/// Is the event a synthesized mouse event?
#[inline]
pub const fn nckey_mouse_p(r: char) -> bool {
    r >= NCKEY_BUTTON1 && r <= NCKEY_RELEASE
}
