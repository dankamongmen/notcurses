use crate::{ncpile_create, NcDimension, NcOffset, NcPlane, NcPlaneOptions, Notcurses};

/// Helper function for a new NcPlane on C style tests.
#[allow(dead_code)]
pub(crate) unsafe fn ncplane_new_test<'a>(
    nc: &mut Notcurses,
    y: NcOffset,
    x: NcOffset,
    rows: NcDimension,
    cols: NcDimension,
) -> &'a mut NcPlane {
    &mut *ncpile_create(nc, &NcPlaneOptions::new(y, x, rows, cols))
}
