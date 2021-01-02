use crate::{notcurses_init, NcNotcurses, NcNotcursesOptions, NCOPTION_SUPPRESS_BANNERS};

/// Helper function for initializing NcNotcurses on C style tests.
#[allow(dead_code)]
pub(crate) unsafe fn notcurses_init_test<'a>() -> &'a mut NcNotcurses {
    &mut *notcurses_init(
        &NcNotcursesOptions::with_flags(NCOPTION_SUPPRESS_BANNERS),
        core::ptr::null_mut(),
    )
}
