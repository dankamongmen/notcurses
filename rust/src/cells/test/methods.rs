//! Test `NcCell` methods and associated functions.

use crate::{Nc, NcCell, NcPlane};

use serial_test::serial;

#[test]
#[serial]
fn constructors() -> crate::NcResult<()> {
    let _c1 = NcCell::new();
    let _c2 = NcCell::from_char7b('C');

    let nc = Nc::new()?;
    let plane = NcPlane::new(nc, 0, 0, 10, 10)?;
    let _c3 = NcCell::from_char('௵', plane);
    nc.stop()?;
    Ok(())
}
