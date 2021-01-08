//! Test `NcCell` methods and associated functions.

use crate::{FullMode, NcCell, NcPlane};

use serial_test::serial;

#[test]
#[serial]
fn constructors() -> crate::NcResult<()> {
    let _c1 = NcCell::new();
    let _c2 = NcCell::with_char7b('C');

    let mut nc = FullMode::new()?;
    let plane = NcPlane::new(&mut nc, 0, 0, 10, 10)?;
    let _c3 = NcCell::with_char('௵', plane);
    Ok(())
}
