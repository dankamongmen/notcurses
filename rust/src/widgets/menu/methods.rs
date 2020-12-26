//! `NcMenu*` methods and associated functions.

use core::ptr::null_mut;

use crate::{
    cstring, error, error_ptr, error_str, ncmenu_create, NcChannelPair, NcInput, NcMenu,
    NcMenuItem, NcMenuOptions, NcMenuSection, NcPlane, NcResult,
};

/// # `NcMenu` constructors & destructors
impl NcMenu {
    /// New NcMenu.
    ///
    /// *C style function: [ncmenu_create()][crate::ncmenu_create].*
    pub fn new<'a>(plane: &mut NcPlane) -> NcResult<&'a mut Self> {
        Self::with_options(plane, NcMenuOptions::new())
    }

    /// Creates an [NcMenu] with the specified options.
    ///
    /// Menus are currently bound to an overall [Notcurses][crate::Notcurses]
    /// object (as opposed to a particular plane), and are implemented as
    /// [NcPlane]s kept atop other NcPlanes.
    ///
    /// *C style function: [ncmenu_create()][crate::ncmenu_create].*
    pub fn with_options<'a>(plane: &mut NcPlane, options: NcMenuOptions) -> NcResult<&'a mut Self> {
        error_ptr![
            unsafe { ncmenu_create(plane, &options) },
            "Creating NcMenu"
        ]
    }

    /// Destroys an NcMenu created with [create()][NcMenu#method.create].
    ///
    /// *C style function: [ncmenu_destroy()][crate::ncmenu_destroy].*
    pub fn ncmenu_destroy(&mut self) -> NcResult<()> {
        error![unsafe { crate::ncmenu_destroy(self) }]
    }
}

/// # `NcMenu` methods
impl NcMenu {
    /// Disables or enables an [NcMenuItem].
    ///
    /// *C style function: [ncmenu_item_set_status()][crate::ncmenu_item_set_status].*
    pub fn ncmenu_item_set_status(
        &mut self,
        section: &str,
        item: &str,
        enabled: bool,
    ) -> NcResult<()> {
        error![unsafe {
            crate::ncmenu_item_set_status(self, cstring![section], cstring![item], enabled)
        }]
    }

    /// Returns the [NcMenuItem] description corresponding to the mouse click `click`.
    ///
    /// The [NcMenuItem] must be on an actively unrolled section, and the click
    /// must be in the area of a valid item.
    ///
    /// If `ninput` is provided, and the selected item has a shortcut,
    /// it will be filled in with that shortcut.
    ///
    /// *C style function: [ncmenu_mouse_selected()][crate::ncmenu_mouse_selected].*
    pub fn ncmenu_mouse_selected(
        &self,
        click: &NcInput,
        shortcut: Option<&mut NcInput>,
    ) -> NcResult<String> {
        let ninput;
        if let Some(i) = shortcut {
            ninput = i as *mut _;
        } else {
            ninput = null_mut();
        }
        error_str![
            unsafe { crate::ncmenu_mouse_selected(self, click, ninput) },
            "Getting NcMenuItem description"
        ]
    }

    /// Moves to the next item within the currently unrolled section.
    ///
    /// If no section is unrolled, the first section will be unrolled.
    ///
    /// *C style function: [ncmenu_nextitem()][crate::ncmenu_nextitem].*
    pub fn ncmenu_nextitem(&mut self) -> NcResult<()> {
        error![unsafe { crate::ncmenu_nextitem(self) }]
    }

    /// Unrolls the next section (relative to current unrolled).
    ///
    /// If no section is unrolled, the first section will be unrolled.
    ///
    /// *C style function: [ncmenu_nextsection()][crate::ncmenu_nextsection].*
    pub fn ncmenu_nextsection(&mut self) -> NcResult<()> {
        error![unsafe { crate::ncmenu_nextsection(self) }]
    }

    /// Offers the `input` to this NcMenu.
    ///
    /// If it's relevant, this function returns true,
    /// and the input ought not be processed further.
    /// If it's irrelevant to the menu, false is returned.
    ///
    /// Relevant inputs include:
    /// - mouse movement over a hidden menu
    /// - a mouse click on a menu section (the section is unrolled)
    /// - a mouse click outside of an unrolled menu (the menu is rolled up)
    /// - left or right on an unrolled menu (navigates among sections)
    /// - up or down on an unrolled menu (navigates among items)
    /// - escape on an unrolled menu (the menu is rolled up)
    ///
    /// *C style function: [ncmenu_offer_input()][crate::ncmenu_offer_input].*
    pub fn ncmenu_offer_input(&mut self, input: NcInput) -> bool {
        unsafe { crate::ncmenu_offer_input(self, &input) }
    }

    /// Returns the [NcPlane] backing this NcMenu.
    ///
    /// *C style function: [ncmenu_plane()][crate::ncmenu_plane].*
    pub fn ncmenu_plane(&mut self) -> NcResult<&NcPlane> {
        error_ptr![
            unsafe { crate::ncmenu_plane(self) },
            "Getting the backing NcPlane"
        ]
    }

    /// Moves to the previous item within the currently unrolled section.
    ///
    /// If no section is unrolled, the first section will be unrolled.
    ///
    /// *C style function: [ncmenu_previtem()][crate::ncmenu_previtem].*
    pub fn ncmenu_previtem(&mut self) -> NcResult<()> {
        error![unsafe{ crate::ncmenu_previtem(self) }]
    }

    /// Unrolls the previous section (relative to current unrolled).
    ///
    /// If no section is unrolled, the first section will be unrolled.
    ///
    /// *C style function: [ncmenu_prevsection()][crate::ncmenu_prevsection].*
    pub fn ncmenu_prevsection(&mut self) -> NcResult<()> {
        error![unsafe{ crate::ncmenu_prevsection(self) }]
    }

    /// Rolls up any unrolled [NcMenuSection],
    /// and hides this NcMenu if using hiding.
    ///
    /// *C style function: [ncmenu_rollup()][crate::ncmenu_rollup].*
    pub fn ncmenu_rollup(&mut self) -> NcResult<()> {
        error![unsafe{ crate::ncmenu_rollup(self) }]
    }

    /// Returns the selected item description,
    /// or an error if no section is unrolled.
    ///
    /// If `shortcut` is provided, and the selected item has a shortcut,
    /// it will be filled in with that shortcut--this can allow faster matching.
    ///
    /// *C style function: [ncmenu_selected()][crate::ncmenu_selected].*
    pub fn ncmenu_selected(
        &mut self,
        shortcut: Option<&mut NcInput>,
    ) -> NcResult<String> {
        let ninput;
        if let Some(i) = shortcut {
            ninput = i as *mut _;
        } else {
            ninput = null_mut();
        }
        error_str![
            unsafe { crate::ncmenu_selected(self, ninput) },
            "Getting the selected NcMenuItem description"
        ]
    }

    /// Unrolls the specified [NcMenuSection], making the menu visible if it was
    /// invisible, and rolling up any NcMenuSection that is already unrolled.
    ///
    /// *C style function: [ncmenu_unroll()][crate::ncmenu_unroll].*
    pub fn ncmenu_unroll(&mut self, sectionindex: u32) -> NcResult<()> {
        error![unsafe{ crate::ncmenu_unroll(self, sectionindex as i32) }]
    }
}

/// # `NcMenuOptions` Constructors
impl NcMenuOptions {
    /// New NcMenuOptions.
    pub fn new() -> Self {
        Self::with_options(&mut [], 0, 0, 0, 0)
    }

    /// New NcMenuOptions with width options.
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

/// # `NcMenuItem` Constructors
impl NcMenuItem {
    /// New NcMenuItem.
    pub fn new(mut desc: i8, shortcut: NcInput) -> Self {
        Self {
            // utf-8 menu item, NULL for horizontal separator
            desc: &mut desc,

            // ´NcInput´ shortcut, all should be distinct
            shortcut,
        }
    }
}

/// # `NcMenuSection` Constructors
impl NcMenuSection {
    /// New NcMenuSection.
    pub fn new(name: &str, itemcount: i32, items: &mut [NcMenuItem], shortcut: NcInput) -> Self {
        Self {
            // utf-8 name string
            name: cstring![name] as *mut i8,

            //
            itemcount,

            // array of itemcount `NcMenuItem`s
            items: items as *mut _ as *mut NcMenuItem,

            // shortcut, will be underlined if present in name
            shortcut,
        }
    }
}
