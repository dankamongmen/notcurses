//! `NcMenu*` methods and associated functions.

use super::{NcMenuItem, NcMenuSection};
use crate::{cstring_mut, NcInput};
use core::ptr::null_mut;

#[allow(unused_imports)]
use crate::widgets::NcMenu;

mod menu;
mod options;

pub use menu::*;
pub use options::*;

/// # `NcMenuItem` Constructors
impl NcMenuItem {
    /// New NcMenuItem for [`NcMenu`].
    pub fn new(desc: &str, shortcut: NcInput) -> Self {
        Self {
            // utf-8 menu item, NULL for horizontal separator
            desc: cstring_mut![desc],

            // ´NcInput´ shortcut, all should be distinct
            shortcut,
        }
    }

    /// New empty NcMenuItem for [`NcMenu`].
    pub fn new_empty() -> Self {
        Self {
            desc: null_mut(),
            shortcut: NcInput::new_empty(),
        }
    }
}

/// # `NcMenuSection` Constructors
///
// Must contain at least 1 NcMenuItem.
impl NcMenuSection {
    /// New NcMenuSection for [`NcMenu`].
    pub fn new(name: &str, items: &mut [NcMenuItem], shortcut: NcInput) -> Self {
        Self {
            // utf-8 name string
            name: cstring_mut![name],

            // array of itemcount `NcMenuItem`s
            items: items.as_mut_ptr(),

            //
            itemcount: items.len() as i32,

            // shortcut, will be underlined if present in name
            shortcut,
        }
    }

    /// New NcMenuSection separator for [`NcMenu`].
    ///
    pub fn new_separator() -> Self {
        Self {
            name: null_mut(),
            items: null_mut(),
            itemcount: 0,
            shortcut: NcInput::new_empty(),
        }
    }
}
