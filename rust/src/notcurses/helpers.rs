use crate::{notcurses_init, Notcurses, NotcursesOptions, NCOPTION_SUPPRESS_BANNERS};

/// Helper function for initializing Notcurses on C style tests.
#[allow(dead_code)]
pub(crate) unsafe fn notcurses_init_test<'a>() -> &'a mut Notcurses {
    &mut *notcurses_init(
        &NotcursesOptions::with_flags(NCOPTION_SUPPRESS_BANNERS),
        core::ptr::null_mut(),
    )
}
