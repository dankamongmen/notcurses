//! `NcTree` widget

// functions already exported by bindgen : 13
// ------------------------------------------
// (#) test:  0
// (W) wrap: 13
// ------------------------------------------
//W nctree_create,
//W nctree_destroy,
//W nctree_focused,
//~ nctree_goto,
//W nctree_next,
//W nctree_offer_input,
//W nctree_plane,
//W nctree_prev,
//W nctree_redraw,

use cty::c_int;
use std::ffi::c_void;

use crate::NcPlane;

mod methods;

/// High-level hierarchical line-based data.
///
/// `NcTree`s organize static hierarchical items, and allow them to be browsed.
///
/// An NcTree cannot be empty, count must be non-zero.
///
/// - Each item can have arbitrary subitems.
/// - Items can be collapsed and expanded.
/// - The display supports scrolling and searching.
/// - Items cannot be added or removed, however; they must be provided in their
///   entirety at creation time.
///
/// NOTE: `NcTree` shares many properties with `NcReel`. Unlike the latter,
/// `NcTree`s support arbitrary hierarchical levels, but they do not allow
/// elements to come and go across the lifetime of the widget.
///
/// `type in C: nctree (struct)`
pub type NcTree = crate::bindings::ffi::nctree;

/// Item for [`NcTree`].
///
/// each item has a curry, and zero or more subitems.
pub type NcTreeItem = crate::bindings::ffi::nctree_item;

/// Options struct for [`NcTree`].
pub type NcTreeOptions = crate::bindings::ffi::nctree_options;

// e.g.:
//
// ```c
// int treecb(struct ncplane* n, void* curry, int pos){
//   ncplane_printf_yx(n, 0, 0, "item: %s pos: %d",
//                     static_cast<const char*>(curry), pos);
//   return 0;
// }
// ```

/// An [NcTreeItem] callback function (unsafe).
pub type NcTreeItemCbUnsafe = unsafe extern "C" fn(*mut NcPlane, *mut c_void, c_int) -> c_int;

/// An [NcTreeItem] callback function.
pub type NcTreeItemCb = fn(&mut NcPlane, &str, i32);

// WIP TODO: create callback type and conversion functions
//
// /// Converts [NcTreeItemCbUnsafe] to [NcTreeItemCb].
// pub fn nctreeitemcb_to_rust(resizecb: Option<NcTreeItemCbUnsafe>) -> Option<NcTreeItemCb> {
//     if let Some(cb) = resizecb {
//         return Some(unsafe { core::mem::transmute(cb) });
//     } else {
//         None
//     }
// }
//
// /// Converts [NcTreeItemCb] to [NcTreeItemCbUnsafe].
// ///
// // waiting for https://github.com/rust-lang/rust/issues/53605
// // to make this function const, and then NcPlaneOptions constructors.
// pub fn nctreeitemcb_to_c(resizecb: Option<NcTreeItemCb>) -> Option<NcTreeItemCbUnsafe> {
//     if let Some(cb) = resizecb {
//         return Some(unsafe { core::mem::transmute(cb) });
//     } else {
//         None
//     }
// }
