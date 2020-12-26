//! `NcMenu` widget

// functions already exported by bindgen : 13
// ------------------------------------------
// (#) test:  0
// (W) wrap: 13
// ------------------------------------------
//W ncmenu_create
//W ncmenu_destroy
//W ncmenu_item_set_status
//W ncmenu_mouse_selected
//W ncmenu_nextitem
//W ncmenu_nextsection
//W ncmenu_offer_input
//W ncmenu_plane
//W ncmenu_previtem
//W ncmenu_prevsection
//W ncmenu_rollup
//W ncmenu_selected
//W ncmenu_unroll

mod methods;

/// menus on the top or bottom rows
///
/// A [Notcurses][crate::Notcurses] instance supports menu bars
/// on the top or bottom row of the true screen.
///
/// An NcMenu is composed of [NcMenuSection]s, which are in turn composed of
/// [NcMenuItem]s.
///
/// Either no sections are visible, and the menu is rolled up, or exactly one
/// section is unrolled.
///
/// - [rollup()][NcMenu#method.rollup]
///     places an `NcMenu` in the rolled up state.
/// - [unroll()][NcMenu#method.]
///     rolls up any unrolled section and unrolls the specified one.
/// - [destroy()][NcMenu#method.destroy]
///     removes a menu bar, and frees all associated resources.
///
/// `type in C: ncmenu (struct)`
pub type NcMenu = crate::bindings::ffi::ncmenu;

/// Options struct for [`NcMenu`]
pub type NcMenuOptions = crate::bindings::ffi::ncmenu_options;

/// Item for [`NcMenu`]
pub type NcMenuItem = crate::bindings::ffi::ncmenu_item;

/// Section for [`NcMenu`]
pub type NcMenuSection = crate::bindings::ffi::ncmenu_section;

/// Bottom row (as opposed to top row)
pub const NCMENU_OPTION_BOTTOM: u32 = crate::bindings::ffi::NCMENU_OPTION_BOTTOM;

/// Hide the menu when not unrolled
pub const NCMENU_OPTION_HIDING: u32 = crate::bindings::ffi::NCMENU_OPTION_HIDING;
