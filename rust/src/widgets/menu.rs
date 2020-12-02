// functions already exported by bindgen : 13
// ------------------------------------------
// ncmenu_create
// ncmenu_destroy
// ncmenu_item_set_status
// ncmenu_mouse_selected
// ncmenu_nextitem
// ncmenu_nextsection
// ncmenu_offer_input
// ncmenu_plane
// ncmenu_previtem
// ncmenu_prevsection
// ncmenu_rollup
// ncmenu_selected
// ncmenu_unroll

use std::ffi::CString;

use crate::{
    ncmenu_create, NcChannelPair, NcInput, NcMenu, NcMenuItem, NcMenuOptions, NcMenuSection,
    NcPlane,
};

impl NcMenu {
    /// `NcMenu` simple constructor
    pub unsafe fn new<'a>(plane: &mut NcPlane) -> &'a mut Self {
        Self::with_options(plane, &NcMenuOptions::new())
    }

    /// `NcMenu` constructor with options
    pub unsafe fn with_options<'a>(plane: &mut NcPlane, options: &NcMenuOptions) -> &'a mut Self {
        &mut *ncmenu_create(plane, options)
    }
}

impl NcMenuOptions {
    /// `NcMenuOptions` simple constructor
    pub fn new() -> Self {
        Self::with_options(&mut [], 0, 0, 0, 0)
    }

    /// `NcMenuOptions` width options
    pub fn with_options(
        sections: &mut [NcMenuSection],
        count: u32,
        headerc: NcChannelPair,
        sectionc: NcChannelPair,
        flags: u64,
    ) -> Self {
        Self {
            // array of 'sectioncount' `MenuSection`s
            sections: sections as *mut _ as *mut NcMenuSection, /// XXX TEST

            // must be positive TODO
            sectioncount: count as i32,

            // styling for header
            headerchannels: headerc,

            // styling for sections
            sectionchannels: sectionc,

            // flag word of NCMENU_OPTION_*
            flags: flags,
        }
    }
}

impl NcMenuItem {
    /// `NcMenuItem` simple constructor
    pub fn new(mut desc: i8, shortcut: NcInput) -> Self {
        Self {
            // utf-8 menu item, NULL for horizontal separator
            desc: &mut desc,

            // ´NcInput´ shortcut, all should be distinct
            shortcut,
        }
    }
}

impl NcMenuSection {
    /// `NcMenuSection` simple constructor
    pub fn new(name: &str, itemcount: i32, items: &mut [NcMenuItem], shortcut: NcInput) -> Self {
        Self {
            // utf-8 name string
            name: CString::new(name).expect("Bad string").as_ptr() as *mut i8,

            //
            itemcount,

            // array of itemcount `NcMenuItem`s
            items: items as *mut _ as *mut NcMenuItem,

            // shortcut, will be underlined if present in name
            shortcut,
        }
    }
}
