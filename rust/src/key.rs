// functions already exported by bindgen : 0
// ------------------------------------------
//
// static inline functions to reimplement: 2
// ------------------------------------------ (done / (x) wont / remaining)
// (+) implement : 2 / 0 / 0
// (#) unit tests: 0 / 0 / 2
// ------------------------------------------
//+nckey_mouse_p
//+nckey_supppuab_p

use crate as nc;

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
    r >= nc::NCKEY_BUTTON1 && r <= nc::NCKEY_RELEASE
}

#[cfg(test)]
mod test {
    // use super::nc;
    // use serial_test::serial;
    /*
    #[test]
    #[serial]
    fn () {
    }
    */
}
