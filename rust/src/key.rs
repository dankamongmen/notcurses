// functions already exported by bindgen : 0
// ------------------------------------------
//
// static inline functions to reimplement: 2
// ------------------------------------------ (done / wont / remaining)
// - implement : 2 / 0 / 0
// - unit tests: 0 / 0 / 2
// --------------- (+) implemented (#) + unit test (x) wont implement
//+nckey_mouse_p
//+nckey_supppuab_p

use crate as ffi;

/// Is this char32_t a Supplementary Private Use Area-B codepoint?
// TODO: TEST
#[inline]
pub fn nckey_supppuab_p(w: u32) -> bool {
    w >= 0x100000 && w <= 0x10fffd
}

/// Is the event a synthesized mouse event?
// TODO: TEST
#[inline]
pub fn nckey_mouse_p(r: u32) -> bool {
    r >= ffi::NCKEY_BUTTON1 && r <= ffi::NCKEY_RELEASE
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
