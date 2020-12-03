use crate::{ncdirect_init, NcDirect, NcDirectFlags};
use core::ptr::{null, null_mut};

impl NcDirect {
    /// A simple ncdirect_init() wrapper using the default options.
    ///
    /// Initialize a direct-mode notcurses context on the tty.
    ///
    /// Direct mode supports a limited subset of notcurses routines,
    /// and neither supports nor requires notcurses_render(). This can be
    /// used to add color and styling to text in the standard output paradigm.
    //
    // Returns NULL on error, including any failure initializing terminfo.
    pub unsafe fn new<'a>() -> &'a mut NcDirect {
        Self::with_flags(0)
    }

    /// A simple ncdirect_init() wrapper with optional flags.
    ///
    /// `flags` is a bitmask over:
    /// - NCDIRECT_OPTION_INHIBIT_CBREAK
    /// - NCDIRECT_OPTION_INHIBIT_SETLOCALE
    ///
    pub unsafe fn with_flags<'a>(flags: NcDirectFlags) -> &'a mut NcDirect {
        &mut *ncdirect_init(null(), null_mut(), flags)
    }
}

// Explicitly implementing both `Drop` and `Copy` trait on a type is currently
// disallowed (rustc --explain E0184)
// https://github.com/rust-lang/rust/issues/20126
//
// impl Drop for NcDirect {
//     fn drop(&mut self) {
//         unsafe {
//             ncdirect_stop(self);
//         }
//     }
// }
