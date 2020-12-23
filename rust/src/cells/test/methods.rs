//! Test `NcCell` methods and associated functions.

use crate::NcCell;

use serial_test::serial;

#[test]
#[serial]
fn constructors() {
    let _c1 = NcCell::new();

    let _c2 = NcCell::with_7bitchar('C');
}
