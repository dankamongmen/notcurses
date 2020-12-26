//! `NcMenu*` methods and associated functions.

use core::ptr::null_mut;

use crate::{cstring, cstring_mut, NcInput, NcMenuItem, NcMenuSection};

mod menu;
mod options;

pub use menu::*;
pub use options::*;

/// # `NcMenuItem` Constructors
impl NcMenuItem {
    /// New NcMenuItem.
    pub fn new(desc: &str, shortcut: NcInput) -> Self {
        Self {
            // utf-8 menu item, NULL for horizontal separator
            desc: cstring_mut![desc],

            // ´NcInput´ shortcut, all should be distinct
            shortcut,
        }
    }

    /// New empty NcMenuItem.
    pub fn new_empty() -> Self {
        Self {
            desc: null_mut(),
            shortcut: NcInput::new_empty(),
        }
    }
}

/// # `NcMenuSection` Constructors
impl NcMenuSection {
    /// New NcMenuSection.
    pub fn new(name: &str, mut items: Vec<NcMenuItem>, shortcut: NcInput) -> Self {
        Self {
            // utf-8 name string
            name: cstring![name] as *mut i8,

            //
            itemcount: items.len() as i32,

            // array of itemcount `NcMenuItem`s
            items: items.as_mut_ptr() as *mut NcMenuItem,

            // shortcut, will be underlined if present in name
            shortcut,
        }
    }
}
