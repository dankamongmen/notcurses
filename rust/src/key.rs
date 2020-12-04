// functions manually reimplemented: 2
// ------------------------------------------
// (+) done: 2 / 0
// (#) test: 0 / 2
// ------------------------------------------
// + nckey_mouse_p
// + nckey_supppuab_p

use crate::{NCKEY_BUTTON1, NCKEY_RELEASE};

/// Is this u32 a Supplementary Private Use Area-B codepoint?
///
/// Links:
/// - https://en.wikipedia.org/wiki/Private_Use_Areas
/// - https://codepoints.net/supplementary_private_use_area-b
#[inline]
pub fn nckey_supppuab_p(w: u32) -> bool {
    w >= 0x100000 && w <= 0x10fffd
}

/// Is the event a synthesized mouse event?
#[inline]
pub fn nckey_mouse_p(r: u32) -> bool {
    r >= NCKEY_BUTTON1 && r <= NCKEY_RELEASE
}
