use crate::{
    widgets::{NcTreeItem, NcTreeItemCbUnsafe, NcTreeOptions},
    NcDim,
};

#[allow(unused_imports)]
use crate::widgets::NcTree;

/// # `NcTreeOptions` constructors
impl NcTreeOptions {
    /// New NcTreeOptions for [`NcTree`].
    pub fn new(items: &[NcTreeItem], indentcols: NcDim) -> Self {
        Self::with_all_args(items, items.len(), None, indentcols, 0)
    }

    /// New NcTreeOptions for [`NcTree`], with all args.
    pub fn with_all_args(
        // top-level nctree_item array
        items: &[NcTreeItem],

        // size of |items|
        count: usize,

        // item callback function
        // TODO: use NcTreeItemCb and convert to NcTreeItemCbUnsafe
        nctreecb: Option<NcTreeItemCbUnsafe>,

        // columns to indent per level of hierarchy
        indentcols: NcDim,

        // bitfield of `NCTREE_OPTION_*` (there's none for now)
        flags: u64,
    ) -> Self {
        Self {
            items: items as *const _ as *const NcTreeItem,
            count: count as u32,
            nctreecb,
            indentcols: indentcols as i32,
            flags,
        }
    }
}
