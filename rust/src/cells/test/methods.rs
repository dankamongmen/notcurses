//! Test `NcCell` methods and associated functions.

use crate::{NcCell, NcPlane, Notcurses};

use serial_test::serial;

#[test]
#[serial]
fn constructors() {
    let _c1 = NcCell::new();
    let _c2 = NcCell::with_char7b('C');

    let _n0 = Notcurses::new();
    let _p0 = NcPlane::new(_n0, 0, 0, 10, 10);
    let _c3 = NcCell::with_char('௵', _p0);
    _n0.stop();
}
