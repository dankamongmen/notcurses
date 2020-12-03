//! ´NcReader´ widget types

/// Provides a freeform input in a (possibly multiline) region
///
/// Supports optional readline keybindings (opt out using
/// NCREADER_OPTION_NOCMDKEYS flag)
///
/// Takes ownership of its [`NcPlane`], destroying it on any
/// error (`ncreader_destroy`() otherwise destroys the ncplane).
///
/// `type in C: ncreader (struct)`
///
pub type NcReader = crate::bindings::bindgen::ncreader;

/// Options struct for [`NcReader`]
///
/// `type in C: ncreader_options (struct)`
///
pub type NcReaderOptions = crate::bindings::bindgen::ncreader_options;

/// Make the terminal cursor visible across the lifetime of the ncreader, and
/// have the ncreader manage the cursor's placement.
pub const NCREADER_OPTION_CURSOR: u32 = crate::bindings::bindgen::NCREADER_OPTION_CURSOR;

/// Enable horizontal scrolling. Virtual lines can then grow arbitrarily long.
pub const NCREADER_OPTION_HORSCROLL: u32 = crate::bindings::bindgen::NCREADER_OPTION_HORSCROLL;

/// Disable all editing shortcuts. By default, emacs-style keys are available.
pub const NCREADER_OPTION_NOCMDKEYS: u32 = crate::bindings::bindgen::NCREADER_OPTION_NOCMDKEYS;

/// Enable vertical scrolling. You can then use arbitrarily many virtual lines.
pub const NCREADER_OPTION_VERSCROLL: u32 = crate::bindings::bindgen::NCREADER_OPTION_VERSCROLL;
