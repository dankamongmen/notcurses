use crate::{NcDimension, NcOffset, NcPlane, NcPlaneOptions, Notcurses};

/// Helper function for a new NcPlane on C style tests.
#[allow(dead_code)]
pub(crate) unsafe fn ncplane_new_test<'a>(
    nc: &mut Notcurses,
    y: NcOffset,
    x: NcOffset,
    rows: NcDimension,
    cols: NcDimension,
) -> &'a mut NcPlane {
    &mut *crate::ncpile_create(nc, &NcPlaneOptions::new(y, x, rows, cols))
}

/// Helper function for a new bound NcPlane on C style tests.
#[allow(dead_code)]
pub(crate) unsafe fn ncplane_new_bound_test<'a>(
    plane: &mut NcPlane,
    y: NcOffset,
    x: NcOffset,
    rows: NcDimension,
    cols: NcDimension,
) -> &'a mut NcPlane {
    &mut *crate::ncplane_create(plane, &NcPlaneOptions::new(y, x, rows, cols))
}
