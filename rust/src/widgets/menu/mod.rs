//! `NcMenu` widget

// functions already exported by bindgen : 13
// ------------------------------------------
//  ncmenu_create
//  ncmenu_destroy
//  ncmenu_item_set_status
//  ncmenu_mouse_selected
//  ncmenu_nextitem
//  ncmenu_nextsection
//  ncmenu_offer_input
//  ncmenu_plane
//  ncmenu_previtem
//  ncmenu_prevsection
//  ncmenu_rollup
//  ncmenu_selected
//  ncmenu_unroll

mod methods;

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
