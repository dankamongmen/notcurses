use crate::{notcurses_init, Nc, NcOptions, NCOPTION_SUPPRESS_BANNERS};

/// Helper function for initializing Nc on C style tests.
#[allow(dead_code)]
pub(crate) unsafe fn notcurses_init_test<'a>() -> &'a mut Nc {
    &mut *notcurses_init(
        &NcOptions::with_flags(NCOPTION_SUPPRESS_BANNERS),
        core::ptr::null_mut(),
    )
}
