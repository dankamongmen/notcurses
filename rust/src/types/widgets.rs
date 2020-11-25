//! Types for notcurses widgets

#[allow(unused_imports)] // for docblocks
use crate::NcPlane;

// NcMenu ----------------------------------------------------------------------

/// menus on the top or bottom rows
///
/// A notcurses instance supports menu bars on the top or bottom row of the true
/// screen.
///
/// A menu is composed of sections, which are in turn composed of items.
/// Either no sections are visible, and the menu is rolled up, or exactly one
/// section is unrolled.
///
/// `ncmenu_rollup` places an `NcMenu` in the rolled up state.
/// `ncmenu_unroll` rolls up any unrolled section and unrolls the specified one.
/// `ncmenu_destroy` removes a menu bar, and frees all associated resources.
///
/// `type in C: ncmenu (struct)`
pub type NcMenu = crate::ncmenu;

/// Options struct for [`NcMenu`]
pub type NcMenuOptions = crate::ncmenu_options;

/// Item for [`NcMenu`]
pub type NcMenuItem = crate::ncmenu_item;

/// Section for [`NcMenu`]
pub type NcMenuSection = crate::ncmenu_section;

/// Bottom row (as opposed to top row)
pub const NCMENU_OPTION_BOTTOM: u32 = crate::bindings::NCMENU_OPTION_BOTTOM;

/// Hide the menu when not unrolled
pub const NCMENU_OPTION_HIDING: u32 = crate::bindings::NCMENU_OPTION_HIDING;

// NcReader --------------------------------------------------------------------

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
pub type NcReader = crate::ncreader;

/// Options struct for [`NcReader`]
///
/// `type in C: ncreader_options (struct)`
///
pub type NcReaderOptions = crate::ncreader_options;

/// Make the terminal cursor visible across the lifetime of the ncreader, and
/// have the ncreader manage the cursor's placement.
pub const NCREADER_OPTION_CURSOR: u32 = crate::bindings::NCREADER_OPTION_CURSOR;

/// Enable horizontal scrolling. Virtual lines can then grow arbitrarily long.
pub const NCREADER_OPTION_HORSCROLL: u32 = crate::bindings::NCREADER_OPTION_HORSCROLL;

/// Disable all editing shortcuts. By default, emacs-style keys are available.
pub const NCREADER_OPTION_NOCMDKEYS: u32 = crate::bindings::NCREADER_OPTION_NOCMDKEYS;

/// Enable vertical scrolling. You can then use arbitrarily many virtual lines.
pub const NCREADER_OPTION_VERSCROLL: u32 = crate::bindings::NCREADER_OPTION_VERSCROLL;

// NcReel ----------------------------------------------------------------------

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
pub type NcReel = crate::ncreel;

/// Options struct for [`NcReel`]
pub type NcReelOptions = crate::ncreel_options;

/// Visual tablet for [`NcReel`]
pub type NcTablet = crate::nctablet;

/// is navigation circular (does moving down from the last tablet move to the
/// first, and vice versa)? only meaningful when infinitescroll is true. if
/// infinitescroll is false, this must be false.
pub const NCREEL_OPTION_CIRCULAR: u32 = crate::bindings::NCREEL_OPTION_CIRCULAR;
/// is scrolling infinite (can one move down or up forever, or is an end
/// reached?). if true, 'circular' specifies how to handle the special case of
/// an incompletely-filled reel.
pub const NCREEL_OPTION_INFINITESCROLL: u32 = crate::bindings::NCREEL_OPTION_INFINITESCROLL;

// NcPlot ----------------------------------------------------------------------

/// A histogram, bound to an [`NcPlane`]
/// (uses non-negative `f64`s)
pub type NcPlotF64 = crate::ncdplot;

/// A histogram, bound to an [`NcPlane`] (uses `u64`s)
pub type NcPlotU64 = crate::ncuplot;

/// Options struct for
/// [`NcPlotF64`] or [`NcPlotU64`]
pub type NcPlotOptions = crate::ncplot_options;

/// Use domain detection only for max
pub const NCPLOT_OPTION_DETECTMAXONLY: u32 = crate::bindings::NCPLOT_OPTION_DETECTMAXONLY;

/// Exponential dependent axis
pub const NCPLOT_OPTION_EXPONENTIALD: u32 = crate::bindings::NCPLOT_OPTION_EXPONENTIALD;

/// Show labels for dependent axis
pub const NCPLOT_OPTION_LABELTICKSD: u32 = crate::bindings::NCPLOT_OPTION_LABELTICKSD;

/// Use domain detection only for max
pub const NCPLOT_OPTION_NODEGRADE: u32 = crate::bindings::NCPLOT_OPTION_NODEGRADE;

/// Independent axis is vertical
pub const NCPLOT_OPTION_VERTICALI: u32 = crate::bindings::NCPLOT_OPTION_VERTICALI;

// NcSelector ------------------------------------------------------------------

/// high-level widget for selecting one item from a set
pub type NcSelector = crate::ncselector;
/// an item for [`NcSelector`]
pub type NcSelectorItem = crate::ncselector_item;
/// Options structur for [`NcSelector`]
pub type NcSelectorOptions = crate::ncselector_options;

// NcStats ---------------------------------------------------------------------

/// notcurses runtime statistics
pub type NcStats = crate::ncstats;

// NcMultiSelector -------------------------------------------------------------

/// high-level widget for selecting items from a set
pub type NcMultiSelector = crate::ncmultiselector;

/// an item for [`NcMultiSelector`]
pub type NcMultiSelectorItem = crate::ncmselector_item;

/// Options structure for [`NcMultiSelector`]
pub type NcMultiSelectorOptions = crate::ncmultiselector_options;
