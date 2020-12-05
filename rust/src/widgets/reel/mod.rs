//! `NcReel` widget.

// TODO: implement constructors & Drop

/// A wheel with `NcTablet`s on the outside
///
/// An `NcReel` is projected onto the 2d rendering area, showing some portion of
/// the `NcReel`, and zero or more `NcTablet`s.
///
/// An `NcReel` is a `Notcurses` region devoted to displaying zero or more
/// line-oriented, contained `NcTablet`s between which the user may navigate.
///
/// If at least one `NcTablet`s exists, there is a "focused tablet".
/// As much of the focused tablet as is possible is always displayed.
///
/// If there is space left over, other tablets are included in the display.
/// Tablets can come and go at any time, and can grow or shrink at any time.
pub type NcReel = crate::bindings::bindgen::ncreel;

/// Options struct for [`NcReel`]
pub type NcReelOptions = crate::bindings::bindgen::ncreel_options;

/// Visual tablet for [`NcReel`]
pub type NcTablet = crate::bindings::bindgen::nctablet;

/// is navigation circular (does moving down from the last tablet move to the
/// first, and vice versa)? only meaningful when infinitescroll is true. if
/// infinitescroll is false, this must be false.
pub const NCREEL_OPTION_CIRCULAR: u32 = crate::bindings::bindgen::NCREEL_OPTION_CIRCULAR;
/// is scrolling infinite (can one move down or up forever, or is an end
/// reached?). if true, 'circular' specifies how to handle the special case of
/// an incompletely-filled reel.
pub const NCREEL_OPTION_INFINITESCROLL: u32 =
    crate::bindings::bindgen::NCREEL_OPTION_INFINITESCROLL;
