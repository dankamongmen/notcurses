//! `NcSelector` widget.

// TODO: implement constructors & Drop

/// high-level widget for selecting one item from a set
pub type NcSelector = crate::bindings::bindgen::ncselector;

/// an item for [`NcSelector`]
pub type NcSelectorItem = crate::bindings::bindgen::ncselector_item;

/// Options structur for [`NcSelector`]
pub type NcSelectorOptions = crate::bindings::bindgen::ncselector_options;
